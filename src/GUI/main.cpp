//
// Created by saffa on 6/27/2026.
//
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
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
// Updated Globals to match UCI
extern std::atomic<unsigned long long> nodesSearched;
extern thread_local int killerMoves[MAX_PLY][2];
extern thread_local int historyTable[2][64][64];
extern thread_local unsigned int threadRNG_state;
extern thread_local bool isHelperThread;

extern std::atomic<bool> stopSearch;
extern long long searchTimeLimit;
extern std::chrono::time_point<std::chrono::steady_clock> searchStartTime;
int numThreads = 4; // defaulting GUI to 4 threads for Lazy SMP

// Thread pool global vars
std::vector<std::thread> threadPool;
std::mutex poolMutex;
std::condition_variable wakeCondition;
std::condition_variable sleepCondition;

bool engineShutdown = false;
bool searchActive = false;
int activeHelpers = 0;
Board globalSearchBoard;
int currentTargetDepth = MAX_PLY;

// GUI asynchronous trackers
std::atomic<bool> engineIsThinking = false;
std::atomic<int> engineMoveToPlay = 0;

// Copied helper functions from UCI\main.cpp
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

// Copied helper from UCI\main.cpp
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

void drawBoard(sf::RenderWindow& window, float squareSize) {
    // Chess.com theme colors
    sf::Color lightColor(238, 238, 210);
    sf::Color darkColor(118, 150, 86);

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            sf::RectangleShape square(sf::Vector2f(squareSize, squareSize));
            square.setPosition(col * squareSize, row * squareSize);

            // alternate between light and dark colors
            if ((row + col) % 2 == 0) {
                square.setFillColor(lightColor);
            } else {
                square.setFillColor(darkColor);
            }

            window.draw(square);
        }
    }
}

