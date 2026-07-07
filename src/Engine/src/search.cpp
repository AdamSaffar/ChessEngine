//
// Created by saffa on 6/25/2026.
//
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <include/search.h>
#include <include/board.h>
#include <include/evaluation.h>
#include <include/move.h>
#include <include/moveGeneration.h>
#include <include/transposition.h>

#define MAX_PLY 64

std::atomic<unsigned long long> nodesSearched = 0; // cumulative total node count
thread_local int localNodeCounter = 0; // private counter for each thread
thread_local int killerMoves[MAX_PLY][2]= {0}; // [ply][slot 0 or 1]
thread_local int historyTable[2][64][64] = {0}; // [Color][Start Square][Target Square]

const int TT_MOVE_SCORE = 2000000;
const int CAPTURE_BASE_SCORE = 1000000;
const int KILLER_1_SCORE = 90000;
const int KILLER_2_SCORE = 80000;

std::atomic<bool> stopSearch = false;
long long searchTimeLimit = -1; // time allowed in milliseconds
std::chrono::time_point<std::chrono::steady_clock> searchStartTime;


// each thread gets its on RNG
thread_local unsigned int threadRNG_state = 1804289383;

// Fast PRNG (Xorshift32 algorithm)
inline unsigned int fastRand() {
    threadRNG_state ^= threadRNG_state << 13;
    threadRNG_state ^= threadRNG_state >> 17;
    threadRNG_state ^= threadRNG_state << 5;
    return threadRNG_state;
}
bool isRepetition(const Board& board) {
    // repetition requries a min of 4 plies
    // stop search at the halfMoveClock limit
    int limit = board.historyPly - board.halfMoveClock;
    if (limit < 0) limit = 0; // safety catch

    // Loop backwards for last 2 full moves
    for (int i = board.historyPly - 4; i >= limit; i -= 2) {
        if (board.repetitionTable[i] == board.hashKey) {
            return true;
        }
    }
    return false;
}

void checkTime() {
    if (searchTimeLimit == -1) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStartTime).count();

    if (elapsed >= searchTimeLimit) {
        stopSearch = true;
    }
}

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
/* 1. Transposition Table first for an exact move match(Highest Score)
 * 2. MVV-LVA lookup table(Second Highest Scoring)
 * 3. killerMoves array(Third highest scoring)
 * 4. historyTable 3D array( 4th highest scoring)
 * 5. All other moves(scored very low)
 */
int scoreMove(int move, Board& board, int ttMove, int ply) {
    // if this move is the exact move from TT, give it the highest possible score
    if (move == ttMove) {
        return TT_MOVE_SCORE;
    }
    int flag = getFlag(move);
    bool isCapture = (flag == CAPTURE || flag == EN_PASSANT || (flag >= PC_KNIGHT && flag <= PC_QUEEN));
    int color = board.getSideToMove();
    int start = getStart(move);
    int target = getTarget(move);

    // Sorting non-capture moves:
    // 1. Quiet Queen Promotions 2. First Killer Move 3. Second Killer Move 4. History Table move 5. All others
    if (!isCapture) {
        if (flag == PR_QUEEN) {
            return CAPTURE_BASE_SCORE - 50;
        } else if (move == killerMoves[ply][0]) {
            return KILLER_1_SCORE;
        } else if (move == killerMoves[ply][1]) {
            return KILLER_2_SCORE;
        } else if (historyTable[color][start][target] != 0){
            // cap the history score just below the killer 2 score to protect hierarchy
            int score = std::min(historyTable[color][start][target], KILLER_2_SCORE - 1);

            // artifical jitter: add up to 15 points to break ties differentely across threads
            return score + (fastRand() % 16);
        } else {
            // add artifical jitter to randomize un-scored quite moves across threads
            return fastRand() % 16; // Non-capture non-queen-promos non-killer non-history moves get sorted to back
        }
    }

    // En Passant Logic(Target square is empty, handle separately)
    if (flag == EN_PASSANT) {
        // Always pawn attacking pawn
        return CAPTURE_BASE_SCORE + MVV_LVA[PIECE_TYPE::Pawn][PIECE_TYPE::Pawn];
    }

    // Extract piece type
    int attacker = board.getPieceAt(start) % 6;
    int victim = board.getPieceAt(target) % 6;

    // return score from lookup table
    return CAPTURE_BASE_SCORE + MVV_LVA[victim][attacker];
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
        return currentScore;
    }
    // baseline score is standing pat
    int maxScore = currentScore;

    // update highest score
    if (currentScore > alpha) {
        alpha = currentScore;
    }

    MoveList moveList;
    generateMoves(moveList, board);

    // Score and sort moves
    int moveScores[256];
    for (int i = 0; i < moveList.count; i++) {
        moveScores[i] = scoreMove(moveList.moves[i], board, 0, 0);
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
        localNodeCounter++; // increment node count
        if ((localNodeCounter & 2047) == 0) {
            nodesSearched += 2048;
        }
        // recurse until there are no captures left
        int score = -quiescence(board, -beta, -alpha);
        unmakeMove(move, board);

        // keep the highest score found
        if (score > maxScore) {
            maxScore = score;
        }

        // Alpha-beta pruning
        alpha = std::max(alpha, maxScore);
        if (alpha >= beta) {
            return maxScore; // FIX: Cut off the dead branch!
        }
    }
    return maxScore;
}

