//
// Created by saffa on 6/28/2026.
//
#include <random>
#include "include/zobrist.h"

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