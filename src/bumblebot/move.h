# ifndef MOVE_H
# define MOVE_H

#include <cstdint>
#include <type_traits>

#include "types.h"
#include "bitboard.h"

class Position;

struct Move {
    constexpr Move() : bits{0} {}

    constexpr Move(Square from, Square to, bool isEnPassant, bool isCastle, bool isPromotion,
                   bool isCapture, bool isPawnMove,
                   PieceType promotionPieceType = PieceType::None,
                   PieceType capturedPieceType = PieceType::None)
        : bits{
            (static_cast<uint32_t>(from) & 0x3Fu) |
            ((static_cast<uint32_t>(to) & 0x3Fu) << 6) |
            (static_cast<uint32_t>(isCapture)   << 12) |
            (static_cast<uint32_t>(isCastle)    << 13) |
            (static_cast<uint32_t>(isPromotion) << 14) |
            (static_cast<uint32_t>(isEnPassant) << 15) |
            (static_cast<uint32_t>(isPawnMove)  << 16) |
            (static_cast<uint32_t>(encodePieceType(capturedPieceType))  << 17) |
            (static_cast<uint32_t>(encodePieceType(promotionPieceType)) << 20)
          } {}

    constexpr Square from()  const { return  bits        & 0x3Fu; }
    constexpr Square to()    const { return (bits >> 6)  & 0x3Fu; }
    constexpr bool isCapture()   const { return ((bits >> 12) & 1u) != 0; }
    constexpr bool isCastle()    const { return ((bits >> 13) & 1u) != 0; }
    constexpr bool isPromotion() const { return ((bits >> 14) & 1u) != 0; }
    constexpr bool isEnPassant() const { return ((bits >> 15) & 1u) != 0; }
    constexpr bool isPawnMove()  const { return ((bits >> 16) & 1u) != 0; }
    constexpr PieceType capturedPieceType()  const { return decodePieceType((bits >> 17) & 7u); }
    constexpr PieceType promotionPieceType() const { return decodePieceType((bits >> 20) & 7u); }

    uint32_t bits;

    static constexpr uint8_t encodePieceType(PieceType pt) {
        switch (pt) {
            case PieceType::None:   return 0;
            case PieceType::Pawn:   return 1;
            case PieceType::Knight: return 2;
            case PieceType::Bishop: return 3;
            case PieceType::Rook:   return 4;
            case PieceType::Queen:  return 5;
            case PieceType::King:   return 6;
            default:                return 0;
        }
    }

    static constexpr PieceType decodePieceType(uint8_t v) {
        constexpr PieceType table[] = {
            PieceType::None, PieceType::Pawn, PieceType::Knight,
            PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King,
            PieceType::None,
        };
        return table[v & 7u];
    }
};

static_assert(sizeof(Move) == 4, "Move must be packed into 4 bytes");
static_assert(std::is_trivially_copyable<Move>::value, "Move must be trivially copyable");

struct MoveList {
    MoveList() : moves{}, size{0} {}

    std::array<Move, 256> moves;
    unsigned size;

    void append(const Move& move){
        if(size >= moves.size()){
            return;
        }
        moves[size] = move;
        size++;
    }
};

template<MoveType T>
void generateMoves(Position& position, MoveList& moveList);

# endif
