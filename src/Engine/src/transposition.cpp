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
        transpositionTable[i].zobristKey = 0;
        transpositionTable[i].depth = 0;
        transpositionTable[i].flag = 0;
        transpositionTable[i].score = 0;
        transpositionTable[i].bestMove = 0;
    }
}
// Write to Transposition table
void storeTT(U64 zobristKey, int depth, int flag, int score, int bestMove) {
    // find the correct array index
    int index = zobristKey % ttEntryCount;
    // REPLACEMENT: Only overwrite if new search is as deep or deeper
    if (transpositionTable[index].depth <= depth) {
        transpositionTable[index].zobristKey = zobristKey;
        transpositionTable[index].depth = depth;
        transpositionTable[index].flag = flag;
        transpositionTable[index].score = score;
        transpositionTable[index].bestMove = bestMove;
    }
}

int probeTT(U64 zobristKey, int depth, int alpha, int beta, int& ttMove) {
    int index = zobristKey % ttEntryCount;
    TTEntry entry = transpositionTable[index];

    // Check if key matches the current board
    if (entry.zobristKey == zobristKey) {
        // Save the best move
        ttMove = entry.bestMove;

        // check if the saved memory is deep enough to skip search
        if (entry.depth >= depth) {
            // We have the exact evaluation
            if (entry.flag == HASH_EXACT) {
                return entry.score;
            }
            // Score was Bad
            if (entry.flag == HASH_ALPHA && entry.score <= alpha) {
                return alpha;
            }
            // Score was good
            if (entry.flag == HASH_BETA && entry.score >= beta) {
                return beta;
            }
        }
    }
    // No valid key match or depth wasnt deep enough to cutoff
    return UNKNOWN_SCORE;
}
