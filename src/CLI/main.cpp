#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <cstring>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "../Engine/include/board.h"
#include "../Engine/include/moveGeneration.h"
#include "../Engine/include/search.h"
#include "../Engine/include/move.h"
#include "../Engine/include/zobrist.h"
#include "../Engine/include/transposition.h"
#include "../Engine/include/evaluation.h"
#include "../Engine/include/polyglot.h"
#include "../Engine/include/tbprobe.h"

#define MAX_PLY 64

// Updated globals to match UCI
extern std::atomic<unsigned long long> nodesSearched;
extern thread_local int killerMoves[MAX_PLY][2];
extern thread_local int historyTable[2][64][64];
extern thread_local unsigned int threadRNG_state;
extern thread_local bool isHelperThread;

extern std::atomic<bool> stopSearch;
extern long long searchTimeLimit;
extern std::chrono::time_point<std::chrono::steady_clock> searchStartTime;
int numThreads = 4;

// thread pool globals
std::vector<std::thread> threadPool;
std::mutex poolMutex;
std::condition_variable wakeCondition;
std::condition_variable sleepCondition;

bool engineShutdown = false;
bool searchActive = false;
int activeHelpers = 0;
Board globalSearchBoard;
int currentTargetDepth = MAX_PLY;

// helper functions copied from UCI
void persistentHelperLoop(int threadID) {
    isHelperThread = true;
    // Seed specific threads RNG
    threadRNG_state += threadID * 0x9E3779B9;
    while (true) {
        // ensures thread-safe access
        std::unique_lock<std::mutex> lock(poolMutex);
        // Sleep without using CPU until main thread calls helper thread
        wakeCondition.wait(lock, [] { return searchActive || engineShutdown;});

        if (engineShutdown) break;

        Board localBoard = globalSearchBoard;
        int targetDepth = currentTargetDepth;
        lock.unlock(); // Let other threads wake up

        // clear killer moves upon every turn
        std::memset(killerMoves, 0, sizeof(killerMoves));

        // Offset depths so threads dont search the same horizon
        int startDepth = 1 + (threadID % 4);

        // Search
        for (int d = startDepth; d <= targetDepth; d++) {
            searchHelper(localBoard, d);
            if (stopSearch) break;
        }

        // put threads to sleep
        lock.lock();
        activeHelpers--;
        // if thisi the last helper to finish, notify main thread
        if (activeHelpers == 0) sleepCondition.notify_one();

        // keep thread waiting until main thread officially ends turn
        wakeCondition.wait(lock, [] { return !searchActive || engineShutdown; });
    }
}

