# include "bench.h"

# include <chrono>
# include <cstdint>
# include <cstdlib>
# include <iostream>

# include "position.h"
# include "perft.h"
# include "search/search.h"


namespace {

constexpr unsigned kDefaultNodesPerPosition = 500;

}


void run_bench(int argc, char* argv[]){
    unsigned nodesPerPosition{kDefaultNodesPerPosition};
    if(argc > 2){
        const long parsed{std::strtol(argv[2], nullptr, 10)};
        if(parsed > 0){
            nodesPerPosition = static_cast<unsigned>(parsed);
        }
    }

    Search search{};
    search.setNumThreads(1);

    Position warmup{};
    search.benchPosition(warmup, 64);

    uint64_t totalNodes{0};
    const auto start = std::chrono::steady_clock::now();
    Position position{};
    totalNodes += search.benchPosition(position, nodesPerPosition);
    const auto end = std::chrono::steady_clock::now();

    const int64_t elapsedMs{
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
    };
    const uint64_t nps{
        (elapsedMs > 0)
            ? (totalNodes * 1000ull / static_cast<uint64_t>(elapsedMs))
            : 0ull
    };

    std::cout << totalNodes << " nodes " << nps << " nps" << std::endl;
}
