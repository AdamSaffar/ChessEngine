//
// Created by saffa on 6/22/2026.
//

#ifndef CHESSENGINE_SEARCH_H
#define CHESSENGINE_SEARCH_H

// Search Constants
const int INF = 50000; // safe boundary for infinity

// Checkmate is worth more than any combination of pieces
const int MATE_VALUE = 50000;

int negamax(Board& board, int depth);
#endif //CHESSENGINE_SEARCH_H