void drawPieces(sf::RenderWindow& window, sf::Texture& pieceTexture, Board& board, float squareSize,
                bool isDragging, int startSquare, int draggedPiece) {

    float sourceSpriteSize = 140.0f; // Sprite sheet has pieces that are exactly 140x140 pixels

    // Calculate how much to shrink or grow the sprite to match the current board size
    float scaleFactor = squareSize / sourceSpriteSize;

    sf::Sprite pieceSprite(pieceTexture);
    pieceSprite.setScale(scaleFactor, scaleFactor);

    // loop through all 64-squares
    for (int sq = 0; sq < 64; sq++) {
        int currentPiece = board.getPieceAt(sq);

        // if currentPiece = -1, the square is empty
        if (currentPiece == -1) {
            continue;
        }
        // If we are dragging this specific piece, DO NOT DRAW it on the board
        if (isDragging && sq == startSquare) {
            continue;
        }
        int col = sq % 8;
        int row = 7 - (sq / 8); // invert row


        // calulate sprite sheet coordinates
        int pieceColor = currentPiece / 6; // 0 for white, 1 for black
        int pieceType = currentPiece % 6; // 0-5: P, N, B, R, Q, K

        // grab exact piece from the sprite sheet
        int spriteX = pieceType * sourceSpriteSize;
        int spriteY = pieceColor * sourceSpriteSize;

        // crop the exact piece from the texture
        pieceSprite.setTextureRect(sf::IntRect(spriteX, spriteY, sourceSpriteSize, sourceSpriteSize));
        // position
        pieceSprite.setPosition(col * squareSize, row * squareSize);
        window.draw(pieceSprite);
    }

    // DRAWING DRAGGED PIECE ON TOP
    if (isDragging && draggedPiece != -1) {
        // grab live mouse position
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mouseWorldPos = window.mapPixelToCoords(mousePixelPos);

        // find correct sprite sheet
        int pieceColor = draggedPiece / 6;
        int pieceType = draggedPiece % 6;
        int spriteX = pieceType * sourceSpriteSize;
        int spriteY = pieceColor * sourceSpriteSize;

        pieceSprite.setTextureRect(sf::IntRect(spriteX, spriteY, sourceSpriteSize, sourceSpriteSize));
        // offset the sprite by half the square size so the mouse holds the piece!
        pieceSprite.setPosition(mouseWorldPos.x - (squareSize / 2.0f), mouseWorldPos.y - (squareSize / 2.0f));
        window.draw(pieceSprite);
    }

}
int main() {
    std::srand(std::time(0)); // Seed random for Polyglot
    initPolyglot("C:\\Users\\saffa\\CLionProjects\\ChessEngine\\data\\Cerebellum3Merge.bin");

    bool tbActive = tb_init("C:\\Users\\saffa\\CLionProjects\\ChessEngine\\data\\syzygy");
    if (tbActive) {
        std::cout << "Syzygy tablebases found: " << TB_LARGEST << " men\n";
    } else {
        std::cout << "Failed to load Syzygy tablebases.\n";
    }

    // Initialize Engine
    initAllMoveGen();
    initZobrist(); // init random hash keys
    initTT(1024); // upgraded to 1024 MB for lazy SMP
    initPawnMasks(); // init pawn structure masks

    resizeThreadPool(numThreads); // boot up lazy SMP threads

    Board board;
    board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // init starting chess position
    board.setHashKey(generateHashKey(board)); // generate zobrist key

    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chess Game");
    window.setVerticalSyncEnabled(true);
    // Grab window dimensions
    float windowWidth = window.getSize().x;
    float windowHeight = window.getSize().y;

    // find shortest side of window
    float minDimension = std::min(windowWidth, windowHeight);
    float boardTotalSize = minDimension * 0.85f; // take 85% of shortest side

    // dynamic square size
    float squareSize = boardTotalSize / 8.0f;

    // Create a view that matches the size of the window
    sf::View boardView;
    boardView.setSize(window.getSize().x, window.getSize().y);

    // Center the board to the window
    boardView.setCenter(boardTotalSize / 2.0f, boardTotalSize / 2.0f);

    sf::Texture spriteSheet;
    if (!spriteSheet.loadFromFile("../images/piece_spritesheet.png")) {
        std::cerr << "Failed to load sprite sheet!" << std::endl;
    }
    // !IMPORTANT: Smoothes out scaled pixels(pieces can look very pixelated without)
    spriteSheet.setSmooth(true);

    /* Tracking variables */
    bool isDragging = false; // track if piece is currently picked up
    int startSquare = -1; // remember which square the piece was picked up from
    int targetSquare = -1;
    int draggedPiece = -1; // remember what piece is being dragged

    std::thread backgroundEngineThread;

    // clear killer moves array
    std::memset(killerMoves, 0, sizeof(killerMoves));
    // clear history table
    std::memset(historyTable, 0, sizeof(historyTable));


    // run the program as long as the window is open
    while (window.isOpen()) {
        MoveList moveList;
        generateMoves(moveList, board);

        // only check for game over if the engines isnt currently thinking
        if (!engineIsThinking.load()) {
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

                // if legalMoves = 0 and the king is attacked -> CHECKMATE
                if (board.isSquareAttacked(kingSquare, board.getSideToMove() ^ 1)) {
                    std::cout << "CHECKMATE! Game Over. \n";
                    // If legalMoves = 0 and the king is not attacked -> STALEMATE
                } else {
                    std::cout << "STALEMATE! Game Over. \n";
                }
            }
        }
        // --- ENGINES TURN ---
        if (board.getSideToMove() == COLOR::BLACK && !engineIsThinking.load() && engineMoveToPlay.load() == 0) {
            engineIsThinking.store(true);
            engineMoveToPlay.store(0);

            if (backgroundEngineThread.joinable()) {
                backgroundEngineThread.join();
            }

            // make local board copy so search doesn't mess with GUI
            Board searchBoard = board;

            backgroundEngineThread = std::thread([searchBoard]() mutable {
                nodesSearched = 0; // reset node count

                // Hardcode GUI time bounds (1 sec soft, 3 sec hard) to prevent freezing
                long long optimalTime = 1000;
                long long maxTime = 3000;
                searchTimeLimit = maxTime;

                stopSearch = false;
                int bestMoveOverall = 0;
                searchStartTime = std::chrono::steady_clock::now();
                int targetDepth = MAX_PLY;

                // polyglot intercept
                int bookMove = getBookMove(searchBoard);
                if (bookMove != 0) {
                    bestMoveOverall = bookMove;
                    std::cout << "Played Polyglot book move\n";
                } else {
                    // Normal Lazy SMP search
                    poolMutex.lock();
                    globalSearchBoard = searchBoard;
                    currentTargetDepth = targetDepth;
                    searchActive = true;
                    activeHelpers = threadPool.size();
                    poolMutex.unlock();
                    wakeCondition.notify_all();

                    int previousScore = 0;

                    // iterative deepening
                    for (int currentDepth = 1; currentDepth <= targetDepth; currentDepth++) {
                        auto startTime = std::chrono::steady_clock::now();

                        int currentScore = searchRoot(searchBoard, currentDepth, startTime);
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
                // Fail safe
                // if the search completely failed or timed out before evaluating
                if (bestMoveOverall == 0) {
                    MoveList backupList;
                    generateMoves(backupList, searchBoard);
                    for (int i = 0; i < backupList.count; i++) {
                        if (makeMove(backupList.moves[i], searchBoard)) {
                            bestMoveOverall = backupList.moves[i];
                            unmakeMove(backupList.moves[i], searchBoard);
                            break;
                        }
                    }
                }
                // pass move back to main thread
                engineMoveToPlay.store(bestMoveOverall);
                engineIsThinking.store(false);
            });
        }

        // process engine move on main thread
        if (engineMoveToPlay.load() != 0 && !engineIsThinking.load()) {
            if (!makeMove(engineMoveToPlay.load(), board)) {
                std::cout << "Engine Failed: Attempted illegal move.\n";
            }
            engineMoveToPlay.store(0);// reset for next turn
        }

        // Check all the window events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    window.close();
                    break;
                // Resize event
                case sf::Event::Resized: {
                    // Grab new window dimensions
                    float newWidth = event.size.width;
                    float newHeight = event.size.height;

                    // recalculate board size
                    float minDimension = std::min(newWidth, newHeight);
                    float boardTotalSize = minDimension * 0.85f;

                    // update squareSize so drawBoard uses new size
                    squareSize = boardTotalSize / 8.0f;

                    // update view size and center board to new window
                    boardView.setSize(newWidth, newHeight);
                    boardView.setCenter(boardTotalSize / 2.0f, boardTotalSize / 2.0f);
                    break;
                }
                case sf::Event::MouseButtonPressed:
                    // Only allow picking up pieces if it is the human's turn
                    if (event.mouseButton.button == sf::Mouse::Left && !engineIsThinking && board.getSideToMove() == COLOR::WHITE) {
                        // grab coords from mouse click
                        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                        sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos, boardView);

                        // check if the click falls inside the chess board
                        if (worldPos.x >= 0 && worldPos.x < boardTotalSize && worldPos.y >= 0 && worldPos.y < boardTotalSize) {
                            int clickedCol = static_cast<int>(worldPos.x / squareSize);
                            int clickedRow = static_cast<int>(worldPos.y / squareSize);
                            // Calculate the exact square the user clicked on
                            int clickedSquareIndex = (7 - clickedRow) * 8 + clickedCol;

                            draggedPiece = board.getPieceAt(clickedSquareIndex);
                            // only pick up of there is actually a piece there
                            if (draggedPiece != -1) {
                                // Set trackers
                                isDragging = true;
                                startSquare = clickedSquareIndex;
                            }
                        }
                    }
                    break;
                // track coords of released piece
                case sf::Event::MouseButtonReleased:
                    // If its humans move
                    if (event.mouseButton.button == sf::Mouse::Left && isDragging) {
                        // Drop piece so we dont infinitenly drag
                        isDragging = false;
                        if (board.getSideToMove() == COLOR::WHITE) {
                            sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
                            sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos, boardView);
                            // check if the dropped piece falls within the board
                            if (worldPos.x >= 0 && worldPos.x < boardTotalSize && worldPos.y >= 0 && worldPos.y < boardTotalSize) {
                                int droppedCol = static_cast<int>(worldPos.x / squareSize);
                                int droppedRow = static_cast<int>(worldPos.y / squareSize);
                                // Calculate the exact square the user clicked on
                                targetSquare = (7 - droppedRow) * 8 + droppedCol;

                                int userMove = 0;
                                // Find the users move in the move list that contains all possible moves for Human
                                for (int i = 0; i < moveList.count; i++) {
                                    int currentMove = moveList.moves[i];
                                    int currentFlag = getFlag(moveList.moves[i]);
                                    if (getStart(currentMove) == startSquare && getTarget(currentMove) == targetSquare) {
                                        bool isPromotion =  (currentFlag >= PR_KNIGHT && currentFlag <= PC_QUEEN);
                                        if (isPromotion) {
                                            char promotedPiece = ' ';
                                            std::cout << "Promotion  (q, r, b, n): ";
                                            std::cin >> promotedPiece;
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
                                if (userMove != 0) {
                                    if (!makeMove(userMove, board)) {
                                        std::cout << "Illegal Move!\n";
                                    }
                                } else {
                                    std::cout << "Invalid Move!\n";
                                }
                            }
                        }
                    }
                    break;
            }
        }

        window.clear(sf::Color::Black); // clear window with black color

        /* Draw to window */
        window.setView(boardView); // Apply the view
        drawBoard(window, squareSize); // Draw board
        drawPieces(window, spriteSheet, board, squareSize, isDragging, startSquare, draggedPiece); // Draw pieces

        window.display();
    }
    // shutdown threads
    stopSearch = true;
    engineShutdown = true;
    wakeCondition.notify_all();
    for (auto& t : threadPool) t.join();

    if (backgroundEngineThread.joinable()) {
        backgroundEngineThread.join();
    }
    return 0;
}
