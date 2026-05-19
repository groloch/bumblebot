# ifndef SEARCH_H
# define SEARCH_H

# include <atomic>
# include <cstddef>
# include <cstdint>
# include <functional>
# include <vector>

# include "types.h"
# include "move.h"
# include "position.h"
# include "search/limits.h"
# include "search/node.h"


struct SearchStats{
    unsigned nodes{0};
    unsigned avgDepth{0};
    unsigned maxDepth{0};
    int64_t elapsedMs{0};
    float rootQ{ZERO_VALUE};

    unsigned nps() const {
        if(elapsedMs <= 0) return 0;
        return static_cast<unsigned>((static_cast<int64_t>(nodes) * 1000) / elapsedMs);
    }
};


struct SearchProfile{
    int64_t selectNs{0};
    int64_t expandNs{0};
    int64_t nnNs{0};
    int64_t backpropNs{0};
    uint64_t iterations{0};
    uint64_t nnCalls{0};
    uint64_t expandRaces{0};
};


class Search{
public:
    struct Result{
        Move best;
        Move ponder;
    };

    using InfoCallback = std::function<void(const SearchStats&, const std::vector<Move>&)>;

    Result search(Position& position,
                  const SearchLimits& limits,
                  std::atomic<bool>& stop,
                  InfoCallback onInfo);

    void setNumThreads(unsigned n){
        if(n == 0) n = 1;
        numThreads = n;
    }

    void setHashSizeMb(unsigned mb){}

    SearchProfile profile() const;

private:
    struct Counters {
        std::atomic<int64_t> selectNs{0};
        std::atomic<int64_t> expandNs{0};
        std::atomic<int64_t> nnNs{0};
        std::atomic<int64_t> backpropNs{0};
        std::atomic<uint64_t> iterations{0};
        std::atomic<uint64_t> nnCalls{0};
        std::atomic<uint64_t> expandRaces{0};
        std::atomic<uint32_t> maxDepth{0};
        std::atomic<uint64_t> cumDepth{0};

        void reset();
    };

    float expand(Position& position, Node& node);

    void worker(Position const& rootPosition,
                Node& root,
                std::atomic<bool> const& stop);

    Counters counters_;
    unsigned numThreads{1};
};

# endif
