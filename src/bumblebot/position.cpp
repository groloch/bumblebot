# include "position.h"
# include "zobrist.h"
# include "debugging.h"


namespace {

inline Hash castleRightsHash(CastleRights const& cr){
    Hash h{0};
    if(cr.whiteKingside)  h ^= zobrist::castle[0];
    if(cr.whiteQueenside) h ^= zobrist::castle[1];
    if(cr.blackKingside)  h ^= zobrist::castle[2];
    if(cr.blackQueenside) h ^= zobrist::castle[3];
    return h;
}

}


Position::Position() : board{},
sideToMove{Color::White}, positionData{}, plyCount{0}, zobristHash{0} {
    history.reserve(64);

    setSquareContent(squares::a1, SquareContent(Color::White, PieceType::Rook));
    setSquareContent(squares::b1, SquareContent(Color::White, PieceType::Knight));
    setSquareContent(squares::c1, SquareContent(Color::White, PieceType::Bishop));
    setSquareContent(squares::d1, SquareContent(Color::White, PieceType::Queen));
    setSquareContent(squares::e1, SquareContent(Color::White, PieceType::King));
    setSquareContent(squares::f1, SquareContent(Color::White, PieceType::Bishop));
    setSquareContent(squares::g1, SquareContent(Color::White, PieceType::Knight));
    setSquareContent(squares::h1, SquareContent(Color::White, PieceType::Rook));

    for (Square square = squares::a2; square <= squares::h2; ++square) {
        setSquareContent(square, SquareContent(Color::White, PieceType::Pawn));
    }

    setSquareContent(squares::a8, SquareContent(Color::Black, PieceType::Rook));
    setSquareContent(squares::b8, SquareContent(Color::Black, PieceType::Knight));
    setSquareContent(squares::c8, SquareContent(Color::Black, PieceType::Bishop));
    setSquareContent(squares::d8, SquareContent(Color::Black, PieceType::Queen));
    setSquareContent(squares::e8, SquareContent(Color::Black, PieceType::King));
    setSquareContent(squares::f8, SquareContent(Color::Black, PieceType::Bishop));
    setSquareContent(squares::g8, SquareContent(Color::Black, PieceType::Knight));
    setSquareContent(squares::h8, SquareContent(Color::Black, PieceType::Rook));

    for (Square square = squares::a7; square <= squares::h7; ++square) {
        setSquareContent(square, SquareContent(Color::Black, PieceType::Pawn));
    }

    positionData.castleRights.whiteKingside = true;
    positionData.castleRights.whiteQueenside = true;
    positionData.castleRights.blackKingside = true;
    positionData.castleRights.blackQueenside = true;

    positionData.enPassantTarget = squares::NoneSquare;
    positionData.pliesSinceLastCaptureOrPawnMove = 0;

    updateBitboards();
    updateAttackMap();
    updatePins();
    recomputeZobristHash();
}

