# ifndef NODE_H
# define NODE_H

# include <atomic>
# include <cstdint>
# include <vector>

# include "move.h"


struct Edge;


struct Node{
    Node(float prior);
    Node();
    Node(Node&& other) noexcept;

    Node(Node const&) = delete;
    Node& operator=(Node const&) = delete;
    Node& operator=(Node&&) = delete;

    float qValue() const;

    float getValue(unsigned parentVisitCount) const;

    void update(float leafValue);

    void addVirtualLoss();
    void removeVirtualLoss();

    Edge* selectChild();

    static constexpr int64_t kQScale = 1ll << 24;

    std::atomic<int64_t> sumQFixed;
    std::atomic<uint32_t> visitCount;
    std::atomic<int32_t> virtualLoss;
    std::atomic<bool> expanding;
    std::atomic<bool> expanded;

    bool isTerminal;
    float terminalValue;
    float cPuct;
    const float prior;

    std::vector<Edge> children;
};


struct Edge{
    Move move;
    Node child;
};

# endif
