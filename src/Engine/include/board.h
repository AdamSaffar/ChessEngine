//
// Created by saffa on 6/7/2026.
//

#ifndef CHESSENGINE_BOARD_H
#define CHESSENGINE_BOARD_H

#include <cstdint>
#include <string>
#include <sstream>
#include "moveGeneration.h"
using U64 = uint64_t;

/* Enums to make indexing readable */
enum COLOR {
    WHITE,
    BLACK,
    BOTH
};
enum PIECE_TYPE {
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};
// Used to store current game-state for unmakeMove() func
struct GameState {
    int castlingRights;
    int enPassantSquare;
    int halfMoveClock;
    int capturedPieceType; // Store type of piece captured
    U64 hashKey;
};
class Board {
private:
    // 12 distinct bitboards(6 for white, 6 for black)
    U64 pieceBitBoards[12];

    // Summary bitboards for fast occupancy checks(indexed by color)
    U64 occupancies[3];

    // Game state variables
    int sideToMove;
    int enPassantSquare;
    /** 4 bit integer to represent castlingRights (1 = true, 0 = false):
     * Bit 0: White KingSide
     * Bit 1: White QueenSide
     * Bit 2: Black KingSide
     * Bit 3: Black QueenSide
     */
    int castlingRights;
    // ADD TWO NEW INSTANCE VARIABLES
    int halfMoveClock = 0;
    int fullMoveNumber = 1;

    // History stack
    GameState history[512]; // Longest chess game was 538 half-moves 
    // pointer to track current depth in the history stack
    int historyPly = 0;

    U64 hashKey = 0; // unique key for the current board state
public:
    // Constructor
    Board();