void resizeThreadPool(int newSize) {
    if (newSize == threadPool.size() + 1) return; // +1 accounts for main thread

    // Shutdown existing pool
    if (!threadPool.empty()) {
        engineShutdown = true;
        wakeCondition.notify_all();
        for (auto& t : threadPool) t.join();
        threadPool.clear();
        engineShutdown = false;
    }

    // create new thread pool
    for (int i = 1; i < newSize; i++) {
        threadPool.emplace_back(persistentHelperLoop, i);
    }
}
// Helper func to translate square index to chess notation(12 -> "e2")
std::string indexToChessNotation(int sq) {
    std::string result = "";
    result += (char)('a' + (sq % 8)); // File
    result += (char)('1' + (sq / 8)); // Rank
    return result;
}
// Helper func to parse users move
bool parseUserMove(std::string input, int& start, int& target, int color, char& promotedPiece) { // User input example: "e2e3"
    promotedPiece = ' '; // default to no promotion
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

    // Promotion check
    if (input.length() >= 5) {
        promotedPiece = std::tolower(input[4]); // example input: e7e8q
    }
    return true;
}
int main() {
    std::srand(std::time(0));
    initPolyglot("C:\\Users\\saffa\\CLionProjects\\ChessEngine\\data\\Cerebellum3Merge.bin");

    bool tbActive = tb_init("C:\\Users\\saffa\\CLionProjects\\ChessEngine\\data\\syzygy");
    if (tbActive) {
        std::cout << "Syzygy tablebases found: " << TB_LARGEST << " men\n";
    } else {
        std::cout << "Failed to load Syzygy tablebases.\n";
    }

    // Calculate all attack tables and magic bitboards
    initAllMoveGen();
    initZobrist(); // init random hash keys
    initTT(1024);
    initPawnMasks(); // init pawn structure masks

    resizeThreadPool(numThreads); // Boot up Lazy SMP threads

    // init board
    Board board;

    //board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // put all pieces on their starting squares
    board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.setHashKey(generateHashKey(board)); // generate zobrist key
    std::cout << "Hash Key: " << board.getHashKey() << '\n';
    std::cout << "=====================================\n";
    std::cout << "      CHESS ENGINE TERMINAL          \n";
    std::cout << "=====================================\n";
    std::cout << "Type moves in format 'e2e4' or 'quit'. King side castle: O-O, Queen side castle: O-O-O\n";

    // clear killer moves array
    std::memset(killerMoves, 0, sizeof(killerMoves));
    // clear history table
    std::memset(historyTable, 0, sizeof(historyTable));

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
            char promotedPiece;

            if (!parseUserMove(input, start, target, board.getSideToMove(), promotedPiece)) {
                std::cout << "Invalid Move. Try again. \n";
                continue;
            }
            int userMove = 0;
            // Find the users move in the move list that contains all possible moves for Human
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
                            userMove = currentMove;
                            break; // found correct promotion
                        }
                    } else {
                        // Non-promotion move
                        userMove = currentMove;
                        break; // exit loop since we found users move
                    }
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
            nodesSearched = 0; // reset node count
            long long optimalTime = 1000; // 1 second soft limit
            long long maxTime = 3000;     // 3 seconds hard limit
            searchTimeLimit = maxTime;

            stopSearch = false;
            int bestMoveOverall = 0;
            searchStartTime = std::chrono::steady_clock::now();
            int targetDepth = MAX_PLY;

            // polgot intercept
            int bookMove = getBookMove(board);
            if (bookMove != 0) {
                bestMoveOverall = bookMove;
                std::cout << "Played Polyglot book move\n";
            } else {
                // normal lazy SMP search
                poolMutex.lock();
                globalSearchBoard = board;
                currentTargetDepth = targetDepth;
                searchActive = true;
                activeHelpers = threadPool.size();
                poolMutex.unlock(); // Safely unlock before search begins
                wakeCondition.notify_all();

                int previousScore = 0;
                // iterative deepening
                for (int currentDepth = 1; currentDepth <= targetDepth; currentDepth++) {
                    auto startTime = std::chrono::steady_clock::now();
                    int currentScore = searchRoot(board, currentDepth, startTime);

                    if (stopSearch) break;
                    bestMoveOverall = bestMoveToPlay;

                    if (currentScore > MATE_VALUE - MAX_PLY || currentScore == 19999 || currentScore == -19999) {
                        break;
                    }

                    if (currentDepth > 1 && currentScore < previousScore - 50) {
                        optimalTime += (optimalTime / 2);
                        if (optimalTime > maxTime) optimalTime = maxTime;
                    }
                    previousScore = currentScore;

                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStartTime).count();

                    if (elapsed >= optimalTime) {
                        break;
                    }
                }
                stopSearch = true;
                std::unique_lock<std::mutex> sleepLock(poolMutex);
                sleepCondition.wait(sleepLock, [] { return activeHelpers == 0; });
                searchActive = false;
                wakeCondition.notify_all();
            }
            if (bestMoveOverall == 0) {
                MoveList backupList;
                generateMoves(backupList, board);
                for (int i = 0; i < backupList.count; i++) {
                    if (makeMove(backupList.moves[i], board)) {
                        bestMoveOverall = backupList.moves[i];
                        unmakeMove(backupList.moves[i], board);
                        break;
                    }
                }
            }

            // If the computer attempts an illegal move -> exit
            if (!makeMove(bestMoveOverall, board)) {
                std::cout << "Engine Failed: Attempted illegal move.\n";
                break;
            }
            std::string computerMove = indexToChessNotation(getStart(bestMoveOverall)) +
                indexToChessNotation(getTarget(bestMoveOverall));

            // format promotion characters
            int flag = getFlag(bestMoveOverall);
            if (flag == PR_QUEEN || flag == PC_QUEEN) computerMove += 'q';
            else if (flag == PR_ROOK || flag == PC_ROOK) computerMove += 'r';
            else if (flag == PR_BISHOP || flag == PC_BISHOP) computerMove += 'b';
            else if (flag == PR_KNIGHT || flag == PC_KNIGHT) computerMove += 'n';

            std::cout << "COMPUTER PLAYS: " << computerMove << "\n";
        }
    }
    // shutdown threads
    engineShutdown = true;
    wakeCondition.notify_all();
    for (auto& t : threadPool) t.join();

    return 0;
}