Position::Position(const char* fen) : board{}, sideToMove{Color::White},
positionData{}, plyCount{0}, zobristHash{0} {
    history.reserve(64);

    Rank currentRank {7};
    File currentFile {0};

    bool inPiecePlacement{true};
    bool inSideToMove{false};
    bool inCastleRights{false};
    bool inEnPassant{false};
    bool inPliesSinceLastCaptureOrPawnMove{false};
    bool inPlyCount{false};

    while (*fen != '\0') {
        char c = *fen;

        if (inPiecePlacement){
            if (c == '/') {
                currentRank -= 1;
                currentFile = 0;
            } else if (c >= '1' && c <= '8') {
                currentFile += c - '0';
            } else if (c >= 'a' && c <= 'z') {
                SquareContent content(Color::Black, static_cast<PieceType>(c));
                setSquareContent(currentFile, currentRank, content);
                currentFile++;
            } else if (c >= 'A' && c <= 'Z') {
                SquareContent content(Color::White, static_cast<PieceType>(c - 'A' + 'a'));
                setSquareContent(currentFile, currentRank, content);
                currentFile++;
            } else if (c == ' ') {
                inPiecePlacement = false;
                inSideToMove = true;
            } else {
                // Invalid FEN: piece placement
            }
        }else if (inSideToMove) {
            if (c == 'w') {
                sideToMove = Color::White;
            } else if (c == 'b') {
                sideToMove = Color::Black;
            } else if (c == ' ') {
                inSideToMove = false;
                inCastleRights = true;
                positionData.castleRights.whiteKingside = false;
                positionData.castleRights.whiteQueenside = false;
                positionData.castleRights.blackKingside = false;
                positionData.castleRights.blackQueenside = false;
            } else {
                // Invalid FEN : side to move
            }
        } else if (inCastleRights) {
            if (c == '-'){
                positionData.castleRights.whiteKingside = false;
                positionData.castleRights.whiteQueenside = false;
                positionData.castleRights.blackKingside = false;
                positionData.castleRights.blackQueenside = false;
            } else if (c == 'K') {
                positionData.castleRights.whiteKingside = true;
            } else if (c == 'Q') {
                positionData.castleRights.whiteQueenside = true;
            } else if (c == 'k') {
                positionData.castleRights.blackKingside = true;
            } else if (c == 'q') {
                positionData.castleRights.blackQueenside = true;
            } else if (c == ' ') {
                inCastleRights = false;
                inEnPassant = true;
            } else {
                // Invalid FEN : castle rights
            }
        } else if (inEnPassant) {
            if (c >= 'a' && c <= 'h') {
                File file = c - 'a';
                fen++;
                c = *fen;
                if (c >= '1' && c <= '8') {
                    Rank rank = c - '1';
                    positionData.enPassantTarget = rank * 8 + file;
                } else {
                    // Invalid FEN : en passant square
                }
            } else if (c == '-') {
                positionData.enPassantTarget = squares::NoneSquare;
            } else if (c == ' ') {
                inEnPassant = false;
                inPliesSinceLastCaptureOrPawnMove = true;
            } else {
                // Invalid FEN : en passant square
            }
        } else if (inPliesSinceLastCaptureOrPawnMove) {
            if (c >= '0' && c <= '9') {
                positionData.pliesSinceLastCaptureOrPawnMove =
                positionData.pliesSinceLastCaptureOrPawnMove * 10 + (c - '0');
            } else if (c == ' ') {
                inPliesSinceLastCaptureOrPawnMove = false;
                inPlyCount = true;
            } else {
                // Invalid FEN : plies since last capture or pawn move
            }
        } else if (inPlyCount) {
            if (c >= '0' && c <= '9') {
                plyCount = plyCount * 10 + (c - '0');
            } else {
                // Invalid FEN : ply count
            }
        }

        ++fen;
    }

    updateBitboards();
    updateAttackMap();
    updatePins();
    recomputeZobristHash();
}

SquareContent Position::getSquareContent(Square square) const {
    assert(square < 64, "Invalid index (getSquareContent)");
    return board[square];
}

SquareContent Position::getSquareContent(File file, Rank rank) const {
    return getSquareContent({rank * 8 + file});
}

void Position::setSquareContent(Square square, SquareContent content) {
    assert(square < 64, "Invalid index (setSquareContent)");
    SquareContent& currentContent = board[square];
    currentContent.pieceType = content.pieceType;
    currentContent.color = content.color;
}

void Position::setSquareContent(File file, Rank rank, SquareContent content) {
    setSquareContent({rank * 8 + file}, content);
}

