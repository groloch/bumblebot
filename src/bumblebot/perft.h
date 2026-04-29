# ifndef PERFT_H
# define PERFT_H

# include "types.h"
# include "position.h"

# include <vector>
# include <string>


namespace perft{
struct NodeResult{
    std::string moveUci;
    u64 nodes;
};

struct PerftResult{
    std::vector<NodeResult> nodeResults;
    u64 nodes;
};

struct PerftTest{
    const char* fen;
    int depth;
    u64 expectedNodes;
};
}

perft::PerftResult run_perft(Position& position, int depth);
perft::PerftResult run_perft(const char* fen, int depth);

void run_perft_tests();

# endif
