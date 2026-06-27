//
// Created by saffa on 6/25/2026.
//
#include <algorithm>
#include <include/search.h>
#include <include/board.h>
#include <include/evaluation.h>
#include <include/move.h>
#include <include/moveGeneration.h>

/** Core recursive search tree using the Negamax algorithm.
 * Explores the game tree using Depth-First search,
 * Negamax assumes every node wants to maximize its own score.
 */
int negamax(Board& board, int depth, int alpha, int beta) {
    // Base case: Reached the end depth
    if (depth == 0) {
        return evaluate(board);
    }
    // Start with the worst possible score
    int maxScore = -INF;
    int legalMoves = 0; // track how many moves played

    // Get all possible moves for this board state
    MoveList moveList;
    generateMoves(moveList, board);

    for (int i = 0; i < moveList.count; i++) {
        int move = moveList.moves[i];

        // attempt move
        if (!makeMove(move, board)) {
            // Invalid move: left our king in check
            continue;
        }
        legalMoves++;
        // --- NEGAMAX LOGIC ---

        int score = -negamax(board, depth - 1, -beta, -alpha);

        unmakeMove(move, board);

        // Keep the highest score
        maxScore = std::max(maxScore, score);

        // --- ALPHA BETA PRUNING ---

        alpha = std::max(alpha, maxScore); // update minimum guranteed score

        // the cutoff
        if (alpha >= beta) {
            // min gauranteed score is higher than max score
            // opponent will avoid branch entirely
            break;
        }
    }
    // Engine has reached a board state where it is impossible to make a move
    // Game is over: Either computer is checkmated or Stalemated
    if (legalMoves == 0) {
        int friendlyColor = board.getSideToMove();
        int kingPieceIndex = PIECE_TYPE::King + ((friendlyColor == COLOR::BLACK) ? 6 : 0);
        U64 kingBitBoard = board.getPieceBitBoard(kingPieceIndex);

        int kingSquare = __builtin_ctzll(kingBitBoard);
        bool inCheck = board.isSquareAttacked(kingSquare, friendlyColor ^ 1);

        if (inCheck) {
            return -MATE_VALUE; // Computer is checkmated
        } else {
            return 0; //Stalemate
        }
    }
    return maxScore;
}
int bestMoveToPlay = 0;
/** catch the physical move associated with the absolute highest score from the search */
void searchRoot(Board& board, int depth) {
    int maxScore = -INF;
    int bestRootMove = 0;

    int alpha = -INF;
    int beta = INF;
    MoveList moveList;
    generateMoves(moveList, board);

    for (int i = 0; i < moveList.count; i++) {
        int move = moveList.moves[i];
        if (!makeMove(move, board)) continue;
        // pass inverted bounds
        int score = -negamax(board, depth - 1, -beta, -alpha);

        unmakeMove(move, board);

        if (score > maxScore) {
            maxScore = score;
            bestRootMove = move; // Save the phyiscal move that caused the highest score
        }

        // update minimum acceptable score(alpha)
        alpha = std::max(alpha, maxScore);
    }
    bestMoveToPlay = bestRootMove;
}