//
// Created by saffa on 6/7/2026.
//
#include "include/moveGeneration.h"
#include "include/board.h"
#include "include/magics.h"
#include <vector>
#include <bit>
#include <iostream>
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
const U64 RANK_2 = 0x000000000000FF00ULL; // equivalently: 00000000 00000000 00000000 00000000 00000000 00000000 11111111 00000000
const U64 RANK_3 = 0x0000000000FF0000ULL; // Mask for the 3rd Rank, equivalently: 00000000 00000000 00000000 00000000 00000000 11111111 00000000 00000000
const U64 RANK_6 = 0x0000FF0000000000ULL; // Mask for the 6th Rank, equivalently:00000000 00000000 11111111 00000000 00000000 00000000 00000000 00000000
const U64 RANK_7 = 0x00FF000000000000ULL;// Mask for the 7th Rank, equivalently:00000000 11111111 00000000 00000000 00000000 00000000 00000000 00000000

const int castlingRightsUpdate[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11
};
/*this lookup table is meant to serve as an exhaustive list of every POSSIBLE ATTACKING MOVE
 that can be made with each individual piece. Not necessary every LEGAL move that can be made. */
void attackLookupTable() {
    for (int i = 0; i < 64; i++) {
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
        int rank = square / 8;
        int file = square % 8;
        U64 mask = 0ULL;

        // --- ROOK MASK (ignore Rank 0, Rank 7, File 0, File 7 bounds) ---
        // UP
        for (int r = rank + 1; r <= 6; r++) mask |= (1ULL << (r * 8 + file));
        // DOWN
        for (int r = rank - 1; r >= 1; r--) mask |= (1ULL << (r * 8 + file));
        // RIGHT
        for (int f = file + 1; f <= 6; f++) mask |= (1ULL << (rank * 8 + f));
        // LEFT
        for (int f = file - 1; f >= 1; f--) mask |= (1ULL << (rank * 8 + f));

        rookMasks[square] = mask;
        mask = 0ULL;

        // --- BISHOP MASK (ignore outer edges) ---
        // UP RIGHT
        for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; r++, f++) mask |= (1ULL << (r * 8 + f));
        // UP LEFT
        for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; r++, f--) mask |= (1ULL << (r * 8 + f));
        // DOWN RIGHT
        for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; r--, f++) mask |= (1ULL << (r * 8 + f));
        // DOWN LEFT
        for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; r--, f--) mask |= (1ULL << (r * 8 + f));

        bishopMasks[square] = mask;
        queenMasks[square] = rookMasks[square] | bishopMasks[square];
    }
}

/**
 * Translates our 'index' (a specific, randomized placement of pieces)
 * onto a full 64-bit board.
 *
 * We loop this index from 0 up to 2^(bitsInMask) during initialization
 * to hallucinate every single possible arrangement of blockers. Each call
 * returns ONE specific iteration of those pieces mapped to the global board.
 */
U64 setOccupancyHelper(int index, int bitsInMask, U64 mask) {
    U64 occupancy = 0ULL;

    // Loop through however many 1s are in our mask (e.g., 12)
    for (int i = 0; i < bitsInMask; i++) {

        // Finds the exact square of the lowest 1 bit in the mask
        int square = __builtin_ctzll(mask);

        // Turn that bit OFF in our temporary mask so __builtin_ctzll finds the next one next time
        mask &= mask - 1;
        // check if i-th bit of the index is turned on
        if (index & (1 << i)) {
            // turn on that exact square
            occupancy |= (1ULL << square);
        }
    }
    return occupancy;
}
// Simulates a Rook's true attack "ray/vision" against a specific arrangement of blockers.
U64 simulateRookAttacks(int square, U64 blockers) {
    U64 attacks = 0ULL;
    int rank = square / 8; // rank(0-7)
    int file = square % 8; // file(0-7)

    // --- ROOK ATTACK RAYS ---

    // UP (start at file + 1 and safely stop at top edge)
    for (int r = rank + 1; r <= 7; r++) {
        U64 raySquare = 1ULL << (r * 8 + file); // convert coordinates back to 1D bit
        attacks |= raySquare;
        if (raySquare & blockers) { // if we hit the piece --> stop(include blocker in ray)
            break;
        }
    }

    // DOWN (start at file - 1 and safely stop at bottom edge)
    for (int r = rank - 1; r >= 0; r--) {
        U64 raySquare = 1ULL << (r * 8 + file);
        attacks |= raySquare;
        if (raySquare & blockers) {
            break;
        }
    }

    // RIGHT (start at right + 1 and safely stop at right edge)
    for (int f = file + 1; f <= 7; f++) {
        U64 raySquare = 1ULL << (rank * 8 + f);
        attacks |= raySquare;
        if (raySquare & blockers) {
            break;
        }
    }

    // LEFT (start at left - 1 and safley stop at left edge)
    for (int f = file - 1; f >= 0; f--) {
        U64 raySquare = 1ULL << (rank * 8 + f);
        attacks |= raySquare;
        if (raySquare & blockers) {
            break;
        }
    }
    return attacks;
}

