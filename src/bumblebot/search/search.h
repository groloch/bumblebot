# ifndef SEARCH_H
# define SEARCH_H

# include <array>
# include <atomic>
# include <cstddef>
# include <cstdint>
# include <functional>
# include <unordered_map>
# include <vector>

# include "move.h"
# include "position.h"
# include "search/limits.h"
# include "search/node.h"


struct EvalEntry{
    std::array<float, 4288> policy;
    float                   value;
};


struct SearchStats{
    unsigned nodes     = 0;
    unsigned avgDepth  = 0;
    unsigned maxDepth  = 0;
    int64_t  elapsedMs = 0;
    float    rootQ     = 0.5f;

    unsigned nps() const {
        if(elapsedMs <= 0) return 0;
        return static_cast<unsigned>((static_cast<int64_t>(nodes) * 1000) / elapsedMs);
    }
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

    void setHashSizeMb(unsigned mb){
        hashBudgetBytes = static_cast<std::size_t>(mb) * 1024ull * 1024ull;
    }

private:
    static constexpr std::size_t kBytesPerEntry = sizeof(Hash) + sizeof(EvalEntry);

    float expand(Position& position, Node& node);

    std::unordered_map<Hash, EvalEntry> evalCache;
    std::size_t hashBudgetBytes{16ull * 1024 * 1024};
};

# endif
