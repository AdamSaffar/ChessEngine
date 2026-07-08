//
// Created by saffa on 7/7/2026.
//

#ifndef CHESSENGINE_POLYGOT_H
#define CHESSENGINE_POLYGOT_H
#include "board.h"
#include <cstdint>
#include <string>

// extract 16 byte struct of a single Polyglot book entry
struct PolyglotEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight; //
    uint32_t learn; // unused variable
};

// Polyglot .bin files are Big-Endian meaning they store Most Significant Bit first
// CPUs on the otherhand are Little-Endian meaning they store Least Significant Bit first

// --- Convert Big-Endian to Little Endian ---
inline uint16_t swapEndian16(uint16_t val) {
// use preprocessor directive here to find compiler type
#if defined(_MSC_VER)
    return _byteswap_ushort(val); // MSVC (Windows)
#else
    return __builtin_bswap16(val); // GCC/Clang
#endif
}

inline uint32_t swapEndian32(uint32_t val) {
#if defined(_MSC_VER)
    return _byteswap_ulong(val);
#else
    return __builtin_bswap32(val);
#endif
}

inline uint64_t swapEndian64(uint64_t val) {
#if defined(_MSC_VER)
    return _byteswap_uint64(val); // MSVC (windows)
#else
    return __builtin_bswap64(val); // GCC/Clang
#endif
}


void initPolyglot(const std::string& bookPath);
int getBookMove(Board& board);
#endif //CHESSENGINE_POLYGOT_H
