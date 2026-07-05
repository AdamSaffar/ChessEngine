//
// Created by saffa on 6/29/2026.
//
#include "include/transposition.h"
#include <iostream>

// Global Vars
TTEntry* transpositionTable = nullptr;
int ttEntryCount = 0;

// initalizes new transposition table using allotted size, and frees old table memory
void initTT(int megabytes) {
    // find how many bytes we're allowed to use
    int hashSize = megabytes * 1024 * 1024; // 1 MB = 1024 * 1024
    // find how many TTEntry structs can fit in that allotted size
    ttEntryCount = hashSize / sizeof(TTEntry);

    // delete old table
    if (transpositionTable != nullptr) {
        delete[] transpositionTable;
    }

    // dynamically allocate new table
    transpositionTable = new TTEntry[ttEntryCount];

    clearTT(); // clear garbage data
}
// Helper to clear garbage data
void clearTT() {
    // loop through each cell and reset data
    for (int i = 0; i < ttEntryCount; i++) {
        transpositionTable[i].checksum = 0ULL;
        transpositionTable[i].data = 0ULL;
    }
}
// Write to Transposition table
void storeTT(U64 zobristKey, int depth, int flag, int score, int bestMove) {
    // find the correct array index
    int index = zobristKey % ttEntryCount;

    // Depth represents lowest 8 bits
    int exisitingDepth = (int)(transpositionTable[index].data & 0xFF);

    // REPLACEMENT: Only overwrite if new search is as deep or deeper
    if (exisitingDepth <= depth) {
        // pack the data into a single 64 bit integer
        U64 data = ((U64)(uint32_t)score << 32) |
            ((U64)(uint16_t)bestMove << 16) |
            ((U64)(uint8_t)flag << 8) |
            ((U64)(uint8_t)depth);

        // create checksum
        U64 checksum = zobristKey ^ data;

        // Write to memory
        transpositionTable[index].data = data;
        transpositionTable[index].checksum = checksum;
    }
}

int probeTT(U64 zobristKey, int depth, int alpha, int beta, int& ttMove) {
    int index = zobristKey % ttEntryCount;
    // Read entry from memory
    U64 checksum = transpositionTable[index].checksum;
    U64 data = transpositionTable[index].data;


    // Check if key matches the current board

    // XOR trick: Since checksum = zobristKey ^ data. If we XOR checksum and data
    // the result should exactly match the zobristKey
    if ((checksum ^ data) == zobristKey) {

        // Unpack data payload
        int entryDepth = (int)(data & 0xFF);
        int entryFlag  = (int)((data >> 8) & 0xFF);
        int entryMove  = (int)((data >> 16) & 0xFFFF);
        int entryScore = (int32_t)(data >> 32);

        // Save best move for move ordering
        ttMove = entryMove;

        // check if the saved memory is deep enough to skip search
        if (entryDepth >= depth) {
            // We have the exact evaluation
            if (entryFlag == HASH_EXACT) {
                return entryScore;
            }
            // Score was Bad
            if (entryFlag == HASH_ALPHA && entryScore <= alpha) {
                return alpha;
            }
            // Score was good
            if (entryFlag == HASH_BETA && entryScore >= beta) {
                return beta;
            }
        }
    }
    // No valid key match or depth wasnt deep enough to cutoff
    return UNKNOWN_SCORE;
}
