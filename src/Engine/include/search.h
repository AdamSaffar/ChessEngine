//
// Created by saffa on 6/22/2026.
//

#ifndef CHESSENGINE_SEARCH_H
#define CHESSENGINE_SEARCH_H
#include "board.h"
// Search Constants
const int INF = 50000; // safe boundary for infinity

// Checkmate is worth more than any combination of pieces
const int MATE_VALUE = 50000;

extern int bestMoveToPlay; // GUI will read this var when search finishes

int negamax(Board& board, int depth, int alpha, int beta, int ply);
void searchRoot(Board& board, int depth, std::chrono::time_point<std::chrono::steady_clock> startTime);
void searchHelper(Board board, int depth);
#endif //CHESSENGINE_SEARCH_H
