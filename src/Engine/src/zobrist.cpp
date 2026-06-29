//
// Created by saffa on 6/28/2026.
//
#include <random>
#include "include/zobrist.h"
#include "include/board.h"
U64 pieceKeys[12][64];
U64 enPassantKeys[64];
U64 castleKeys[16];
U64 sideKey;

std::mt19937_64 rng(1804289373); // FIXED seed(must be fixed to generate same zobrist keys every time
// set the dist range to cover all 64 bits
std::uniform_int_distribution<U64> dist(0, UINT64_MAX);

// generate random 64-bit num
U64 getRandomU64() {
    return dist(rng);
}

// Fill arrays with random numbers
void initZobrist() {
    // Fill piece keys (12 piece types, 64 squares)
    for (int piece = 0; piece < 12; piece++) {
        for (int square = 0; square < 64; square++) {
            pieceKeys[piece][square] = getRandomU64();
        }
    }
    // Fill En Passant Keys
    for (int square = 0; square < 64; square++) {
        enPassantKeys[square] = getRandomU64();
    }
    // Fill castling keys(castling rights are 4-bit integers 0-15)
    for (int i = 0; i < 16; i++) {
        castleKeys[i] = getRandomU64();
    }
    // Fill side to move key
    sideKey = getRandomU64();

}

/** Generate Zobrist Key(Will run this func exactly once after passing initial board setup)
 * No matter how many times this func is run, it will generate the same key every time
 * since we passed a fixed seed
 */
U64 generateHashKey(const Board& board) {
    U64 finalKey = 0;

    // XOR the pieces
    for (int piece = 0; piece < 12; piece++) {
        U64 bitboard = board.getPieceBitBoard(piece);
        while (bitboard) {
            int square = __builtin_ctzll(bitboard);
            finalKey ^= pieceKeys[piece][square];

            bitboard &= bitboard - 1;
        }
    }

    // XOR the pieces
    int enPassantSquare = board.getEnpassantSquare();
    if (enPassantSquare != -1) {
        finalKey ^= enPassantKeys[enPassantSquare];
    }

    // XOR castling right
    int castlingRights = board.getCastlingRights();
    finalKey ^= castleKeys[castlingRights];

    // XOR side to move
    int sideToMove = board.getSideToMove();
    // Only XOR sidekey if its blacks turn
    if (board.getSideToMove() == COLOR::BLACK) {
        finalKey ^= sideKey;
    }

    return finalKey;
}