// helper func to make null move
void makeNullMove(Board& board) {
    // save current state board (so we can unmake null move later)
    board.history[board.historyPly].castlingRights = board.castlingRights;
    board.history[board.historyPly].enPassantSquare = board.enPassantSquare;
    board.history[board.historyPly].halfMoveClock = board.halfMoveClock;

    board.history[board.historyPly].capturedPieceType = -1;
    board.history[board.historyPly].hashKey = board.hashKey;
    board.historyPly++;

    U64 hashKey = board.getHashKey();
    // clear the en passant square
    if (board.getEnpassantSquare() != -1) {
        // XOR current EP square
        hashKey ^= enPassantKeys[board.getEnpassantSquare()];
        board.setEnpassantSquare(-1);
    }
    // Flip side to move
    board.setSideToMove(board.getSideToMove() ^ 1);
    hashKey ^= sideKey;

    board.setHashKey(hashKey);
}
// helper func to unmake null move
void unmakeNullMove(Board& board) {
    // Move history pointer back by 1 to view the game state before this move
    board.historyPly--;
    GameState state = board.history[board.historyPly];

    // Restore State
    board.castlingRights = state.castlingRights;
    board.enPassantSquare = state.enPassantSquare;
    board.halfMoveClock = state.halfMoveClock;
    board.sideToMove = (board.sideToMove == COLOR::WHITE) ? COLOR::BLACK : COLOR::WHITE;;
    board.hashKey = state.hashKey; // restore zobrist key
}
/** Core recursive search tree using the Negamax algorithm.
 * Explores the game tree using Depth-First search,
 * Negamax assumes every node wants to maximize its own score.
 */
