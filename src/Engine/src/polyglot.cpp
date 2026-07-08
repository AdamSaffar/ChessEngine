//
// Created by saffa on 7/7/2026.
//
#include "include/polyglot.h"
#include "include/polyglot_keys.h"
#include "include/moveGeneration.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

// maps engine piece index to Polyglot piece index (0-11)
// My Engine: White Pawn, White Knight, White Bishop, White Queen, Black Pawn, etc
// Polyglot: Black pawn, White pawn, Black Knight, White Knight, Black Bishop, White Bishop, Black Rook
// White Rook, Black Queen, White Queen, Black King, White King
const int polyglotPieceMap[12] = {1, 3, 5, 7, 9, 11, 0, 2, 4, 6, 8, 10};
// Global book file stream
std::ifstream polyglotBook;
bool hasBook = false;

// Helper to swap an entire entry into Little-Endian
PolyglotEntry swapEntry(PolyglotEntry entry) {
    PolyglotEntry swapped;
    swapped.key = swapEndian64(entry.key);
    swapped.move = swapEndian16(entry.move);
    swapped.weight = swapEndian16(entry.weight);
    swapped.learn = swapEndian32(entry.learn);
    return swapped;
}

void initPolyglot(const std::string& bookPath) {
    // std::ios::binary is IMPORTANT
    polyglotBook.open(bookPath, std::ios::binary);

    if (polyglotBook.is_open()) {
        std::cout << "info string Polyglot book loaded successfully!" << std::endl;
        hasBook = true;
    } else {
        std::cout << "info string Failed to load Polyglot book at: " << bookPath << std::endl;
        hasBook = false;
    }
}

// Helper function to find all book entries for a given Polyglot hash key
std::vector<PolyglotEntry> findAllEntries(uint64_t polyglotKey) {
    std::vector<PolyglotEntry> entries;

    // safety check
    if (!hasBook || !polyglotBook.is_open()) return entries;
    // clear any error flags from previous search
    polyglotBook.clear();

    // find total number of entries in the file
    polyglotBook.seekg(0,std::ios::end);
    long long numEntries = polyglotBook.tellg() / sizeof(PolyglotEntry);

    long long low = 0;
    long long high = numEntries - 1;
    long long firstMatch = -1;

    // standard binary search since book is sorted
    while (low <= high) {
        long long mid = low + (high - low) / 2;
        PolyglotEntry entry;
        // jump to the middle entry and read 16 bytes
        polyglotBook.seekg(mid * sizeof(PolyglotEntry), std::ios::beg);
        polyglotBook.read(reinterpret_cast<char*>(&entry), sizeof(PolyglotEntry));

        // swap from big-endian to little-endian
        entry = swapEntry(entry);

        if (entry.key < polyglotKey) {
            low = mid + 1;
        } else if (entry.key > polyglotKey) {
            high = mid - 1;
        } else {
            // match found
            firstMatch = mid;
            break;
        }
    }

    // if no match was found return an empty list
    if (firstMatch == -1) return entries;

    // Rewind to the first matching move
    while (firstMatch > 0) {
        PolyglotEntry entry;
        polyglotBook.seekg((firstMatch - 1) * sizeof(PolyglotEntry), std::ios::beg);
        polyglotBook.read(reinterpret_cast<char*>(&entry), sizeof(PolyglotEntry));
        entry = swapEntry(entry);

        if (entry.key == polyglotKey) {
            firstMatch--;
        } else {
            break; // key changed, found the beginning of the block
        }
    }

    // Scan forward and collect all moves tried to this board state
    polyglotBook.seekg(firstMatch * sizeof(PolyglotEntry),std::ios::beg);
    while (true) {
        PolyglotEntry entry;
        if (!polyglotBook.read(reinterpret_cast<char*>(&entry), sizeof(PolyglotEntry))) break; // EOF

        entry = swapEntry(entry);

        if (entry.key == polyglotKey) {
            entries.push_back(entry);
        } else {
            break; // end of block reached
        }
    }
    return entries;
}

