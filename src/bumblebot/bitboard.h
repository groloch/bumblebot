# ifndef BITBOARD_H
# define BITBOARD_H

# include <array>

# include "types.h"


struct Magic {
    Magic(Bitboard mask, Bitboard magic, unsigned shift)
        : mask{mask},
          magic{magic},
          attack_table{nullptr},
          shift{shift}
    {}

    Bitboard mask;
    u64 magic;
    Bitboard* attack_table;
    unsigned shift;

    unsigned index(Bitboard occupancy) const {
        return (occupancy & mask) * magic >> shift;
    }

    Bitboard attacks_bb(Bitboard occupancy) const {
        return attack_table[index(occupancy)];
    }
};

void init();


template<PieceType T>
Bitboard inBetween(Square from, Square to);


constexpr Bitboard bitboard_of(Square square){
    return 1ULL << square;
}

constexpr bool more_than_one(Bitboard b){
    return b & (b - 1);
}

constexpr bool exactly_one(Bitboard b){
    return b && !more_than_one(b);
}

inline constexpr unsigned popcount(Bitboard b){
    return __builtin_popcountll(b);
}
inline constexpr unsigned lsb(Bitboard b){
    return __builtin_ctzll(b);
}
inline constexpr unsigned poplsb(Bitboard& b){
    unsigned index{lsb(b)};
    b &= b - 1;
    return index;
}

constexpr Square square_of(Bitboard b){
    return lsb(b);
}


namespace bitboards{
constexpr Bitboard bb_a1 = bitboard_of(squares::a1);
constexpr Bitboard bb_b1 = bitboard_of(squares::b1);
constexpr Bitboard bb_c1 = bitboard_of(squares::c1);
constexpr Bitboard bb_d1 = bitboard_of(squares::d1);
constexpr Bitboard bb_e1 = bitboard_of(squares::e1);
constexpr Bitboard bb_f1 = bitboard_of(squares::f1);
constexpr Bitboard bb_g1 = bitboard_of(squares::g1);
constexpr Bitboard bb_h1 = bitboard_of(squares::h1);

constexpr Bitboard bb_a2 = bitboard_of(squares::a2);
constexpr Bitboard bb_b2 = bitboard_of(squares::b2);
constexpr Bitboard bb_c2 = bitboard_of(squares::c2);
constexpr Bitboard bb_d2 = bitboard_of(squares::d2);
constexpr Bitboard bb_e2 = bitboard_of(squares::e2);
constexpr Bitboard bb_f2 = bitboard_of(squares::f2);
constexpr Bitboard bb_g2 = bitboard_of(squares::g2);
constexpr Bitboard bb_h2 = bitboard_of(squares::h2);

constexpr Bitboard bb_a3 = bitboard_of(squares::a3);
constexpr Bitboard bb_b3 = bitboard_of(squares::b3);
constexpr Bitboard bb_c3 = bitboard_of(squares::c3);
constexpr Bitboard bb_d3 = bitboard_of(squares::d3);
constexpr Bitboard bb_e3 = bitboard_of(squares::e3);
constexpr Bitboard bb_f3 = bitboard_of(squares::f3);
constexpr Bitboard bb_g3 = bitboard_of(squares::g3);
constexpr Bitboard bb_h3 = bitboard_of(squares::h3);

constexpr Bitboard bb_a4 = bitboard_of(squares::a4);
constexpr Bitboard bb_b4 = bitboard_of(squares::b4);
constexpr Bitboard bb_c4 = bitboard_of(squares::c4);
constexpr Bitboard bb_d4 = bitboard_of(squares::d4);
constexpr Bitboard bb_e4 = bitboard_of(squares::e4);
constexpr Bitboard bb_f4 = bitboard_of(squares::f4);
constexpr Bitboard bb_g4 = bitboard_of(squares::g4);
constexpr Bitboard bb_h4 = bitboard_of(squares::h4);

constexpr Bitboard bb_a5 = bitboard_of(squares::a5);
constexpr Bitboard bb_b5 = bitboard_of(squares::b5);
constexpr Bitboard bb_c5 = bitboard_of(squares::c5);
constexpr Bitboard bb_d5 = bitboard_of(squares::d5);
constexpr Bitboard bb_e5 = bitboard_of(squares::e5);
constexpr Bitboard bb_f5 = bitboard_of(squares::f5);
constexpr Bitboard bb_g5 = bitboard_of(squares::g5);
constexpr Bitboard bb_h5 = bitboard_of(squares::h5);

constexpr Bitboard bb_a6 = bitboard_of(squares::a6);
constexpr Bitboard bb_b6 = bitboard_of(squares::b6);
constexpr Bitboard bb_c6 = bitboard_of(squares::c6);
constexpr Bitboard bb_d6 = bitboard_of(squares::d6);
constexpr Bitboard bb_e6 = bitboard_of(squares::e6);
constexpr Bitboard bb_f6 = bitboard_of(squares::f6);
constexpr Bitboard bb_g6 = bitboard_of(squares::g6);
constexpr Bitboard bb_h6 = bitboard_of(squares::h6);

constexpr Bitboard bb_a7 = bitboard_of(squares::a7);
constexpr Bitboard bb_b7 = bitboard_of(squares::b7);
constexpr Bitboard bb_c7 = bitboard_of(squares::c7);
constexpr Bitboard bb_d7 = bitboard_of(squares::d7);
constexpr Bitboard bb_e7 = bitboard_of(squares::e7);
constexpr Bitboard bb_f7 = bitboard_of(squares::f7);
constexpr Bitboard bb_g7 = bitboard_of(squares::g7);
constexpr Bitboard bb_h7 = bitboard_of(squares::h7);

constexpr Bitboard bb_a8 = bitboard_of(squares::a8);
constexpr Bitboard bb_b8 = bitboard_of(squares::b8);
constexpr Bitboard bb_c8 = bitboard_of(squares::c8);
constexpr Bitboard bb_d8 = bitboard_of(squares::d8);
constexpr Bitboard bb_e8 = bitboard_of(squares::e8);
constexpr Bitboard bb_f8 = bitboard_of(squares::f8);
constexpr Bitboard bb_g8 = bitboard_of(squares::g8);
constexpr Bitboard bb_h8 = bitboard_of(squares::h8);

constexpr Bitboard bb_fileA = bb_a1 | bb_a2 | bb_a3 | bb_a4 | bb_a5 | bb_a6 | bb_a7 | bb_a8;
constexpr Bitboard bb_fileB = bb_fileA << 1;
constexpr Bitboard bb_fileC = bb_fileA << 2;
constexpr Bitboard bb_fileD = bb_fileA << 3;
constexpr Bitboard bb_fileE = bb_fileA << 4;
constexpr Bitboard bb_fileF = bb_fileA << 5;
constexpr Bitboard bb_fileG = bb_fileA << 6;
constexpr Bitboard bb_fileH = bb_fileA << 7;

constexpr Bitboard bb_rank1 = bb_a1 | bb_b1 | bb_c1 | bb_d1 | bb_e1 | bb_f1 | bb_g1 | bb_h1;
constexpr Bitboard bb_rank2 = bb_rank1 << (8 * 1);
constexpr Bitboard bb_rank3 = bb_rank1 << (8 * 2);
constexpr Bitboard bb_rank4 = bb_rank1 << (8 * 3);
constexpr Bitboard bb_rank5 = bb_rank1 << (8 * 4);
constexpr Bitboard bb_rank6 = bb_rank1 << (8 * 5);
constexpr Bitboard bb_rank7 = bb_rank1 << (8 * 6);
constexpr Bitboard bb_rank8 = bb_rank1 << (8 * 7);

constexpr Bitboard empty = 0ull;
constexpr Bitboard full = ~empty;
}