void Position::updateBitboards() {
    whitePawns = 0;
    whiteKnights = 0;
    whiteBishops = 0;
    whiteRooks = 0;
    whiteQueens = 0;
    whiteKings = 0;

    blackPawns = 0;
    blackKnights = 0;
    blackBishops = 0;
    blackRooks = 0;
    blackQueens = 0;
    blackKings = 0;

    for (Square square = squares::a1; square <= squares::h8; ++square) {
        SquareContent content{getSquareContent(square)};
        if(content.color == Color::White){
            switch (content.pieceType) {
                case PieceType::Pawn:
                    whitePawns |= bitboard_of(square);
                    break;
                case PieceType::Knight:
                    whiteKnights |= bitboard_of(square);
                    break;
                case PieceType::Bishop:
                    whiteBishops |= bitboard_of(square);
                    break;
                case PieceType::Rook:
                    whiteRooks |= bitboard_of(square);
                    break;
                case PieceType::Queen:
                    whiteQueens |= bitboard_of(square);
                    break;
                case PieceType::King:
                    whiteKings |= bitboard_of(square);
                    break;
                default:
                    break;
            }
        } else if(content.color == Color::Black){
            switch (content.pieceType) {
                case PieceType::Pawn:
                    blackPawns |= bitboard_of(square);
                    break;
                case PieceType::Knight:
                    blackKnights |= bitboard_of(square);
                    break;
                case PieceType::Bishop:
                    blackBishops |= bitboard_of(square);
                    break;
                case PieceType::Rook:
                    blackRooks |= bitboard_of(square);
                    break;
                case PieceType::Queen:
                    blackQueens |= bitboard_of(square);
                    break;
                case PieceType::King:
                    blackKings |= bitboard_of(square);
                    break;
                default:
                    break;
            }
        }
    }

    whitePieces = whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKings;
    blackPieces = blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKings;
    allPieces = whitePieces | blackPieces;
}

Bitboard Position::getAttackMap(Square square, SquareContent content, bool ponder) const {
    Bitboard occupancy{allPieces};
    if(ponder){
        occupancy &= ~kingBitboard();
    }

    Bitboard attacks{0};
    switch (content.pieceType) {
        case PieceType::Pawn:
            attacks = pawnAttacks[content.color == Color::White ? 0 : 1][square];
            break;
        case PieceType::Knight:
            attacks = knightAttacks[square];
            break;
        case PieceType::Bishop:
            attacks = bishopMagics[square].attacks_bb(occupancy);
            break;
        case PieceType::Rook:
            attacks = rookMagics[square].attacks_bb(occupancy);
            break;
        case PieceType::Queen:
            attacks = rookMagics[square].attacks_bb(occupancy) |
                      bishopMagics[square].attacks_bb(occupancy);
            break;
        case PieceType::King:
            attacks = kingAttacks[square];
            break;
        default:
            break;
    }
    if(!ponder){
        const Bitboard friendlyPieces{
            (content.color == Color::White) ? whitePieces : blackPieces
        };
        attacks &= ~friendlyPieces;
    }
    return attacks;
}

void Position::updateAttackMap() {
    opponentAttacks = 0;
    checkers = 0;

    const Bitboard kingBb{kingBitboard()};
    const Bitboard occupancy{allPieces & ~kingBb};

    Bitboard oppPawns, oppKnights, oppBishops, oppRooks, oppQueens, oppKings;
    unsigned oppColorIdx;
    if(sideToMove == Color::White){
        oppPawns   = blackPawns;
        oppKnights = blackKnights;
        oppBishops = blackBishops;
        oppRooks   = blackRooks;
        oppQueens  = blackQueens;
        oppKings   = blackKings;
        oppColorIdx = 1;
    } else {
        oppPawns   = whitePawns;
        oppKnights = whiteKnights;
        oppBishops = whiteBishops;
        oppRooks   = whiteRooks;
        oppQueens  = whiteQueens;
        oppKings   = whiteKings;
        oppColorIdx = 0;
    }

    while(oppPawns){
        Square sq{poplsb(oppPawns)};
        Bitboard a{pawnAttacks[oppColorIdx][sq]};
        opponentAttacks |= a;
        if(a & kingBb) checkers |= bitboard_of(sq);
    }
    while(oppKnights){
        Square sq{poplsb(oppKnights)};
        Bitboard a{knightAttacks[sq]};
        opponentAttacks |= a;
        if(a & kingBb) checkers |= bitboard_of(sq);
    }
    Bitboard bq{oppBishops | oppQueens};
    while(bq){
        Square sq{poplsb(bq)};
        Bitboard a{bishopMagics[sq].attacks_bb(occupancy)};
        opponentAttacks |= a;
        if(a & kingBb) checkers |= bitboard_of(sq);
    }
    Bitboard rq{oppRooks | oppQueens};
    while(rq){
        Square sq{poplsb(rq)};
        Bitboard a{rookMagics[sq].attacks_bb(occupancy)};
        opponentAttacks |= a;
        if(a & kingBb) checkers |= bitboard_of(sq);
    }
    if(oppKings){
        opponentAttacks |= kingAttacks[lsb(oppKings)];
    }
}

