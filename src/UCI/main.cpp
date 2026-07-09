//
// Created by saffa on 6/28/2026.
//
#include <iostream>
#include <condition_variable>
#include <algorithm>
#include <mutex>
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
#include "../Engine/include/evaluation.h"
#include "../Engine/include/polyglot.h"
#include "../Engine/include/tbprobe.h"

#define MAX_PLY 64
// std::atomic allow multiple threads to access shared data
extern std::atomic<unsigned long long> nodesSearched;
// thread local means that every thread gets its own independent copy of the var
extern thread_local int killerMoves[MAX_PLY][2];
extern thread_local int historyTable[2][64][64];
extern thread_local unsigned int threadRNG_state;
extern thread_local bool isHelperThread;

extern std::atomic<bool> stopSearch;
extern long long searchTimeLimit; // time allowed in milliseconds
extern std::chrono::time_point<std::chrono::steady_clock> searchStartTime;
int numThreads = 1; // default to single-thread

// --- THREAD POOL GLOBALS ---
std::vector<std::thread> threadPool;
std::mutex poolMutex;
// condition_variable is used to block one or more threads until another thread modifies a shared variable
std::condition_variable wakeCondition;
std::condition_variable sleepCondition;

bool engineShutdown = false;
bool searchActive = false;
int activeHelpers = 0;
Board globalSearchBoard;
int currentTargetDepth = MAX_PLY;

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

    // support threading, default 1, max set to 256 threads (My CPU is i7-12700, which has 20 threads)
    std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
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
/*
void helperThreadLoop(Board board, int targetDepth) {
    // helper runs its own iterative deepening
    for (int d = 1; d <= targetDepth; d++) {
        searchHelper(board, d);

        if (stopSearch) break;
    }
}
*/
int main() {
    std::srand(std::time(0)); // Seed random for Polyglot
    initPolyglot("C:\\Users\\saffa\\CLionProjects\\ChessEngine\\data\\Cerebellum3Merge.bin"); // initialize polyglot(opening book file)

    // Initialize Syzygy Tablebases
    bool tbActive = tb_init("C:\\Users\\saffa\\CLionProjects\\ChessEngine\\data\\syzygy");
    if (tbActive) {
        std::cout << "info string Syzygy tablebases found: " << TB_LARGEST << " men" << std::endl;
    } else {
        std::cout << "info string Failed to load Syzygy tablebases." << std::endl;
    }

    setbuf(stdout, NULL);
    // Initialize Engine
    initAllMoveGen();
    initZobrist(); // init random hash keys

    // 512 MB ~= 32,000,000 unique board states
    initTT(1024); // increase default TT size to 1024 MB to account for multithreading
    initPawnMasks(); // init pawn structure masks

    resizeThreadPool(numThreads); // initialize pool upon boot

    Board board;
    std::string line;
    // Infinite CLI loop
    while (std::getline(std::cin, line)) {
        // Use string stream to separate string by spaces
        std::istringstream iss(line);
        std::string command;
        iss >> command; // first word of the string

        if (command == "quit") {
            // kill thread before exiting
            engineShutdown = true;
            wakeCondition.notify_all();
            for (auto& t : threadPool) t.join();
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
            int targetDepth = MAX_PLY; // default to maximum depth
            long long wtime = -1, btime = -1, winc = 0, binc = 0, moveTime = -1;
            std::string token;
            // read input string for depth
            while (iss >> token) {
                if (token == "depth") iss >> targetDepth;
                else if (token == "wtime") iss >> wtime;
                else if (token == "btime") iss >> btime;
                else if (token == "winc") iss >> winc;
                else if (token == "binc") iss >> binc;
                else if (token == "movetime") iss >> moveTime;
            }

            // calculate time we can spend
            long long optimalTime = -1;
            long long maxTime = -1;

            if (moveTime != -1) {
                optimalTime = moveTime;
                maxTime = moveTime; // GUI request exact move time
            } else if (wtime != -1 && btime != -1) {
                int color = board.getSideToMove();
                long long myTime = (color == COLOR::WHITE) ? wtime : btime;
                long long myInc = (color == COLOR::WHITE) ? winc : binc;

                // Soft target: Spend 1/40th of time remaining + 3/4th of increment
                optimalTime = (myTime / 40) + (myInc * 3 / 4);

                // Hard limit: never spend more than 20% of remaining time on 1 move
                maxTime = myTime / 5;

                // safety buffer to prevent flagging
                if (optimalTime > myTime - 50) optimalTime = std::max(10LL, myTime - 50);
                if (maxTime > myTime - 50) maxTime = std::max(10LL, myTime - 50);
            }
            // reset state for each new search
            searchTimeLimit = maxTime;
            stopSearch = false;
            int bestMoveOverall = 0;
            nodesSearched = 0; // reset node count
            searchStartTime = std::chrono::steady_clock::now();

            // --- POLYGLOT INTERCEPT ---
            int bookMove = getBookMove(board);
            if (bookMove != 0) {
                // we found a book move. Save it, and skip search
                bestMoveOverall = bookMove;
                targetDepth = 0;
                std::cout << "info string Playing Polyglot book move" << std::endl;
            } else { // Normal search
                // Wake up helper threads
                std::lock_guard<std::mutex> lock(poolMutex);
                globalSearchBoard = board; // assign current board state to global board
                currentTargetDepth = targetDepth;
                searchActive = true;

                // pre-load helpers
                activeHelpers = threadPool.size();
                wakeCondition.notify_all();
            }

            int previousScore = 0; // Track score between depths

            // main thread iterative deepening
            for (int currentDepth = 1; currentDepth <= targetDepth; currentDepth++) {

                auto startTime = std::chrono::steady_clock::now();

                // get the score returned by search at this depth
                int currentScore = searchRoot(board, currentDepth, startTime); // CALL SEARCH FUNCTION

                if (stopSearch) {
                    // if time ran out mid-depth, the move in bestMoveToPlay is incomplete.
                    // break loop and rely on move from previously completed depth
                    break;
                }

                bestMoveOverall = bestMoveToPlay;

                // If engine found forced mate, stop thinking and play a move immediately to save time
                if (currentScore > MATE_VALUE - MAX_PLY) {
                    break;
                }
                // if score dropped by more than 50 points from previous depth,
                // the position is highly volatile and critical. Extend soft time
                if (currentDepth > 1 && currentScore < previousScore - 50) {
                    if (optimalTime != -1) {
                        optimalTime += (optimalTime / 2);
                        // safeguard so engine doesn't exceed hard limit
                        if (optimalTime > maxTime) optimalTime = maxTime;
                    }
                }
                previousScore = currentScore; // update prev score

                // Soft-time check
                if (optimalTime != -1) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStartTime).count();

                    // if we finished a depth but exceeded our soft-time limit, STOP
                    // starting a new depth will likely faily halfway through and waste clock time
                    if (elapsed >= optimalTime) {
                        break;
                    }
                }
            }

            stopSearch = true; // manually tell helpers to stop searching

            // Wait for all helper threads to sleep
            std::unique_lock<std::mutex> lock(poolMutex);
            sleepCondition.wait(lock, [] { return activeHelpers == 0; });
            searchActive = false;

            // release helpers so they can loop back to the top
            wakeCondition.notify_all();
            lock.unlock();

            if (bestMoveOverall == 0) {
                // add safety catch
                std::cout << "bestmove 0000" << std::endl;
            } else {
                // translate start and target squares
                std::string computerMove = indexToChessNotation(getStart(bestMoveOverall)) +
                                           indexToChessNotation(getTarget(bestMoveOverall));
                // handle promotion flags
                int flag = getFlag(bestMoveOverall);
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
            std::string optionToken, valToken, optionValue;
            iss >> skip; // skip "name"
            iss >> optionToken >> valToken; // reads "Hash" and "value"

            if (valToken == "value") {
                iss >> optionValue;
                if (optionToken == "Hash") {
                    try {
                        int targetMB = std::stoi(optionValue);
                        initTT(targetMB); // resize transposition table
                    } catch (const std::exception& e) {
                        std::cout << "Error resizing hash" << std::endl;
                    }
                } else if (optionToken == "Threads") {
                    try {
                        int newThreads = std::stoi(optionValue);
                        if (newThreads < 1) newThreads = 1; // safety catch
                        numThreads = newThreads;

                        resizeThreadPool(numThreads);
                    } catch (const std::exception& e) {
                        std::cout << "Error setting threads" << std::endl;
                    }
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