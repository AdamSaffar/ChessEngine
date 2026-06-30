//
// Created by saffa on 6/25/2026.
//
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <include/search.h>
#include <include/board.h>
#include <include/evaluation.h>
#include <include/move.h>
#include <include/moveGeneration.h>
#include <include/transposition.h>
// Most Valuable Victim - Least Valuable Attacker [Victim][Attacker]
// Victim Base Scores: Pawn = 100, Knight = 200, Bishop = 300, Rook = 400, Queen = 500
// Attacker Adjustments: Pawn += 5, Knight += 4, Bishop += 3, Rook += 2, Queen += 1, King += 0
const int MVV_LVA[6][6] = {
    // Attacker: P N B R Q K
    {105, 104, 103, 102, 101, 100}, // Victim: Pawn
    {205, 204, 203, 202, 201, 200}, // Victim: Knight
    {305, 304, 303, 302, 301, 300}, // Victim: Bishop
    {405, 404, 403, 402, 401, 400}, // Victim: Rook
    {505, 504, 503, 502, 501, 500}, // Victim: Queen
    {0, 0, 0, 0, 0, 0} // Victim: King
};
/* Checks Transposition Table first for an exact move match(Highest Score)
 * Then check the MVV-LVA lookup table(Second Highest Scoring)
 * All other moves(sorted to the back)
 */
int scoreMove(int move, Board& board, int ttMove) {
    // if this move is the exact move from TT, give it the highest possible score
    if (move == ttMove) {
        return 2000000;
    }
    int flag = getFlag(move);
    bool isCapture = (flag == CAPTURE || flag == EN_PASSANT || (flag >= PC_KNIGHT && flag <= PC_QUEEN));

    if (!isCapture) {
        return 0; // Non-capture moves get sorted to back
    }

    int start = getStart(move);
    int target = getTarget(move);

    // En Passant Logic(Target square is empty, handle separately)
    if (flag == EN_PASSANT) {
        // Always pawn attacking pawn
        return MVV_LVA[PIECE_TYPE::Pawn][PIECE_TYPE::Pawn];
    }

    // Extract piece type
    int attacker = board.getPieceAt(start) % 6;
    int victim = board.getPieceAt(target) % 6;

    // return score from lookup table
    return MVV_LVA[victim][attacker];
}
// Insertion sort Algorithm(from largest to smallest scores) to sort move order
void sortMoves(MoveList& moveList, int moveScores[]) {
    for (int i = 1; i < moveList.count; i++) {
        int keyScore = moveScores[i];
        int keyMove = moveList.moves[i];
        int j = i - 1;

        while (j >= 0 && moveScores[j] < keyScore) {
            moveScores[j + 1] = moveScores[j]; // shift score
            moveList.moves[j + 1] = moveList.moves[j]; // shift the associated move
            j--;
        }
        moveScores[j + 1] = keyScore;
        moveList.moves[j + 1] = keyMove;
    }
}
/** Quiescence Search ignores depth limit and recursively plays out all available captures. */
int quiescence(Board& board, int alpha, int beta) {
    // get standalone score before doing anything
    int currentScore = evaluate(board);

    // if currentSCore is already enough for beta cutoff, stop here
    if (currentScore >= beta) {
        return beta;
    }

    // update highest score
    if (currentScore > alpha) {
        alpha = currentScore;
    }

    MoveList moveList;
    generateMoves(moveList, board);

    // Score and sort moves
    int moveScores[256];
    for (int i = 0; i < moveList.count; i++) {
        moveScores[i] = scoreMove(moveList.moves[i], board, 0);
    }
    sortMoves(moveList, moveScores);

    for (int i = 0; i < moveList.count; i++) {
        int move = moveList.moves[i];
        int flag = getFlag(move);

        // Filter out quiet moves
        bool isCapture = (flag == CAPTURE || flag == EN_PASSANT || (flag >= PC_KNIGHT && flag <= PC_QUEEN));
        if (!isCapture) {
            continue;
        }

        if (!makeMove(move, board)) {
            continue; // illegal move
        }
        // recurse until there are no captures left
        int score = -quiescence(board, -beta, -alpha);
        unmakeMove(move, board);

        // Alpha-beta pruning
        if (score >= beta) {
            return beta; // FIX: Cut off the dead branch!
        }
        if (score > alpha) {
            alpha = score;
        }
    }
    return alpha;
}
/** Core recursive search tree using the Negamax algorithm.
 * Explores the game tree using Depth-First search,
 * Negamax assumes every node wants to maximize its own score.
 */
