# include "search/search.h"

# include <algorithm>
# include <array>
# include <chrono>
# include <cstddef>
# include <thread>
# include <utility>
# include <vector>

# include "types.h"
# include "movegen.h"
# include "nn/evaluator.h"
# include "nn/nn_utils.h"
# include "search/node.h"


namespace {

using clock = std::chrono::steady_clock;

int64_t elapsedNs(clock::time_point a, clock::time_point b){
    return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
}

void atomicMax(std::atomic<uint32_t>& target, uint32_t candidate){
    uint32_t prev{target.load(std::memory_order_relaxed)};
    while(prev < candidate
          && !target.compare_exchange_weak(prev, candidate,
                                           std::memory_order_relaxed)){}
}

std::vector<Move> extractPV(const Node& root, std::size_t maxLen = 64){
    std::vector<Move> pv;
    const Node* node{&root};
    while(pv.size() < maxLen && !node->children.empty()){
        const Edge* best{nullptr};
        uint32_t bestVisits{0};
        for(const Edge& e : node->children){
            const uint32_t v{e.child.visitCount.load(std::memory_order_relaxed)};
            if(best == nullptr || v > bestVisits){
                best = &e;
                bestVisits = v;
            }
        }
        if(best == nullptr || bestVisits == 0) break;
        pv.push_back(best->move);
        node = &best->child;
    }
    return pv;
}

}


void Search::Counters::reset(){
    selectNs.store(0, std::memory_order_relaxed);
    expandNs.store(0, std::memory_order_relaxed);
    nnNs.store(0, std::memory_order_relaxed);
    backpropNs.store(0, std::memory_order_relaxed);
    iterations.store(0, std::memory_order_relaxed);
    nnCalls.store(0, std::memory_order_relaxed);
    expandRaces.store(0, std::memory_order_relaxed);
    maxDepth.store(0, std::memory_order_relaxed);
    cumDepth.store(0, std::memory_order_relaxed);
}


SearchProfile Search::profile() const {
    SearchProfile p;
    p.selectNs = counters_.selectNs.load(std::memory_order_relaxed);
    p.expandNs = counters_.expandNs.load(std::memory_order_relaxed);
    p.nnNs = counters_.nnNs.load(std::memory_order_relaxed);
    p.backpropNs = counters_.backpropNs.load(std::memory_order_relaxed);
    p.iterations = counters_.iterations.load(std::memory_order_relaxed);
    p.nnCalls = counters_.nnCalls.load(std::memory_order_relaxed);
    p.expandRaces = counters_.expandRaces.load(std::memory_order_relaxed);
    return p;
}


