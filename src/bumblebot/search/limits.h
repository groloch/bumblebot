# ifndef SEARCH_LIMITS_H
# define SEARCH_LIMITS_H

# include <cstdint>


struct SearchLimits{
    unsigned maxNodes{0};
    unsigned maxDepth{0};
    int64_t movetimeMs{0};
    bool infinite{false};
};

# endif
