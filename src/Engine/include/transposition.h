//
// Created by saffa on 6/28/2026.
//

#ifndef CHESSENGINE_TRANSPOSITION_H
#define CHESSENGINE_TRANSPOSITION_H
#include "include/zobrist.h"

enum TT_FLAG {
    HASH_EXACT, // fully search node and know the exact score
    HASH_ALPHA, // upper bound
    HASH_BETA // lower bound
};
// signal a failed lookup
const int UNKNOWN_SCORE = 1000000;
// Single memory cell struct
struct TTEntry { // Size of TTEntry: 24 bytes
    U64 zobristKey;
    int depth;
    int flag;
    int score;
    int bestMove;
};

void initTT(int megabytes);
void clearTT();
void storeTT(U64 zobristKey, int depth, int flag, int score, int bestMove);
int probeTT(U64 zobristKey, int depth, int alpha, int beta, int& ttMove);
#endif //CHESSENGINE_TRANSPOSITION_H
