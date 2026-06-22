//
// Created by saffa on 6/21/2026.
//
#include "include/board.h"
#include "include/evaluation.h"
int evaluate(const Board& board) {
    int midGameScore{};
    int endGameScore{};
    int totalPhaseWeight{};
    int finalEvalScore{};
    // loop through all 12 piece types
    for (int pieceType = 0; pieceType < 12; pieceType++) {
        U64 pieceBitBoard = board.getPieceBitBoard(pieceType);
        while (pieceBitBoard != 0ULL) {
            // Finds the exact square of the lowest 1 bit on the board
            int square = __builtin_ctzll(pieceBitBoard);

            // This ensures peice index is always between 0-5 for black and white
            int normalizedPiece = pieceType % 6;

            // Update Phase Weight
            totalPhaseWeight += phaseWeights[pieceType]; // Use pieceType(0-11 index) since our phaseWeights holds 12 elements

            // Lookup Standard Piece Values
            int material = pieceValues[pieceType];

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