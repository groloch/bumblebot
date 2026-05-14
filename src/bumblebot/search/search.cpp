# include "search/search.h"

# include <array>
# include <utility>
# include <vector>

# include "types.h"
# include "movegen.h"
# include "nn/evaluator.h"
# include "nn/nn_utils.h"
# include "search/node.h"


Move Search::search(Position& position, unsigned iterations){
    Node root{0.0f};
    expand(position, root);

    if(root.children.empty()){
        return Move{};
    }

    std::vector<Edge*> path;
    path.reserve(128);

    for(unsigned i{0}; i < iterations; ++i){
        path.clear();
        Node* node{&root};

        while(!node->isTerminal && !node->children.empty()){
            Edge* edge{node->selectChild()};
            if(edge == nullptr) break;
            path.push_back(edge);
            position.applyMove(edge->move);
            node = &edge->child;
        }

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

    Edge* best{&root.children[0]};
    for(Edge& e : root.children){
        if(e.child.visitCount > best->child.visitCount){
            best = &e;
        }
    }
    return best->move;
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

    const Hash key{position.hash()};
    auto it = evalCache.find(key);
    if(it == evalCache.end()){
        nn::ModelOutput out{nn::Evaluator::instance().evaluate(nn::encode_board(position))};
        EvalEntry entry;
        entry.policy = out.policy;
        entry.value  = out.value;
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
