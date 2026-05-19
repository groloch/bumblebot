# include "movegen.h"
# include "position.h"

# include "debugging.h"


namespace movegen {

template<bool CheckLegal>
inline void appendPromotions(Ctx const& ctx, Square from, Square to,
                             bool isCapture, PieceType capturedPt){
    for(PieceType pt : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}){
        Move m{from, to, false, false, true, isCapture, true, pt, capturedPt};
        ctx.list.append(m);
    }
}


template<Color C, bool CheckLegal>
void generatePawnMoves(Ctx const& ctx, Bitboard pawns){
    if(pawns == 0) return;

    const Bitboard empty{~ctx.occ};
    const Bitboard enemyPieces{ctx.enemy};

    constexpr Bitboard promoRank{
        (C == Color::White) ? bitboards::bb_rank8 : bitboards::bb_rank1
    };
    constexpr Bitboard doublePushRank{
        (C == Color::White) ? bitboards::bb_rank4 : bitboards::bb_rank5
    };
    constexpr Bitboard preDoublePushRank{
        (C == Color::White) ? bitboards::bb_rank3 : bitboards::bb_rank6
    };

    constexpr Direction pushDir{
        (C == Color::White) ? Direction::North : Direction::South
    };
    constexpr Direction capLDir{
        (C == Color::White) ? Direction::NorthWest : Direction::SouthWest
    };
    constexpr Direction capRDir{
        (C == Color::White) ? Direction::NorthEast : Direction::SouthEast
    };

    Bitboard singlePush{shift<pushDir>(pawns) & empty};
    Bitboard doublePush{shift<pushDir>(singlePush & preDoublePushRank) & empty & doublePushRank};

    Bitboard quietPush{singlePush & ~promoRank};
    Bitboard promoPush{singlePush & promoRank};

    Bitboard capL{shift<capLDir>(pawns) & enemyPieces};
    Bitboard promoCapL{capL & promoRank};
    Bitboard quietCapL{capL & ~promoRank};

    Bitboard capR{shift<capRDir>(pawns) & enemyPieces};
    Bitboard promoCapR{capR & promoRank};
    Bitboard quietCapR{capR & ~promoRank};

    if constexpr(CheckLegal){
        quietPush  &= ctx.targetMask;
        doublePush &= ctx.targetMask;
        promoPush  &= ctx.targetMask;
        quietCapL  &= ctx.targetMask;
        quietCapR  &= ctx.targetMask;
        promoCapL  &= ctx.targetMask;
        promoCapR  &= ctx.targetMask;
    }

    constexpr int pushOff{(C == Color::White) ? 8 : -8};
    constexpr int dpushOff{(C == Color::White) ? 16 : -16};
    constexpr int capLOff{(C == Color::White) ? 7 : -9};
    constexpr int capROff{(C == Color::White) ? 9 : -7};

    auto emitQuiet = [&](Bitboard targets, int fromOffset){
        while(targets){
            Square to{poplsb(targets)};
            Square from{static_cast<Square>(static_cast<int>(to) - fromOffset)};
            if constexpr(CheckLegal){
                if((bitboard_of(to) & (*ctx.pinMasks)[from]) == 0) continue;
            }
            Move m{from, to, false, false, false, false, true, PieceType::None, PieceType::None};
            ctx.list.append(m);
        }
    };

    auto emitCapture = [&](Bitboard targets, int fromOffset){
        while(targets){
            Square to{poplsb(targets)};
            Square from{static_cast<Square>(static_cast<int>(to) - fromOffset)};
            if constexpr(CheckLegal){
                if((bitboard_of(to) & (*ctx.pinMasks)[from]) == 0) continue;
            }
            PieceType cap{ctx.pos.getSquareContent(to).pieceType};
            Move m{from, to, false, false, false, true, true, PieceType::None, cap};
            ctx.list.append(m);
        }
    };

    auto emitPromoQuiet = [&](Bitboard targets, int fromOffset){
        while(targets){
            Square to{poplsb(targets)};
            Square from{static_cast<Square>(static_cast<int>(to) - fromOffset)};
            if constexpr(CheckLegal){
                if((bitboard_of(to) & (*ctx.pinMasks)[from]) == 0) continue;
            }
            appendPromotions<CheckLegal>(ctx, from, to, false, PieceType::None);
        }
    };

    auto emitPromoCapture = [&](Bitboard targets, int fromOffset){
        while(targets){
            Square to{poplsb(targets)};
            Square from{static_cast<Square>(static_cast<int>(to) - fromOffset)};
            if constexpr(CheckLegal){
                if((bitboard_of(to) & (*ctx.pinMasks)[from]) == 0) continue;
            }
            PieceType cap{ctx.pos.getSquareContent(to).pieceType};
            appendPromotions<CheckLegal>(ctx, from, to, true, cap);
        }
    };

    emitQuiet(quietPush, pushOff);
    emitQuiet(doublePush, dpushOff);
    emitCapture(quietCapL, capLOff);
    emitCapture(quietCapR, capROff);
    emitPromoQuiet(promoPush, pushOff);
    emitPromoCapture(promoCapL, capLOff);
    emitPromoCapture(promoCapR, capROff);

    Square epSq{ctx.pos.enPassant()};
    if(epSq != squares::NoneSquare){
        Bitboard epBb{bitboard_of(epSq)};
        Bitboard epL{shift<capLDir>(pawns) & epBb};
        if(epL){
            Square to{lsb(epL)};
            Square from{static_cast<Square>(static_cast<int>(to) - capLOff)};
            Move m{from, to, true, false, false, true, true, PieceType::None, PieceType::Pawn};
            if constexpr(CheckLegal){
                const Square epPawnSq{
                    (C == Color::White) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8)
                };
                bool resolvesCheck{
                    (bitboard_of(to) & ctx.targetMask) != 0 ||
                    (bitboard_of(epPawnSq) & ctx.checkers) != 0
                };
                if(resolvesCheck
                   && (bitboard_of(to) & (*ctx.pinMasks)[from]) != 0
                   && ctx.pos.isLegal(m)){
                    ctx.list.append(m);
                }
            } else {
                ctx.list.append(m);
            }
        }
        Bitboard epR{shift<capRDir>(pawns) & epBb};
        if(epR){
            Square to{lsb(epR)};
            Square from{static_cast<Square>(static_cast<int>(to) - capROff)};
            Move m{from, to, true, false, false, true, true, PieceType::None, PieceType::Pawn};
            if constexpr(CheckLegal){
                const Square epPawnSq{
                    (C == Color::White) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8)
                };
                bool resolvesCheck{
                    (bitboard_of(to) & ctx.targetMask) != 0 ||
                    (bitboard_of(epPawnSq) & ctx.checkers) != 0
                };
                if(resolvesCheck
                   && (bitboard_of(to) & (*ctx.pinMasks)[from]) != 0
                   && ctx.pos.isLegal(m)){
                    ctx.list.append(m);
                }
            } else {
                ctx.list.append(m);
            }
        }
    }
}


