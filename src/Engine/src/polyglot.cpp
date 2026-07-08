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