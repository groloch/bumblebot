# ifndef MOVEGEN_H
# define MOVEGEN_H

# include <array>

# include "types.h"
# include "bitboard.h"
# include "move.h"

class Position;


namespace movegen {

struct Ctx {
    Position& pos;
    MoveList& list;
    Color stm;
    Bitboard friendly;     // whitePieces or blackPieces
    Bitboard enemy;        // the other one
    Bitboard occ;          // allPieces
    Bitboard notFriendly;  // ~friendly

    // Fields below are only meaningful when CheckLegal == true.
    // For pseudo-legal generation, targetMask stays bitboards::full and the
    // generators skip pin-mask/target-mask filtering entirely.
    Bitboard targetMask;             // squares non-king pieces may LAND on
    Bitboard opponentAttacks;        // squares attacked by opponent
    Bitboard checkers;               // opponent pieces checking us
    const std::array<Bitboard, 64>* pinMasks; // per-square pin masks (legal mode only)
    Square kingSq;                   // friendly king square
    bool inCheck;                    // checkers != 0
};


template<bool CheckLegal>
inline void appendPromotions(Ctx const& ctx, Square from, Square to,
                             bool isCapture, PieceType capturedPt);

template<Color C, bool CheckLegal>
void generatePawnMoves(Ctx const& ctx, Bitboard pawns);

template<bool CheckLegal>
void generateKnightMoves(Ctx const& ctx, Bitboard knights);

template<bool CheckLegal>
void generateBishopMoves(Ctx const& ctx, Bitboard bishops);

template<bool CheckLegal>
void generateRookMoves(Ctx const& ctx, Bitboard rooks);

template<bool CheckLegal>
void generateQueenMoves(Ctx const& ctx, Bitboard queens);

template<Color C, bool CheckLegal>
void generateKingMoves(Ctx const& ctx, Bitboard kings);

void generatePseudoLegalAll(Position& position, MoveList& moveList);
void generateLegalAll(Position& position, MoveList& moveList);

} // namespace movegen


# endif
