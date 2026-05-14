# include "uci.h"

# include <iostream>
# include <sstream>
# include <string>

# include "types.h"
# include "move.h"
# include "movegen.h"
# include "perft.h"
# include "position.h"
# include "debugging.h"
# include "search/search.h"


namespace {

constexpr unsigned DEFAULT_ITERATIONS = 800;


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
        // Reassemble the 6-token FEN, stopping if we hit "moves".
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


void handleGo(std::istringstream& ss, Position& position, Search& search){
    std::string token;
    unsigned iterations{DEFAULT_ITERATIONS};

    while(ss >> token){
        if(token == "perft"){
            int depth{0};
            if(ss >> depth){
                perft::PerftResult result{run_perft(position, depth)};
                std::cout << "Nodes searched: " << result.nodes << std::endl;
            }
            return;
        } else if(token == "nodes"){
            ss >> iterations;
        }
        // Other params (wtime, btime, depth, movetime, infinite, ...) are
        // ignored for now; we always run `iterations` PUCT iterations.
    }

    Move best{search.search(position, iterations)};
    if(best.bits == 0){
        std::cout << "bestmove 0000" << std::endl;
    } else {
        std::cout << "bestmove " << uciOf(best) << std::endl;
    }
}

}


void run_uci(){
    Position position{};
    Search search{};

    std::string line;
    while(std::getline(std::cin, line)){
        std::istringstream ss{line};
        std::string command;
        if(!(ss >> command)) continue;

        if(command == "uci"){
            std::cout << "id name bumblebot" << std::endl;
            std::cout << "id author baptiste" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if(command == "isready"){
            std::cout << "readyok" << std::endl;
        } else if(command == "ucinewgame"){
            position = Position{};
            search   = Search{};
        } else if(command == "position"){
            handlePosition(ss, position);
        } else if(command == "go"){
            handleGo(ss, position, search);
        } else if(command == "perft"){
            run_perft_tests();
        } else if(command == "quit"){
            break;
        }
    }
}