void Position::updatePins(){
    for(Square square{squares::a1}; square <= squares::h8; ++square){
        pinnedAvailableSquares[square] = bitboards::full;
    }

    Square kingSquare(lsb(kingBitboard()));
    Bitboard friendlies{
        turn() == Color::White ? whitePieces : blackPieces
    };
    Bitboard ennemies{
        turn() == Color::White ? blackPieces : whitePieces
    };

    Bitboard rookLikes{
        turn() == Color::White ? blackRooks | blackQueens : whiteRooks | whiteQueens
    };
    Bitboard bishopLikes{
        turn() == Color::White ? blackBishops | blackQueens : whiteBishops | whiteQueens
    };

    Bitboard rookLikeSnipers{
        rookMagics[kingSquare].attacks_bb(bitboards::empty) & rookLikes
    };
    Bitboard bishopLikeSnipers{
        bishopMagics[kingSquare].attacks_bb(bitboards::empty) & bishopLikes
    };

    while(rookLikeSnipers != 0){
        Square sniperSquare{poplsb(rookLikeSnipers)};
        Bitboard between{inBetween<PieceType::Rook>(kingSquare, sniperSquare)};
        Bitboard potentiallyPinned{between & friendlies};
        Bitboard potentialEnnemyBlockers{between & ennemies & ~bitboard_of(sniperSquare)};
        if(exactly_one(potentiallyPinned) && potentialEnnemyBlockers == 0){
            Square pinnedSquare{lsb(potentiallyPinned)};
            pinnedAvailableSquares[pinnedSquare] = between;
        }
    }
    while(bishopLikeSnipers != 0){
        Square sniperSquare{poplsb(bishopLikeSnipers)};
        Bitboard between{inBetween<PieceType::Bishop>(kingSquare, sniperSquare)};
        Bitboard potentiallyPinned{between & friendlies};
        Bitboard potentialEnnemyBlockers{between & ennemies & ~bitboard_of(sniperSquare)};
        if(exactly_one(potentiallyPinned) && potentialEnnemyBlockers == 0){
            Square pinnedSquare{lsb(potentiallyPinned)};
            pinnedAvailableSquares[pinnedSquare] = between;
        }
    }
}

Bitboard Position::kingBitboard() const{
    return (sideToMove == Color::White) ? whiteKings : blackKings;
}

Color Position::turn() const{
    return sideToMove;
}

Square Position::enPassant() const{
    return positionData.enPassantTarget;
}

bool Position::canCastle(Color color, CastleDirection direction) const{
    if(color == Color::White){
        if(direction == CastleDirection::Kingside){
            return positionData.castleRights.whiteKingside;
        } else if (direction == CastleDirection::Queenside){
            return positionData.castleRights.whiteQueenside;
        }
    } else {
        if(direction == CastleDirection::Kingside){
            return positionData.castleRights.blackKingside;
        } else if (direction == CastleDirection::Queenside){
            return positionData.castleRights.blackQueenside;
        }
    }
    return false;
}

