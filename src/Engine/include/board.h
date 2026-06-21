//
// Created by saffa on 6/7/2026.
//

#ifndef CHESSENGINE_BOARD_H
#define CHESSENGINE_BOARD_H

#include <cstdint>
#include "moveGeneration.h"
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
    /** 4 bit integer to represent castlingRights (1 = true, 0 = false):
     * Bit 0: White KingSide
     * Bit 1: White QueenSide
     * Bit 2: Black KingSide
     * Bit 3: Black QueenSide
     */
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
    inline int getPieceAt(unsigned int startSquare) const {
        U64 squareMask = 1ULL << startSquare;
        // loop through all 12 bitoboards
        for (int i = 0; i < 12; i++) {
            // if a bitboard overalps with our square mask, piece found
            if (pieceBitBoards[i] & squareMask) {
                return i; // return index (0 to 11) of the piece
            }
        }
        // square is empty
        return -1;
    }
    inline bool isSquareAttacked(int square, int enemyColor) const {
        // use offset to grab correct enemy bitboard
        int offset = (enemyColor == COLOR::BLACK) ? 6 : 0;
        // use defenders colors to look up the pawn attack mask
        int defenderColor = enemyColor ^ 1;
        // --- PAWNS ---
        if (pawnAttacks[defenderColor][square] & pieceBitBoards[PIECE_TYPE:: Pawn + offset]) return true;
        // --- KNIGHTS ---
        if (knightAttacks[square] & pieceBitBoards[PIECE_TYPE::Knight + offset]) return true;

        // --- KINGS ---
        if (kingAttacks[square] & pieceBitBoards[PIECE_TYPE::King + offset]) return true;

        // --- BISHOPS & QUEENS ---
        U64 diagonalAttackers = pieceBitBoards[PIECE_TYPE::Bishop + offset] | pieceBitBoards[PIECE_TYPE::Queen + offset];
        if (getBishopAttacks(square, occupancies[2]) & diagonalAttackers) return true;

        // --- ROOKS & QUEENS ---
        U64 straightAttackers = pieceBitBoards[PIECE_TYPE::Rook + offset] | pieceBitBoards[PIECE_TYPE::Queen + offset];
        if (getRookAttacks(square, occupancies[2]) & straightAttackers) return true;

        // If it passed all checks, the square is completely safe
        return false;
    }
    friend bool makeMove(uint16_t move, Board& board); // Use "friend" to give function access to private members
    void initStandardPosition();
    void printBoard();
};
#endif //CHESSENGINE_BOARD_H