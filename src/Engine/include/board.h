//
// Created by saffa on 6/7/2026.
//

#ifndef CHESSENGINE_BOARD_H
#define CHESSENGINE_BOARD_H

#include <cstdint>

using U64 = uint64_t;

/* Enums to make indexing readable */
enum COLOR {
    WHITE,
    BLACK,
    BOTH
};
enum PIECE_TYPE {
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

class Board {
private:
    // 12 distinct bitboards(6 for white, 6 for black)
    U64 pieceBitBoards[12];

    // Summary bitboards for fast occupancy checks(indexed by color)
    U64 occupancies[3];

    // Game state variables
    int sideToMove;
    int enPassantSquare;
    int castlingRights;

public:
    // Constructor
    Board();

    // --- Fast inline getters to fetch game state ---
    inline int getSideToMove() const { return sideToMove;}
    inline int getEnpassantSquare() const { return enPassantSquare;}
    inline int getCastlingRights() const { return castlingRights;}
    inline U64 getOccupancies(int color) const { return occupancies[color];} // Get colors total occupancy
    inline U64 getPieceBitBoard(int pieceType) const { return pieceBitBoards[pieceType];}

    void initStandardPosition();
    void printBoard();
};
#endif //CHESSENGINE_BOARD_H