template<bool CheckLegal>
void generateKnightMoves(Ctx const& ctx, Bitboard knights){
    while(knights){
        Square from{poplsb(knights)};
        if constexpr(CheckLegal){
            if((*ctx.pinMasks)[from] != bitboards::full) continue;
        }
        Bitboard targets{knightAttacks[from] & ctx.notFriendly};
        if constexpr(CheckLegal){
            targets &= ctx.targetMask;
        }
        while(targets){
            Square to{poplsb(targets)};
            PieceType cap{ctx.pos.getSquareContent(to).pieceType};
            bool isCap{cap != PieceType::None};
            Move m{from, to, false, false, false, isCap, false, PieceType::None, cap};
            ctx.list.append(m);
        }
    }
}


template<bool CheckLegal>
void generateBishopMoves(Ctx const& ctx, Bitboard bishops){
    while(bishops){
        Square from{poplsb(bishops)};
        Bitboard targets{bishopMagics[from].attacks_bb(ctx.occ) & ctx.notFriendly};
        if constexpr(CheckLegal){
            targets &= ctx.targetMask & (*ctx.pinMasks)[from];
        }
        while(targets){
            Square to{poplsb(targets)};
            PieceType cap{ctx.pos.getSquareContent(to).pieceType};
            bool isCap{cap != PieceType::None};
            Move m{from, to, false, false, false, isCap, false, PieceType::None, cap};
            ctx.list.append(m);
        }
    }
}