template<Direction D>
constexpr Bitboard shift(Bitboard b){
    if constexpr (D == Direction::North) {
        return (b & ~bitboards::bb_rank8) << 8;
    } else if constexpr (D == Direction::South) {
        return (b & ~bitboards::bb_rank1) >> 8;
    } else if constexpr (D == Direction::East) {
        return (b & ~bitboards::bb_fileH) << 1;
    } else if constexpr (D == Direction::West) {
        return (b & ~bitboards::bb_fileA) >> 1;
    } else if constexpr (D == Direction::NorthEast) {
        return (b & ~bitboards::bb_fileH & ~bitboards::bb_rank8) << 9;
    } else if constexpr (D == Direction::NorthWest) {
        return (b & ~bitboards::bb_fileA & ~bitboards::bb_rank8) << 7;
    } else if constexpr (D == Direction::SouthEast) {
        return (b & ~bitboards::bb_fileH & ~bitboards::bb_rank1) >> 7;
    } else if constexpr (D == Direction::SouthWest) {
        return (b & ~bitboards::bb_fileA & ~bitboards::bb_rank1) >> 9;
    }
}

constexpr Bitboard bitboard_of_rank(Rank rank){
    return bitboards::bb_rank1 << (rank * 8);
}


extern std::array<Magic, 64> rookMagics;
extern std::array<Magic, 64> bishopMagics;


extern std::array<Bitboard, 64> pawnAttacks[2];
extern std::array<Bitboard, 64> knightAttacks;
extern std::array<Bitboard, 64> kingAttacks;

# endif
