//
// Created by saffa on 6/13/2026.
//

#ifndef CHESSENGINE_MOVEGENERATION_H
#define CHESSENGINE_MOVEGENERATION_H
#include "move.h"
using U64 = uint64_t;
class Board; // forward declaration

// Use extern so other files can see these tables without duplicate memory
extern U64 rookMasks[64];
extern U64 bishopMasks[64];
extern U64 pawnAttacks[2][64];
extern U64 knightAttacks[64];
extern U64 bishopAttacks[64];
extern U64 rookAttacks[64];
extern U64 queenAttacks[64];
extern U64 kingAttacks[64];

// Initialize functions
void initAllMoveGen();
void attackLookupTable();
void relevantBlockerMask();
void generateBlockedRookAttacks();
void generateBlockedBishopAttacks();
void generateMoves(MoveList& list, const Board &board);

// Helper functions
U64 setOccupancyHelper(int index, int bitsInMask, U64 mask);
U64 simulateRookAttacks(int square, U64 blockers);
U64 simulateBishopAttacks(int square, U64 blockers);
U64 getBishopAttacks(int square, U64 liveBoard);
U64 getRookAttacks(int square, U64 liveBoard);
#endif //CHESSENGINE_MOVEGENERATION_H
