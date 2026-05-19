# include "nn/nn_utils.h"

# include <algorithm>

# include "position.h"
# include "debugging.h"


namespace nn {

namespace {

constexpr int64_t CASTLING_BASE = 13;
constexpr int64_t HALFMOVE_TOKEN = 21;
constexpr int64_t CLS_TOKEN_ID = 22;


inline Square mirror(Square sq){
    return sq ^ 56u;
}


int64_t pieceTokenId(SquareContent c){
    if(c.pieceType == PieceType::None) return 0;
    int64_t base{(c.color == Color::Black) ? 6 : 0};
    int64_t offset{0};
    switch(c.pieceType){
        case PieceType::Pawn:   offset = 1; break;
        case PieceType::Knight: offset = 2; break;
        case PieceType::Bishop: offset = 3; break;
        case PieceType::Rook:   offset = 4; break;
        case PieceType::Queen:  offset = 5; break;
        case PieceType::King:   offset = 6; break;
        default: return 0;
    }
    return base + offset;
}

}


ModelInput encode_board(Position const& pos){
    ModelInput in{};
    const bool flip{pos.turn() == Color::Black};

    for(Square s{0}; s < 64; ++s){
        const Square actualSq{flip ? mirror(s) : s};
        SquareContent c{pos.getSquareContent(actualSq)};
        if(flip && c.pieceType != PieceType::None){
            c.color = (c.color == Color::White) ? Color::Black : Color::White;
        }
        in.tokens[s] = pieceTokenId(c);
    }

    in.tokens[64] = CLS_TOKEN_ID;

    const Color whiteSide{flip ? Color::Black : Color::White};
    const Color blackSide{flip ? Color::White : Color::Black};

    const bool wk{pos.canCastle(whiteSide, CastleDirection::Kingside)};
    const bool wq{pos.canCastle(whiteSide, CastleDirection::Queenside)};
    const bool bk{pos.canCastle(blackSide, CastleDirection::Kingside)};
    const bool bq{pos.canCastle(blackSide, CastleDirection::Queenside)};

    in.tokens[65] = CASTLING_BASE + (wk ? 0 : 1);
    in.tokens[66] = CASTLING_BASE + (wq ? 2 : 3);
    in.tokens[67] = CASTLING_BASE + (bk ? 4 : 5);
    in.tokens[68] = CASTLING_BASE + (bq ? 6 : 7);

    in.tokens[69] = HALFMOVE_TOKEN;

    in.hm = std::min(pos.halfmoveClock(), 50u) / 50.0f;

    const Square ep{pos.enPassant()};
    in.epsq = (ep == squares::NoneSquare)
              ? -1
              : static_cast<int64_t>(flip ? mirror(ep) : ep);

    return in;
}


Move moveFromPolicyIndex(unsigned index, Position const& pos){
    assert(index < 4288u, "policy index out of range");

    const bool flip{pos.turn() == Color::Black};

    Square from;
    Square to;
    PieceType promotionPieceType{PieceType::None};
    bool isPromotion{false};

    if(index < 4096u){
        const Square modelFrom{index / 64u};
        const Square modelTo{index % 64u};
        from = flip ? mirror(modelFrom) : modelFrom;
        to = flip ? mirror(modelTo) : modelTo;

        const SquareContent fromContent{pos.getSquareContent(from)};
        if(fromContent.pieceType == PieceType::Pawn){
            const Rank backRank{(pos.turn() == Color::White) ? ranks::r8 : ranks::r1};
            if(rank_of(to) == backRank){
                isPromotion = true;
                promotionPieceType = PieceType::Queen;
            }
        }
    } else {
        // Underpromotion encoding: 4096 + fromFile*24 + toFile*3 + pieceIdx (white POV).
        const unsigned promoIdx{index - 4096u};
        const unsigned modelFromFile{promoIdx / 24u};
        const unsigned modelToFile{(promoIdx / 3u) % 8u};
        const unsigned pieceIdx{promoIdx % 3u};
        const Square modelFrom{48u + modelFromFile};
        const Square modelTo{56u + modelToFile};
        from = flip ? mirror(modelFrom) : modelFrom;
        to = flip ? mirror(modelTo) : modelTo;

        constexpr PieceType promoTable[]{
            PieceType::Rook, PieceType::Bishop, PieceType::Knight
        };
        promotionPieceType = promoTable[pieceIdx];
        isPromotion = true;
    }

    const SquareContent fromContent{pos.getSquareContent(from)};
    const SquareContent toContent{pos.getSquareContent(to)};

    const bool isPawnMove{fromContent.pieceType == PieceType::Pawn};

    const unsigned fileDelta{(to > from) ? (to - from) : (from - to)};
    const bool isCastle{fromContent.pieceType == PieceType::King && fileDelta == 2u};

    const bool isEnPassant{isPawnMove && to == pos.enPassant()
                           && toContent.pieceType == PieceType::None};

    const bool isCapture{toContent.pieceType != PieceType::None || isEnPassant};

    const PieceType capturedPieceType{
        isEnPassant ? PieceType::Pawn : toContent.pieceType
    };

    return Move{
        from, to,
        isEnPassant, isCastle, isPromotion,
        isCapture, isPawnMove,
        promotionPieceType,
        capturedPieceType
    };
}


unsigned policyIndexOf(Move const& move, Position const& pos){
    const bool flip{pos.turn() == Color::Black};
    const Square from{flip ? mirror(move.from()) : move.from()};
    const Square to{flip ? mirror(move.to()) : move.to()};

    if(move.isPromotion() && move.promotionPieceType() != PieceType::Queen){
        // 4096 + fromFile*24 + toFile*3 + pieceIdx, where 0=R, 1=B, 2=N.
        const unsigned fromFile{file_of(from)};
        const unsigned toFile{file_of(to)};
        unsigned pieceIdx{0};
        switch(move.promotionPieceType()){
            case PieceType::Rook:   pieceIdx = 0; break;
            case PieceType::Bishop: pieceIdx = 1; break;
            case PieceType::Knight: pieceIdx = 2; break;
            default: pieceIdx = 0; break;
        }
        return 4096u + fromFile * 24u + toFile * 3u + pieceIdx;
    }
    return static_cast<unsigned>(from) * 64u + static_cast<unsigned>(to);
}

}