int negamax(Board& board, int depth, int alpha, int beta) {
    int originalAlpha = alpha;
    int ttMove = 0;

    // probe transposition table
    int ttScore = probeTT(board.getHashKey(), depth, alpha, beta, ttMove);

    // if transposition table gave a valid score, skip the entire branch
    if (ttScore != UNKNOWN_SCORE) {
        return ttScore;
    }

    // Base case: Reached the end depth
    if (depth == 0) {
        return quiescence(board, alpha, beta); // q-search resolves captures and returns eval
    }
    // Start with the worst possible score
    int maxScore = -INF;
    int bestMoveThisNode = 0;
    int legalMoves = 0; // track how many moves played

    // Get all possible moves for this board state
    MoveList moveList;
    generateMoves(moveList, board);

    // Score every move
    int moveScores[256];
    for (int i = 0; i < moveList.count ; i++) {
        moveScores[i] = scoreMove(moveList.moves[i], board, ttMove);
    }
    // Sort move list based on scores(highest to lowest)
    sortMoves(moveList, moveScores);

    // Apply evaluation on sorted move list
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
        if (score > maxScore) {
            maxScore = score;
            bestMoveThisNode = move;
        }

        // --- ALPHA BETA PRUNING ---

        alpha = std::max(alpha, maxScore); // update minimum guranteed score

        // the BETA cutoff
        if (alpha >= beta) {
            // min gauranteed score is higher than max score
            // opponent will avoid branch entirely

            storeTT(board.getHashKey(), depth, HASH_BETA, maxScore, bestMoveThisNode);
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
    int hashFlag = HASH_ALPHA;
    if (maxScore > originalAlpha) {
        hashFlag = HASH_EXACT; // found a move that improves position
    }
    // Save to memory
    if (bestMoveThisNode != 0) {
        storeTT(board.getHashKey(), depth, hashFlag, maxScore, bestMoveThisNode);
    }
    return maxScore;
}

int bestMoveToPlay = 0;
/** catch the physical move associated with the absolute highest score from the search */
void searchRoot(Board& board, int depth, std::chrono::time_point<std::chrono::steady_clock> startTime) {
    int maxScore = -INF;
    int bestRootMove = 0;

    int alpha = -INF;
    int beta = INF;

    int ttMove = 0;
    probeTT(board.getHashKey(), depth, alpha, beta, ttMove);

    MoveList moveList;
    generateMoves(moveList, board);

    // Score every move
    int moveScores[256];
    for (int i = 0; i < moveList.count ; i++) {
        moveScores[i] = scoreMove(moveList.moves[i], board, ttMove);
    }
    // Sort move list based on scores(highest to lowest)
    sortMoves(moveList, moveScores);

    for (int i = 0; i < moveList.count; i++) {
        int move = moveList.moves[i];
        if (!makeMove(move, board)) continue;

        // guarantees engine doesn't infintley loop if it knows it is getting checkmated
        if (bestRootMove == 0) {
            bestRootMove = move;
        }
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

    // Calculate elapsed time
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime).count();

    // Print debugging information like: depth, score, and time spent at depth
    std::cout << "info depth " << depth
              << " score cp " << maxScore
              << " time " << duration
              << std::endl;
}