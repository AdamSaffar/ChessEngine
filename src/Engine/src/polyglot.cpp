//
// Created by saffa on 7/7/2026.
//
#include "include/polyglot.h"
#include "include/moveGeneration.h"
#include <fstream>
#include <iostream>
#include <vector>

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

    // find total number of entries in the file
    polyglotBook.seekg(0,std::ios::end);
    long long numEntries = polyglotBook.tellg() / sizeof(PolyglotEntry);

    long long low = 0;
    long long high = numEntries - 1;
    long long firstMatch = -1;

    // standard binary search
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
        return entries;
    }
}