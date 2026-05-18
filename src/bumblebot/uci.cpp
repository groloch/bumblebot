# include "uci.h"

# include <algorithm>
# include <atomic>
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
# include "search/search.h"


namespace {

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
    const Square to  {static_cast<Square>((uci[2] - 'a') + 8 * (uci[3] - '1'))};
    const PieceType promo{(uci.size() >= 5) ? promoFromChar(uci[4]) : PieceType::None};

    MoveList legal{};
    movegen::generateLegalAll(position, legal);
    for(unsigned i{0}; i < legal.size; ++i){
        Move m{legal.moves[i]};
        if(m.from() != from || m.to() != to) continue;
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


std::string uciOf(Move const& m){
    return moveUci(
        m.from(), m.to(),
        m.isPromotion() ? m.promotionPieceType() : PieceType::None
    );
}


// Win% -> centipawns, from https://lichess.org/page/accuracy
// Inverse of  win% = 1 / (1 + exp(-magic * cp))  with magic = 0.00368208.
int qToCp(float q){
    constexpr double magic{0.00368208};
    constexpr int kCap{1000};
    if(q <= 0.0f) return -kCap;
    if(q >= 1.0f) return  kCap;
    double cp = -std::log(1.0 / static_cast<double>(q) - 1.0) / magic;
    if(cp >  kCap) cp =  kCap;
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


void spawnSearch(Position& position, Search& search,
                 const SearchLimits& limits,
                 std::atomic<bool>& stopFlag, std::thread& worker,
                 std::mutex& coutMutex){
    stopFlag.store(false, std::memory_order_relaxed);
    worker = std::thread([&, limits]{
        auto onInfo = [&](const SearchStats& s, const std::vector<Move>& pv){
            std::lock_guard<std::mutex> lk{coutMutex};
            std::cout << "info"
                      << " depth "    << s.avgDepth
                      << " seldepth " << s.maxDepth
                      << " nodes "    << s.nodes
                      << " nps "      << s.nps()
                      << " time "     << s.elapsedMs
                      << " score cp " << qToCp(s.rootQ);
            if(!pv.empty()){
                std::cout << " pv";
                for(const Move& m : pv) std::cout << ' ' << uciOf(m);
            }
            std::cout << std::endl;
        };

        Search::Result r{search.search(position, limits, stopFlag, onInfo)};

        std::lock_guard<std::mutex> lk{coutMutex};
        std::cout << "bestmove " << (r.best.bits ? uciOf(r.best) : std::string{"0000"});
        if(r.ponder.bits){
            std::cout << " ponder " << uciOf(r.ponder);
        }
        std::cout << std::endl;
    });
}


void handleGo(std::istringstream& ss, Position& position, Search& search,
              std::atomic<bool>& stopFlag, std::thread& worker,
              std::mutex& coutMutex){
    // Peek for `perft N` short-circuit before consuming the rest as go params.
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
    spawnSearch(position, search, limits, stopFlag, worker, coutMutex);
}

}


void run_uci(){
    Position position{};
    Search search{};
    std::atomic<bool> stopFlag{false};
    std::thread worker;
    std::mutex coutMutex;

    std::string line;
    while(std::getline(std::cin, line)){
        std::istringstream ss{line};
        std::string command;
        if(!(ss >> command)) continue;

        if(command == "uci"){
            std::lock_guard<std::mutex> lk{coutMutex};
            std::cout << "id name bumblebot" << std::endl;
            std::cout << "id author baptiste" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if(command == "isready"){
            std::lock_guard<std::mutex> lk{coutMutex};
            std::cout << "readyok" << std::endl;
        } else if(command == "ucinewgame"){
            stopAndJoin(stopFlag, worker);
            position = Position{};
            search   = Search{};
        } else if(command == "position"){
            stopAndJoin(stopFlag, worker);
            handlePosition(ss, position);
        } else if(command == "go"){
            stopAndJoin(stopFlag, worker);
            handleGo(ss, position, search, stopFlag, worker, coutMutex);
        } else if(command == "stop"){
            stopAndJoin(stopFlag, worker);
        } else if(command == "perft"){
            stopAndJoin(stopFlag, worker);
            run_perft_tests();
        } else if(command == "quit"){
            stopAndJoin(stopFlag, worker);
            break;
        }
    }

    stopAndJoin(stopFlag, worker);
}