bool Position::isLegal(Move const& move) const{
    // Caller (movegen::generatePseudoLegalAll) guarantees move.from() holds a friendly piece
    // and move.to() is not occupied by a friendly piece, so the redundant sanity checks are
    // skipped here.
    const Square moveFrom{move.from()};
    const Square moveTo{move.to()};
    const SquareContent fromContent{board[moveFrom]};

    if(doubleCheck() && fromContent.pieceType != PieceType::King) return false;
    if(inCheck()){
        Square kingSquare{lsb(kingBitboard())};
        if(fromContent.pieceType == PieceType::King){
            return !move.isCastle() && (opponentAttacks & bitboard_of(moveTo)) == 0;
        }
        Square checkerSquare{lsb(checkers)};
        SquareContent checker{getSquareContent(checkerSquare)};
        Bitboard inBetweenMask{0};

        bool queenRookCheck{checker.pieceType == PieceType::Queen && (
            rank_of(checkerSquare) == rank_of(kingSquare) || file_of(checkerSquare) == file_of(kingSquare)
        )};
        bool queenBishopCheck{checker.pieceType == PieceType::Queen && !queenRookCheck};
        if(queenRookCheck|| checker.pieceType == PieceType::Rook){
            inBetweenMask = inBetween<PieceType::Rook>(kingSquare, checkerSquare);
        } else if(queenBishopCheck || checker.pieceType == PieceType::Bishop){
            inBetweenMask = inBetween<PieceType::Bishop>(kingSquare, checkerSquare);
        } else {
            inBetweenMask = bitboard_of(checkerSquare);
        }
        bool stopsCheck{(bitboard_of(moveTo) & inBetweenMask) != 0};
        if(move.isEnPassant()){
            Square epPawnSquare{(sideToMove == Color::White) ? moveTo - 8 : moveTo + 8};
            stopsCheck = stopsCheck || (bitboard_of(epPawnSquare) & inBetweenMask) != 0;
        }
        bool available{(bitboard_of(moveTo) & pinnedAvailableSquares[moveFrom]) != 0};
        return stopsCheck && available;
    }
    if(fromContent.pieceType == PieceType::King){
        if(move.isCastle()){
            CastleDirection direction{
                (moveTo > moveFrom) ? CastleDirection::Kingside : CastleDirection::Queenside
            };
            if(!canCastle(sideToMove, direction)) return false;

            Bitboard checkSquares{inBetween<PieceType::Rook>(moveFrom, moveTo)};
            return (checkSquares & opponentAttacks) == 0;
        }
        return (opponentAttacks & bitboard_of(moveTo)) == 0;
    } else if(!move.isEnPassant()){
        return (bitboard_of(moveTo) & pinnedAvailableSquares[moveFrom]) != 0;
    } else{
        if((bitboard_of(moveTo) & pinnedAvailableSquares[moveFrom]) == 0) return false;

        // check for rook-like alignment because ep removes 2 pieces on a rank at once
        Bitboard kingBb{kingBitboard()};
        Square kingSquare{lsb(kingBb)};
        Square epPawnSquare{(sideToMove == Color::White) ? moveTo - 8 : moveTo + 8};
        Bitboard opponentRookLikes{
            (sideToMove == Color::White) ? blackRooks | blackQueens : whiteRooks | whiteQueens
        };
        opponentRookLikes &= (bitboard_of_rank(rank_of(kingSquare)));
        while(opponentRookLikes != 0){
            Square sniperSquare{poplsb(opponentRookLikes)};
            Bitboard between{inBetween<PieceType::Rook>(kingSquare, sniperSquare)};
            between &= ~bitboard_of(moveFrom) & ~bitboard_of(epPawnSquare);
            between &= ~bitboard_of(sniperSquare);
            if((between & allPieces) == 0){
                return false;
            }
        }
        return true;
    }
    return true;
}

bool Position::inCheck() const{
    return checkers != 0;
}

bool Position::doubleCheck() const{
    return more_than_one(checkers);
}

Bitboard& Position::pieceBb(Color color, PieceType pieceType){
    if(color == Color::White){
        switch(pieceType){
            case PieceType::Pawn:   return whitePawns;
            case PieceType::Knight: return whiteKnights;
            case PieceType::Bishop: return whiteBishops;
            case PieceType::Rook:   return whiteRooks;
            case PieceType::Queen:  return whiteQueens;
            case PieceType::King:   return whiteKings;
            default: break;
        }
    } else {
        switch(pieceType){
            case PieceType::Pawn:   return blackPawns;
            case PieceType::Knight: return blackKnights;
            case PieceType::Bishop: return blackBishops;
            case PieceType::Rook:   return blackRooks;
            case PieceType::Queen:  return blackQueens;
            case PieceType::King:   return blackKings;
            default: break;
        }
    }
    static Bitboard sink;
    return sink;
}