// Simulates a Bishops's true attack "ray/vision" against a specific arrangement of blockers.
U64 simulateBishopAttacks(int square, U64 blockers) {
    U64 attacks = 0ULL;
    int rank = square / 8; // rank(0-7)
    int file = square % 8; // file(0-7)

    // --- BISHOP ATTACK RAYS ---

    // UP RIGHT
    for (int r = rank + 1, f = file + 1; r <= 7 && f <= 7; r++, f++) { // because two vars are changing at once, we track BOTH file and rank in the for loop
        U64 raySquare = 1ULL << (r * 8 + f);
        attacks |= raySquare;
        if (raySquare & blockers) {
            break;
        }
    }

    // UP LEFT
    for (int r = rank + 1, f = file - 1; r <= 7 && f >= 0; r++,f--) {
        U64 raySquare = 1ULL << (r * 8 + f);
        attacks |= raySquare;
        if (raySquare & blockers) {
            break;
        }
    }
    // DOWN RIGHT
    for (int r = rank - 1, f = file + 1; r >= 0 && f <= 7; r--, f++) {
        U64 raySquare = 1ULL << (r * 8 + f);
        attacks |= raySquare;
        if (raySquare & blockers) {
            break;
        }
    }
    // DOWN LEFT
    for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--,f--) {
        U64 raySquare = 1ULL << (r * 8 + f);
        attacks |= raySquare;
        if (raySquare & blockers) {
            break;
        }
    }
    return attacks;
}
// massive global lookup array for rook, bishop, and queen attacks
U64 rookAttackTable[64][4096]; // [64 squares on a board][absolute maximum number of permutations any Rook mask can have(2^12 squares)]
U64 bishopAttackTable[64][512]; // [64 squares on board][absolute maximum number of permutations any Bishop mask can have(2^9 squares)]
// U64 queenAttackTable[64][524288];// [64 squares on board][absolute maximum number of permutations any Bishop mask can have(2^19 squares)]

/** @brief Populates the bitboard attack lookup table for Rooks
 * *This builder function pre-calculates every possible blocked attack ray for a Rook
 * on all 64 squares. It uses the "Carry-Rippling" occupancy generator to create
 * every physical arrangement of blocker pieces for a given square's mask, and then
 * shoots a simulated laser to find the true attack ray for that permutation.
 * * Performance Note:
 * The inner loop dynamically scales to 2^(numOfBitsInMask).
 * Across all 64 squares, the simulation runs exactly 102,400 times.
 */
void generateBlockedRookAttacks() {
    for (int square = 0; square < 64; square++) {
        // count how many squares are in this specific mask
        int numOfBitsInMask = std::popcount(rookMasks[square]);

        // find total number of permutations(2^num of bits)
        int totalPermutations = 1 <<  numOfBitsInMask;
        for (int index = 0; index < totalPermutations; index++) {
            // hallucinate the blocking pieces for this specific index
            U64 blockers = setOccupancyHelper(index, numOfBitsInMask, rookMasks[square]);

            // shoot "lasers" at those blocking pieces to get the real attack array
            U64 realRay = simulateRookAttacks(square, blockers);

            // USE MAGIC NUMBER to find where this specific board state belongs
            int magicIndex = (blockers * rookMagicNumbers[square]) >> (64 - numOfBitsInMask);
            rookAttackTable[square][magicIndex] = realRay;
        }
    }
}

void generateBlockedBishopAttacks() {
    for (int square = 0; square < 64; square++) {
        // count how many squares are in this specific mask
        int numOfBitsInMask = std::popcount(bishopMasks[square]);

        // find total number of permutations(2^num of bits)
        int totalPermutations = 1 <<  numOfBitsInMask;
        for (int index = 0; index < totalPermutations; index++) {
            // hallucinate the blocking pieces for this specific index
            U64 blockers = setOccupancyHelper(index, numOfBitsInMask, bishopMasks[square]);

            // shoot "lasers" at those blocking pieces to get the real attack array
            U64 realRay = simulateBishopAttacks(square, blockers);
            // USE MAGIC NUMBER to find where this specific board state belongs
            int magicIndex = (blockers * bishopMagicNumbers[square]) >> (64 - numOfBitsInMask);

            bishopAttackTable[square][magicIndex] = realRay;
        }
    }
}
/* Getter functions for sliding pieces */

U64 getBishopAttacks(int square, U64 liveBoard) {
    // mask the live board so we only look at the squares the bishop has vision to
    U64 blockers = bishopMasks[square] & liveBoard;

    // count num of bits in maks
    int bitsInMask = std::popcount(bishopMasks[square]);

    // Magic Formula to convert 64-bit integer blockers to a tiny number between 0-511
    int magicIndex = (blockers * bishopMagicNumbers[square]) >> (64 - bitsInMask);
    return bishopAttackTable[square][magicIndex];
}

U64 getRookAttacks(int square, U64 liveBoard) {

    U64 blockers = rookMasks[square] & liveBoard;
    int bitsInMask = std::popcount(rookMasks[square]);

    // Magic Formula to convert 64-bit integer blockers to a tiny number between 0-4095
    int magicIndex = (blockers * rookMagicNumbers[square]) >> (64 - bitsInMask);
    return rookAttackTable[square][magicIndex];

}

U64 getQueenAttacks(int square, U64 liveBoard) {
    /** Although the maximum number of bits in a mask for a queen is 2^19 = 524,288
     * Which is much greater than total of max num of bits in a bishop and rooks mask = 2^12 + 2^9 = 4,608
     * We can still bitwise OR them together because straight lines and diagonal lines are independent
     */
    return getRookAttacks(square, liveBoard) | getBishopAttacks(square, liveBoard);
}