int negamax(Board& board, int depth, int alpha, int beta, int ply) {
    // HANDLE DRAW BY REPETITION
    // Check for repetition or 50-move rule
    if (board.getHistoryPly() > 0 && (isRepetition(board) || board.getHalfMoveClock() >= 100)) {
        return 0;
    }
    // prevent array out of bounds
    if (ply >= MAX_PLY) {
        return  quiescence(board, alpha, beta);
    }
    localNodeCounter++; // atomic increment

    // Check time and update global atomic only once every 2048 nodes (checking every node is computationaly expensive)
    if ((localNodeCounter & 2047) == 0) {
        nodesSearched += 2048;
        checkTime();
    }
    // instantly return the search if time is up
    if (stopSearch) return 0;

    int originalAlpha = alpha;
    int ttMove = 0;

    // probe transposition table
    int ttScore = probeTT(board.getHashKey(), depth, alpha, beta, ttMove);

    // if transposition table gave a valid score, skip the entire branch
    if (ttScore != UNKNOWN_SCORE) {
        if (ttScore > MATE_VALUE - MAX_PLY) ttScore -= ply;
        else if (ttScore < -MATE_VALUE + MAX_PLY) ttScore += ply;

        return ttScore;
    }
    // Check King Status
    int friendlyColor = board.getSideToMove();
    int kingPieceIndex = PIECE_TYPE::King + ((friendlyColor == COLOR::BLACK) ? 6 : 0);
    U64 kingBitBoard = board.getPieceBitBoard(kingPieceIndex);
    int kingSquare = __builtin_ctzll(kingBitBoard);
    int colorOffset = (friendlyColor == COLOR::BLACK) ? 6 : 0;

    bool inCheck = board.isSquareAttacked(kingSquare, friendlyColor ^ 1);

    // Base case: Reached the end depth and not in check
    if (depth <= 0 && !inCheck) {
        return quiescence(board, alpha, beta); // q-search resolves captures and returns eval
    }
    // Start with the worst possible score
    int maxScore = -INF;
    int bestMoveThisNode = 0;
    int legalMoves = 0; // track how many moves played

    // ---NULL MOVE PRUNING---
    int R = 2; // depth reduction factor

    // check if current player has non-pawn material
    bool pawnsOnly = (board.pieceBitBoards[Knight + colorOffset] | board.pieceBitBoards[Bishop + colorOffset] |
    board.pieceBitBoards[Rook + colorOffset]  | board.pieceBitBoards[Queen + colorOffset]) == 0ULL;

    // only attempt NMP if: not in check, enough depth to justify reduction, and non-pawn material remaining on board
    if (depth >= 3 && !inCheck && !pawnsOnly) {
        makeNullMove(board);

        // search with reduced depth
        int score = -negamax(board, depth - 1 - R, -beta, -beta + 1, ply + 1);

        unmakeNullMove(board);

        // KEY LOGIC: if the score still beats beta even after a free move
        // position is incredibly strong. Prune branch.
        if (score >= beta) return beta;
    }
    // Get all possible moves for this board state
    MoveList moveList;
    generateMoves(moveList, board);

    // Score every move
    int moveScores[256];
    for (int i = 0; i < moveList.count ; i++) {
        moveScores[i] = scoreMove(moveList.moves[i], board, ttMove, ply);
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
        int score;
        int flag = getFlag(move);
        bool isCapture = (flag == CAPTURE || flag == EN_PASSANT || (flag >= PC_KNIGHT && flag <= PC_QUEEN));
        bool isKiller = (move == killerMoves[ply][0] || move == killerMoves[ply][1]);

        int oppColor = board.getSideToMove();
        int oppKingIndex = PIECE_TYPE::King + ((oppColor == COLOR::BLACK) ? 6 : 0);
        int oppKingSquare = __builtin_ctzll(board.getPieceBitBoard(oppKingIndex));
        bool givesCheck = board.isSquareAttacked(oppKingSquare, oppColor ^ 1);
        // PRINCIPAL VARIATION
        if (legalMoves == 1) {
            // first move gets a full-depth search
            score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
        } else {
            /* LATE MOVE REDUCTIONS
             * Reduce only if:
             * Depth is high enough to matter
             * Already searched most promising moves
             * not currently in check
             * is NOT a killerMove
             */
            if (depth >= 3  && legalMoves > 3 && !isCapture && !inCheck && !isKiller && !givesCheck) {
                // scale aggresively based on depth and move lateness
                int reduction = 1 + (depth / 4) + (legalMoves / 6);

                // If this move isn't a killer, but it has historically
                // caused cutoffs in other branches, trust it a bit more and reduce the penalty.
                int color = board.getSideToMove();
                int start = getStart(move);
                int target = getTarget(move);

                if (historyTable[color][start][target] > KILLER_2_SCORE / 2) {
                    reduction -= 1; // Give it one ply back
                }
                // never reduce the depth below 1
                if (reduction >= depth) {
                    reduction = depth - 1;
                }
                // search with reduced depth and zero window
                score = -negamax(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1);
            } else {
                // if LMR didnt apply, force PVS
                score = alpha + 1;
            }
            // If the move is very good, re-search at full depth
            if (score > alpha) {
                // search at full depth with a zero window
                score = -negamax(board, depth - 1, -alpha - 1, -alpha, ply + 1);

                // re-search with full window
                if (score > alpha && score < beta) {
                    score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
                }
            }
        }
        unmakeMove(move, board);

        if (stopSearch) return 0;

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

            // --- KILLER HEURISTIC & HISTORY HEURISTIC ---
            int flag = getFlag(bestMoveThisNode);
            // if a non-capture move caused a beta cutoff, save the move
            // when searching sibling branches, check if move is one of the killer moves from previous branch
            if (flag != CAPTURE && flag != EN_PASSANT && !(flag >= PC_KNIGHT && flag <= PC_QUEEN)) {
                /* Store two most recent non-capture moves that caused beta cut-off in killerMoves*/
                if (move != killerMoves[ply][0]) {
                    // Shift old killer down
                    killerMoves[ply][1] = killerMoves[ply][0]; // ply = distance from root
                    // Save new killer
                    killerMoves[ply][0] = bestMoveThisNode;
                }
                int color = board.getSideToMove();
                int start = getStart(bestMoveThisNode);
                int target = getTarget(bestMoveThisNode);
                historyTable[color][start][target] += depth * depth;

                // if score gets too high, apply decay to entire table
                if (historyTable[color][start][target] >= KILLER_2_SCORE) {
                    for (int c = 0; c < 2; c++) {
                        for (int s = 0; s < 64; s++) {
                            for (int t = 0; t < 64; t++) {
                                // right shift by 1 to divide by 2
                                historyTable[c][s][t] >>= 1;
                            }
                        }
                    }
                }
            }
            return maxScore; // FIX: return immediately
        }
    }
    // Engine has reached a board state where it is impossible to make a move
    // Game is over: Either computer is checkmated or Stalemated
    if (legalMoves == 0) {
        if (inCheck) {
            return -MATE_VALUE + ply; // Computer is checkmated(add ply to prefer faster checkmates)
        } else {
            return 0; //Stalemate
        }
    }
    int hashFlag = HASH_ALPHA;
    if (maxScore > originalAlpha) {
        hashFlag = HASH_EXACT; // found a move that improves position
    }

    int storeScore = maxScore;
    if (storeScore > MATE_VALUE - MAX_PLY) storeScore += ply;
    else if (storeScore < -MATE_VALUE + MAX_PLY) storeScore -= ply;

    // Save to memory
    if (bestMoveThisNode != 0) {
        storeTT(board.getHashKey(), depth, hashFlag, storeScore, bestMoveThisNode);
    }
    return maxScore;
}

