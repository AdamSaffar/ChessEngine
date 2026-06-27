//
// Created by saffa on 6/27/2026.
//
#include <iostream>
#include <algorithm>
#include <SFML/Graphics.hpp>


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
int main() {
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
                case sf::Event::Resized:
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
        }

        window.clear(sf::Color::Black); // clear window with black color

        /* Draw to window */
        window.setView(boardView); // Apply the view
        drawBoard(window, squareSize); // Draw board

        window.display();
    }

    return 0;
}
