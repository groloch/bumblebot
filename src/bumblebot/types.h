# ifndef TYPES_H
# define TYPES_H


using u64 = unsigned long long;
using Bitboard = u64;

using Square = unsigned;
using File = unsigned;
using Rank = unsigned;


constexpr File file_of(Square square){
    return square % 8;
}

constexpr Rank rank_of(Square square){
    return square / 8;
}

constexpr Square square_of(File file, Rank rank){
    return rank * 8 + file;
}


enum class Color {
    White = 'w',
    Black = 'b',
    None = ' '
};

enum class PieceType: char {
    Pawn = 'p',
    Knight = 'n',
    Bishop = 'b',
    Rook = 'r',
    Queen = 'q',
    King = 'k',
    None = ' '
};

enum class Direction: int{
    North = 8,
    South = -8,
    East = 1,
    West = -1,
    NorthEast = North + East,
    NorthWest = North + West,
    SouthEast = South + East,
    SouthWest = South + West,
    None = 0
};

struct SquareContent{
    SquareContent() : color{Color::None}, pieceType{PieceType::None} {}
    SquareContent(Color color, PieceType pieceType) : color{color}, pieceType{pieceType} {}

    Color color;
    PieceType pieceType;
};

struct CastleRights {
    CastleRights(): whiteKingside{true}, whiteQueenside{true}, blackKingside{true}, blackQueenside{true} {}
    bool whiteKingside;
    bool whiteQueenside;
    bool blackKingside;
    bool blackQueenside;
};

enum class CastleDirection {
    Kingside,
    Queenside
};

enum class MoveType {
    Escape,
    Legal,
    PseudoLegal
};

inline Direction& operator+=(Direction& lhs, Direction rhs){
    lhs = static_cast<Direction>(static_cast<int>(lhs) + static_cast<int>(rhs));
    return lhs;
}

constexpr Direction direction_of(Square from, Square to){
    Rank rankFrom{rank_of(from)};
    File fileFrom{file_of(from)};

    Rank rankTo{rank_of(to)};
    File fileTo{file_of(to)};

    Direction dir{Direction::None};

    if(rankFrom < rankTo) dir += Direction::North;
    else if(rankFrom > rankTo) dir += Direction::South;

    if(fileFrom < fileTo) dir += Direction::East;
    else if(fileFrom > fileTo) dir += Direction::West;

    return dir;
}


namespace squares{
constexpr Square NoneSquare = 64;

constexpr Square a1 = 0;
constexpr Square b1 = 1;
constexpr Square c1 = 2;
constexpr Square d1 = 3;
constexpr Square e1 = 4;
constexpr Square f1 = 5;
constexpr Square g1 = 6;
constexpr Square h1 = 7;

constexpr Square a2 = 8;
constexpr Square b2 = 9;
constexpr Square c2 = 10;
constexpr Square d2 = 11;
constexpr Square e2 = 12;
constexpr Square f2 = 13;
constexpr Square g2 = 14;
constexpr Square h2 = 15;

constexpr Square a3 = 16;
constexpr Square b3 = 17;
constexpr Square c3 = 18;
constexpr Square d3 = 19;
constexpr Square e3 = 20;
constexpr Square f3 = 21;
constexpr Square g3 = 22;
constexpr Square h3 = 23;

constexpr Square a4 = 24;
constexpr Square b4 = 25;
constexpr Square c4 = 26;
constexpr Square d4 = 27;
constexpr Square e4 = 28;
constexpr Square f4 = 29;
constexpr Square g4 = 30;
constexpr Square h4 = 31;

constexpr Square a5 = 32;
constexpr Square b5 = 33;
constexpr Square c5 = 34;
constexpr Square d5 = 35;
constexpr Square e5 = 36;
constexpr Square f5 = 37;
constexpr Square g5 = 38;
constexpr Square h5 = 39;

constexpr Square a6 = 40;
constexpr Square b6 = 41;
constexpr Square c6 = 42;
constexpr Square d6 = 43;
constexpr Square e6 = 44;
constexpr Square f6 = 45;
constexpr Square g6 = 46;
constexpr Square h6 = 47;

constexpr Square a7 = 48;
constexpr Square b7 = 49;
constexpr Square c7 = 50;
constexpr Square d7 = 51;
constexpr Square e7 = 52;
constexpr Square f7 = 53;
constexpr Square g7 = 54;
constexpr Square h7 = 55;

constexpr Square a8 = 56;
constexpr Square b8 = 57;
constexpr Square c8 = 58;
constexpr Square d8 = 59;
constexpr Square e8 = 60;
constexpr Square f8 = 61;
constexpr Square g8 = 62;
constexpr Square h8 = 63;
}
namespace ranks{
constexpr Rank r1 = 0;
constexpr Rank r2 = 1;
constexpr Rank r3 = 2;
constexpr Rank r4 = 3;
constexpr Rank r5 = 4;
constexpr Rank r6 = 5;
constexpr Rank r7 = 6;
constexpr Rank r8 = 7;
}

# endif
