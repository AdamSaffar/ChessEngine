#include <iostream>
#include "Engine/include/board.h"
#include "Engine/include/moveGeneration.h"
#include "Engine/include/magicNumbers.h"
int main() {
    // Call entire move generation system
    initAllMoveGen();

    // print ALL 128 magic numbers for bishops and rooks to terminal
    initMagicNumbers();

    Board board;
    //board.printBoard();
    return 0;
}
