# include "uci.h"

# include <algorithm>
# include <array>
# include <atomic>
# include <cctype>
# include <chrono>
# include <cmath>
# include <iostream>
# include <mutex>
# include <sstream>
# include <string>
# include <thread>
# include <vector>

# include "types.h"
# include "move.h"
# include "movegen.h"
# include "perft.h"
# include "position.h"
# include "debugging.h"
# include "nn/evaluator.h"
# include "nn/nn_utils.h"
# include "search/search.h"


namespace {

constexpr unsigned kHashDefaultMb = 16;
constexpr unsigned kHashMinMb = 1;
constexpr unsigned kHashMaxMb = 33554432;
constexpr unsigned kThreadsMax = 64;
constexpr unsigned kBatchSizeDefault = 32;
constexpr unsigned kBatchSizeMin = 1;
constexpr unsigned kBatchSizeMax = 256;


std::string toLower(std::string s){
    for(char& c : s){
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}


PieceType promoFromChar(char c){
    switch(c){
        case 'q': return PieceType::Queen;
        case 'r': return PieceType::Rook;
        case 'b': return PieceType::Bishop;
        case 'n': return PieceType::Knight;
        default:  return PieceType::None;
    }
}


bool applyUciMove(Position& position, std::string const& uci){
    if(uci.size() < 4) return false;
    if(uci[0] < 'a' || uci[0] > 'h') return false;
    if(uci[1] < '1' || uci[1] > '8') return false;
    if(uci[2] < 'a' || uci[2] > 'h') return false;
    if(uci[3] < '1' || uci[3] > '8') return false;

    const Square from{static_cast<Square>((uci[0] - 'a') + 8 * (uci[1] - '1'))};
    const Square to{static_cast<Square>((uci[2] - 'a') + 8 * (uci[3] - '1'))};
    const PieceType promo{(uci.size() >= 5) ? promoFromChar(uci[4]) : PieceType::None};
    const Color stm{position.turn()};

    MoveList legal{};
    movegen::generateLegalAll(position, legal);
    for(unsigned i{0}; i < legal.size; ++i){
        Move m{legal.moves[i]};
        if(m.from() != from) continue;
        bool toMatches{m.to() == to};
        if(!toMatches && m.isCastle()){
            const CastleDirection dir{
                (file_of(m.to()) == 6) ? CastleDirection::Kingside : CastleDirection::Queenside
            };
            if(position.castleRookSquare(stm, dir) == to) toMatches = true;
        }
        if(!toMatches) continue;
        if(m.isPromotion()){
            if(m.promotionPieceType() == promo){
                position.applyMove(m);
                return true;
            }
        } else if(promo == PieceType::None){
            position.applyMove(m);
            return true;
        }
    }
    return false;
}


void handlePosition(std::istringstream& ss, Position& position){
    std::string token;
    if(!(ss >> token)) return;

    if(token == "startpos"){
        position = Position{};
        if(!(ss >> token)) return;
    } else if(token == "fen"){
        std::string fen;
        std::string part;
        bool seenMoves{false};
        for(int i = 0; i < 6 && ss >> part; ++i){
            if(part == "moves"){ seenMoves = true; break; }
            if(!fen.empty()) fen += ' ';
            fen += part;
        }
        if(fen.empty()) return;
        position = Position{fen.c_str()};
        if(seenMoves){
            token = "moves";
        } else if(!(ss >> token)){
            return;
        }
    } else {
        return;
    }

    if(token != "moves") return;
    std::string moveStr;
    while(ss >> moveStr){
        if(!applyUciMove(position, moveStr)){
            std::cout << "info string illegal move " << moveStr << std::endl;
            break;
        }
    }
}


std::string uciOf(Move const& m, Position const& pos, bool chess960){
    Square to{m.to()};
    if(chess960 && m.isCastle()){
        const CastleDirection dir{
            (file_of(to) == 6) ? CastleDirection::Kingside : CastleDirection::Queenside
        };
        const Color c{(rank_of(m.from()) == 0) ? Color::White : Color::Black};
        to = pos.castleRookSquare(c, dir);
    }
    return moveUci(
        m.from(), to,
        m.isPromotion() ? m.promotionPieceType() : PieceType::None
    );
}


// Inverse of win% = 1 / (1 + exp(-magic * cp)), magic from https://lichess.org/page/accuracy.
int qToCp(float q){
    constexpr double magic{0.00368208};
    constexpr int kCap{1000};
    if(q <= 0.0f) return -kCap;
    if(q >= 1.0f) return kCap;
    double cp = -std::log(1.0 / static_cast<double>(q) - 1.0) / magic;
    if(cp > kCap) cp = kCap;
    if(cp < -kCap) cp = -kCap;
    return static_cast<int>(cp);
}


SearchLimits parseGoLimits(std::istringstream& ss){
    SearchLimits limits{};
    std::string token;
    while(ss >> token){
        if(token == "nodes"){
            ss >> limits.maxNodes;
        } else if(token == "depth"){
            ss >> limits.maxDepth;
        } else if(token == "movetime"){
            ss >> limits.movetimeMs;
        } else if(token == "infinite"){
            limits.infinite = true;
        } else if(token == "wtime" || token == "btime" ||
                  token == "winc"  || token == "binc"  ||
                  token == "movestogo" || token == "mate" ||
                  token == "searchmoves" || token == "ponder"){
            std::string dummy;
            ss >> dummy;
        }
    }
    if(limits.maxNodes == 0 && limits.maxDepth == 0 &&
       limits.movetimeMs == 0 && !limits.infinite){
        limits.infinite = true;
    }
    return limits;
}


void stopAndJoin(std::atomic<bool>& stopFlag, std::thread& worker){
    if(worker.joinable()){
        stopFlag.store(true, std::memory_order_relaxed);
        worker.join();
    }
}


void printProfile(std::ostream& os, const SearchProfile& p){
    const int64_t totalNs{p.selectNs + p.expandNs + p.nnNs + p.backpropNs};
    auto pct = [&](int64_t ns) -> double {
        return (totalNs <= 0) ? 0.0 : (100.0 * static_cast<double>(ns) / static_cast<double>(totalNs));
    };

    os << "info string profile"
       << " iters=" << p.iterations
       << " select_ms=" << (p.selectNs / 1'000'000)
       << " expand_ms=" << (p.expandNs / 1'000'000)
       << " nn_ms=" << (p.nnNs / 1'000'000)
       << " backprop_ms=" << (p.backpropNs / 1'000'000)
       << " select_pct=" << pct(p.selectNs)
       << " expand_pct=" << pct(p.expandNs)
       << " nn_pct=" << pct(p.nnNs)
       << " backprop_pct=" << pct(p.backpropNs)
       << " nn_calls=" << p.nnCalls
       << " expand_races=" << p.expandRaces
       << std::endl;
}


void runBenchNn(std::ostream& os){
    constexpr std::array<unsigned, 8> kBatchSizes{1, 2, 4, 8, 16, 32, 64, 128};
    constexpr unsigned kWarmup = 8;
    constexpr unsigned kIters  = 200;

    Position startPos{};
    const nn::ModelInput sample{nn::encode_board(startPos)};

    std::vector<nn::ModelInput> batch;
    for(unsigned N : kBatchSizes){
        batch.assign(N, sample);

        for(unsigned w{0}; w < kWarmup; ++w){
            auto outs = nn::Evaluator::instance().evaluate_batch(batch.data(), batch.size());
            (void)outs;
        }

        const auto t0 = std::chrono::steady_clock::now();
        for(unsigned i{0}; i < kIters; ++i){
            auto outs = nn::Evaluator::instance().evaluate_batch(batch.data(), batch.size());
            (void)outs;
        }
        const auto t1 = std::chrono::steady_clock::now();

        const int64_t elapsedUs{
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()
        };
        const double msPerBatch{
            static_cast<double>(elapsedUs) / (static_cast<double>(kIters) * 1000.0)
        };
        const double evalsPerSec{
            (elapsedUs <= 0) ? 0.0
                : (static_cast<double>(N) * kIters * 1'000'000.0 / static_cast<double>(elapsedUs))
        };
        os << "info string bench_nn batch=" << N
           << " ms_per_batch=" << msPerBatch
           << " evals_per_sec=" << static_cast<int64_t>(evalsPerSec)
           << std::endl;
    }
}


void spawnSearch(Position& position, Search& search,
                 const SearchLimits& limits,
                 std::atomic<bool>& stopFlag, std::thread& worker,
                 std::mutex& coutMutex,
                 bool chess960){
    stopFlag.store(false, std::memory_order_relaxed);
    worker = std::thread([&, limits, chess960]{
        auto onInfo = [&](const SearchStats& s, const std::vector<Move>& pv){
            std::lock_guard<std::mutex> lk{coutMutex};
            std::cout << "info"
                      << " depth " << s.avgDepth
                      << " seldepth " << s.maxDepth
                      << " nodes " << s.nodes
                      << " nps " << s.nps()
                      << " time " << s.elapsedMs
                      << " score cp " << qToCp(s.rootQ);
            if(!pv.empty()){
                std::cout << " pv";
                for(const Move& m : pv) std::cout << ' ' << uciOf(m, position, chess960);
            }
            std::cout << std::endl;
        };

        Search::Result r{search.search(position, limits, stopFlag, onInfo)};

        std::lock_guard<std::mutex> lk{coutMutex};
        #ifndef NDEBUG
        printProfile(std::cout, search.profile());
        #endif
        std::cout << "bestmove " << (r.best.bits ? uciOf(r.best, position, chess960) : std::string{"0000"});
        if(r.ponder.bits){
            std::cout << " ponder " << uciOf(r.ponder, position, chess960);
        }
        std::cout << std::endl;
    });
}


void handleSetOption(std::istringstream& ss,
                     Search& search,
                     unsigned& hashSizeMb,
                     unsigned& numThreads,
                     unsigned& batchSize,
                     bool& chess960){
    std::string token;
    if(!(ss >> token) || token != "name") return;

    std::string name;
    std::string value;
    bool readingValue{false};
    while(ss >> token){
        if(!readingValue && token == "value"){
            readingValue = true;
            continue;
        }
        std::string& target = readingValue ? value : name;
        if(!target.empty()) target += ' ';
        target += token;
    }

    const std::string nameLower{toLower(name)};
    if(nameLower == "hash"){
        try{
            long long mb{std::stoll(value)};
            if(mb < static_cast<long long>(kHashMinMb)) mb = kHashMinMb;
            if(mb > static_cast<long long>(kHashMaxMb)) mb = kHashMaxMb;
            hashSizeMb = static_cast<unsigned>(mb);
            nn::Evaluator::instance().setCacheSizeMb(hashSizeMb);
        } catch(...){}
    } else if(nameLower == "threads"){
        try{
            long long n{std::stoll(value)};
            if(n < 1) n = 1;
            if(n > static_cast<long long>(kThreadsMax)) n = kThreadsMax;
            numThreads = static_cast<unsigned>(n);
            search.setNumThreads(numThreads);
        } catch(...){}
    } else if(nameLower == "batchsize"){
        try{
            long long n{std::stoll(value)};
            if(n < static_cast<long long>(kBatchSizeMin)) n = kBatchSizeMin;
            if(n > static_cast<long long>(kBatchSizeMax)) n = kBatchSizeMax;
            batchSize = static_cast<unsigned>(n);
            nn::Evaluator::instance().setBatchSize(batchSize);
        } catch(...){}
    } else if(nameLower == "uci_chess960"){
        const std::string v{toLower(value)};
        chess960 = (v == "true" || v == "1" || v == "on");
    }
}


void handleGo(std::istringstream& ss, Position& position, Search& search,
              std::atomic<bool>& stopFlag, std::thread& worker,
              std::mutex& coutMutex,
              bool chess960){
    auto savedPos = ss.tellg();
    std::string first;
    if(ss >> first && first == "perft"){
        int depth{0};
        if(ss >> depth){
            perft::PerftResult result{run_perft(position, depth)};
            std::lock_guard<std::mutex> lk{coutMutex};
            std::cout << "Nodes searched: " << result.nodes << std::endl;
        }
        return;
    }
    ss.clear();
    ss.seekg(savedPos);

    SearchLimits limits{parseGoLimits(ss)};
    spawnSearch(position, search, limits, stopFlag, worker, coutMutex, chess960);
}

}


void run_uci(){
    Position position{};
    Search search{};
    std::atomic<bool> stopFlag{false};
    std::thread worker;
    std::mutex coutMutex;

    unsigned hashSizeMb{kHashDefaultMb};
    unsigned numThreads{1};
    unsigned batchSize{kBatchSizeDefault};
    bool chess960{false};
    nn::Evaluator::instance().setCacheSizeMb(hashSizeMb);
    search.setNumThreads(numThreads);
    nn::Evaluator::instance().setBatchSize(batchSize);

    std::string line;
    while(std::getline(std::cin, line)){
        std::istringstream ss{line};
        std::string command;
        if(!(ss >> command)) continue;

        if(command == "uci"){
            std::lock_guard<std::mutex> lk{coutMutex};
            std::cout << "id name bumblebot" << std::endl;
            std::cout << "id author groloch" << std::endl;
            std::cout << "option name Hash type spin default " << kHashDefaultMb
                      << " min " << kHashMinMb << " max " << kHashMaxMb << std::endl;
            std::cout << "option name Threads type spin default 1 min 1 max "
                      << kThreadsMax << std::endl;
            std::cout << "option name BatchSize type spin default " << kBatchSizeDefault
                      << " min " << kBatchSizeMin << " max " << kBatchSizeMax << std::endl;
            std::cout << "option name UCI_Chess960 type check default false" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if(command == "isready"){
            std::lock_guard<std::mutex> lk{coutMutex};
            std::cout << "readyok" << std::endl;
        } else if(command == "setoption"){
            stopAndJoin(stopFlag, worker);
            handleSetOption(ss, search, hashSizeMb, numThreads, batchSize, chess960);
        } else if(command == "ucinewgame"){
            stopAndJoin(stopFlag, worker);
            nn::Evaluator::instance().clearCache();
            position = Position{};
        } else if(command == "position"){
            stopAndJoin(stopFlag, worker);
            handlePosition(ss, position);
        } else if(command == "go"){
            stopAndJoin(stopFlag, worker);
            handleGo(ss, position, search, stopFlag, worker, coutMutex, chess960);
        } else if(command == "stop"){
            stopAndJoin(stopFlag, worker);
        } else if(command == "perft"){
            stopAndJoin(stopFlag, worker);
            run_perft_tests();
        } else if(command == "bench"){
            stopAndJoin(stopFlag, worker);
            std::string what;
            if(ss >> what && what == "nn"){
                std::lock_guard<std::mutex> lk{coutMutex};
                runBenchNn(std::cout);
            }
        } else if(command == "quit"){
            stopAndJoin(stopFlag, worker);
            break;
        }
    }

    stopAndJoin(stopFlag, worker);
}
