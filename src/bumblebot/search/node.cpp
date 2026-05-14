# include "search/node.h"

# include <cmath>

# include "types.h"


Node::Node(float prior) : qValue{ZERO_VALUE}, visitCount{0}, cPuct{1.4f},
isTerminal{false}, terminalValue{ZERO_VALUE}, prior{prior}, children{} {}

Node::Node(): Node(0.0f) {}

Node::Node(const Node& node) : qValue{node.qValue}, visitCount{node.visitCount},
cPuct{node.cPuct}, isTerminal{node.isTerminal}, terminalValue{node.terminalValue},
prior{node.prior}, children{node.children} {}

float Node::getValue(unsigned parentVisitCount) const {
    float exploitation{(visitCount == 0) ? ZERO_VALUE : qValue};
    float u{cPuct * prior * std::sqrt(static_cast<float>(parentVisitCount))
            / (1.0f + static_cast<float>(visitCount))};
    return exploitation + u;
}

void Node::update(float leafValue) {
    visitCount++;
    qValue += (leafValue - qValue) / static_cast<float>(visitCount);
}

Edge* Node::selectChild(){
    if (children.empty()) {
        return nullptr;
    }
    Edge* bestChild{&children[0]};
    float bestValue{bestChild->child.getValue(visitCount)};
    for (Edge& edge : children) {
        float childValue{edge.child.getValue(visitCount)};
        if (childValue > bestValue) {
            bestValue = childValue;
            bestChild = &edge;
        }
    }
    return bestChild;
}
