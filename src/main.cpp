#include <iostream>
#include "Engine/include/board.h"
#include "Engine/include/moveGeneration.h"
#include "Engine/include/magicNumbers.h"
#include "Engine/include/perft.h"
#include "Engine/include/evaluation.h"
int main() {
    // Calculate all attack tables and magic bitboards
    initAllMoveGen();
    // init board
    Board board;
    std::string fen = "rrnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    board.parseFEN(fen);
    board.printBoard();
    //board.initStandardPosition(); // put all pieces on their starting squares
    //int score = evaluate(board);
    //std::cout << score;
    //uint64_t totalPositions = perft(1, board);
    //std::cout << totalPositions;

    // print ALL 128 magic numbers for bishops and rooks to terminal
    //initMagicNumbers();
    //board.printBoard();
    return 0;
}
