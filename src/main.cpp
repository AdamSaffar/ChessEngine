#include <iostream>
#include <string>
#include "Engine/include/board.h"
#include "Engine/include/moveGeneration.h"
#include "Engine/include/search.h"
#include "Engine/include/move.h"


// Helper func to translate square index to chess notation(12 -> "e2")
std::string indexToChessNotation(int sq) {
    std::string result = "";
    result += (char)('a' + (sq % 8)); // File
    result += (char)('1' + (sq / 8)); // Rank
    return result;
}
// Helper func to parse users move
bool parseUserMove(std::string input, int& start, int& target, int color) { // User input example: "e2e3"
    // Queen Side Castle
    if (input == "O-O-O") {
        start = (color == COLOR::WHITE) ? 4 : 60; // e1 or e8
        target = (color == COLOR::WHITE) ? 2 : 58; // c1 or c8
        return true;
    } else if (input == "O-O") { // King Side Castle
        start  = (color == COLOR::WHITE) ? 4 : 60;  // e1 or e8
        target = (color == COLOR::WHITE) ? 6 : 62;  // g1 or g8
        return true;
    }
    if (input.length() < 4) return false; // safety check

    // Standard move decoding
    int startFile = input[0] - 'a';
    int startRank = input[1] - '1';
    int targetFile = input[2] - 'a';
    int targetRank = input[3] - '1';

    if (startFile < 0 || startFile > 7 || startRank < 0 || startRank > 7) return false;
    if (targetFile < 0 || targetFile > 7 || targetRank < 0 || targetRank > 7) return false;

    // pass board index for start and end squares
    start = startRank * 8 + startFile;
    target = targetRank * 8 + targetFile;
    return true;
}
int main() {
    // Calculate all attack tables and magic bitboards
    initAllMoveGen();
    // init board
    Board board;
    board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // put all pieces on their starting squares
    std::cout << "=====================================\n";
    std::cout << "      CHESS ENGINE TERMINAL          \n";
    std::cout << "=====================================\n";
    std::cout << "Type moves in format 'e2e4' or 'quit'. King side castle: O-O, Queen side castle: O-O-O\n";


    while (true) {
        board.printBoard();
        MoveList moveList;
        generateMoves(moveList, board);
        // check if game is over
        int legalMoves = 0;
        for (int i = 0; i < moveList.count; i++) {
            if (makeMove(moveList.moves[i], board)) {
                legalMoves++;
                unmakeMove(moveList.moves[i], board);
            }
        }
        if (legalMoves == 0) {
            int kingPiece = (board.getSideToMove() == COLOR::WHITE) ? PIECE_TYPE::King : PIECE_TYPE::King + 6;
            int kingSquare = __builtin_ctzll(board.getPieceBitBoard(kingPiece));

            if (board.isSquareAttacked(kingSquare, board.getSideToMove() ^ 1)) {
                std::cout << "CHECKMATE! Game Over. \n";

            } else {
                std::cout << "STALEMATE! Game Over. \n";
            }
            break;
        }
        // --- HUMAN TURN (WHITE) ---
        if (board.getSideToMove() == COLOR::WHITE) {
            std::string input;
            std::cout << "Enter Move: ";
            std::cin >> input;

            if (input == "quit") break;
            int start, target;

            if (!parseUserMove(input, start, target, board.getSideToMove())) {
                std::cout << "Invalid Move. Try again. \n";
                continue;
            }
            int userMove = 0;
            // Find the users move in the move list that contains all possible moves for Human
            for (int i = 0; i < moveList.count; i++) {
                if (getStart(moveList.moves[i]) == start && getTarget(moveList.moves[i]) == target) {
                    userMove = moveList.moves[i];
                    break; // exit loop since we found users move
                }
            }
            // Make users move if it was found
            if (userMove == 0 || !makeMove(userMove, board)) {
                std::cout << "Illegal Move! Try again.\n";
                continue;
            }
         }
        // --- COMPUTERS MOVE (BLACK) ---
        else {
            searchRoot(board, 6); // Chose Depth 6. Smaller depths are faster searches, larger depths are more accurate
            makeMove(bestMoveToPlay, board);
            std::string computerMove = indexToChessNotation(getStart(bestMoveToPlay)) +
                indexToChessNotation(getTarget(bestMoveToPlay));

            std::cout << "COMPUTER PLAYS: " << computerMove << "\n";
        }
    }
    return 0;
}
