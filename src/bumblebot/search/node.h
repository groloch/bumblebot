# ifndef NODE_H
# define NODE_H

# include "move.h"

# include <vector>


struct Edge;


struct Node{
    Node(float prior);
    Node();
    Node(const Node& node);

    float getValue(unsigned parentVisitCount) const;

    void update(float leafValue);

    Edge* selectChild();

    float qValue;
    unsigned visitCount;
    float cPuct;
    bool isTerminal;
    float terminalValue;
    const float prior;

    std::vector<Edge> children;
};


struct Edge{
    Move move;
    Node child;
};

# endif
