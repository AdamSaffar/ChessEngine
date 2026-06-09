//
// Created by saffa on 6/7/2026.
//
#include "board.h"

/*
 * There are 64 possible squares for a piece to be.
 * Each piece has a certain number of possible moves.
 */
U64 pawnAttacks[2][64]; // [COLOR][SQUARE], pawns are the only piece that cares about direction
U64 knightAttacks[64];
U64 bishopAttacks[64];
U64 rookAttacks[64];
U64 queenAttacks[64];
U64 kingAttacks[64];

// Boundaries (Prevents warping/teleporting)
const U64 notAFile =  0xFEFEFEFEFEFEFEFEULL; // equivalently: 11111110 11111110 11111110 11111110 11111110 11111110 11111110 11111110
const U64 notHFile = 0x7F7F7F7F7F7F7F7FULL; // equivalently: 01111111 01111111 01111111 01111111 01111111 01111111 01111111 01111111
// Knight moves(Left 2 OR Right 2):
const U64 notGHFile = 0x3F3F3F3F3F3F3F3FULL; // equivalently: 00111111 00111111 00111111 00111111 00111111 00111111 00111111 00111111
const U64 notABFile = 0xFCFCFCFCFCFCFCFCULL; // equivalently: 11111100 11111100 11111100 11111100 11111100 11111100 11111100 11111100


/*this lookup table is meant to serve as an exhaustive list of every POSSIBLE ATTACKING MOVE
 that can be made with each individual piece. Not necessary every LEGAL move that can be made. */
void attackLookupTable() {
    for (int i = 0; i <= 63; i++) {
        // single piece placed on the current square
        U64 piece = 1ULL << i;

        // store possibleAttacks
        U64 attacks = 0ULL;

        // ---WHITE PAWN ATTACKS---
        attacks |= ((piece << 7) & notHFile) | ((piece << 9) & notAFile); // Up 1 Left 1, Up 1 Right 1
        pawnAttacks[COLOR::WHITE][i] = attacks;
        attacks = 0ULL; // reset attacks

        // ---BLACK PAWN ATTACKS---
        attacks |= (piece >> 7 & notAFile) | (piece >> 9 & notHFile); // Down 1 Right 1, Down 1 Left 1
        pawnAttacks[COLOR::BLACK][i] = attacks;
        attacks = 0ULL;

        // ---KNIGHT ATTACKS (8 attacking moves)---
        attacks |= ((piece << 15) & notHFile) | ((piece << 17) & notAFile); // Up 2 Left 1, Up 2 Right 1
        attacks |= ((piece << 6) & notGHFile) | ((piece << 10) & notABFile); // Up 1 Left 2, Up 1 Right 2
        attacks |= ((piece >> 10) &  notGHFile) | ((piece >> 6) & notABFile); // Down 1 Left 2, Down 1 Right 2
        attacks |= ((piece >> 17) & notHFile) | ((piece >> 15) & notAFile); // Down 2 Left 1, Down 2 Right 1
        knightAttacks[i] = attacks;
        attacks = 0ULL;

        // ---BISHOP ATTACKS & QUEEN DIAGONALS---

        // Up Right (Left Shift)
        int x = 9;
        while ((x + i) <= 63 && ((piece << x) & notAFile) != 0ULL) {
            attacks |= (piece << x);
            x += 9;
        }
        // Down Left (Right Shift)
        x = 9;
        while ((i - x) >= 0 && ((piece >> x) & notHFile) != 0ULL) {
            attacks |= (piece >> x);
            x += 9;
        }
        // Up Left (Left Shift)
        x = 7;
        while ((x + i) <= 63 && ((piece << x) & notHFile) != 0ULL) {
            attacks |= (piece << x);
            x += 7;
        }

        // Down Right (Right Shift)
        x = 7;
        while ((i - x) >= 0 && ((piece >> x) & notAFile) != 0ULL) {
            attacks |= (piece >> x);
            x += 7;
        }

        bishopAttacks[i] = attacks;
        // In chess, a queen is the combination of a Rook and a Bishop
        queenAttacks[i] |= attacks;
        attacks = 0ULL;

        // --- ROOK ATTACKS & HORIZONTAL + VERTICAL QUEEN ---

        // UP (Left Shift)
        x = 8;
        while ((i + x) <= 63 && ((piece << x) != 0ULL)) {
            attacks |= piece << x;
            x += 8;
        }
        // DOWN (Right Shift)
        x = 8;
        while ((i - x) >= 0 && ((piece >> x) != 0ULL)) {
            attacks |= piece >> x;
            x += 8;
        }
        // RIGHT (LEFT SHIFT)
        x = 1;
        while ((i + x) <= 63 && ((piece << x & notAFile) != 0ULL)) {
            attacks |= piece << x;
            x += 1;
        }
        // LEFT (RIGHT SHIFT)
        x = 1;
        while ((i - x) >= 0 && ((piece >> x & notHFile) != 0ULL)) {
            attacks |= piece >> x;
            x += 1;
        }
        rookAttacks[i] = attacks;
        // In chess, a queen is the combination of a Rook and a Bishop
        queenAttacks[i] |= attacks;
        attacks = 0ULL;

        // UP & DOWN does not need file masks since vertical wrap around does not exist
        attacks |= (piece << 1 & notAFile) | (piece >> 1 & notHFile) | (piece << 8) | (piece >> 8); // RIGHT, LEFT, UP, DOWN
        attacks |= ((piece << 9) & notAFile) | ((piece >> 9) & notHFile) | ((piece << 7) & notHFile) | ((piece >> 7) & notAFile); // UP RIGHT, DOWN LEFT, UP LEFT, DOWN RIGHT
        kingAttacks[i] = attacks;
    }

}


