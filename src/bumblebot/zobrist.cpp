# include "zobrist.h"


namespace zobrist {

std::array<std::array<Hash, 64>, 12> pieceSquare{};
std::array<Hash, 4> castle{};
std::array<Hash, 8> enPassantFile{};
Hash sideToMoveBlack{0};

unsigned pieceIndex(Color color, PieceType pieceType){
    unsigned ptIdx{0};
    switch(pieceType){
        case PieceType::Pawn:   ptIdx = 0; break;
        case PieceType::Knight: ptIdx = 1; break;
        case PieceType::Bishop: ptIdx = 2; break;
        case PieceType::Rook:   ptIdx = 3; break;
        case PieceType::Queen:  ptIdx = 4; break;
        case PieceType::King:   ptIdx = 5; break;
        default:                ptIdx = 0; break;
    }
    return (color == Color::White ? 0u : 6u) + ptIdx;
}

}


namespace {

u64 rngState = 0x9E3779B97F4A7C15ULL;

u64 nextRandom(){
    u64 x{rngState};
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rngState = x;
    return x;
}

}


void init_zobrist(){
    rngState = 0x9E3779B97F4A7C15ULL;

    for(unsigned p{0}; p < 12; ++p){
        for(unsigned s{0}; s < 64; ++s){
            zobrist::pieceSquare[p][s] = nextRandom();
        }
    }
    for(unsigned i{0}; i < 4; ++i){
        zobrist::castle[i] = nextRandom();
    }
    for(unsigned i{0}; i < 8; ++i){
        zobrist::enPassantFile[i] = nextRandom();
    }
    zobrist::sideToMoveBlack = nextRandom();
}
