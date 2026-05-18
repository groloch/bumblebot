# include "search/search.h"

# include <array>
# include <chrono>
# include <cstddef>
# include <utility>
# include <vector>

# include "types.h"
# include "movegen.h"
# include "nn/evaluator.h"
# include "nn/nn_utils.h"
# include "search/node.h"


namespace {

std::vector<Move> extractPV(const Node& root, std::size_t maxLen = 64){
    std::vector<Move> pv;
    const Node* node{&root};
    while(pv.size() < maxLen && !node->children.empty()){
        const Edge* best{nullptr};
        for(const Edge& e : node->children){
            if(best == nullptr || e.child.visitCount > best->child.visitCount){
                best = &e;
            }
        }
        if(best == nullptr || best->child.visitCount == 0) break;
        pv.push_back(best->move);
        node = &best->child;
    }
    return pv;
}

}


Search::Result Search::search(Position& position,
                              const SearchLimits& limits,
                              std::atomic<bool>& stop,
                              InfoCallback onInfo){
    using clock = std::chrono::steady_clock;

    Node root{0.0f};
    expand(position, root);

    if(root.children.empty()){
        return Result{Move{}, Move{}};
    }

    std::vector<Edge*> path;
    path.reserve(128);

    unsigned maxDepth{0};
    unsigned long long cumDepth{0};

    const auto t0 = clock::now();
    auto tLastInfo = t0;

    auto elapsedMsAt = [&](clock::time_point tp){
        return std::chrono::duration_cast<std::chrono::milliseconds>(tp - t0).count();
    };

    auto avgDepthNow = [&]() -> unsigned {
        return (root.visitCount == 0)
            ? 0u
            : static_cast<unsigned>(cumDepth / root.visitCount);
    };

    auto buildStats = [&](clock::time_point now){
        SearchStats s;
        s.nodes     = root.visitCount;
        s.avgDepth  = avgDepthNow();
        s.maxDepth  = maxDepth;
        s.elapsedMs = elapsedMsAt(now);
        // Backprop walks all the way up to root, applying one extra flip;
        // root.qValue ends up stored from the opponent's POV. Flip it back so
        // rootQ is the win probability for the side to move at the root.
        s.rootQ     = (root.visitCount == 0) ? ZERO_VALUE : (1.0f - root.qValue);
        return s;
    };

    auto shouldStopNow = [&](clock::time_point now){
        if(stop.load(std::memory_order_relaxed)) return true;
        if(limits.maxNodes != 0 && root.visitCount >= limits.maxNodes) return true;
        if(limits.maxDepth != 0 && avgDepthNow() >= limits.maxDepth) return true;
        if(limits.movetimeMs > 0 && elapsedMsAt(now) >= limits.movetimeMs) return true;
        return false;
    };

    while(true){
        auto now = clock::now();
        if(shouldStopNow(now)) break;
        if(onInfo &&
           std::chrono::duration_cast<std::chrono::milliseconds>(now - tLastInfo).count() >= 1000){
            onInfo(buildStats(now), extractPV(root));
            tLastInfo = now;
        }

        path.clear();
        Node* node{&root};

        while(!node->isTerminal && !node->children.empty()){
            Edge* edge{node->selectChild()};
            if(edge == nullptr) break;
            path.push_back(edge);
            position.applyMove(edge->move);
            node = &edge->child;
        }

        if(path.size() > maxDepth){
            maxDepth = static_cast<unsigned>(path.size());
        }
        cumDepth += path.size();

        float value{expand(position, *node)};

        Node* current{node};
        unsigned idx{static_cast<unsigned>(path.size())};
        while(true){
            value = 1.0f - value;
            current->update(value);
            if(idx == 0) break;
            --idx;
            position.undoMove(path[idx]->move);
            current = (idx == 0) ? &root : &path[idx - 1]->child;
        }
    }

    std::vector<Move> pv{extractPV(root)};

    if(onInfo){
        onInfo(buildStats(clock::now()), pv);
    }

    Result r;
    r.best   = pv.empty()       ? Move{} : pv[0];
    r.ponder = (pv.size() >= 2) ? pv[1] : Move{};
    return r;
}


float Search::expand(Position& position, Node& node){
    if(node.isTerminal){
        return node.terminalValue;
    }

    MoveList legalMoves{};
    movegen::generateLegalAll(position, legalMoves);

    if(legalMoves.size == 0){
        node.isTerminal = true;
        node.terminalValue = position.inCheck() ? 0.0f : ZERO_VALUE;
        return node.terminalValue;
    }
    if(position.halfmoveClock() >= 100){
        node.isTerminal = true;
        node.terminalValue = ZERO_VALUE;
        return node.terminalValue;
    }
    if(position.isRepetition()){
        node.isTerminal = true;
        node.terminalValue = ZERO_VALUE;
        return node.terminalValue;
    }

    const Hash key{position.hash()};
    auto it = evalCache.find(key);
    if(it == evalCache.end()){
        nn::ModelOutput out{nn::Evaluator::instance().evaluate(nn::encode_board(position))};
        EvalEntry entry;
        entry.policy = out.policy;
        entry.value  = out.value;
        if((evalCache.size() + 1) * kBytesPerEntry > hashBudgetBytes){
            evalCache.clear();
        }
        it = evalCache.emplace(key, std::move(entry)).first;
    }
    EvalEntry const& eval{it->second};

    std::array<float, 256> priors{};
    float priorSum{0.0f};
    for(unsigned i{0}; i < legalMoves.size; ++i){
        const unsigned idx{nn::policyIndexOf(legalMoves.moves[i], position)};
        priors[i] = eval.policy[idx];
        priorSum += priors[i];
    }
    const float uniform{1.0f / static_cast<float>(legalMoves.size)};
    const float invSum{(priorSum > 0.0f) ? (1.0f / priorSum) : 0.0f};

    node.children.reserve(legalMoves.size);
    for(unsigned i{0}; i < legalMoves.size; ++i){
        const float p{(priorSum > 0.0f) ? priors[i] * invSum : uniform};
        node.children.push_back(Edge{legalMoves.moves[i], Node{p}});
    }

    return eval.value;
}
