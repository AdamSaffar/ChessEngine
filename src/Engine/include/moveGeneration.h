//
// Created by saffa on 6/13/2026.
//

#ifndef CHESSENGINE_MOVEGENERATION_H
#define CHESSENGINE_MOVEGENERATION_H

#include "board.h"
#include "move.h"
extern U64 rookMasks[64];
extern U64  bishopMasks[64];


// Initialize functions
void initAllMoveGen();
void attackLookupTable();
void relevantBlockerMask();
void generateBlockedRookAttacks();
void generateBlockedBishopAttacks();
void generateMoves(MoveList& list);

// Helper functions
U64 setOccupancyHelper(int index, int bitsInMask, U64 mask);
U64 simulateRookAttacks(int square, U64 blockers);
U64 simulateBishopAttacks(int square, U64 blockers);

#endif //CHESSENGINE_MOVEGENERATION_H
