//
// Created by saffa on 6/28/2026.
//
#include <iostream>
#include <string>
#include <sstream>
#include "../Engine/include/board.h"
#include "../Engine/include/moveGeneration.h"
#include "../Engine/include/search.h"
#include "../Engine/include/move.h"

// required for UCI identification
void printEngineInfo() {
    std::cout << "id name Dummy\n";
    std::cout << "id author Adam S\n";
    std::cout << "uciok\n";
}

void parseMove(std::string input) {

}
int main() {
    // Initialize Engine
    initAllMoveGen();
    Board board;

    std::string line;
    // Infinite CLI loop
    while (std::getline(std::cin, line)) {
        // Use string stream to separate string by spaces
        std::istringstream iss(line);
        std::string command;
        iss >> command; // first word of the string

        if (command == "quit") {
            break;
        } else if (command == "uci"){
            printEngineInfo();
        } else if (command == "isready") {
            std::cout << "readyok\n";
        } else if (command == "position") {
            // TODO: Parse startpos string and update board
        } else if (command == "go") {
            // TODO: Call searchRoot() and print bestMoveToPlay
        }
    }

    return 0;
}