    // --- Fast inline getters to fetch game state ---
    inline int getSideToMove() const { return sideToMove;}
    inline int getEnpassantSquare() const { return enPassantSquare;}
    inline int getCastlingRights() const { return castlingRights;}
    inline U64 getOccupancies(int color) const { return occupancies[color];} // Get colors total occupancy
    inline U64 getPieceBitBoard(int pieceType) const { return pieceBitBoards[pieceType];}
    inline U64 getHashKey() const { return hashKey; }
    inline void setHashKey(U64 hashKey) {this->hashKey = hashKey;}
    inline int getPieceAt(unsigned int startSquare) const {
        U64 squareMask = 1ULL << startSquare;
        // loop through all 12 bitoboards
        for (int i = 0; i < 12; i++) {
            // if a bitboard overalps with our square mask, piece found
            if (pieceBitBoards[i] & squareMask) {
                return i; // return index (0 to 11) of the piece
            }
        }
        // square is empty
        return -1;
    }
    inline bool isSquareAttacked(int square, int enemyColor) const {
        // use offset to grab correct enemy bitboard
        int offset = (enemyColor == COLOR::BLACK) ? 6 : 0;
        // use defenders colors to look up the pawn attack mask
        int defenderColor = enemyColor ^ 1;
        // --- PAWNS ---
        if (pawnAttacks[defenderColor][square] & pieceBitBoards[PIECE_TYPE:: Pawn + offset]) return true;
        // --- KNIGHTS ---
        if (knightAttacks[square] & pieceBitBoards[PIECE_TYPE::Knight + offset]) return true;

        // --- KINGS ---
        if (kingAttacks[square] & pieceBitBoards[PIECE_TYPE::King + offset]) return true;

        // --- BISHOPS & QUEENS ---
        U64 diagonalAttackers = pieceBitBoards[PIECE_TYPE::Bishop + offset] | pieceBitBoards[PIECE_TYPE::Queen + offset];
        if (getBishopAttacks(square, occupancies[2]) & diagonalAttackers) return true;

        // --- ROOKS & QUEENS ---
        U64 straightAttackers = pieceBitBoards[PIECE_TYPE::Rook + offset] | pieceBitBoards[PIECE_TYPE::Queen + offset];
        if (getRookAttacks(square, occupancies[2]) & straightAttackers) return true;

        // If it passed all checks, the square is completely safe
        return false;
    }
    friend bool makeMove(uint16_t move, Board& board); // Use "friend" to give function access to private members
    friend void unmakeMove(uint16_t move, Board& board);
    void initStandardPosition();
    void printBoard();
    // Translate FEN strings into internal bitboard representations
    // FEN Strings appear in this form: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"(standard start setup)
    void parseFEN(std::string fen) {
        // Clear all bitboards
        for (U64& board: pieceBitBoards) board = 0ULL;
        // Clear occupancies
        for (U64& occupancy: occupancies) occupancy = 0ULL;
        // Reset trackers so they dont overflow across multiple games
        historyPly = 0;
        halfMoveClock = 0;
        fullMoveNumber = 1;
        enPassantSquare = -1;
        castlingRights = 0;
        // Use string stream to separate fen string by spaces
        std::istringstream ss(fen);
        std::string boardSection, turnSection, castlingSection, enPassantSection, halfMoveSection, fullMoveSection;

        // Load all 4 chunks of string into their own variables( '>>' skips white space)
        ss >> boardSection >> turnSection >> castlingSection >> enPassantSection >> halfMoveSection >> fullMoveSection;

        // --- PARSE BOARD LAYOUT SECTION ---
        int rank = 7; // Start at Rank 8
        int file = 0; // Start at File A

        for (char c : boardSection) {
            if (c == ' ') break; // finished the board layout section
            if (c == '/') {
                rank--; // move down a rank
                file = 0; // reset to File A
            } else if (isdigit(c)) {
                // Its a number(empty squares)
                file += (c - '0'); // convert char to int
            } else {
                // Otherwise, its a piece.
                int color = isupper(c) ? COLOR::WHITE : COLOR::BLACK; // Uppercase = White, LowerCase = Black
                int pieceType = -1;
                // Find the piece type
                switch (tolower(c)) {
                    case 'p':
                        pieceType = PIECE_TYPE::Pawn;
                        break;
                    case 'n':
                        pieceType = PIECE_TYPE::Knight;
                        break;
                    case 'b':
                        pieceType = PIECE_TYPE::Bishop;
                        break;
                    case 'r':
                        pieceType = PIECE_TYPE::Rook;
                        break;
                    case 'q':
                        pieceType = PIECE_TYPE::Queen;
                        break;
                    case 'k':
                        pieceType = PIECE_TYPE::King;
                        break;
                }
                // Calculate bitboard index
                // White piece(+0 offset), Black Piece(+6 offset)
                int bitBoardIndex = pieceType + (color == COLOR::BLACK ? 6 : 0);

                // Find 0-63 square index
                int square = rank * 8 + file;

                // Flip the bit to 1 on the correct bitboard and square
                pieceBitBoards[bitBoardIndex] |= 1ULL << square;

                // Update occupancy boards
                occupancies[color] |= 1ULL << square;
                occupancies[COLOR::BOTH] |= 1ULL << square;

                file++; // Move to next square
            }
        }
        // --- PARSE TURN SECTION ---
        sideToMove = (turnSection == "w") ? COLOR::WHITE : COLOR::BLACK;

        // --- PARSE CASTLING SECTION ---
        castlingRights = 0; // Base case if neither side can castle
        if (castlingSection != "-") {
            for (char c : castlingSection) {
                switch (c) {
                    // White can castle king side
                    case 'K':
                        castlingRights |= 0b0001;
                        break;
                        // White can castle queen side
                    case 'Q':
                        castlingRights |= 0b0010;
                        break;
                        // Black can castle king side
                    case 'k':
                        castlingRights |= 0b0100;
                        break;
                        // Black can castle queen side
                    case 'q':
                        castlingRights |= 0b1000;
                        break;
                }
            }
        }
        // --- PARSE ENPASSANT SECTION ---
        enPassantSquare = -1; // Base Case: No enpassant avail
        if (enPassantSection != "-") {
            // En passant strings are always two chars: "e3"

            // Subtract 'a' to get file index(0-7)
            int fileIndex = enPassantSection[0] - 'a';
            // subtract rank(1-8) by 1 to convert to int (0-7)
            int rankIndex = enPassantSection[1] - '1';

            enPassantSquare = rankIndex * 8 + fileIndex;
        }
        // --- PARSE CLOCK SECTIONS ---
        halfMoveClock = std::stoi(halfMoveSection); // use std::stoi to convert string to int
        fullMoveNumber = std::stoi(fullMoveSection);
    }
};
#endif //CHESSENGINE_BOARD_H