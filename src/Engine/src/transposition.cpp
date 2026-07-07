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
        transpositionTable[i].checksum.store(0ULL, std::memory_order_relaxed);
        transpositionTable[i].data.store(0ULL, std::memory_order_relaxed);
    }
}
// Write to Transposition table
void storeTT(U64 zobristKey, int depth, int flag, int score, int bestMove) {
    // find the correct array index
    int index = zobristKey % ttEntryCount;

    U64 existingData = transpositionTable[index].data.load(std::memory_order_relaxed);
    U64 existingChecksum = transpositionTable[index].checksum.load(std::memory_order_relaxed);

    // Depth represents lowest 8 bits
    int existingDepth = (int)(existingData & 0xFF);

    // Check if the current entry belongs to the exact same board state
    bool isSamePosition = ((existingChecksum ^ existingData) == zobristKey);

    bool shouldReplace = false;

    if (isSamePosition) {
        // NO COLLISION: overwrite if new search is as deep or deeper
        if (depth >= existingDepth || flag == HASH_EXACT) {
            shouldReplace = true;
        }
    } else {
        // HASH COLLISION: protect high-depth evaluations from low-depth evals from helper threads
        if (depth >= existingDepth - 3) {
            shouldReplace = true;
        }
    }

    // Apply the save 
    if (shouldReplace) {
        // pack the data into a single 64 bit integer
        U64 data = ((U64)(uint32_t)score << 32) |
            ((U64)(uint16_t)bestMove << 16) |
            ((U64)(uint8_t)flag << 8) |
            ((U64)(uint8_t)depth);

        // create checksum
        U64 checksum = zobristKey ^ data;

        // Write to memory
        transpositionTable[index].data.store(data, std::memory_order_relaxed);
        // std::memory_order_release forces CPU to finish writing data to mem before it writes checksum
        transpositionTable[index].checksum.store(checksum, std::memory_order_release);
    }
}

int probeTT(U64 zobristKey, int depth, int alpha, int beta, int& ttMove) {
    int index = zobristKey % ttEntryCount;
    // Read entry from memory

    // std::memory_order_acquire forces CPU to finish reading checksum before reading data
    U64 checksum = transpositionTable[index].checksum.load(std::memory_order_acquire);
    U64 data = transpositionTable[index].data.load(std::memory_order_relaxed);


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
