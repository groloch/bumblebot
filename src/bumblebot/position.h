# ifndef POSITION_H
# define POSITION_H

# include <array>
# include <vector>

# include "types.h"
# include "bitboard.h"
# include "move.h"


struct PositionData{
    PositionData() : castleRights{}, enPassantTarget{squares::NoneSquare}, 
    pliesSinceLastCaptureOrPawnMove{0}, plyCount{0}, previous{nullptr} {}

    PositionData(CastleRights castleRights, Square enPassantTarget, unsigned pliesSinceLastCaptureOrPawnMove, 
        unsigned plyCount, PositionData* previous)
    : castleRights{castleRights}, enPassantTarget{enPassantTarget}, 
    pliesSinceLastCaptureOrPawnMove{pliesSinceLastCaptureOrPawnMove}, plyCount{plyCount}, previous{previous} {}

    CastleRights castleRights;
    Square enPassantTarget;
    unsigned pliesSinceLastCaptureOrPawnMove;
    unsigned plyCount;

    PositionData* previous;
};


class Position {
public:
    Position();
    Position(const Position& other) = default;
    Position(const char* fen);

    SquareContent getSquareContent(Square square) const;
    SquareContent getSquareContent(File file, Rank rank) const;

    void setSquareContent(Square square, SquareContent content);
    void setSquareContent(File file, Rank rank, SquareContent content);

    void updateBitboards();

    Bitboard getAttackMap(Square square, SquareContent content, bool ponder = false) const;

    void updateAttackMap();

    void updatePins();

    Bitboard kingBitboard() const;

    Color turn() const;

    Square enPassant() const;

    bool canCastle(Color color, CastleDirection direction) const;

    bool isLegal(Move const& move) const;

    bool inCheck() const;

    bool doubleCheck() const;

    void applyMove(Move const& move);
    void undoMove(Move const& move);

    Bitboard whitePawns, whiteKnights, whiteBishops, whiteRooks, whiteQueens, whiteKings;
    Bitboard blackPawns, blackKnights, blackBishops, blackRooks, blackQueens, blackKings;
    
    Bitboard whitePieces, blackPieces, allPieces;

private:
    std::array<SquareContent, 64> board;
    
    Color sideToMove;

    PositionData* positionData;

    Bitboard opponentAttacks, checkers;
    Bitboard rookAttacks, bishopAttacks;

    std::array<Bitboard, 64> pinnedAvailableSquares;

    bool dirtyBitboards, dirtyAttacks, dirtyPins;
};


# endif
