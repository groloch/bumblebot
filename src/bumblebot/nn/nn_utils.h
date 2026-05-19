# ifndef NN_UTILS_H
# define NN_UTILS_H

# include <array>
# include <cstdint>

# include "move.h"


class Position;

namespace nn {

struct ModelInput {
    std::array<int64_t, 70> tokens;
    float hm;
    int64_t epsq;
};

ModelInput encode_board(Position const& pos);

Move moveFromPolicyIndex(unsigned index, Position const& pos);

unsigned policyIndexOf(Move const& move, Position const& pos);

}


# endif
