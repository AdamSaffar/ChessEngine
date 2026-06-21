//
// Created by saffa on 6/20/2026.
//
#include "include/perft.h"
#include "include/moveGeneration.h"
/** Brute-force node counter
 * Returns the exact total number of possible, strictly
 * legal board positions that exist exactly depth moves
 * into the future.
 */
U64 perft(int depth, Board board) {
    // BASE CASE: if we hit depth 0, found 1 valid leaf node
    if (depth == 0) {
        return 1ULL;
    }
    MoveList list;
    generateMoves(list, board);

    U64 nodes = 0;
    for (int i = 0; i < list.count; i++) {
        // create clone of the board state for this specific branch
        Board copy = board;

        // Attempt to play the move on copied board
        // if makeMove returns false, it means the move left our king in check
        if (!makeMove(list.moves[i], copy)) {
            continue;
        }

        // if move is legal, dive one depth depper and add the results to our total
        nodes += perft(depth - 1, copy);
    }
    return nodes;
}