template<bool CheckLegal>
void generateRookMoves(Ctx const& ctx, Bitboard rooks){
    while(rooks){
        Square from{poplsb(rooks)};
        Bitboard targets{rookMagics[from].attacks_bb(ctx.occ) & ctx.notFriendly};
        if constexpr(CheckLegal){
            targets &= ctx.targetMask & (*ctx.pinMasks)[from];
        }
        while(targets){
            Square to{poplsb(targets)};
            PieceType cap{ctx.pos.getSquareContent(to).pieceType};
            bool isCap{cap != PieceType::None};
            Move m{from, to, false, false, false, isCap, false, PieceType::None, cap};
            ctx.list.append(m);
        }
    }
}


template<bool CheckLegal>
void generateQueenMoves(Ctx const& ctx, Bitboard queens){
    while(queens){
        Square from{poplsb(queens)};
        Bitboard targets{
            (rookMagics[from].attacks_bb(ctx.occ) | bishopMagics[from].attacks_bb(ctx.occ))
            & ctx.notFriendly
        };
        if constexpr(CheckLegal){
            targets &= ctx.targetMask & (*ctx.pinMasks)[from];
        }
        while(targets){
            Square to{poplsb(targets)};
            PieceType cap{ctx.pos.getSquareContent(to).pieceType};
            bool isCap{cap != PieceType::None};
            Move m{from, to, false, false, false, isCap, false, PieceType::None, cap};
            ctx.list.append(m);
        }
    }
}


template<Color C, bool CheckLegal>
void generateKingMoves(Ctx const& ctx, Bitboard kings){
    if(kings == 0) return;

    Square from{lsb(kings)};

    Bitboard targets{kingAttacks[from] & ctx.notFriendly};
    if constexpr(CheckLegal){
        targets &= ~ctx.opponentAttacks;
    }
    while(targets){
        Square to{poplsb(targets)};
        PieceType cap{ctx.pos.getSquareContent(to).pieceType};
        bool isCap{cap != PieceType::None};
        Move m{from, to, false, false, false, isCap, false, PieceType::None, cap};
        ctx.list.append(m);
    }

    Bitboard kingsideLookup, queensideLookup;
    Square kingsideTo, queensideTo;
    if constexpr(C == Color::White){
        kingsideLookup = bitboards::bb_f1 | bitboards::bb_g1;
        queensideLookup = bitboards::bb_b1 | bitboards::bb_c1 | bitboards::bb_d1;
        kingsideTo = squares::g1;
        queensideTo = squares::c1;
    } else {
        kingsideLookup = bitboards::bb_f8 | bitboards::bb_g8;
        queensideLookup = bitboards::bb_b8 | bitboards::bb_c8 | bitboards::bb_d8;
        kingsideTo = squares::g8;
        queensideTo = squares::c8;
    }

    if(ctx.pos.canCastle(C, CastleDirection::Kingside)
       && !(ctx.occ & kingsideLookup)){
        bool ok{true};
        if constexpr(CheckLegal){
            Bitboard through{bitboard_of(from) | bitboard_of(from + 1u) | bitboard_of(from + 2u)};
            ok = (through & ctx.opponentAttacks) == 0;
        }
        if(ok){
            Move m{from, kingsideTo, false, true, false, false, false};
            ctx.list.append(m);
        }
    }
    if(ctx.pos.canCastle(C, CastleDirection::Queenside)
       && !(ctx.occ & queensideLookup)){
        bool ok{true};
        if constexpr(CheckLegal){
            Bitboard through{bitboard_of(from) | bitboard_of(from - 1u) | bitboard_of(from - 2u)};
            ok = (through & ctx.opponentAttacks) == 0;
        }
        if(ok){
            Move m{from, queensideTo, false, true, false, false, false};
            ctx.list.append(m);
        }
    }
}