Search::Result Search::search(Position& position,
                              const SearchLimits& limits,
                              std::atomic<bool>& stop,
                              InfoCallback onInfo){
    counters_.reset();

    Node root{0.0f};
    expand(position, root);

    if(root.children.empty()){
        return Result{Move{}, Move{}};
    }

    auto rootVisits = [&]() -> uint32_t {
        return root.visitCount.load(std::memory_order_relaxed);
    };

    auto avgDepthNow = [&]() -> unsigned {
        const uint32_t v{rootVisits()};
        return (v == 0)
            ? 0u
            : static_cast<unsigned>(counters_.cumDepth.load(std::memory_order_relaxed) / v);
    };

    const auto t0 = clock::now();

    auto elapsedMsAt = [&](clock::time_point tp){
        return std::chrono::duration_cast<std::chrono::milliseconds>(tp - t0).count();
    };

    auto buildStats = [&](clock::time_point now){
        SearchStats s;
        s.nodes = rootVisits();
        s.avgDepth = avgDepthNow();
        s.maxDepth = counters_.maxDepth.load(std::memory_order_relaxed);
        s.elapsedMs = elapsedMsAt(now);
        s.rootQ = (rootVisits() == 0) ? ZERO_VALUE : (1.0f - root.qValue());
        return s;
    };

    auto shouldStopNow = [&](clock::time_point now){
        if(stop.load(std::memory_order_relaxed)) return true;
        if(limits.maxNodes != 0 && rootVisits() >= limits.maxNodes) return true;
        if(limits.maxDepth != 0 && avgDepthNow() >= limits.maxDepth) return true;
        if(limits.movetimeMs > 0 && elapsedMsAt(now) >= limits.movetimeMs) return true;
        return false;
    };

    const unsigned workerCount{std::max(1u, numThreads)};
    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for(unsigned i{0}; i < workerCount; ++i){
        workers.emplace_back([this, &position, &root, &stop]{
            this->worker(position, root, stop);
        });
    }

    auto tLastInfo = t0;
    while(!stop.load(std::memory_order_relaxed)){
        const auto now = clock::now();
        if(shouldStopNow(now)){
            stop.store(true, std::memory_order_relaxed);
            break;
        }
        if(onInfo &&
           std::chrono::duration_cast<std::chrono::milliseconds>(now - tLastInfo).count() >= 1000){
            onInfo(buildStats(now), extractPV(root));
            tLastInfo = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    for(std::thread& w : workers){
        w.join();
    }

    std::vector<Move> pv{extractPV(root)};

    if(onInfo){
        onInfo(buildStats(clock::now()), pv);
    }

    Result r;
    r.best = pv.empty() ? Move{} : pv[0];
    r.ponder = (pv.size() >= 2) ? pv[1] : Move{};
    return r;
}


void Search::runIteration(Position const& rootPosition,
                          Node& root,
                          std::vector<Edge*>& path){
    path.clear();
    Position scratch{rootPosition};
    Node* node{&root};

    // Selection
    const auto tSel0 = clock::now();
    while(!node->isTerminal && node->expanded.load(std::memory_order_acquire)){
        Edge* edge{node->selectChild()};
        if(edge == nullptr) break;
        edge->child.addVirtualLoss();
        path.push_back(edge);
        scratch.applyMove(edge->move);
        node = &edge->child;
    }
    const auto tSel1 = clock::now();
    counters_.selectNs.fetch_add(elapsedNs(tSel0, tSel1),
                                 std::memory_order_relaxed);

    const uint32_t depth{static_cast<uint32_t>(path.size())};
    counters_.cumDepth.fetch_add(depth, std::memory_order_relaxed);
    atomicMax(counters_.maxDepth, depth);

    // Expansion
    float value{0.0f};
    bool doBackprop{true};

    if(node->isTerminal){
        value = expand(scratch, *node);
    } else {
        bool expected{false};
        const bool claimed{
            node->expanding.compare_exchange_strong(expected, true,
                std::memory_order_acq_rel,
                std::memory_order_acquire)
        };
        if(claimed){
            value = expand(scratch, *node);
        } else {
            for(Edge* e : path){
                e->child.removeVirtualLoss();
            }
            counters_.expandRaces.fetch_add(1, std::memory_order_relaxed);
            doBackprop = false;
        }
    }

    if(!doBackprop) return;

    // Backpropagation
    const auto tBp0 = clock::now();
    Node* current{node};
    unsigned idx{static_cast<unsigned>(path.size())};
    while(true){
        value = 1.0f - value;
        current->update(value);
        if(current != &root){
            current->removeVirtualLoss();
        }
        if(idx == 0) break;
        --idx;
        current = (idx == 0) ? &root : &path[idx - 1]->child;
    }
    const auto tBp1 = clock::now();
    counters_.backpropNs.fetch_add(elapsedNs(tBp0, tBp1),
                                   std::memory_order_relaxed);
    counters_.iterations.fetch_add(1, std::memory_order_relaxed);
}


void Search::worker(Position const& rootPosition,
                    Node& root,
                    std::atomic<bool> const& stop){
    std::vector<Edge*> path;
    path.reserve(128);

    while(!stop.load(std::memory_order_relaxed)){
        runIteration(rootPosition, root, path);
    }
}


unsigned Search::benchPosition(Position& position, unsigned iterations){
    counters_.reset();

    Node root{0.0f};
    expand(position, root);

    if(!root.children.empty()){
        std::vector<Edge*> path;
        path.reserve(128);
        for(unsigned i{0}; i < iterations; ++i){
            runIteration(position, root, path);
        }
    }

    return root.visitCount.load(std::memory_order_relaxed);
}


float Search::expand(Position& position, Node& node){
    const auto tExpand0 = clock::now();
    int64_t nnDeltaNs{0};
    auto accountExpand = [&](){
        const int64_t totalNs{elapsedNs(tExpand0, clock::now())};
        counters_.expandNs.fetch_add(totalNs - nnDeltaNs,
                                     std::memory_order_relaxed);
        counters_.nnNs.fetch_add(nnDeltaNs, std::memory_order_relaxed);
    };

    if(node.isTerminal){
        accountExpand();
        return node.terminalValue;
    }

    MoveList legalMoves{};
    movegen::generateLegalAll(position, legalMoves);

    if(legalMoves.size == 0){
        node.isTerminal = true;
        node.terminalValue = position.inCheck() ? 0.0f : ZERO_VALUE;
        node.expanded.store(true, std::memory_order_release);
        accountExpand();
        return node.terminalValue;
    }
    if(position.halfmoveClock() >= 100){
        node.isTerminal = true;
        node.terminalValue = ZERO_VALUE;
        node.expanded.store(true, std::memory_order_release);
        accountExpand();
        return node.terminalValue;
    }
    if(position.isRepetition()){
        node.isTerminal = true;
        node.terminalValue = ZERO_VALUE;
        node.expanded.store(true, std::memory_order_release);
        accountExpand();
        return node.terminalValue;
    }

    const Hash key{position.hash()};
    nn::ModelInput input{nn::encode_board(position)};

    const auto tNn0 = clock::now();
    std::shared_future<nn::ModelOutput> fut{
        nn::Evaluator::instance().submit(key, std::move(input))
    };
    nn::ModelOutput out{fut.get()};
    const auto tNn1 = clock::now();
    nnDeltaNs += elapsedNs(tNn0, tNn1);
    counters_.nnCalls.fetch_add(1, std::memory_order_relaxed);

    std::array<float, 256> priors{};
    float priorSum{0.0f};
    for(unsigned i{0}; i < legalMoves.size; ++i){
        const unsigned pIdx{nn::policyIndexOf(legalMoves.moves[i], position)};
        priors[i] = out.policy[pIdx];
        priorSum += priors[i];
    }
    const float uniform{1.0f / static_cast<float>(legalMoves.size)};
    const float invSum{(priorSum > 0.0f) ? (1.0f / priorSum) : 0.0f};

    node.children.reserve(legalMoves.size);
    for(unsigned i{0}; i < legalMoves.size; ++i){
        const float p{(priorSum > 0.0f) ? priors[i] * invSum : uniform};
        node.children.push_back(Edge{legalMoves.moves[i], Node{p}});
    }
    node.expanded.store(true, std::memory_order_release);

    accountExpand();
    return out.value;
}
