# include "position.h"
# include "debugging.h"


Position::Position() : board{},
sideToMove{Color::White}, dirtyBitboards{true},
dirtyAttacks{true}, dirtyPins{true}, positionData{}, plyCount{0} {

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
}

Position::Position(const char* fen) : board{}, sideToMove{Color::White}, dirtyBitboards{true},
dirtyAttacks{true}, dirtyPins{true}, positionData{}, plyCount{0} {

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
    bitboardToggle(currentContent.pieceType, currentContent.color, square);
    bitboardToggle(content.pieceType, content.color, square);
    currentContent.pieceType = content.pieceType;
    currentContent.color = content.color;
}

void Position::setSquareContent(File file, Rank rank, SquareContent content) {
    setSquareContent({rank * 8 + file}, content);
}

void Position::updateBitboards() {
    if (!dirtyBitboards) return;

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

    dirtyBitboards = false;
}

Bitboard Position::getAttackMap(Square square, SquareContent content, bool ponder) const {
    const Bitboard square_bb{bitboard_of(square)};
    Bitboard allRelevantBlockers{
        allPieces
    };
    if(ponder){
        allRelevantBlockers &= ~kingBitboard();
    }
    const Bitboard friendlyPieces{
        (content.color == Color::White) ? whitePieces : blackPieces
    };

    Bitboard attacks{0};
    switch (content.pieceType) {
        case PieceType::Pawn:
            if(content.color == Color::White){
                attacks |= shift<Direction::NorthEast>(square_bb);
                attacks |= shift<Direction::NorthWest>(square_bb);
            } else {
                attacks |= shift<Direction::SouthEast>(square_bb);
                attacks |= shift<Direction::SouthWest>(square_bb);
            }
            break;
        case PieceType::Knight:
            attacks |= shift<Direction::NorthEast>(shift<Direction::North>(square_bb));
            attacks |= shift<Direction::NorthEast>(shift<Direction::East>(square_bb));
            attacks |= shift<Direction::NorthWest>(shift<Direction::North>(square_bb));
            attacks |= shift<Direction::NorthWest>(shift<Direction::West>(square_bb));
            attacks |= shift<Direction::SouthEast>(shift<Direction::South>(square_bb));
            attacks |= shift<Direction::SouthEast>(shift<Direction::East>(square_bb));
            attacks |= shift<Direction::SouthWest>(shift<Direction::South>(square_bb));
            attacks |= shift<Direction::SouthWest>(shift<Direction::West>(square_bb));
            break;
        case PieceType::Bishop:
            attacks = bishopMagics[square].attacks_bb(allRelevantBlockers);
            break;
        case PieceType::Rook:
            attacks = rookMagics[square].attacks_bb(allRelevantBlockers);
            break;
        case PieceType::Queen:
            attacks = rookMagics[square].attacks_bb(allRelevantBlockers);
            attacks |= bishopMagics[square].attacks_bb(allRelevantBlockers);
            break;
        case PieceType::King:
            attacks |= shift<Direction::North>(square_bb);
            attacks |= shift<Direction::South>(square_bb);
            attacks |= shift<Direction::East>(square_bb);
            attacks |= shift<Direction::West>(square_bb);
            attacks |= shift<Direction::NorthEast>(square_bb);
            attacks |= shift<Direction::NorthWest>(square_bb);
            attacks |= shift<Direction::SouthEast>(square_bb);
            attacks |= shift<Direction::SouthWest>(square_bb);
            break;
        default:
            break;
    }
    if(!ponder){
        attacks &= ~friendlyPieces;
    }
    return attacks;
}

void Position::updateAttackMap() {
    if (!dirtyAttacks) return;

    updateBitboards();

    opponentAttacks = 0;
    rookAttacks = 0;
    bishopAttacks = 0;
    checkers = 0;

    Bitboard attackers{
        turn() == Color::White ? blackPieces : whitePieces
    };
    while(attackers != 0){
        Square square{poplsb(attackers)};
        SquareContent content{getSquareContent(square)};

        Bitboard attackMap{getAttackMap(square, content, true)};
        opponentAttacks |= attackMap;

        if(content.pieceType == PieceType::Rook || content.pieceType == PieceType::Queen){
            rookAttacks |= attackMap;
        }
        if(content.pieceType == PieceType::Bishop || content.pieceType == PieceType::Queen){
            bishopAttacks |= attackMap;
        }
        if(attackMap & kingBitboard()){
            checkers |= bitboard_of(square);
        }
    }

    dirtyAttacks = false;
}