inline void Position::movePiece(Color c, PieceType pt, Square from, Square to){
    const Bitboard bb{bitboard_of(from) | bitboard_of(to)};
    pieceBb(c, pt) ^= bb;
    if(c == Color::White) whitePieces ^= bb;
    else                  blackPieces ^= bb;
    allPieces ^= bb;
    board[from] = SquareContent{};
    board[to] = SquareContent{c, pt};
    zobristHash ^= zobrist::pieceKey(c, pt, from) ^ zobrist::pieceKey(c, pt, to);
}

inline void Position::placePiece(Color c, PieceType pt, Square sq){
    const Bitboard bb{bitboard_of(sq)};
    pieceBb(c, pt) ^= bb;
    if(c == Color::White) whitePieces ^= bb;
    else                  blackPieces ^= bb;
    allPieces ^= bb;
    board[sq] = SquareContent{c, pt};
    zobristHash ^= zobrist::pieceKey(c, pt, sq);
}

inline void Position::removePiece(Color c, PieceType pt, Square sq){
    const Bitboard bb{bitboard_of(sq)};
    pieceBb(c, pt) ^= bb;
    if(c == Color::White) whitePieces ^= bb;
    else                  blackPieces ^= bb;
    allPieces ^= bb;
    board[sq] = SquareContent{};
    zobristHash ^= zobrist::pieceKey(c, pt, sq);
}

void Position::recomputeZobristHash(){
    zobristHash = 0;
    for(Square sq{squares::a1}; sq <= squares::h8; ++sq){
        SquareContent content{board[sq]};
        if(content.pieceType != PieceType::None && content.color != Color::None){
            zobristHash ^= zobrist::pieceKey(content.color, content.pieceType, sq);
        }
    }
    zobristHash ^= castleRightsHash(positionData.castleRights);
    if(positionData.enPassantTarget != squares::NoneSquare){
        zobristHash ^= zobrist::enPassantFile[file_of(positionData.enPassantTarget)];
    }
    if(sideToMove == Color::Black){
        zobristHash ^= zobrist::sideToMoveBlack;
    }
}

