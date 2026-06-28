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

void drawPieces(sf::RenderWindow& window, sf::Texture& pieceTexture, Board& board, float squareSize) {
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

}
int main() {
    // Initialize Engine
    initAllMoveGen();
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

    // run the program as long as the window is open
    while (window.isOpen()) {
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
                        int mouseX = event.mouseButton.x;
                        int mouseY = event.mouseButton.y;
                    }
                    break;
            }
        }

        window.clear(sf::Color::Black); // clear window with black color

        /* Draw to window */
        window.setView(boardView); // Apply the view
        drawBoard(window, squareSize); // Draw board
        drawPieces(window, spriteSheet, board, squareSize); // Draw pieces

        window.display();
    }

    return 0;
}