/* Getter functions for remaining pieces(pawn, knight, and king) */

U64 getPawnAttacks(int square, COLOR color) {
    return pawnAttacks[color][square];
}

U64 getKnightAttacks(int square) {
    return knightAttacks[square];
}

U64 getKingAttacks(int square) {
    return kingAttacks[square];
}

/** MASTER MOVE GENERATOR:
 *  Reads the current board state and dumps every pseudo-legal move into our
 *  pre-allocated array. Relies on magic bitboards and lookup tables to
 *  avoid branching and keep the CPU pipeline moving fast.
 */
void generateMoves(MoveList& list, const Board &board) {
    // --- STATE MASKS --
    int turn = board.getSideToMove();
    U64 friendlyPieces = board.getOccupancies(turn); // aka engines pieces(only called when its the engines turn)
    U64 enemyPieces = board.getOccupancies(turn ^ 1); // XOR trick
    U64 allPieces = board.getOccupancies(COLOR::BOTH);
    U64 emptySquares = ~allPieces; // '~' flips bits

    if (turn == COLOR::WHITE) {
        // --- WHITE PAWNS(ATTACKS ONLY) ---
        U64 all_white_pawns = board.getPieceBitBoard(PIECE_TYPE::Pawn);
        U64 white_promotionPawns = all_white_pawns & RANK_7;
        U64 white_pawns = all_white_pawns & ~RANK_7; // these are safe to use for normal pushes/attacks

        // Shove all pawns up 1 square, keep only the ones that landed on an empty square
        U64 singlePushes = (white_pawns << 8) & emptySquares;

        // Take the valid single pushes, keep only the ones on Rank 3, shift them up again, check if empty
        U64 doublePushes = ((singlePushes & RANK_3) << 8) & emptySquares;
        U64 temp_white_pawns = white_pawns; // Create a disposable copy
        while (temp_white_pawns != 0ULL) {
            // Finds the exact square of the lowest 1 bit in the white_pawns mask
            int startSquare = __builtin_ctzll(temp_white_pawns);
            U64 attacks = pawnAttacks[turn][startSquare] & enemyPieces;
            while (attacks != 0ULL) {
                // Finds the exact square of the lowest 1 bit in the white_pawns attack mask
                int targetSquare = __builtin_ctzll(attacks);
                list.addMove(encodeMove(startSquare, targetSquare, CAPTURE));
                attacks &= (attacks - 1); // removes the lowest set bit(rightmost 1)
            }
            temp_white_pawns &= (temp_white_pawns - 1); // destroy the copy
        }

        // --- WHITE PAWNS(SPECIALTY CASE, forward pawn moves(non-attacking moves)) ---
        while (singlePushes != 0ULL) {
            int targetSquare = __builtin_ctzll(singlePushes);
            // if we know where the pawn landed, the start square is just 8 squares behind it
            int startSquare = targetSquare - 8;
            list.addMove(encodeMove(startSquare, targetSquare, QUIET_MOVE));
            singlePushes &= (singlePushes - 1);
        }
        while (doublePushes != 0ULL) {
            int targetSquare = __builtin_ctzll(doublePushes);
            // The pawn moved two squares (16 bits) to get here
            int startSquare = targetSquare - 16;
            // Pass a '1' flag to represent double pawn push
            list.addMove(encodeMove(startSquare,targetSquare, DOUBLE_PAWN_PUSH));
            doublePushes &= (doublePushes - 1);
        }

        // --- WHITE PAWN PROMOTIONS ---
        while (white_promotionPawns != 0ULL) {
            int startSquare = __builtin_ctzll(white_promotionPawns);

            // --- STRAIGHT PUSH ---
            int pushSquare = startSquare + 8;
            // if the square in front of the pawn is empty, we can promote
            if ((1ULL << pushSquare) & emptySquares) {
                // In chess, there are 4 types of pawn promotions:
                list.addMove(encodeMove(startSquare, pushSquare, PR_KNIGHT)); // knight push promotion
                list.addMove(encodeMove(startSquare, pushSquare, PR_BISHOP)); // bishop push promotion
                list.addMove(encodeMove(startSquare, pushSquare, PR_ROOK)); // rook push promotion
                list.addMove(encodeMove(startSquare, pushSquare, PR_QUEEN)); // queen push promotion
            }
            // --- DIAGONAL CAPTURES ---
            U64 captureAttacks = pawnAttacks[turn][startSquare] & enemyPieces;
            while (captureAttacks != 0ULL) {
                int targetSquare = __builtin_ctzll(captureAttacks);
                list.addMove(encodeMove(startSquare, targetSquare, PC_KNIGHT)); // knight capture promotion
                list.addMove(encodeMove(startSquare, targetSquare, PC_BISHOP)); // bishop capture promotion
                list.addMove(encodeMove(startSquare, targetSquare, PC_ROOK)); // rook capture promotion
                list.addMove(encodeMove(startSquare, targetSquare, PC_QUEEN)); // queen capture promotion

                captureAttacks &= (captureAttacks - 1);
            }
            white_promotionPawns &= (white_promotionPawns - 1);
        }
        // --- WHITE PAWNS ENPASSANT ---
        int epSquare = board.getEnpassantSquare();
        // Check if an En Passant square actually exists on this turn
        if (epSquare != -1) {
            // TRICK: Pretend a BLACK pawn is standing on the enPassant square.
            U64 epAttackers = pawnAttacks[COLOR::BLACK][epSquare] & white_pawns;
            while (epAttackers != 0ULL) {
                int startSquare = __builtin_ctzll(epAttackers);

                // target is the ghost square
                list.addMove(encodeMove(startSquare, epSquare, EN_PASSANT)); // pass the enPassant flag
                epAttackers &= (epAttackers - 1);
            }
        }
        // --- WHITE KING ---
        U64 white_king = board.getPieceBitBoard(PIECE_TYPE::King);
        while (white_king != 0ULL) {
            // Finds the exact square of the lowest 1 bit in the white_pawns mask
            int startSquare = __builtin_ctzll(white_king);
            // Get the exact attacks(excluding attacks to friendly pieces)
            U64 attacks = kingAttacks[startSquare] & ~friendlyPieces;
            while (attacks != 0ULL) {
                // Finds the exact square of the lowest 1 bit in the white_pawns attack mask
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1); // removes the lowest set bit(rightmost 1)
            }
            white_king &= (white_king - 1);
        }
        // --- WHITE KNIGHT ---
        U64 white_knights = board.getPieceBitBoard(PIECE_TYPE::Knight);
        while (white_knights != 0ULL) {
            // Finds the exact square of the lowest 1 bit in the white_pawns mask
            int startSquare = __builtin_ctzll(white_knights);
            // Get the exact attacks(excluding attacks to friendly pieces)
            U64 attacks = knightAttacks[startSquare] & ~friendlyPieces;
            while (attacks != 0ULL) {
                // Finds the exact square of the lowest 1 bit in the white_pawns attack mask
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1); // removes the lowest set bit(rightmost 1)
            }
            white_knights &= (white_knights - 1);
        }
        /* --- SLIDING PIECES --- */

        // --- WHITE BISHOP ---
        U64 white_bishop = board.getPieceBitBoard(PIECE_TYPE::Bishop);
        while (white_bishop != 0ULL) {
            int startSquare = __builtin_ctzll(white_bishop);
            U64 attacks = getBishopAttacks(startSquare, allPieces) & ~friendlyPieces;
            while (attacks != 0ULL) {
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare,targetSquare,moveFlag));
                attacks &= (attacks - 1);
            }
            white_bishop &= (white_bishop - 1);
        }
        // --- WHITE ROOK ---
        U64 white_rook = board.getPieceBitBoard(PIECE_TYPE::Rook);
        while (white_rook != 0ULL) {
            int startSquare = __builtin_ctzll(white_rook);
            U64 attacks = getRookAttacks(startSquare, allPieces) & ~friendlyPieces;
            while (attacks != 0ULL) {
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1);
            }
            white_rook &= (white_rook - 1);
        }
        // --- WHITE QUEEN ---
        U64 white_queen = board.getPieceBitBoard(PIECE_TYPE::Queen);
        while (white_queen != 0ULL) {
            int startSquare = __builtin_ctzll(white_queen);
            U64 attacks = getQueenAttacks(startSquare, allPieces) & ~friendlyPieces;
            while (attacks != 0ULL) {
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1);
            }
            white_queen &= (white_queen - 1);
        }
        /* --- WHITE CASTLING --- */

        // --- King Side Castle(O-O) ---
        if (board.getCastlingRights() & 1) {
            // The squares F1 and G1 MUST be empty
            if ((allPieces & 0x0000000000000060ULL) == 0ULL) { // eq = 00000000 00000000 00000000 00000000 00000000 00000000 00000000 01100000
                // The King cannot be in check (E1), and cannot pass through check (F1)
                if (!board.isSquareAttacked(4, turn ^ 1) && !board.isSquareAttacked(5, turn ^ 1)) {
                    // encode king move from E1(index 4) to G1 (index 6) with KING SIDE CASTLE FLAG(flag '2' handles rook move later)
                    list.addMove(encodeMove(4,6,KING_CASTLE));
                }
            }
        }
        // ---Queen Side Castle(O-O-O)---
        if (board.getCastlingRights() & 2) {
            // The squares on B1, C1, and D1 must be empty
            if ((allPieces & 0x000000000000000EULL) == 0ULL) { // eq = 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00001110
                // The king cannot be in check (E1), and cannot pass through check (D1)
                if (!board.isSquareAttacked(4, turn ^ 1) && !board.isSquareAttacked(3, turn ^ 1)) {
                    // encode king move from E1(index 4) to C1 (index 2) with QUEEN SIDE CASTLE FLAG
                    list.addMove(encodeMove(4, 2, QUEEN_CASTLE)); // flag '3' handles rook later
                }
            }
        }
    } else if (turn == COLOR::BLACK) {
        // --- BLACK PAWNS(ATTACKS ONLY) ---
        U64 all_black_pawns = board.getPieceBitBoard(PIECE_TYPE::Pawn + 6);
        U64 black_promotionPawns = all_black_pawns & RANK_2;
        U64 black_pawns = all_black_pawns & ~RANK_2;
        // Shove all pawns down 1 square, keep only the ones that landed on an empty square
        U64 singlePushes = (black_pawns >> 8) & emptySquares;

        // Take the valid single pushes, keep only the ones on Rank 6, shift them down again, check if empty
        U64 doublePushes = ((singlePushes & RANK_6) >> 8) & emptySquares;
        U64 temp_black_pawns = black_pawns; // Create a disposable copy
        while (temp_black_pawns != 0ULL) {
            // Finds the exact square of the lowest 1 bit in the white_pawns mask
            int startSquare = __builtin_ctzll(temp_black_pawns);
            U64 attacks = pawnAttacks[turn][startSquare] & enemyPieces;
            while (attacks != 0ULL) {
                // Finds the exact square of the lowest 1 bit in the white_pawns attack mask
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1); // removes the lowest set bit(rightmost 1)
            }
            temp_black_pawns &= (temp_black_pawns - 1);
        }

        // --- BLACK PAWNS(SPECIALTY CASE, forward pawn moves(non-attacking moves)) ---
        while (singlePushes != 0ULL) {
            int targetSquare = __builtin_ctzll(singlePushes);
            // if we know where the pawn landed, the start square is just 8 squares behind it
            int startSquare = targetSquare + 8;
            list.addMove(encodeMove(startSquare, targetSquare, QUIET_MOVE));
            singlePushes &= (singlePushes - 1);
        }
        while (doublePushes != 0ULL) {
            int targetSquare = __builtin_ctzll(doublePushes);
            // The pawn moved two squares (16 bits) to get here
            int startSquare = targetSquare + 16;
            // Pass a '1' flag to represent double pawn push
            list.addMove(encodeMove(startSquare,targetSquare, DOUBLE_PAWN_PUSH));
            doublePushes &= (doublePushes - 1);
        }
        // --- BLACK PAWN PROMOTION ---
        while (black_promotionPawns != 0ULL) {
            int startSquare = __builtin_ctzll(black_promotionPawns);

            // --- STRAIGHT PUSH ---
            int pushSquare = startSquare - 8;
            // if the square in front of the pawn is empty, we can promote
            if ((1ULL << pushSquare) & emptySquares) {
                // In chess, there are 4 types of pawn promotions:
                list.addMove(encodeMove(startSquare, pushSquare, PR_KNIGHT)); // knight
                list.addMove(encodeMove(startSquare, pushSquare, PR_BISHOP)); // bishop
                list.addMove(encodeMove(startSquare, pushSquare, PR_ROOK)); // rook
                list.addMove(encodeMove(startSquare, pushSquare, PR_QUEEN)); // queen
            }
            // --- DIAGONAL CAPTURES ---
            U64 captureAttacks = pawnAttacks[turn][startSquare] & enemyPieces;
            while (captureAttacks != 0ULL) {
                int targetSquare = __builtin_ctzll(captureAttacks);
                list.addMove(encodeMove(startSquare, targetSquare, PC_KNIGHT)); // knight
                list.addMove(encodeMove(startSquare, targetSquare, PC_BISHOP)); // bishop
                list.addMove(encodeMove(startSquare, targetSquare, PC_ROOK)); // rook
                list.addMove(encodeMove(startSquare, targetSquare, PC_QUEEN)); // queen

                captureAttacks &= (captureAttacks - 1);
            }
            black_promotionPawns &= (black_promotionPawns - 1);
        }
        // --- BLACK PAWNS ENPASSANT ---
        int epSquare = board.getEnpassantSquare();
        // Check if an En Passant square actually exists on this turn
        if (epSquare != -1) {
            // TRICK: Pretend a BLACK pawn is standing on the enPassant square.
            U64 epAttackers = pawnAttacks[COLOR::WHITE][epSquare] & black_pawns;
            while (epAttackers != 0ULL) {
                int startSquare = __builtin_ctzll(epAttackers);

                // target is the ghost square
                list.addMove(encodeMove(startSquare, epSquare, EN_PASSANT)); // pass the enPassant flag
                epAttackers &= (epAttackers - 1);
            }
        }
        // --- BLACK KING ---
        U64 black_king = board.getPieceBitBoard(PIECE_TYPE::King + 6);
        while (black_king != 0ULL) {
            // Finds the exact square of the lowest 1 bit in the white_pawns mask
            int startSquare = __builtin_ctzll(black_king);
            // Get the exact attacks(excluding attacks to friendly pieces)
            U64 attacks = kingAttacks[startSquare] & ~friendlyPieces;
            while (attacks != 0ULL) {
                // Finds the exact square of the lowest 1 bit in the white_pawns attack mask
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1); // removes the lowest set bit(rightmost 1)
            }
            black_king &= (black_king - 1);
        }
        // --- BLACK KNIGHT ---
        U64 black_knights = board.getPieceBitBoard(PIECE_TYPE::Knight + 6);
        while (black_knights != 0ULL) {
            // Finds the exact square of the lowest 1 bit in the white_pawns mask
            int startSquare = __builtin_ctzll(black_knights);
            // Get the exact attacks(excluding attacks to friendly pieces)
            U64 attacks = knightAttacks[startSquare] & ~friendlyPieces;
            while (attacks != 0ULL) {
                // Finds the exact square of the lowest 1 bit in the white_pawns attack mask
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1); // removes the lowest set bit(rightmost 1)
            }
            black_knights &= (black_knights - 1);
        }
        /* --- SLIDING PIECES --- */

        // --- BLACK BISHOP ---
        U64 black_bishop = board.getPieceBitBoard(PIECE_TYPE::Bishop + 6);
        while (black_bishop != 0ULL) {
            int startSquare = __builtin_ctzll(black_bishop);
            U64 attacks = getBishopAttacks(startSquare, allPieces) & ~friendlyPieces;
            while (attacks != 0ULL) {
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare,targetSquare,moveFlag));
                attacks &= (attacks - 1);
            }
            black_bishop &= (black_bishop - 1);
        }
        // --- BLACK ROOK ---
        U64 black_rook = board.getPieceBitBoard(PIECE_TYPE::Rook + 6);
        while (black_rook != 0ULL) {
            int startSquare = __builtin_ctzll(black_rook);
            U64 attacks = getRookAttacks(startSquare, allPieces) & ~friendlyPieces;
            while (attacks != 0ULL) {
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1);
            }
            black_rook &= (black_rook - 1);
        }
        // --- BLACK QUEEN ---
        U64 black_queen = board.getPieceBitBoard(PIECE_TYPE::Queen + 6);
        while (black_queen != 0ULL) {
            int startSquare = __builtin_ctzll(black_queen);
            U64 attacks = getQueenAttacks(startSquare, allPieces) & ~friendlyPieces;
            while (attacks != 0ULL) {
                int targetSquare = __builtin_ctzll(attacks);

                // Check if the target square overlaps with the enemy pieces bitboard
                int moveFlag = ((1ULL << targetSquare) & enemyPieces) ? CAPTURE : QUIET_MOVE;

                list.addMove(encodeMove(startSquare, targetSquare, moveFlag));
                attacks &= (attacks - 1);
            }
            black_queen &= (black_queen - 1);
        }

        /* --- BLACK CASTLING --- */

        // --- King Side Castle(O-O) ---
        if (board.getCastlingRights() & 4) {
            // The squares F8 and G8 MUST be empty
            if ((allPieces & 0x6000000000000000ULL) == 0ULL) { // eq = 01100000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
                // King cannot be in check (E8), and cannot pass through check (F8)
                if (!board.isSquareAttacked(60, turn ^ 1) && !board.isSquareAttacked(61, turn ^ 1)) {
                    // encode king move from E8(index 60) to G8 (index 62) with KING SIDE CASTLE FLAG(flag '2' handles rook move later)
                    list.addMove(encodeMove(60, 62,KING_CASTLE));
                }
            }
        }
        // ---Queen Side Castle(O-O-O)---
        if (board.getCastlingRights() & 8) {
            // The squares on B8, C8, and D8 must be empty
            if ((allPieces & 0x0E00000000000000ULL) == 0ULL) { // eq = 00001110 00000000 00000000 00000000 00000000 00000000 00000000 00000000
                if (!board.isSquareAttacked(60, turn ^ 1) && !board.isSquareAttacked(59, turn ^ 1)) {
                    // encode king move from E8(index 60) to C8(index 58) with QUEEN SIDE CASTLE FLAG
                    list.addMove(encodeMove(60, 58, QUEEN_CASTLE)); // flag '3' handles rook later
                }
            }
        }
    }
}
/** Executes a pseudo-legal move on the board and verifies its legal.
 * This function decodes the 16-bit move and phyiscally updates the bitboards for both
 * normal movements and special rules (captures, en passant, castling, promotion).
 * Then it updates internal game state (occupancies, turn, castling rights)
 * Preforms King safety check to ensure move is legal.
 *
 */