// build polyglot hash key
uint64_t computePolyglotHash(const Board& board) {
    uint64_t hash = 0ULL;

    // hash individual pieces
    for (int pieceType = 0; pieceType < 12; pieceType++) {
        U64 pieceBitBoard = board.getPieceBitBoard(pieceType);

        while (pieceBitBoard != 0ULL) {
            int square = __builtin_ctzll(pieceBitBoard);
            int polyglotPiece = polyglotPieceMap[pieceType];

            // XOR the piece/square
            hash ^= PolyglotRandom[64 * polyglotPiece + square];

            pieceBitBoard &= (pieceBitBoard - 1);
        }
    }

    // Hash Castling Rights
    int castling = board.getCastlingRights();
    // Engine castlingRights format: White King, White Queen, Black King, Black  Queen
    if (castling & 1) hash ^= PolyglotRandom[768];
    if (castling & 2) hash ^= PolyglotRandom[769];
    if (castling & 4) hash ^= PolyglotRandom[770];
    if (castling & 8) hash ^= PolyglotRandom[771];

    // Hash EnPassant
    int epSquare = board.getEnpassantSquare();
    if (epSquare != -1 && epSquare != 0) {
        int epFile = epSquare % 8;
        hash ^= PolyglotRandom[772 + epFile];
    }

    // Hash Side to Move
    if (board.getSideToMove() == COLOR::WHITE) {
        hash ^= PolyglotRandom[780];
    }

    return hash;
}

// Translate polyglot data and picks a weighted random move
int getBookMove(Board& board) {
    uint64_t polyKey = computePolyglotHash(board);
    // debugg line
    std::cout << "info string Polyglot Hash generated: " << std::hex << polyKey << std::dec << std::endl;

    std::vector<PolyglotEntry> entries = findAllEntries(polyKey);

    // Current Board Position was not found in book
    if (entries.empty()) return 0;

    // Calculate total weight
    int totalWeight = 0;
    for (const auto& entry : entries) {
        totalWeight += entry.weight;
    }

    uint16_t chosenPolyglotMove = 0;

    // Weighted random selection
    if (totalWeight > 0) {
        int randomVal = rand() % totalWeight;
        int runningSum = 0;
        for (const auto& entry : entries) {
            runningSum += entry.weight;
            if (runningSum > randomVal) {
                chosenPolyglotMove = entry.move;
                break;
            }
        }
    } else {
        // fallback if all book moves have a weight of 0
        int randomIndex = rand() % entries.size();
        chosenPolyglotMove = entries[randomIndex].move;
    }


    // DECODING POLYGLOT MOVE

    // Polyglot format: bits 0-2(promo), 3-5 (to file), 6-8 (to rank), 9-11 (from file), 12-14 (from rank)
    // FIX: shift bits before applying mask
    int engineTo = chosenPolyglotMove & 0x3F;
    int engineFrom = (chosenPolyglotMove >> 6) & 0x3F;
    int polyPromo = (chosenPolyglotMove >> 12) & 0x07;

    // decode polyglot castling
    if (polyPromo == 0) {
        // White Kingside (e1 to h1 -> e1 to g1)
        if (engineFrom == 4 && engineTo == 7) engineTo = 6;
        // White Queenside (e1 to a1 -> e1 to c1)
        else if (engineFrom == 4 && engineTo == 0) engineTo = 2;
        // Black Kingside (e8 to h8 -> e8 to g8)
        else if (engineFrom == 60 && engineTo == 63) engineTo = 62;
        // Black Queenside (e8 to a8 -> e8 to c8)
        else if (engineFrom == 60 && engineTo == 56) engineTo = 58;
    }

    // generate moves to grab the engines flag
    MoveList moveList;
    generateMoves(moveList, board);

    for (int i = 0; i < moveList.count; i++) {
        uint16_t engineMove = moveList.moves[i];

        if (getStart(engineMove) == engineFrom && getTarget(engineMove) == engineTo) {

            int flag = getFlag(engineMove);
            bool isPromo = (flag >= PR_KNIGHT && flag <= PC_QUEEN);

            // Handle Promotions
            if (polyPromo != 0) {
                if (!isPromo) continue;
                bool wantsKnight = (polyPromo == 1 && (flag == PR_KNIGHT || flag == PC_KNIGHT));
                bool wantsBishop = (polyPromo == 2 && (flag == PR_BISHOP || flag == PC_BISHOP));
                bool wantsRook   = (polyPromo == 3 && (flag == PR_ROOK   || flag == PC_ROOK));
                bool wantsQueen  = (polyPromo == 4 && (flag == PR_QUEEN  || flag == PC_QUEEN));

                if (wantsKnight || wantsBishop || wantsRook || wantsQueen) return engineMove;
            }
            // Handle Normal Moves
            else if (!isPromo) {
                return engineMove;
            }
        }
    }
    return 0; // Failsafe
}