// Ignore Edges of the Board:
U64 not8thRank = 0x00FFFFFFFFFFFFFFULL; // equal: 00000000 11111111 11111111 11111111 11111111 11111111 11111111 11111111
U64 not1stRank = 0xFFFFFFFFFFFFFF00ULL; // equal: 11111111 11111111 11111111 11111111 11111111 11111111 11111111 00000000
// Already initialized not A and H files

// For every square, we define the exact bounding box of the pieces vision
U64 rookMasks[64];
U64 bishopMasks[64];
U64 queenMasks[64]; // Combination of rookMasks and bishopMasks
/* map of the only squares on the entire board that have the power to block this specific piece.
 * DOES NOT INCLUDE EDGES(since any piece lying on edge can't block the scope of other pieces
 */
void relevantBlockerMask() {
    for (int square = 0; square < 64; square++) {
        U64 piece = 1ULL << square;
        U64 mask = 0ULL;
        // ---ROOK MASK---

        // UP (left shift)
        U64 ray = piece;
        while ((ray & not8thRank) && ((ray << 8) & not8thRank)) { // stop BEFORE 8th rank
            ray <<= 8;
            mask |= ray;
        }
        // DOWN (right shift)
        ray = piece;
        while ((ray & not1stRank) && ((ray >> 8) & not1stRank)) { // stop BEFORE 1st rank
            ray >>= 8;
            mask |= ray;
        }
        // RIGHT (LEFT SHIFT)
        ray = piece;
        while ((ray & notHFile) && ((ray << 1) & notHFile)) { // stop BEFORE H file
            ray <<= 1;
            mask |= ray;
        }
        // LEFT (RIGHT SHIFT)
        ray = piece;
        while ((ray & notAFile) && ((ray >> 1) & notAFile)) { // stop BEFORE A file
            ray >>= 1;
            mask |= ray;
        }
        rookMasks[square] = mask;
        queenMasks[square] |= mask;
        mask = 0ULL;

        // --- BISHOP MASKS ---

        // UP RIGHT (Left Shift)
        ray = piece;
        while (((ray & not8thRank) && (ray & notHFile)) && (((ray << 9) & not8thRank) && ((ray << 9) & notHFile))) { // Stop BEFORE 8th rank & H file
            ray <<= 9;
            mask |= ray;
        }
        // UP LEFT (Left Shift)
        ray = piece;
        while (((ray & not8thRank) && (ray & notAFile)) && (((ray << 7) & notAFile) && ((ray << 7) & not8thRank))) { // Stop BEFORE 8th rank & A file
            ray <<= 7;
            mask |= ray;
        }
        // DOWN LEFT(Right Shift)
        ray = piece;
        while (((ray & notAFile) && (ray & not1stRank)) && (((ray >> 9) & notAFile) && ((ray >> 9) & not1stRank))) {
            ray >>= 9;
            mask |= ray;
        }
        // DOWN RIGHT (Right Shift)
        ray = piece;
        while (((ray & not1stRank) && (ray & notHFile)) && (((ray >> 7) & not1stRank) && ((ray >> 7) & notHFile))) {
            ray >>= 7;
            mask |= ray;
        }
        bishopMasks[square] = mask;
        // queen masks is a combo of rook and bishop masks
        queenMasks[square] |= mask;
    }
}

