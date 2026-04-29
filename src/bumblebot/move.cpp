#include "move.h"
#include "position.h"

#include "debugging.h"


Bitboard generatePawnQuietMoves(Position& position, Square square, Color color){
    Bitboard pawnBb{bitboard_of(square)};

    Bitboard possible_to{0};
    if(color == Color::White){
        Bitboard pawnUp{shift<Direction::North>(pawnBb) & ~position.allPieces};
        possible_to |= pawnUp;
        if(rank_of(square) == ranks::r2 && pawnUp != 0){
            possible_to |= shift<Direction::North>(pawnUp) & ~position.allPieces;
        }
    } else if(color == Color::Black){
        Bitboard pawnDown{shift<Direction::South>(pawnBb) & ~position.allPieces};
        possible_to |= pawnDown;
        if(rank_of(square) == ranks::r7 && pawnDown != 0){
            possible_to |= shift<Direction::South>(pawnDown) & ~position.allPieces;
        }
    }
    return possible_to;
}

void generatePseudoLegalMoves(Position& position, MoveList& moveList, bool checkLegal){
    position.updateBitboards();
    position.updateAttackMap();

    for(Square square{squares::a1}; square <= squares::h8; ++square){
        SquareContent content{position.getSquareContent(square)};

        if(content.pieceType == PieceType::None || content.color != position.turn()) continue;

        Bitboard attacks{position.getAttackMap(square, content)};

        bool isPawnMove{content.pieceType == PieceType::Pawn};

        if(isPawnMove){
            attacks |= generatePawnQuietMoves(position, square, position.turn());
        }
        Move move;

        while(attacks != 0){
            Square to{poplsb(attacks)};

            bool isCapture{position.getSquareContent(to).pieceType != PieceType::None};
            PieceType capturedPieceType{position.getSquareContent(to).pieceType};

            if(isPawnMove && file_of(to) != file_of(square)){
                if(!isCapture || position.getSquareContent(to).color == position.turn()) continue;
            }

            if(isPawnMove && rank_of(to) == (position.turn() == Color::White ? ranks::r8 : ranks::r1)){
                // promotion
                for(PieceType promotionPieceType : {
                    PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight
                }){
                    move = {
                        square, to, 
                        false, false, true, isCapture, true,
                        promotionPieceType, capturedPieceType
                    };
                    if(!checkLegal || position.isLegal(move)){
                        moveList.append(move);
                    }
                }
            }
            else{
                // normal move
                move = {
                    square, to, 
                    false, false, false, isCapture, isPawnMove,
                    PieceType::None, capturedPieceType
                };
                if(!checkLegal || position.isLegal(move)){
                    moveList.append(move);
                }
            }
        }

        if(isPawnMove && position.enPassant() != squares::NoneSquare){ // en passant
            Bitboard possible_to{0};
            if(position.turn() == Color::White){
                possible_to |= shift<Direction::NorthEast>(bitboard_of(square));
                possible_to |= shift<Direction::NorthWest>(bitboard_of(square));
            }else if(position.turn() == Color::Black){ 
                possible_to |= shift<Direction::SouthEast>(bitboard_of(square));
                possible_to |= shift<Direction::SouthWest>(bitboard_of(square));
            }

            if(possible_to & bitboard_of(position.enPassant())){
                move = {
                    square, position.enPassant(), true, false, false, true, true, PieceType::None, PieceType::Pawn
                };
                if(!checkLegal || position.isLegal(move)){
                    moveList.append(move);
                }
            }
        } else if(content.pieceType == PieceType::King){
            // castle
            Bitboard kingsideLookup, queensideLookup;
            Square kingsideTo, queensideTo;
            if(position.turn() == Color::White){
                kingsideLookup = bitboards::bb_f1 | bitboards::bb_g1;
                queensideLookup = bitboards::bb_b1 | bitboards::bb_c1 | bitboards::bb_d1;
                kingsideTo = squares::g1;
                queensideTo = squares::c1;
            } else if(position.turn() == Color::Black){
                kingsideLookup = bitboards::bb_f8 | bitboards::bb_g8;
                queensideLookup = bitboards::bb_b8 | bitboards::bb_c8 | bitboards::bb_d8;
                kingsideTo = squares::g8;
                queensideTo = squares::c8;
            }
            if(position.canCastle(position.turn(), CastleDirection::Kingside) 
                && !(position.allPieces & kingsideLookup)){
                move = {
                    square, kingsideTo, false, true, false, false, false
                };
                if(!checkLegal || position.isLegal(move)){
                    moveList.append(move);
                }
            }
            if(position.canCastle(position.turn(), CastleDirection::Queenside) 
                && !(position.allPieces & queensideLookup)){
                move = {
                    square, queensideTo, false, true, false, false, false
                };
                if(!checkLegal || position.isLegal(move)){
                    moveList.append(move);
                }
            }
        }
    }
}

void generateLegalMoves(Position& position, MoveList& moveList){
    generatePseudoLegalMoves(position, moveList, true);
}

template<>
void generateMoves<MoveType::Legal>(Position& position, MoveList& moveList){
    generateLegalMoves(position, moveList);
}

template<>
void generateMoves<MoveType::PseudoLegal>(Position& position, MoveList& moveList){
    generatePseudoLegalMoves(position, moveList);
}

template<>
void generateMoves<MoveType::Escape>(Position& position, MoveList& moveList){
}