/** Silent search function designed strictly for background helper threads */
void searchHelper(Board board, int depth) {
    int maxScore = -INF;
    int alpha = -INF;
    int beta = INF;

    int ttMove = 0;
    probeTT(board.getHashKey(), depth, alpha, beta, ttMove);

    MoveList moveList;
    generateMoves(moveList, board);

    int moveScores[256];
    for (int i = 0; i < moveList.count; i++) {
        moveScores[i] = scoreMove(moveList.moves[i], board, ttMove, 0);
    }
    sortMoves(moveList, moveScores);

    int legalMoves = 0;
    int bestMoveThisNode = 0; // track best move
    for (int i = 0; i < moveList.count; i++) {
        int move = moveList.moves[i];
        if (!makeMove(move, board)) continue;

        legalMoves++;
        int score;

        if (legalMoves == 1) {
            score = -negamax(board, depth - 1, -beta, -alpha, 1);
        } else {
            score = -negamax(board, depth - 1, -alpha - 1, -alpha, 1);
            if (score > alpha && score < beta) {
                score = -negamax(board, depth - 1, -beta, -alpha, 1);
            }
        }
        unmakeMove(move, board);

        if (stopSearch) break;
        if (score > maxScore) {
            maxScore = score;
            bestMoveThisNode = move; // save move
        }

        alpha = std::max(alpha, maxScore);
    }
    // save final eval so main thread can use it
    if (!stopSearch && bestMoveThisNode != 0) {
        storeTT(board.getHashKey(), depth, HASH_EXACT, maxScore, bestMoveThisNode);
    }
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
        moveScores[i] = scoreMove(moveList.moves[i], board, ttMove, 0);
    }
    // Sort move list based on scores(highest to lowest)
    sortMoves(moveList, moveScores);

    int legalMoves = 0;

    for (int i = 0; i < moveList.count; i++) {
        int move = moveList.moves[i];
        if (!makeMove(move, board)) continue;

        // guarantees engine doesn't infintley loop if it knows it is getting checkmated
        if (bestRootMove == 0) {
            bestRootMove = move;
        }

        legalMoves++;
        int score;

        // Principal Variation Search at root
        if (legalMoves == 1) {
            score = -negamax(board, depth - 1, -beta, -alpha, 1);
        } else {
            score = -negamax(board, depth - 1, -alpha - 1, -alpha, 1);
            if (score > alpha && score < beta) {
                // Re-search if it beats alpha
                score = -negamax(board, depth - 1, -beta, -alpha, 1);
            }
        }

        unmakeMove(move, board);

        // instantly abort if time ran out
        if (stopSearch) break;

        if (score > maxScore) {
            maxScore = score;
            bestRootMove = move; // Save the phyiscal move that caused the highest score
        }

        // update minimum acceptable score(alpha)
        alpha = std::max(alpha, maxScore);
    }
    if (!stopSearch) {
        bestMoveToPlay = bestRootMove;

        // save root result
        storeTT(board.getHashKey(), depth, HASH_EXACT, maxScore, bestRootMove);
    } else if (bestMoveToPlay == 0) {
        bestMoveToPlay = bestRootMove; // fallback
    }

    // dont print garbage data
    if (stopSearch) return;

    // Calculate elapsed time
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime).count();

    long long nps = 0;
    // protect against division by zero
    if (duration > 0) {
        // Nodes per second = (nodes searched * 1000 / Total Elapsed Time in ms)
        nps = (nodesSearched * 1000) / duration;
    }
    // Print debugging information like: depth, score, and time spent at depth
    std::cout << "info depth " << depth
              << " score cp " << maxScore
              << " time " << duration
              << " nodes " << nodesSearched
              << " nps " << nps
              << std::endl;
}