#include <iostream>
#include "Engine/board.h"
#include "Engine/moveGeneration.h"
#include "Engine/magicNumbers.h"
int main() {
    // run attack lookup table
    attackLookupTable();
    relevantBlockerMask();
    // print ALL 128 magic numbers for bishops and rooks to terminal
    initMagicNumbers();
    Board board;
    //board.printBoard();
    return 0;
}
