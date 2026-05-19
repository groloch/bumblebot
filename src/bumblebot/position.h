# ifndef POSITION_H
# define POSITION_H

# include <array>
# include <vector>

# include "types.h"
# include "bitboard.h"
# include "move.h"


class Position;

namespace movegen {
    void generatePseudoLegalAll(Position& position, MoveList& moveList);
    void generateLegalAll(Position& position, MoveList& moveList);
}

void comparePositions(Position const& pos1, Position const& pos2);


struct PositionData{
    PositionData()
        : castleRights{},
          enPassantTarget{squares::NoneSquare},
          pliesSinceLastCaptureOrPawnMove{0}
    {}

    PositionData(CastleRights castleRights,
                 Square enPassantTarget,
                 unsigned pliesSinceLastCaptureOrPawnMove)
        : castleRights{castleRights},
          enPassantTarget{enPassantTarget},
          pliesSinceLastCaptureOrPawnMove{pliesSinceLastCaptureOrPawnMove}
    {}

    PositionData(const PositionData& other) = default;

    CastleRights castleRights;
    Square enPassantTarget;
    unsigned pliesSinceLastCaptureOrPawnMove;
};


struct StateSnapshot{
    PositionData positionData;
    Bitboard opponentAttacks;
    Bitboard checkers;
    Hash zobristHash;
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

    unsigned halfmoveClock() const { return positionData.pliesSinceLastCaptureOrPawnMove; }

    bool canCastle(Color color, CastleDirection direction) const;

    bool isLegal(Move const& move) const;

    bool inCheck() const;

    bool doubleCheck() const;

    Hash hash() const { return zobristHash; }

    bool isRepetition() const;

    const std::array<Bitboard, 64>& pinMasks() const { return pinnedAvailableSquares; }

    void applyMove(Move const& move);
    void undoMove(Move const& move);

private:
    friend void movegen::generatePseudoLegalAll(Position&, MoveList&);
    friend void movegen::generateLegalAll(Position&, MoveList&);
    friend void comparePositions(Position const&, Position const&);

    Bitboard whitePawns, whiteKnights, whiteBishops, whiteRooks, whiteQueens, whiteKings;
    Bitboard blackPawns, blackKnights, blackBishops, blackRooks, blackQueens, blackKings;

    Bitboard whitePieces, blackPieces, allPieces;

    Bitboard opponentAttacks, checkers;

    PositionData positionData;

    Bitboard& pieceBb(Color color, PieceType pieceType);

    inline void movePiece(Color c, PieceType pt, Square from, Square to);
    inline void placePiece(Color c, PieceType pt, Square sq);
    inline void removePiece(Color c, PieceType pt, Square sq);

    void recomputeZobristHash();

    std::array<SquareContent, 64> board;

    Color sideToMove;

    std::array<Bitboard, 64> pinnedAvailableSquares;

    unsigned plyCount;

    Hash zobristHash;

    std::vector<StateSnapshot> history;
};


# endif
