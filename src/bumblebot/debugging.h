# ifndef DEBUGGING_H
# define DEBUGGING_H

#include <iostream>
#include <string>

#include "types.h"
#include "bitboard.h"


#define assert(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << (message) << std::endl; \
            std::abort(); \
        } \
    } while (false)


inline void printBitboard(Bitboard b){
    for(int rank{7}; rank >= 0; --rank){
        for(File file{0}; file <= 7; ++file){
            Square square{rank * 8 + file};
            std::cout << ((b & bitboard_of(square)) ? "1 " : "0 ");
        }
        std::cout << std::endl;
    }
}

inline std::string moveUci(Square from, Square to, PieceType promotionPieceType = PieceType::None){
    std::string result = "";
    result += char('a' + file_of(from));
    result += char('1' + rank_of(from));
    result += char('a' + file_of(to));
    result += char('1' + rank_of(to));
    if(promotionPieceType != PieceType::None){
        result += static_cast<char>(promotionPieceType);
    }
    return result;
}

inline void comparePositions(Position const& pos1, Position const& pos2){
    std::cout << "all " << (pos1.allPieces == pos2.allPieces) << std::endl;
    std::cout << "white " << (pos1.whitePieces == pos2.whitePieces) << std::endl;
    std::cout << "black " << (pos1.blackPieces == pos2.blackPieces) << std::endl;

    std::cout << "white kings " << (pos1.whiteKings == pos2.whiteKings) << std::endl;
    std::cout << "white queens " << (pos1.whiteQueens == pos2.whiteQueens) << std::endl;
    std::cout << "white rooks " << (pos1.whiteRooks == pos2.whiteRooks) << std::endl;
    std::cout << "white bishops " << (pos1.whiteBishops == pos2.whiteBishops) << std::endl;
    std::cout << "white knights " << (pos1.whiteKnights == pos2.whiteKnights) << std::endl;
    std::cout << "white pawns " << (pos1.whitePawns == pos2.whitePawns) << std::endl;
    
    std::cout << "black kings " << (pos1.blackKings == pos2.blackKings) << std::endl;
    std::cout << "black queens " << (pos1.blackQueens == pos2.blackQueens) << std::endl;
    std::cout << "black rooks " << (pos1.blackRooks == pos2.blackRooks) << std::endl;
    std::cout << "black bishops " << (pos1.blackBishops == pos2.blackBishops) << std::endl;
    std::cout << "black knights " << (pos1.blackKnights == pos2.blackKnights) << std::endl;
    std::cout << "black pawns " << (pos1.blackPawns == pos2.blackPawns) << std::endl;
}

#endif