bool makeMove(uint16_t move, Board& board) {
    int sideToMove = board.getSideToMove();
    unsigned int flag = getFlag(move);
    unsigned int startSquare = getStart(move);
    unsigned int targetSquare = getTarget(move);

    // identify piece BEFORE moving
    int pieceType = board.getPieceAt(startSquare);
    if (pieceType == -1) return false; // no piece found

    // Handle standard captures
    int capturedPiece = -1;
    if (flag == MoveFlag::CAPTURE || (flag >= MoveFlag::PC_KNIGHT && flag <= MoveFlag::PC_QUEEN)) {
        capturedPiece = board.getPieceAt(targetSquare);
    }
    // Save the current move state
    board.history[board.historyPly].castlingRights = board.castlingRights;
    board.history[board.historyPly].enPassantSquare = board.enPassantSquare;
    board.history[board.historyPly].halfMoveClock = board.halfMoveClock;
    board.history[board.historyPly].capturedPieceType = capturedPiece;
    board.historyPly++;

    // safety check
    if (capturedPiece != -1) {
        // turn off the bit for the captured piece on target square
        board.pieceBitBoards[capturedPiece] &= ~(1ULL << targetSquare);
    }
    // Pick up and put down piece
    board.pieceBitBoards[pieceType] &= ~(1ULL << startSquare); // Turn off start
    board.pieceBitBoards[pieceType] |= (1ULL << targetSquare); // Turn on target

    // Defaults to none every turn, unless flag is Double Pawn Push
    board.enPassantSquare = -1;
    // Update Castling Rights based on what square was left and what square was landed on
    board.castlingRights &= castlingRightsUpdate[startSquare];
    board.castlingRights &= castlingRightsUpdate[targetSquare];
    // Handle Special Flags
    switch (flag) {
        case DOUBLE_PAWN_PUSH:
            // set new en passant square state since we double pushed
            if (sideToMove == COLOR::WHITE) {
                // Ghost square is 1 square behind
                board.enPassantSquare = targetSquare - 8;
            } else {
                board.enPassantSquare = targetSquare + 8;
            }
            break;
        case KING_CASTLE:
            // move kingside rook
            if (sideToMove == COLOR::WHITE) {
                board.pieceBitBoards[PIECE_TYPE::Rook] &= ~(1ULL << 7); // Pick up H1
                board.pieceBitBoards[PIECE_TYPE::Rook] |= (1ULL << 5); // Put down F1
            } else {
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] &= ~(1ULL << 63); // Pick up H8
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] |= (1ULL << 61); // Put down F8
            }
            break;
        case QUEEN_CASTLE:
            if (sideToMove == COLOR::WHITE) {
                board.pieceBitBoards[PIECE_TYPE::Rook] &= ~(1ULL << 0); // Pick up A1
                board.pieceBitBoards[PIECE_TYPE::Rook] |= (1ULL << 3); // Put down D1
            } else {
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] &= ~(1ULL << 56); // Pick up A8
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] |= (1ULL << 59); // Put down D8
            }
            break;
        case EN_PASSANT:
            if (sideToMove == COLOR::WHITE) {
                int capturedPawnSquare = targetSquare - 8;
                // white pawn captures black pawn
                board.pieceBitBoards[PIECE_TYPE::Pawn + 6] &= ~(1ULL << capturedPawnSquare);
            } else {
                int capturedPawnSquare = targetSquare + 8;
                // black pawn captures white pawn
                board.pieceBitBoards[PIECE_TYPE::Pawn] &= ~(1ULL << capturedPawnSquare);
            }
            break;
        // We treat promotional captures and standard promotions the same here, so we stack them
        case PR_KNIGHT:
        case PC_KNIGHT:
            if (sideToMove == COLOR::WHITE) {
                board.pieceBitBoards[PIECE_TYPE::Pawn] &= ~(1ULL << targetSquare); // Delete the Pawn
                board.pieceBitBoards[PIECE_TYPE::Knight] |= (1ULL << targetSquare); // Spawn replacement Knight
            } else {
                board.pieceBitBoards[PIECE_TYPE::Pawn + 6] &= ~(1ULL << targetSquare);
                board.pieceBitBoards[PIECE_TYPE::Knight + 6] |= (1ULL << targetSquare);
            }
            break;
        case PR_BISHOP:
        case PC_BISHOP:
            if (sideToMove == COLOR::WHITE) {
                board.pieceBitBoards[PIECE_TYPE::Pawn] &= ~(1ULL << targetSquare); // Delete the Pawn
                board.pieceBitBoards[PIECE_TYPE::Bishop] |= (1ULL << targetSquare); // Spawn replacement Bishop
            } else {
                board.pieceBitBoards[PIECE_TYPE::Pawn + 6] &= ~(1ULL << targetSquare);
                board.pieceBitBoards[PIECE_TYPE::Bishop + 6] |= (1ULL << targetSquare);
            }
            break;
        case PR_ROOK:
        case PC_ROOK:
            if (sideToMove == COLOR::WHITE) {
                board.pieceBitBoards[PIECE_TYPE::Pawn] &= ~(1ULL << targetSquare); // Delete the Pawn
                board.pieceBitBoards[PIECE_TYPE::Rook] |= (1ULL << targetSquare); // Spawn replacement Rook
            } else {
                board.pieceBitBoards[PIECE_TYPE::Pawn + 6] &= ~(1ULL << targetSquare);
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] |= (1ULL << targetSquare);
            }
            break;
        case PR_QUEEN:
        case PC_QUEEN:
            if (sideToMove == COLOR::WHITE) {
                board.pieceBitBoards[PIECE_TYPE::Pawn] &= ~(1ULL << targetSquare); // Delete the Pawn
                board.pieceBitBoards[PIECE_TYPE::Queen] |= (1ULL << targetSquare); // Spawn replacement Queen
            } else {
                board.pieceBitBoards[PIECE_TYPE::Pawn + 6] &= ~(1ULL << targetSquare);
                board.pieceBitBoards[PIECE_TYPE::Queen + 6] |= (1ULL << targetSquare);
            }
            break;
    }
    // UPDATE BOARD STATES
    board.occupancies[COLOR::WHITE] = 0ULL;
    board.occupancies[COLOR::BLACK] = 0ULL;
    for (int i = 0; i < 6; i++) board.occupancies[COLOR::WHITE] |= board.pieceBitBoards[i];
    for (int i = 6; i < 12; i++) board.occupancies[COLOR::BLACK] |= board.pieceBitBoards[i];
    board.occupancies[COLOR::BOTH] = board.occupancies[COLOR::WHITE] | board.occupancies[COLOR::BLACK];

    board.sideToMove ^= 1;

    // KING SAFETY CHECKS!
    int ourColor = board.sideToMove ^ 1;

    // Find kings exact square index
    int kingPiece = (ourColor == COLOR::WHITE) ? PIECE_TYPE::King : PIECE_TYPE::King + 6;
    int kingSquare = __builtin_ctzll(board.pieceBitBoards[kingPiece]);

    // check if enemy is attacking our kings square
    if (board.isSquareAttacked(kingSquare, board.sideToMove)) {
        // revert the board state
        unmakeMove(move, board);
        return false; // illegal move, king in check
    }
    return true; // VALID MOVE
}
/** Reverses a previously made move and restores boards state */
void unmakeMove(uint16_t move, Board& board) {
    unsigned int startSquare = getStart(move);
    unsigned int targetSquare = getTarget(move);
    unsigned int flag = getFlag(move);
    // flip the turn back to the player that made the move
    int color = (board.sideToMove == COLOR::WHITE) ? COLOR::BLACK : COLOR::WHITE;
    int colorOffset = (color == COLOR::BLACK) ? 6 : 0;

    bool isPromotion = (flag >= PR_KNIGHT && flag <= PC_QUEEN);
    // If there was a promotion, the piece that moved was a pawn
    int pieceType = isPromotion ? PIECE_TYPE::Pawn : board.getPieceAt(targetSquare);
    int bitBoardIndex = pieceType + colorOffset;

    // Move history pointer back by 1 to view the game state before this move
    board.historyPly--;
    GameState state = board.history[board.historyPly];

    // Restore State
    board.castlingRights = state.castlingRights;
    board.enPassantSquare = state.enPassantSquare;
    board.halfMoveClock = state.halfMoveClock;
    board.sideToMove = color;
    board.fullMoveNumber -= (color == COLOR::BLACK) ? 1 : 0;

    // Move the piece back
    if (isPromotion) {
        // Remove the promoted piece
        int promotedPiece = board.getPieceAt(targetSquare);
        board.pieceBitBoards[promotedPiece + colorOffset] ^= (1ULL << targetSquare);
    } else {
        // Normal move
        board.pieceBitBoards[bitBoardIndex] ^= (1ULL << targetSquare); // Use XOR to remove piece from targetSquare
    }
    // Put the moving piece back on the start square
    board.pieceBitBoards[bitBoardIndex] ^= (1ULL << startSquare);

    // Update occupancies
    board.occupancies[color] ^= (1ULL << targetSquare) | (1ULL << startSquare);

    // reinstate captured piece if there was one
    if (state.capturedPieceType != -1) {
        board.pieceBitBoards[state.capturedPieceType] ^= (1ULL << targetSquare);
        // update enemy occupancies
        board.occupancies[color ^ 1] ^= (1ULL << targetSquare);
    }

    switch (flag) {
        // Handle undoing Castle Moves
        case KING_CASTLE:
            // move kingside rook
            if (color == COLOR::WHITE) {
                board.pieceBitBoards[PIECE_TYPE::Rook] ^= (1ULL << 7); // Add rook to H1
                board.pieceBitBoards[PIECE_TYPE::Rook] ^= (1ULL << 5); // Delete rook on F1
                board.occupancies[COLOR::WHITE] ^= (1ULL << 7) | (1ULL << 5);
            } else {
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] ^= (1ULL << 63); // Add rook to H8
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] ^= (1ULL << 61); // Delete rook on F8
                board.occupancies[COLOR::BLACK] ^= (1ULL << 63) | (1ULL << 61);
            }
            break;
        case QUEEN_CASTLE:
            if (color == COLOR::WHITE) {
                board.pieceBitBoards[PIECE_TYPE::Rook] ^= (1ULL << 0); // Add rook to A1
                board.pieceBitBoards[PIECE_TYPE::Rook] ^= (1ULL << 3); // Delete rook on D1
                board.occupancies[COLOR::WHITE] ^= (1ULL << 0) | (1ULL << 3);
            } else {
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] ^= (1ULL << 56); // Add rook to A8
                board.pieceBitBoards[PIECE_TYPE::Rook + 6] ^= (1ULL << 59); // Delete rook on D8
                board.occupancies[COLOR::BLACK] ^= (1ULL << 56)| (1ULL << 59);
            }
            break;
        // Handle undoing of En Passant
        case EN_PASSANT:
            if (color == COLOR::WHITE) {
                int capturedPawnSquare = targetSquare - 8;
                board.pieceBitBoards[PIECE_TYPE::Pawn + 6] ^= (1ULL << capturedPawnSquare);
                board.occupancies[COLOR::BLACK] ^= (1ULL << capturedPawnSquare);
            } else {
                int capturedPawnSquare = targetSquare + 8;
                board.pieceBitBoards[PIECE_TYPE::Pawn] ^= (1ULL << capturedPawnSquare);
                board.occupancies[COLOR::WHITE] ^= (1ULL << capturedPawnSquare);
            }
            break;
    }
    // Final occupancy update
    board.occupancies[COLOR::BOTH] = board.occupancies[COLOR::WHITE] | board.occupancies[COLOR::BLACK];
}
// call all builder functions in correct order
void initAllMoveGen() {
    attackLookupTable(); // Base attacks (Knights, Kings, Pawns, etc)
    relevantBlockerMask(); // Sliding piece masks(Bishops, Rooks, and Queens)
    generateBlockedRookAttacks(); // magic rook table
    generateBlockedBishopAttacks(); // magic bishop table
}