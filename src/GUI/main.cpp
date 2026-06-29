//
// Created by saffa on 6/27/2026.
//
#include <iostream>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include "../Engine/include/board.h"
#include "../Engine/include/moveGeneration.h"
#include "../Engine/include/search.h"
#include "../Engine/include/move.h"
#include "../Engine/include/zobrist.h"

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
    // Initialize Engine
    initAllMoveGen();
    initZobrist(); // init random hash keys

    Board board;
    board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // init starting chess position

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
    // run the program as long as the window is open
    while (window.isOpen()) {
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

            // if legalMoves = 0 and the king is attacked -> CHECKMATE
            if (board.isSquareAttacked(kingSquare, board.getSideToMove() ^ 1)) {
                std::cout << "CHECKMATE! Game Over. \n";
            // If legalMoves = 0 and the king is not attacked -> STALEMATE
            } else {
                std::cout << "STALEMATE! Game Over. \n";
            }
            break;
        }
        // --- ENGINES TURN ---
        if (board.getSideToMove() == COLOR::BLACK) {
            searchRoot(board, 7); // CALL SEARCH FUNCTION
            // If the computer attempts an illegal move -> exit
            if (!makeMove(bestMoveToPlay, board)) {
                std::cout << "Engine Failed: Attempted illegal move.\n";
                break;
            }
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
                    if (event.mouseButton.button == sf::Mouse::Left) {
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

    return 0;
}
