//
// Created by saffa on 6/21/2026.
//
#include "include/board.h"
#include "include/evaluation.h"
#include <algorithm>
// Constant penalty/bonus for Pawn Structure
const int DOUBLED_PAWN_MG = -10;
const int DOUBLED_PAWN_EG = -20;
const int ISOLATED_PAWN_MG = -15;
const int ISOLATED_PAWN_EG = -30;

// Scaled bonuses based on how far the pawn has advanced
const int PASSED_PAWN_MG[8] = {0, 10, 10, 15, 25, 50, 90, 0};
const int PASSED_PAWN_EG[8] = {0, 10, 20, 40, 70, 120, 200, 0};

// pre-calculated pawn masks
U64 fileMasks[8];          // [file]
U64 isolatedMasks[8];      // [file]
U64 whitePassedMasks[64];  // [square]
U64 blackPassedMasks[64];  // [square]
int evaluate(const Board& board) {
    int midGameScore{};
    int endGameScore{};
    int totalPhaseWeight{};
    int finalEvalScore{};

    // Pawn structure evaluation
    evaluatePawns(board, midGameScore, endGameScore);
    // King evaluation
    evaluateKings(board, midGameScore, endGameScore);

    // loop through all 12 piece types
    for (int pieceType = 0; pieceType < 12; pieceType++) {
        U64 pieceBitBoard = board.getPieceBitBoard(pieceType);
        while (pieceBitBoard != 0ULL) {
            // Finds the exact square of the lowest 1 bit on the board
            int square = __builtin_ctzll(pieceBitBoard);

            // This ensures peice index is always between 0-5 for black and white
            int normalizedPiece = pieceType % 6;

            // Update Phase Weight
            totalPhaseWeight += phaseWeights[normalizedPiece]; // Use pieceType(0-11 index) since our phaseWeights holds 12 elements

            // Lookup Standard Piece Values
            int material = pieceValues[normalizedPiece];

            // Flip the square index if we are looking at White piece(White pieces index: 0-5)
            int pstSquare = (pieceType < 6) ? (square ^ 56) : square; // (^ 56) finds the mirror square

            // Calculate Bonus Score
            int midGameBonus = middle_game_PST[normalizedPiece][pstSquare];
            int endGameBonus = end_game_PST[normalizedPiece][pstSquare];

            if (pieceType < 6) { // IF WHITE PIECE ADD
                // Standard Piece Values/Weights + Bonus points
                midGameScore += material + midGameBonus;
                endGameScore += material + endGameBonus;
            } else { // IF BLACK PIECE SUBTRACT(Negative eval indicates black advantage)
                midGameScore -= material + midGameBonus;
                endGameScore -= material + endGameBonus;
            }

            // remove lowest 1 bit
            pieceBitBoard &= (pieceBitBoard - 1);
        }
    }
    // !IMPORTANT: Cap the phase weight at 24 to prevent negative endgame multipliers from pawn promotions
    if (totalPhaseWeight > 24) totalPhaseWeight = 24;

    // Calculate Final Evaluation Score
    finalEvalScore = ((midGameScore * totalPhaseWeight) + (endGameScore * (24 - totalPhaseWeight))) / 24;

    /* Flip Perspective:
     * Although evaluation is the same regardless of perspective.
     * For our search algorithm, it does not care about what color its playing,
     * it wants to find the move that gives the largest positive number.
     */
    int turn = board.getSideToMove();
    return (turn == COLOR::WHITE) ? finalEvalScore : -finalEvalScore;
}
void evaluateKings(const Board& board, int& mgScore, int& egScore) {
    // find kings
    int whiteKingSq = __builtin_ctzll(board.getPieceBitBoard(5));
    int blackKingSq = __builtin_ctzll(board.getPieceBitBoard(11));

    int whiteKingFile = whiteKingSq % 8;
    int blackKingFile = blackKingSq % 8;

    int castlingRights = board.getCastlingRights();

    U64 whitePawns = board.getPieceBitBoard(0);
    U64 blackPawns = board.getPieceBitBoard(6);

    // --- WHITE KING SAFETY ---
    // If the White King is on the D, E, or F file
    if (whiteKingFile >= 3 && whiteKingFile <= 5) {
        // White has entirely lost the right to castle.
        if ((castlingRights & 3) == 0) {
            mgScore -= 40; // give penalty for losing castling rights
        }
    } else {
        // king is tucked away on Files A-C or G-H
        // check the file the king is on plus adjacent files
        for (int f = std::max(0, whiteKingFile - 1); f <= std::min(7, whiteKingFile + 1); f++) {
            U64 rank2Square = 1ULL << (8 + f); // Rank 2
            U64 rank3Square = 1ULL << (16 + f); // Rank 3

            if (whitePawns & rank2Square) mgScore += 15; // give bonus if there is a pawn wall on second rank
            if (whitePawns & rank3Square) mgScore += 5; // give smaller bonus if pawns are on third rank(weaker wall)
            else mgScore -= 15; // missing pawn wall shield
        }
    }

    // --- BLACK KING SAFETY ---
    if (blackKingFile >= 3 && blackKingFile <= 5) {
        if ((castlingRights & 12) == 0) {
            mgScore += 40; // positive score is bad for black
        }
    } else {
        for (int f = std::max(0, blackKingFile - 1); f <= std::min(7, blackKingFile + 1); f++) {
            U64 rank7Square = 1ULL << (48 + f); // rank 7
            U64 rank6Square = 1ULL << (40 + f); // rank 6\

            if (blackPawns & rank7Square) mgScore -= 15;
            else if (blackPawns & rank6Square) mgScore -= 5;
            else mgScore += 15;
        }
    }
}
void evaluatePawns(const Board& board, int& mgScore, int& egScore) {
    U64 whitePawns = board.getPieceBitBoard(0); // White Pawns index
    U64 blackPawns = board.getPieceBitBoard(6); // Black Pawns index

    // --- EVALUATE WHITE PAWNS ---
    U64 wp = whitePawns;
    while (wp != 0ULL) {
        int sq = __builtin_ctzll(wp);
        int file = sq % 8;

        // DOUBLED PAWNS
        // XOR the file with the mask. If anything is left, its doubled.
        if ((whitePawns & fileMasks[file]) ^ (1ULL << sq)) {
            mgScore += DOUBLED_PAWN_MG;
            egScore += DOUBLED_PAWN_EG;
        }

        // ISOLATED PAWNS
        // if the adjacent files have no friendly pawns
        if ((whitePawns & isolatedMasks[file]) == 0ULL) {
            mgScore += ISOLATED_PAWN_MG;
            egScore += ISOLATED_PAWN_EG;
        }

        // PASSED PAWNS
        // If foward runway has zero enemy pawns
        if ((blackPawns & whitePassedMasks[sq]) == 0ULL) {
            int relative_rank = sq / 8;
            mgScore += PASSED_PAWN_MG[relative_rank];
            egScore += PASSED_PAWN_EG[relative_rank];
        }
        wp &= (wp - 1);
    }
    // --- EVALUATE BLACK PAWNS
    U64 bp = blackPawns;
    while (bp != 0ULL) {
        int sq = __builtin_ctzll(bp);
        int file = sq % 8;

        // DOUBLED PAWNS
        if ((blackPawns & fileMasks[file]) ^ (1ULL << sq)) {
            mgScore -= DOUBLED_PAWN_MG;
            egScore -= DOUBLED_PAWN_EG;
        }

        // ISOLATED PAWNS
        if ((blackPawns & isolatedMasks[file]) == 0ULL) {
            mgScore -= ISOLATED_PAWN_MG;
            egScore -= ISOLATED_PAWN_EG;
        }

        // PASSED PAWNS
        if ((whitePawns & blackPassedMasks[sq]) == 0ULL) {
            int relative_rank = 7 - (sq / 8);
            mgScore -= PASSED_PAWN_MG[relative_rank];
            egScore -= PASSED_PAWN_EG[relative_rank];
        }
        bp &= (bp - 1);
    }
}
void initPawnMasks() {
    U64 fileA = 0x0101010101010101ULL; // equivalently: 00000001 00000001 00000001 00000001 00000001 00000001 00000001 00000001

    // generate file masks
    for (int file = 0; file < 8; file++) {
        fileMasks[file] = fileA << file;
    }

    // generate isolatedMasks
    for (int file = 0; file < 8; file++) {

        isolatedMasks[file] = 0ULL;

        if (file == 0) { // if its on the A-file, ONLY turn on the bits of the file to the right
            isolatedMasks[file] |= fileMasks[file + 1];
        } else if (file == 7) { // if its on the H-file, ONLY turn on the bits of the file to the left
            isolatedMasks[file] |= fileMasks[file - 1];
        } else { // Otherwise turn on both files to the left and right
            isolatedMasks[file] |= fileMasks[file - 1] | fileMasks[file + 1];
        }
    }

    // generate passed pawn masks for WHITE and BLACK
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;

            // runway is pawns file + adjacent files
            U64 runway = fileMasks[file] |  isolatedMasks[file];

            // white pawns march up the board
            whitePassedMasks[square] = 0ULL;
            for (int r = rank + 1; r < 8; r++) {
                whitePassedMasks[square] |= (runway & (0xFFULL << (r * 8)));
            }

            // Black pawns march down the board
            blackPassedMasks[square] = 0ULL;
            for (int r = rank - 1; r >= 0; r--) {
                blackPassedMasks[square] |= (runway & (0xFFULL << (r * 8)));
            }
        }
    }
}