void generatePseudoLegalAll(Position& position, MoveList& moveList){
    const Color stm{position.turn()};
    const Bitboard friendly{(stm == Color::White) ? position.whitePieces : position.blackPieces};
    const Bitboard enemy{(stm == Color::White) ? position.blackPieces : position.whitePieces};

    Ctx ctx{
        position,
        moveList,
        stm,
        friendly,
        enemy,
        position.allPieces,
        ~friendly,
        bitboards::full,    // targetMask (unused in pseudo-legal)
        0,                  // opponentAttacks (unused)
        0,                  // checkers (unused)
        nullptr,            // pinMasks (unused)
        0,                  // kingSq (unused)
        false               // inCheck (unused)
    };

    if(stm == Color::White){
        generatePawnMoves<Color::White, false>(ctx, position.whitePawns);
        generateKnightMoves<false>(ctx, position.whiteKnights);
        generateBishopMoves<false>(ctx, position.whiteBishops);
        generateRookMoves<false>(ctx, position.whiteRooks);
        generateQueenMoves<false>(ctx, position.whiteQueens);
        generateKingMoves<Color::White, false>(ctx, position.whiteKings);
    } else {
        generatePawnMoves<Color::Black, false>(ctx, position.blackPawns);
        generateKnightMoves<false>(ctx, position.blackKnights);
        generateBishopMoves<false>(ctx, position.blackBishops);
        generateRookMoves<false>(ctx, position.blackRooks);
        generateQueenMoves<false>(ctx, position.blackQueens);
        generateKingMoves<Color::Black, false>(ctx, position.blackKings);
    }
}


void generateLegalAll(Position& position, MoveList& moveList){
    const Color stm{position.turn()};
    const Bitboard friendly{(stm == Color::White) ? position.whitePieces : position.blackPieces};
    const Bitboard enemy{(stm == Color::White) ? position.blackPieces : position.whitePieces};

    const Bitboard kingBb{position.kingBitboard()};
    const Square kingSq{lsb(kingBb)};
    const Bitboard checkers{position.checkers};
    const unsigned numCheckers{popcount(checkers)};
    const bool inCheck{checkers != 0};

    Bitboard targetMask;
    if(numCheckers == 0){
        targetMask = bitboards::full;
    } else if(numCheckers == 1){
        const Square checkerSq{lsb(checkers)};
        const SquareContent checker{position.getSquareContent(checkerSq)};
        if(checker.pieceType == PieceType::Knight || checker.pieceType == PieceType::Pawn){
            targetMask = bitboard_of(checkerSq);
        } else {
            const bool rookLike{
                checker.pieceType == PieceType::Rook ||
                (checker.pieceType == PieceType::Queen &&
                 (rank_of(checkerSq) == rank_of(kingSq) || file_of(checkerSq) == file_of(kingSq)))
            };
            Bitboard between{
                rookLike
                    ? inBetween<PieceType::Rook>(kingSq, checkerSq)
                    : inBetween<PieceType::Bishop>(kingSq, checkerSq)
            };
            targetMask = between | bitboard_of(checkerSq);
        }
    } else {
        targetMask = bitboards::empty;
    }

    Ctx ctx{
        position,
        moveList,
        stm,
        friendly,
        enemy,
        position.allPieces,
        ~friendly,
        targetMask,
        position.opponentAttacks,
        checkers,
        &position.pinMasks(),
        kingSq,
        inCheck
    };

    if(numCheckers >= 2){
        if(stm == Color::White){
            generateKingMoves<Color::White, true>(ctx, position.whiteKings);
        } else {
            generateKingMoves<Color::Black, true>(ctx, position.blackKings);
        }
        return;
    }

    if(stm == Color::White){
        generatePawnMoves<Color::White, true>(ctx, position.whitePawns);
        generateKnightMoves<true>(ctx, position.whiteKnights);
        generateBishopMoves<true>(ctx, position.whiteBishops);
        generateRookMoves<true>(ctx, position.whiteRooks);
        generateQueenMoves<true>(ctx, position.whiteQueens);
        generateKingMoves<Color::White, true>(ctx, position.whiteKings);
    } else {
        generatePawnMoves<Color::Black, true>(ctx, position.blackPawns);
        generateKnightMoves<true>(ctx, position.blackKnights);
        generateBishopMoves<true>(ctx, position.blackBishops);
        generateRookMoves<true>(ctx, position.blackRooks);
        generateQueenMoves<true>(ctx, position.blackQueens);
        generateKingMoves<Color::Black, true>(ctx, position.blackKings);
    }
}

} // namespace movegen


template<>
void generateMoves<MoveType::Legal>(Position& position, MoveList& moveList){
    movegen::generateLegalAll(position, moveList);
}

template<>
void generateMoves<MoveType::PseudoLegal>(Position& position, MoveList& moveList){
    movegen::generatePseudoLegalAll(position, moveList);
}
