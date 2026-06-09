#include <iostream>
#include "Engine/board.h"
#include "Engine/moveGeneration.cpp"

int main() {
    // run attack lookup table
    attackLookupTable();
    Board board;
    board.printBoard();
    return 0;
}
