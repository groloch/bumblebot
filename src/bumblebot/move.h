# ifndef MOVE_H
# define MOVE_H

#include "types.h"
#include "bitboard.h"

class Position;

struct Move {
    Move() : from{squares::NoneSquare}, to{squares::NoneSquare}, isEnPassant{false}, isCastle{false}, 
    isPromotion{false},isCapture{false}, isPawnMove{false}, promotionPieceType{PieceType::None}, 
    capturedPieceType{PieceType::None} {}

    Move(Square from, Square to, bool isEnPassant, bool isCastle, bool isPromotion,
        bool isCapture, bool isPawnMove, PieceType promotionPieceType = PieceType::None, PieceType capturedPieceType = PieceType::None)
        : from(from), to(to), isEnPassant(isEnPassant), isCastle(isCastle),
        isPromotion(isPromotion), isCapture(isCapture), isPawnMove(isPawnMove),
        promotionPieceType(promotionPieceType), capturedPieceType(capturedPieceType) {}

    Square from, to;

    bool isEnPassant, isCastle, isPromotion, isCapture, isPawnMove;
    PieceType promotionPieceType;
    PieceType capturedPieceType;
};

struct MoveList {
    MoveList() : moves{}, size{0} {}

    std::array<Move, 256> moves;
    unsigned size;

    void append(Move move){
        moves[size] = move;
        size++;
    }
};

Bitboard generatePawnQuietMoves(Position& position, Square square, Color color);

void generatePseudoLegalMoves(Position& position, MoveList& moveList, bool checkLegal = false);

void generateLegalMoves(Position& position, MoveList& moveList);

template<MoveType T>
void generateMoves(Position& position, MoveList& moveList);

# endif