void Position::applyMove(Move const& move){
    history.push_back({positionData, opponentAttacks, checkers, zobristHash});

    const CastleRights oldCastleRights{positionData.castleRights};
    const Square oldEnPassant{positionData.enPassantTarget};

    const Square from{move.from()};
    const Square to{move.to()};

    const SquareContent fromContent{board[from]};
    const Color moverColor{fromContent.color};
    const PieceType moverType{fromContent.pieceType};
    const Color oppColor{(moverColor == Color::White) ? Color::Black : Color::White};

    if(move.isEnPassant()){
        const Square epSq{(moverColor == Color::White) ? to - 8 : to + 8};
        removePiece(oppColor, PieceType::Pawn, epSq);
        movePiece(moverColor, PieceType::Pawn, from, to);
    } else if(move.isPromotion()){
        if(move.isCapture()) removePiece(oppColor, move.capturedPieceType(), to);
        removePiece(moverColor, PieceType::Pawn, from);
        placePiece(moverColor, move.promotionPieceType(), to);
    } else if(move.isCapture()){
        removePiece(oppColor, move.capturedPieceType(), to);
        movePiece(moverColor, moverType, from, to);
    } else if(move.isCastle()){
        movePiece(moverColor, PieceType::King, from, to);
        const Square rookFrom{(to > from) ? from + 3u : from - 4u};
        const Square rookTo{(to > from) ? from + 1u : from - 1u};
        movePiece(moverColor, PieceType::Rook, rookFrom, rookTo);
    } else {
        movePiece(moverColor, moverType, from, to);
    }

    // Castle rights
    if(moverType == PieceType::King){
        if(moverColor == Color::White){
            positionData.castleRights.whiteKingside = false;
            positionData.castleRights.whiteQueenside = false;
        } else {
            positionData.castleRights.blackKingside = false;
            positionData.castleRights.blackQueenside = false;
        }
    } else if(moverType == PieceType::Rook){
        if(from == squares::a1)       positionData.castleRights.whiteQueenside = false;
        else if(from == squares::h1)  positionData.castleRights.whiteKingside  = false;
        else if(from == squares::a8)  positionData.castleRights.blackQueenside = false;
        else if(from == squares::h8)  positionData.castleRights.blackKingside  = false;
    }
    if(move.isCapture() && !move.isEnPassant() && move.capturedPieceType() == PieceType::Rook){
        if(to == squares::a1)        positionData.castleRights.whiteQueenside = false;
        else if(to == squares::h1)   positionData.castleRights.whiteKingside  = false;
        else if(to == squares::a8)   positionData.castleRights.blackQueenside = false;
        else if(to == squares::h8)   positionData.castleRights.blackKingside  = false;
    }

    // En passant target
    if(moverType == PieceType::Pawn){
        const Rank fromRank{rank_of(from)};
        const Rank toRank{rank_of(to)};
        if(moverColor == Color::White && fromRank == ranks::r2 && toRank == ranks::r4){
            positionData.enPassantTarget = from + 8;
        } else if(moverColor == Color::Black && fromRank == ranks::r7 && toRank == ranks::r5){
            positionData.enPassantTarget = from - 8;
        } else {
            positionData.enPassantTarget = squares::NoneSquare;
        }
    } else {
        positionData.enPassantTarget = squares::NoneSquare;
    }

    // 50-move clock, ply count
    if(moverType == PieceType::Pawn || move.isCapture()){
        positionData.pliesSinceLastCaptureOrPawnMove = 0;
    } else {
        ++positionData.pliesSinceLastCaptureOrPawnMove;
    }
    if(sideToMove == Color::Black) ++plyCount;

    // Zobrist: castle rights delta, EP file delta, side-to-move flip.
    zobristHash ^= castleRightsHash(oldCastleRights) ^ castleRightsHash(positionData.castleRights);
    if(oldEnPassant != squares::NoneSquare){
        zobristHash ^= zobrist::enPassantFile[file_of(oldEnPassant)];
    }
    if(positionData.enPassantTarget != squares::NoneSquare){
        zobristHash ^= zobrist::enPassantFile[file_of(positionData.enPassantTarget)];
    }
    zobristHash ^= zobrist::sideToMoveBlack;

    sideToMove = oppColor;

    updateAttackMap();
    updatePins();
}

void Position::undoMove(Move const& move){
    const Square from{move.from()};
    const Square to{move.to()};

    // After applyMove the side flipped; the piece at `to` belongs to the opposite side.
    const Color moverColor{(sideToMove == Color::White) ? Color::Black : Color::White};
    const Color oppColor{sideToMove};

    const PieceType placedType{board[to].pieceType};

    if(move.isEnPassant()){
        const Square epSq{(moverColor == Color::White) ? to - 8 : to + 8};
        movePiece(moverColor, PieceType::Pawn, to, from);
        placePiece(oppColor, PieceType::Pawn, epSq);
    } else if(move.isPromotion()){
        removePiece(moverColor, placedType, to);
        placePiece(moverColor, PieceType::Pawn, from);
        if(move.isCapture()) placePiece(oppColor, move.capturedPieceType(), to);
    } else if(move.isCapture()){
        movePiece(moverColor, placedType, to, from);
        placePiece(oppColor, move.capturedPieceType(), to);
    } else if(move.isCastle()){
        movePiece(moverColor, PieceType::King, to, from);
        const Square rookFrom{(to > from) ? from + 3u : from - 4u};
        const Square rookTo{(to > from) ? from + 1u : from - 1u};
        movePiece(moverColor, PieceType::Rook, rookTo, rookFrom);
    } else {
        movePiece(moverColor, placedType, to, from);
    }

    sideToMove = moverColor;

    StateSnapshot const& snapshot{history.back()};
    positionData = snapshot.positionData;
    opponentAttacks = snapshot.opponentAttacks;
    checkers = snapshot.checkers;
    zobristHash = snapshot.zobristHash;
    history.pop_back();

    updatePins();
}
