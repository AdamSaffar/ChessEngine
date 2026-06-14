//
// Created by saffa on 6/7/2026.
//
#include "include/board.h"
#include <iostream>

Board::Board() {
    initStandardPosition();
}
// Clear board, set individual bitboards, set occupancies, and game state
void Board::initStandardPosition() {
    // Clear the board and occupancies
    for (int i = 0; i < 12; i++) pieceBitBoards[i] = 0ULL; //  0ULL is numeric literal for zero
    for (int i = 0; i < 3; i++) occupancies[i] = 0ULL;

    /* Set initial piece placements */

    /* ---WHITE PIECES(Indicies 0-5)--- */
    // PAWNS(All on 2nd rank), equivilantly = 00000000 00000000 00000000 00000000 00000000 00000000 11111111 00000000
    pieceBitBoards[PIECE_TYPE::Pawn] = 0x000000000000FF00ULL;

    // ROOKS(2 on each end of the first rank): 00000000 00000000 00000000 00000000 00000000 00000000 00000000 10000001
    pieceBitBoards[PIECE_TYPE::Rook] = 0x0000000000000081ULL;

    // KNIGHTS(On index 1 & 6): 00000000 00000000 00000000 00000000 00000000 00000000 00000000 01000010
    pieceBitBoards[PIECE_TYPE::Knight] = 0x0000000000000042ULL;

    // BISHOPS(On index 2 & 5): 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00100100
    pieceBitBoards[PIECE_TYPE::Bishop] = 0x0000000000000024ULL;

    // Queen(On index 3): 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00001000
    pieceBitBoards[PIECE_TYPE::Queen] = 0x0000000000000008ULL;

    // King(On index 4): 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00010000
    pieceBitBoards[PIECE_TYPE::King] = 0x0000000000000010ULL;

    /* ---Black PIECES(Indicies 6-11) --- */
    // Black Pieces are simply a +6 offset of white pieces

    // PAWNS(All on 7th rank): 00000000 11111111 00000000 00000000 00000000 00000000 00000000 00000000
    pieceBitBoards[PIECE_TYPE::Pawn + 6] = 0x00FF000000000000ULL;

    // ROOKS(2 on each end of the last rank): 10000001 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    pieceBitBoards[PIECE_TYPE::Rook + 6] = 0x8100000000000000ULL;

    // KNIGHTS(On index 57 & 62): 01000010 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    pieceBitBoards[PIECE_TYPE::Knight + 6] = 0x4200000000000000ULL;

    // BISHOPS(On index 58 & 61): 00100100 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    pieceBitBoards[PIECE_TYPE::Bishop + 6] = 0x2400000000000000ULL;

    // Queen(On index 59): 00001000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    pieceBitBoards[PIECE_TYPE::Queen + 6] = 0x0800000000000000ULL;

    // King(On index 60): 00010000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
    pieceBitBoards[PIECE_TYPE::King + 6] = 0x1000000000000000ULL;

    /* Occupancy Maps (USE BITWISE OR(|))*/
    occupancies[COLOR::WHITE] = pieceBitBoards[PIECE_TYPE::Pawn] | pieceBitBoards[PIECE_TYPE::Rook]
        | pieceBitBoards[PIECE_TYPE::Knight] | pieceBitBoards[PIECE_TYPE::Bishop]
        | pieceBitBoards[PIECE_TYPE::Queen] | pieceBitBoards[PIECE_TYPE::King];

    occupancies[COLOR::BLACK] = pieceBitBoards[PIECE_TYPE::Pawn + 6] | pieceBitBoards[PIECE_TYPE::Rook + 6]
    | pieceBitBoards[PIECE_TYPE::Knight + 6] | pieceBitBoards[PIECE_TYPE::Bishop + 6]
    | pieceBitBoards[PIECE_TYPE::Queen + 6] | pieceBitBoards[PIECE_TYPE::King + 6];

    occupancies[COLOR::BOTH] = occupancies[COLOR::WHITE] | occupancies[COLOR::BLACK];

    /* Game State */
    sideToMove = COLOR::WHITE; // White always starts first
    enPassantSquare = -1; // no enPassant avail
    castlingRights = 15;  // 1111 in binary (all 4 castling rights avail)
}

// Terminal prints from TOP TO BOTTOM and LEFT TO RIGHT
void Board::printBoard() {
    std::cout << "\n  a b c d e f g h\n"; // print file type at the TOP

    // Loop through ranks, 8 down to 1
    for (int row = 7; row >= 0; row--) {
        std::cout << row + 1 << ' '; // print row num on the RIGHT
        // Loop through A-H files
        for (int col = 0; col < 8; col++) {
            int square = row * 8 + col;
            char pieceChar = '.'; // default empty square
            /*
             * Mask a temporary 64-bit integer where exactly ONE bit is on,
             * and the other 63 bits are zeros.
             * Shifts the 1's bit to exactly the square we're looking at
             */
            U64 squareMask = 1ULL << square;
            // check if the combined board has a piece overlapping our mask
            if (occupancies[COLOR::BOTH] & squareMask) {
                // if there is an overlapping piece, check which type of piece it is
                if (pieceBitBoards[PIECE_TYPE::Pawn] & squareMask) pieceChar = 'P';
                else if (pieceBitBoards[PIECE_TYPE::Knight] & squareMask) pieceChar = 'N';
                else if (pieceBitBoards[PIECE_TYPE::Bishop] & squareMask) pieceChar = 'B';
                else if (pieceBitBoards[PIECE_TYPE::Rook] & squareMask) pieceChar = 'R';
                else if (pieceBitBoards[PIECE_TYPE::Queen] & squareMask) pieceChar = 'Q';
                else if (pieceBitBoards[PIECE_TYPE::King] & squareMask) pieceChar = 'K';
                else if (pieceBitBoards[PIECE_TYPE::Pawn + 6] & squareMask) pieceChar = 'p';
                else if (pieceBitBoards[PIECE_TYPE::Knight + 6] & squareMask) pieceChar = 'n';
                else if (pieceBitBoards[PIECE_TYPE::Bishop + 6] & squareMask) pieceChar = 'b';
                else if (pieceBitBoards[PIECE_TYPE::Rook + 6] & squareMask) pieceChar = 'r';
                else if (pieceBitBoards[PIECE_TYPE::Queen + 6] & squareMask) pieceChar = 'q';
                else if (pieceBitBoards[PIECE_TYPE::King + 6] & squareMask) pieceChar = 'k';
            }
            std::cout << pieceChar << " ";
        }
        std::cout << row + 1 << "\n"; // print row num on the RIGHT
    }
    std::cout << "  a b c d e f g h\n\n"; // print file type at the BOTTOM
}
