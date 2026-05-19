# include "search/node.h"

# include <cmath>

# include "types.h"


Node::Node(float prior)
    : sumQFixed{0},
      visitCount{0},
      virtualLoss{0},
      expanding{false},
      expanded{false},
      isTerminal{false},
      terminalValue{ZERO_VALUE},
      cPuct{0.6f},
      prior{prior},
      children{}
{}

Node::Node() : Node(0.0f) {}

Node::Node(Node&& other) noexcept
    : sumQFixed{other.sumQFixed.load(std::memory_order_relaxed)},
      visitCount{other.visitCount.load(std::memory_order_relaxed)},
      virtualLoss{other.virtualLoss.load(std::memory_order_relaxed)},
      expanding{other.expanding.load(std::memory_order_relaxed)},
      expanded{other.expanded.load(std::memory_order_relaxed)},
      isTerminal{other.isTerminal},
      terminalValue{other.terminalValue},
      cPuct{other.cPuct},
      prior{other.prior},
      children{std::move(other.children)}
{}

float Node::qValue() const {
    const uint32_t n{visitCount.load(std::memory_order_relaxed)};
    if(n == 0) return ZERO_VALUE;
    const int64_t sum{sumQFixed.load(std::memory_order_relaxed)};
    return static_cast<float>(static_cast<double>(sum)
                              / (static_cast<double>(n) * static_cast<double>(kQScale)));
}

float Node::getValue(unsigned parentVisitCount) const {
    const uint32_t realVisits{visitCount.load(std::memory_order_relaxed)};
    const int32_t vloss{virtualLoss.load(std::memory_order_relaxed)};
    const uint32_t effVisits{realVisits + static_cast<uint32_t>(vloss > 0 ? vloss : 0)};

    float exploitation;
    if(effVisits == 0){
        exploitation = ZERO_VALUE;
    } else {
        // Virtual losses contribute 0 to the sum, so divide the real sum by the inflated denominator.
        const int64_t sum{sumQFixed.load(std::memory_order_relaxed)};
        exploitation = static_cast<float>(static_cast<double>(sum)
                                          / (static_cast<double>(effVisits)
                                             * static_cast<double>(kQScale)));
    }
    const float u{cPuct * prior * std::sqrt(static_cast<float>(parentVisitCount))
                  / (1.0f + static_cast<float>(effVisits))};
    return exploitation + u;
}

void Node::update(float leafValue) {
    const int64_t inc{static_cast<int64_t>(leafValue * static_cast<float>(kQScale))};
    sumQFixed.fetch_add(inc, std::memory_order_relaxed);
    visitCount.fetch_add(1, std::memory_order_relaxed);
}

void Node::addVirtualLoss(){
    virtualLoss.fetch_add(1, std::memory_order_relaxed);
}

void Node::removeVirtualLoss(){
    virtualLoss.fetch_sub(1, std::memory_order_relaxed);
}

Edge* Node::selectChild(){
    if(children.empty()){
        return nullptr;
    }
    const uint32_t parentVisits{visitCount.load(std::memory_order_relaxed)};
    Edge* bestChild{&children[0]};
    float bestValue{bestChild->child.getValue(parentVisits)};
    for(Edge& edge : children){
        float childValue{edge.child.getValue(parentVisits)};
        if(childValue > bestValue){
            bestValue = childValue;
            bestChild = &edge;
        }
    }
    return bestChild;
}