void Position::updatePins(){
    if(!dirtyPins) return;

    updateAttackMap();

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
    dirtyPins = false;
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
    // TODO
    SquareContent fromContent{getSquareContent(move.from)};
    SquareContent toContent{getSquareContent(move.to)};

    if(fromContent.pieceType == PieceType::None || fromContent.color != sideToMove) return false;
    if(toContent.pieceType != PieceType::None && toContent.color == sideToMove) return false;

    if(doubleCheck() && fromContent.pieceType != PieceType::King) return false;
    if(inCheck()){
        Square kingSquare{lsb(kingBitboard())};
        if(fromContent.pieceType == PieceType::King){
            return !move.isCastle && (opponentAttacks & bitboard_of(move.to)) == 0;
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
        bool stopsCheck{(bitboard_of(move.to) & inBetweenMask) != 0};
        if(move.isEnPassant){
            Square epPawnSquare{(sideToMove == Color::White) ? move.to - 8 : move.to + 8};
            stopsCheck = stopsCheck || (bitboard_of(epPawnSquare) & inBetweenMask) != 0;
        }
        bool available{(bitboard_of(move.to) & pinnedAvailableSquares[move.from]) != 0};
        return stopsCheck && available;
    }
    if(fromContent.pieceType == PieceType::King){
        if(move.isCastle){
            CastleDirection direction{
                (move.to > move.from) ? CastleDirection::Kingside : CastleDirection::Queenside
            };
            if(!canCastle(sideToMove, direction)) return false;

            Bitboard checkSquares{inBetween<PieceType::Rook>(move.from, move.to)};
            return (checkSquares & opponentAttacks) == 0;
        }
        return (opponentAttacks & bitboard_of(move.to)) == 0;
    } else if(!move.isEnPassant){
        return (bitboard_of(move.to) & pinnedAvailableSquares[move.from]) != 0;
    } else{
        if((bitboard_of(move.to) & pinnedAvailableSquares[move.from]) == 0) return false;

        // check for rook-like alignment because ep removes 2 pieces on a rank at once
        Bitboard kingBb{kingBitboard()};
        Square kingSquare{lsb(kingBb)};
        Square epPawnSquare{(sideToMove == Color::White) ? move.to - 8 : move.to + 8};
        Bitboard opponentRookLikes{
            (sideToMove == Color::White) ? blackRooks | blackQueens : whiteRooks | whiteQueens
        };
        opponentRookLikes &= (bitboard_of_rank(rank_of(kingSquare)));
        while(opponentRookLikes != 0){
            Square sniperSquare{poplsb(opponentRookLikes)};
            Bitboard between{inBetween<PieceType::Rook>(kingSquare, sniperSquare)};
            between &= ~bitboard_of(move.from) & ~bitboard_of(epPawnSquare);
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

void Position::applyMove(Move const& move){
    SquareContent fromContent{getSquareContent(move.from)};
    SquareContent toContent{getSquareContent(move.to)};

    if(move.isPromotion){
        setSquareContent(move.to, SquareContent{turn(), move.promotionPieceType});
    } else {
        setSquareContent(move.to, fromContent);
    }
    setSquareContent(move.from, SquareContent{});
    if(move.isEnPassant){
        Square takenPawnSquare{(turn() == Color::White) ? move.to - 8 : move.to + 8};
        setSquareContent(takenPawnSquare, SquareContent{});
    }
    if(move.isCastle){
        if(move.to > move.from){
            // kingside
            Square rookFrom{move.from + 3};
            Square rookTo{move.from + 1};
            SquareContent rookContent{getSquareContent(rookFrom)};
            setSquareContent(rookTo, rookContent);
            setSquareContent(rookFrom, SquareContent{});
        } else {
            // queenside
            Square rookFrom{move.from - 4};
            Square rookTo{move.from - 1};
            SquareContent rookContent{getSquareContent(rookFrom)};
            setSquareContent(rookTo, rookContent);
            setSquareContent(rookFrom, SquareContent{});
        }
    }
    if(fromContent.pieceType == PieceType::King){
        if(sideToMove == Color::White){
            positionData.castleRights.whiteKingside = false;
            positionData.castleRights.whiteQueenside = false;
        } else {
            positionData.castleRights.blackKingside = false;
            positionData.castleRights.blackQueenside = false;
        }
    } else if (fromContent.pieceType == PieceType::Rook){
        if(move.from == squares::a1){
            positionData.castleRights.whiteQueenside = false;
        } else if(move.from == squares::h1){
            positionData.castleRights.whiteKingside = false;
        } else if(move.from == squares::a8){
            positionData.castleRights.blackQueenside = false;
        } else if(move.from == squares::h8){
            positionData.castleRights.blackKingside = false;
        }
    } else if(move.isCapture && toContent.pieceType == PieceType::Rook){
        if(move.to == squares::a1){
            positionData.castleRights.whiteQueenside = false;
        } else if(move.to == squares::h1){
            positionData.castleRights.whiteKingside = false;
        } else if(move.to == squares::a8){
            positionData.castleRights.blackQueenside = false;
        } else if(move.to == squares::h8){
            positionData.castleRights.blackKingside = false;
        }
    }

    if(fromContent.pieceType == PieceType::Pawn){
        Rank fromRank{rank_of(move.from)};
        Rank toRank{rank_of(move.to)};
        if(turn() == Color::White && fromRank == ranks::r2 && toRank == ranks::r4){
            positionData.enPassantTarget = move.from + 8;
        } else if(turn() == Color::Black && fromRank == ranks::r7 && toRank == ranks::r5){
            positionData.enPassantTarget = move.from - 8;
        } else {
            positionData.enPassantTarget = squares::NoneSquare;
        }
    }else{
        positionData.enPassantTarget = squares::NoneSquare;
    }

    if(fromContent.pieceType == PieceType::Pawn || move.isCapture){
        positionData.pliesSinceLastCaptureOrPawnMove = 0;
    } else {
        ++positionData.pliesSinceLastCaptureOrPawnMove;
    }
    if(sideToMove == Color::Black){
        ++plyCount;
    }

    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
    dirtyAttacks = true;
    dirtyPins = true;

    updateAttackMap();
    updatePins();
}

void Position::undoMove(Move const& move, PositionData const& previousData){
    SquareContent toContent{getSquareContent(move.to)};

    setSquareContent(move.to, SquareContent{});

    if(!move.isPromotion){
        setSquareContent(move.from, toContent);
    }else {
        setSquareContent(move.from, SquareContent{toContent.color, PieceType::Pawn});
    }

    if(!move.isCapture){
        setSquareContent(move.to, SquareContent{});
    }else if(!move.isEnPassant){
        setSquareContent(move.to, SquareContent{
            toContent.color == Color::White ? Color::Black : Color::White, move.capturedPieceType
        });
    }else{
        Square epPawnSquare{(toContent.color == Color::White) ? move.to - 8 : move.to + 8};
        setSquareContent(epPawnSquare, SquareContent{
            toContent.color == Color::White ? Color::Black : Color::White, PieceType::Pawn
        });
    }

    if(move.isCastle){
        Square rookAfter, rookBefore;
        if(move.to > move.from){
            // kingside
            rookAfter = move.from + 1;
            rookBefore = move.from + 3;
        } else {
            // queenside
            rookAfter = move.from - 1;
            rookBefore = move.from - 4;
        }
        setSquareContent(rookAfter, {});
        setSquareContent(rookBefore, SquareContent{
            toContent.color, PieceType::Rook
        });
    }

    sideToMove = (sideToMove == Color::White) ? Color::Black : Color::White;
    // dirtyBitboards = true;
    // dirtyAttacks = true;
    dirtyPins = true;

    positionData = previousData;

    // updateBitboards();
    // updateAttackMap();
    // updatePins();
}


void Position::bitboardToggle(PieceType pieceType, Color color, Square square){
    if (pieceType == PieceType::None || color == Color::None || square == squares::NoneSquare) {
        return;
    }
    Bitboard square_bb{bitboard_of(square)};

    if (color == Color::White){
        if (pieceType == PieceType::Pawn){
            whitePawns ^= square_bb;
        } else if (pieceType == PieceType::Knight){
            whiteKnights ^= square_bb;
        } else if (pieceType == PieceType::Bishop){
            whiteBishops ^= square_bb;
        } else if (pieceType == PieceType::Rook){
            whiteRooks ^= square_bb;
        } else if (pieceType == PieceType::Queen){
            whiteQueens ^= square_bb;
        } else if (pieceType == PieceType::King){
            whiteKings ^= square_bb;
        }
        whitePieces ^= square_bb;
    } else if (color == Color::Black){
        if (pieceType == PieceType::Pawn){
            blackPawns ^= square_bb;
        } else if (pieceType == PieceType::Knight){
            blackKnights ^= square_bb;
        } else if (pieceType == PieceType::Bishop){
            blackBishops ^= square_bb;
        } else if (pieceType == PieceType::Rook){
            blackRooks ^= square_bb;
        } else if (pieceType == PieceType::Queen){
            blackQueens ^= square_bb;
        } else if (pieceType == PieceType::King){
            blackKings ^= square_bb;
        }
        blackPieces ^= square_bb;
    }
    allPieces ^= square_bb;
}
