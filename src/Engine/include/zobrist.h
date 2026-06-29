//
// Created by saffa on 6/28/2026.
//

#ifndef CHESSENGINE_ZOBRIST_H
#define CHESSENGINE_ZOBRIST_H

#include "board.h"

extern U64 pieceKeys[12][64];
extern U64 enPassantKeys[64];
extern U64 castleKeys[16];
extern U64 sideKey;

void initZobrist();
U64 generateHashKey(const Board& board);
#endif //CHESSENGINE_ZOBRIST_H
