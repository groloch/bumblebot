# ifndef SEARCH_H
# define SEARCH_H

# include <array>
# include <unordered_map>

# include "position.h"
# include "search/node.h"


struct EvalEntry{
    std::array<float, 4288> policy;
    float                   value;
};


class Search{
public:
    Move search(Position& position, unsigned iterations);

private:
    float expand(Position& position, Node& node);

    std::unordered_map<Hash, EvalEntry> evalCache;
};

# endif
