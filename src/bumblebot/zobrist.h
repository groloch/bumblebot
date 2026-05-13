# ifndef ZOBRIST_H
# define ZOBRIST_H

# include <array>

# include "types.h"


namespace zobrist {

extern std::array<std::array<Hash, 64>, 12> pieceSquare;

extern std::array<Hash, 4> castle;

extern std::array<Hash, 8> enPassantFile;

extern Hash sideToMoveBlack;

unsigned pieceIndex(Color color, PieceType pieceType);

inline Hash pieceKey(Color color, PieceType pieceType, Square square){
    return pieceSquare[pieceIndex(color, pieceType)][square];
}

}

void init_zobrist();


# endif
