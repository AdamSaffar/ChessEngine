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
// White pawns want to be pushed forward and control central squares
const int mg_pawnPST[64] = {
    0,  0,  0,  0,  0,  0,  0,  0, // Rank 8
    50, 50, 50, 50, 50, 50, 50, 50, // Rank 7, etc
    10, 10, 20, 30, 30, 20, 10, 10,
    5,  5,  10, 27, 27, 10,  5, 5,
    0,  0,  0,  25, 25,  0,  0, 0,
    5, -5, -10, 0,  0, -10, -5, 5,
    5, 10,  10, -25,-25, 10, 10,5,
    0,  0,  0,   0,  0,  0,  0, 0
};
// Knights want to control central squares and stay away from edges to increase mobility
const int mg_knightPST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-20,-30,-30,-20,-40,-50,
};
// Bishops also want to control the center and stay away from edges and corners
const int mg_bishopPST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-40,-10,-10,-40,-10,-20,
};
const int mg_rookPST[64] = {
    0,  0,  0,  5,  5,  0,  0,  0,
    -5, 20, 20, 20, 20, 20, 20, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};
const int mg_queenPST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int mg_kingPST[64] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20
};

// --- ENDGAME PIECE SQUARE TABLES ---

const int eg_pawnPST[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    80,  80,  80,  80,  80,  80,  80,  80, // Rank 7: 1 step from promotion(Highest value pawns)
    50,  50,  50,  50,  50,  50,  50,  50,
    30,  30,  30,  30,  30,  30,  30,  30,
    20,  20,  20,  20,  20,  20,  20,  20,
    10,  10,  10,  10,  10,  10,  10,  10,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};
// Knights act similarily in the endgame, stay away from edges
const int eg_knightPST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-20,-30,-30,-20,-40,-50,
};
// bishops still want long diagonals and central squares
const int eg_bishopPST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-40,-10,-10,-40,-10,-20,
};
// rooks want to centralize and support passed pawns
const int eg_rookPST[64] = {
    -5, -5, -5,  0,  0, -5, -5, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5, -5, -5,  0,  0, -5, -5, -5
};
// queens are much safer to centralize in endgame
const int eg_queenPST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0, 10, 10, 10, 10,  0, -5, // Stronger center focus
      0,  0, 10, 10, 10, 10,  0, -5, // Stronger center focus
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
// king becomes very active in the endgame
const int eg_kingPST[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

// --- PHASE WEIGHTS ---
// Used to determine how far into the endgame we are (Max = 24)
const int phaseWeights[12] = {
    0, 1, 1, 2, 4, 0,  // White: Pawn, Knight, Bishop, Rook, Queen, King
    0, 1, 1, 2, 4, 0   // Black: Pawn, Knight, Bishop, Rook, Queen, King
};
#endif //CHESSENGINE_EVALUATION_H
