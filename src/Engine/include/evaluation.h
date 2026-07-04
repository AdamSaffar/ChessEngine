//
// Created by saffa on 6/21/2026.
//

#ifndef CHESSENGINE_EVALUATION_H
#define CHESSENGINE_EVALUATION_H
#include "board.h"
// Standard Material Values/Weights
// King does not have a set value(it can't ever be captured)
const int pieceValues[12] = {
    100, 300, 300, 500, 900, 0,// WHITE: Pawn, Knight, Bishop, Rook, Queen, and King
    100, 300, 300, 500, 900, 0 //BLACK: Pawn, Knight, Bishop, Rook, Queen, and King
};
// --- PIECE SQUARE TABLES ---
// These tables give bonus points for placing pieces on "good" squares
// Positive = Good Square, Negative = Bad Square

// --- MIDGAME PIECE SQUARE TABLES ---
const int middle_game_PST[6][64] = {
    // PAWN Table:
    // White pawns want to be pushed forward and control central squares
    {
        0,  0,  0,  0,  0,  0,  0,  0, // Rank 8
        50, 50, 50, 50, 50, 50, 50, 50, // Rank 7, etc
        10, 10, 20, 30, 30, 20, 10, 10,
        5,  5,  10, 27, 27, 10,  5, 5,
        0,  0,  0,  25, 25,  0,  0, 0,
        5, -5, -10, 0,  0, -10, -5, 5,
        5, 10,  10, -25,-25, 10, 10,5,
        0,  0,  0,   0,  0,  0,  0, 0
    },
    // KNIGHT Table:
    // Knights want to control central squares and stay away from edges to increase mobility
    {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-20,-30,-30,-20,-40,-50,
    },
    // BISHOP table:
    // Bishops also want to control the center and stay away from edges and corners
    {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-40,-10,-10,-40,-10,-20,
    },
    // Rook Table:
    {
        0,  0,  0,  5,  5,  0,  0,  0,
        -5, 20, 20, 20, 20, 20, 20, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         0,  0,  0,  5,  5,  0,  0,  0
    },
    // Queen Table:
    {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
          0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    },
    // King Table:
    {
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -20, -30, -30, -40, -40, -30, -30, -20,
        -10, -20, -20, -20, -20, -20, -20, -10,
         20,  20,   0,   0,   0,   0,  20,  20,
         20,  30,  10,   0,   0,  10,  30,  20
    }
};

// --- ENDGAME PIECE SQUARE TABLES ---
const int end_game_PST[6][64] = {
    // Pawn Table:
    {
        0,   0,   0,   0,   0,   0,   0,   0,
        80,  80,  80,  80,  80,  80,  80,  80, // Rank 7: 1 step from promotion(Highest value pawns)
        50,  50,  50,  50,  50,  50,  50,  50,
        30,  30,  30,  30,  30,  30,  30,  30,
        20,  20,  20,  20,  20,  20,  20,  20,
        10,  10,  10,  10,  10,  10,  10,  10,
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0
    },
    // Knight Table:
    // Knights act similarily in the endgame, stay away from edges
    {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-20,-30,-30,-20,-40,-50,
    },
    // Bishop Table:
    // bishops still want long diagonals and central squares
    {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-40,-10,-10,-40,-10,-20,
    },
    // Rook Table:
    // rooks want to centralize and support passed pawns
    {
        -5, -5, -5,  0,  0, -5, -5, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5, -5, -5,  0,  0, -5, -5, -5
    },
    // Queen Table:
    // queens are much safer to centralize in endgame
    {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0, 10, 10, 10, 10,  0, -5, // Stronger center focus
          0,  0, 10, 10, 10, 10,  0, -5, // Stronger center focus
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    },
    // King Table:
    // king becomes very active in the endgame

    {
        -50,-40,-30,-20,-20,-30,-40,-50,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -50,-30,-30,-30,-30,-30,-30,-50
    }
};

// --- PHASE WEIGHTS ---
// Phase weights measures the pieces impact on the stage of the game
/* Pawns (0): Pawns do not determine the phase of the game, so they carry zero weight
 * King (0): Because kings can never be captured, they will always be on the board. So, zero weight.
 * Kights & Bishops(1): Small impact on transitioning to an endgame
 * Rooks(2): Trading heavy pieces like rooks transition the game toward an endgame much faster
 * Queens(4): Trading queens is universally recognized as the fastest way to instantly transition to an end-game.
 * 4 Knights(4 x 1 = 4) + 4 Bishops(4 x 1 = 4) + 4 Rooks(4 x 2 = 8) + 2 Queens(2 x 4 = 8) = 24
 * * 24 represents PURE MIDDLE-GAME
 * * 0 represents PURE END-GAME(ONLY kings and pawns left)
 */
const int phaseWeights[12] = {
    0, 1, 1, 2, 4, 0,  // White: Pawn, Knight, Bishop, Rook, Queen, King
    0, 1, 1, 2, 4, 0   // Black: Pawn, Knight, Bishop, Rook, Queen, King
};

int evaluate(const Board& board);
void initPawnMasks();
#endif //CHESSENGINE_EVALUATION_H
