# include "bitboard.h"
# include "zobrist.h"

#include <iostream>

std::array<Bitboard, 64> pawnAttacks[2];
std::array<Bitboard, 64> knightAttacks;
std::array<Bitboard, 64> kingAttacks;

static void init_leaper_attacks(){
    for(Square sq{squares::a1}; sq <= squares::h8; ++sq){
        Bitboard b{bitboard_of(sq)};

        pawnAttacks[0][sq] = shift<Direction::NorthEast>(b) | shift<Direction::NorthWest>(b);
        pawnAttacks[1][sq] = shift<Direction::SouthEast>(b) | shift<Direction::SouthWest>(b);

        knightAttacks[sq] =
            shift<Direction::NorthEast>(shift<Direction::North>(b)) |
            shift<Direction::NorthEast>(shift<Direction::East>(b)) |
            shift<Direction::NorthWest>(shift<Direction::North>(b)) |
            shift<Direction::NorthWest>(shift<Direction::West>(b)) |
            shift<Direction::SouthEast>(shift<Direction::South>(b)) |
            shift<Direction::SouthEast>(shift<Direction::East>(b)) |
            shift<Direction::SouthWest>(shift<Direction::South>(b)) |
            shift<Direction::SouthWest>(shift<Direction::West>(b));

        kingAttacks[sq] =
            shift<Direction::North>(b) | shift<Direction::South>(b) |
            shift<Direction::East>(b)  | shift<Direction::West>(b)  |
            shift<Direction::NorthEast>(b) | shift<Direction::NorthWest>(b) |
            shift<Direction::SouthEast>(b) | shift<Direction::SouthWest>(b);
    }
}

void init_magics(){
    // rook lookup tables
    for (Square square{squares::a1}; square <= squares::h8; ++square){
        File file{file_of(square)};
        Rank rank{rank_of(square)};

        Magic& rook_magic = rookMagics[square];
        rook_magic.attack_table = new Bitboard[1ULL << (64 - rook_magic.shift)];
        
        Bitboard occupancy{rook_magic.mask};
        while(true){
            unsigned idx{rook_magic.index(occupancy)};

            Bitboard attacks{0};

            for(Rank r{rank + 1}; r <= 7; ++r){
                attacks |= bitboard_of(square_of(file, r));
                if (occupancy & bitboard_of(square_of(file, r))) break;
            }
            // need to cast to avoid overflow when file = 0 and file -= 1
            for(int r = rank - 1; r >= 0; --r){
                attacks |= bitboard_of(square_of(file, r));
                if (occupancy & bitboard_of(square_of(file, r))) break;
            }
            for(File f{file + 1}; f <= 7; ++f){
                attacks |= bitboard_of(square_of(f, rank));
                if (occupancy & bitboard_of(square_of(f, rank))) break;
            }
            for(int f = file - 1; f >= 0; --f){
                attacks |= bitboard_of(square_of(f, rank));
                if (occupancy & bitboard_of(square_of(f, rank))) break;
            }

            rook_magic.attack_table[idx] = attacks;

            if (occupancy == 0) break;
            occupancy = (occupancy - 1) & rook_magic.mask;
        }

        // bishop lookup tables
        Magic& bishop_magic = bishopMagics[square];
        bishop_magic.attack_table = new Bitboard[1ULL << (64 - bishop_magic.shift)];
        
        occupancy = bishop_magic.mask;
        while(true){
            unsigned idx{bishop_magic.index(occupancy)};

            Bitboard attacks{0};


            Rank r{rank + 1};
            File f{file + 1};
            for(; r <= 7 && f <= 7; ++r, ++f){
                attacks |= bitboard_of(square_of(f, r));
                if (occupancy & bitboard_of(square_of(f, r))) break;
            }

            int r_ = rank - 1;
            f = file + 1;
            for(; r_ >= 0 && f <= 7; --r_, ++f){
                attacks |= bitboard_of(square_of(f, r_));
                if (occupancy & bitboard_of(square_of(f, r_))) break;
            }

            r = rank + 1;
            int f_ = file - 1;
            for(; r <= 7 && f_ >= 0; ++r, --f_){
                attacks |= bitboard_of(square_of(f_, r));
                if (occupancy & bitboard_of(square_of(f_, r))) break;
            }

            r_ = rank - 1;
            f_ = file - 1;
            for(; r_ >= 0 && f_ >= 0; --r_, --f_){
                attacks |= bitboard_of(square_of(f_, r_));
                if (occupancy & bitboard_of(square_of(f_, r_))) break;
            }

            bishop_magic.attack_table[idx] = attacks;

            if (occupancy == 0) break;
            occupancy = (occupancy - 1) & bishop_magic.mask;
        }
    }
}

void init(){
    init_leaper_attacks();
    init_magics();
    init_zobrist();
}


template<>
Bitboard inBetween<PieceType::Rook>(Square from, Square to){
    Bitboard from_bb{bitboard_of(from)};
    Bitboard to_bb{bitboard_of(to)};

    Bitboard occupancy{to_bb};

    Bitboard additionalOccupancy{0};
    Direction dir{direction_of(from, to)};
    if(dir != Direction::North){
        additionalOccupancy |= shift<Direction::North>(from_bb);
    }
    if(dir != Direction::South){
        additionalOccupancy |= shift<Direction::South>(from_bb);
    }
    if(dir != Direction::East){
        additionalOccupancy |= shift<Direction::East>(from_bb);
    }
    if(dir != Direction::West){
        additionalOccupancy |= shift<Direction::West>(from_bb);
    }
    occupancy |= additionalOccupancy;

    Bitboard between{rookMagics[from].attacks_bb(occupancy)};
    between &= ~additionalOccupancy;
    return between;
}

template<>
Bitboard inBetween<PieceType::Bishop>(Square from, Square to){
    Bitboard from_bb{bitboard_of(from)};
    Bitboard to_bb{bitboard_of(to)};

    Bitboard occupancy{to_bb};

    Bitboard additionalOccupancy{0};
    Direction dir{direction_of(from, to)};
    if(dir != Direction::NorthEast){
        additionalOccupancy |= shift<Direction::NorthEast>(from_bb);
    }
    if(dir != Direction::SouthEast){
        additionalOccupancy |= shift<Direction::SouthEast>(from_bb);
    }
    if(dir != Direction::NorthWest){
        additionalOccupancy |= shift<Direction::NorthWest>(from_bb);
    }
    if(dir != Direction::SouthWest){
        additionalOccupancy |= shift<Direction::SouthWest>(from_bb);
    }
    occupancy |= additionalOccupancy;

    Bitboard between{bishopMagics[from].attacks_bb(occupancy)};
    between &= ~additionalOccupancy;
    return between;
}