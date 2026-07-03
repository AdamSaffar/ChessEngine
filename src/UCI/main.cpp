//
// Created by saffa on 6/28/2026.
//
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <sstream>
#include "../Engine/include/board.h"
#include "../Engine/include/moveGeneration.h"
#include "../Engine/include/search.h"
#include "../Engine/include/move.h"
#include "../Engine/include/zobrist.h"
#include "../Engine/include/transposition.h"
#define MAX_PLY 64

extern unsigned long long nodesSearched;
extern int killerMoves[MAX_PLY][2];
extern int historyTable[2][64][64];

// Helper func to translate square index to chess notation(12 -> "e2")
std::string indexToChessNotation(int sq) {
    std::string result = "";
    result += (char)('a' + (sq % 8)); // File
    result += (char)('1' + (sq / 8)); // Rank
    return result;
}
// required for UCI identification
void printEngineInfo() {
    std::cout << "id name Dummy" << std::endl;
    std::cout << "id author Adam S" << std::endl;
    std::cout << "uciok" << std::endl;
}

// Helper func to parse users move
void parseMove(std::string input, int& start, int& target, char& promotedPiece) { // User input example: "e2e3"
    promotedPiece = ' '; // default to no promotion
    //if (input.length() < 4) return false; // safety check

    // Standard move decoding
    int startFile = input[0] - 'a';
    int startRank = input[1] - '1';
    int targetFile = input[2] - 'a';
    int targetRank = input[3] - '1';

    // if (startFile < 0 || startFile > 7 || startRank < 0 || startRank > 7) return false;
    //if (targetFile < 0 || targetFile > 7 || targetRank < 0 || targetRank > 7) return false;

    // pass board index for start and end squares
    start = startRank * 8 + startFile;
    target = targetRank * 8 + targetFile;

    // Promotion check
    if (input.length() >= 5) {
        promotedPiece = std::tolower(input[4]); // example input: e7e8q
    }
    //return true;
}


int main() {
    // Initialize Engine
    initAllMoveGen();
    initZobrist(); // init random hash keys
    initTT(64); // init transposition table

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
            std::cout << "readyok" << std::endl;
        } else if (command == "position") {
            std::string setupToken;
            iss >> setupToken;
            if (setupToken == "startpos") {
                // reset board
                board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                board.setHashKey(generateHashKey(board)); // generate unique zobrist key
                std::string skip;
                iss >> skip;; // skip word "moves"
            }
            else if (setupToken == "fen") {
                std::string fen = "", fenToken;
                for (int i = 0; i < 6; i++) {
                    iss >> fenToken;
                    fen += fenToken + " ";
                }
                board.parseFEN(fen);
                // generate hash key after parsing FEN
                board.setHashKey(generateHashKey(board));
                std::string skip;
                iss >> skip; // skip word "moves"
            }

            std::string moveString;// Stores entire move history
            // loops through every move word
            while (iss >> moveString) {
                int start, target;
                char promotedPiece;
                parseMove(moveString, start, target, promotedPiece);

                // Generate moves for current board state
                MoveList moveList;
                generateMoves(moveList, board);

                int playMove = 0;
                // Find the played move in the move list that contains all possible moves
                for (int i = 0; i < moveList.count; i++) {
                    int currentMove = moveList.moves[i];
                    int currentFlag = getFlag(moveList.moves[i]);
                    if (getStart(currentMove) == start && getTarget(currentMove) == target) {
                        bool isPromotion = (currentFlag >= PR_KNIGHT && currentFlag <= PC_QUEEN);
                        if (isPromotion) {
                            bool wantsQueen = (promotedPiece == 'q' && (currentFlag == PR_QUEEN || currentFlag == PC_QUEEN));
                            bool wantsRook = (promotedPiece == 'r' && (currentFlag == PR_ROOK || currentFlag == PC_ROOK));
                            bool wantsBishop = (promotedPiece == 'b' && (currentFlag == PR_BISHOP || currentFlag == PC_BISHOP));
                            bool wantsKnight = (promotedPiece == 'n' && (currentFlag == PR_KNIGHT || currentFlag == PC_KNIGHT));

                            if (wantsQueen || wantsRook || wantsBishop || wantsKnight) {
                                playMove = currentMove;
                                break; // found correct promotion
                            }
                        } else {
                            // Non-promotion move
                            playMove = currentMove;
                            break; // exit loop since we found users move
                        }
                    }
                }
                // Play move on board
                if (playMove != 0) {
                    makeMove(playMove, board);
                } else {
                    std::cout << "Invalid string: " << moveString << std::endl;
                }
            }
        } else if (command == "go") {
            int targetDepth = 8;
            std::string token;
            // read input string for depth
            while (iss >> token) {
                if (token == "depth") {
                    iss >> targetDepth;
                }
            }
            nodesSearched = 0; // reset node count

            // find best move via iterative deepening
            for (int currentDepth = 1; currentDepth <= targetDepth; currentDepth++) {
                auto startTime = std::chrono::steady_clock::now();
                searchRoot(board, currentDepth, startTime); // CALL SEARCH FUNCTION
            }

            if (bestMoveToPlay == 0) {
                // add safety catch
                std::cout << "bestmove 0000" << std::endl;
            } else {
                // translate start and target squares
                std::string computerMove = indexToChessNotation(getStart(bestMoveToPlay)) +
                                           indexToChessNotation(getTarget(bestMoveToPlay));
                // handle promotion flags
                int flag = getFlag(bestMoveToPlay);
                if (flag == PR_QUEEN || flag == PC_QUEEN) {
                    computerMove += 'q';
                } else if (flag == PR_ROOK || flag == PC_ROOK) {
                    computerMove += 'r';
                } else if (flag == PR_BISHOP || flag == PC_BISHOP) {
                    computerMove += 'b';
                } else if (flag == PR_KNIGHT || flag == PC_KNIGHT) {
                    computerMove += 'n';
                }

                // Send the move to console
                std::cout << "bestmove " << computerMove << std::endl;
            }
        }  else if (command == "setoption") {
            std::string skip;
            std::string optionToken, valToken, newTTSize;
            iss >> skip; // skip "name"
            iss >> optionToken >> valToken; // reads "Hash" and "value"
            if (optionToken == "Hash" && valToken == "value") {
                iss >> newTTSize;
                try {
                    int targetMB = std::stoi(newTTSize);
                    initTT(targetMB);// resize transposition table
                } catch (const std::exception& e) {
                    // safety catch incase std::stoi fails
                    std::cout << "Error resizing hash" << std::endl;
                }
            }
        } else if (command == "ucinewgame") {
            clearTT(); // wipe memory

            // clear killer moves array
            std::memset(killerMoves, 0, sizeof(killerMoves));
            // clear history table
            std::memset(historyTable, 0, sizeof(historyTable));
        }
    }

    return 0;
}