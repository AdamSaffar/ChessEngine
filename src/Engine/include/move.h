//
// Created by saffa on 6/13/2026.
//

#ifndef CHESSENGINE_MOVE_H
#define CHESSENGINE_MOVE_H
/*
 * Typical struct to represent a move:
 * struct move {
 *  int startSquare;
 *  int targetSquare;
 *  bool isCapture;
 *  bool isPromotion;
 *  bool isCastle;
 * };
 * Rather than writing a bloated struct that takes up to 16 bytes of memory
 * We can compress the struct into a 16-bit unsigned integer to represent moves like so:
 *   FLAGS     TARGET        START
 * 0 0 0 0 | 0 0 0 0 0 0 | 0 0 0 0 0 0
 */
#include <cstdint>
// compress large start, target, and flag into one encoded 16-bit integer
// FIX: add "inline constexpr" to avoid multiple definitions error AND a fast optomization trick(since normal func calls can take many CPU cycles)
inline constexpr uint16_t encodeMove(unsigned int startSquare, unsigned int targetSquare, unsigned int flag) {
    // LAYOUT:
    // FLAG | TARGET |  START
    // 0-3     4-9      10-15

    return static_cast<uint16_t>(flag) | (static_cast<uint16_t>(targetSquare) << 4) | (static_cast<uin16_t>(start) << 10);
}

/* Decoding Logic */

// Extract Starting Square

#endif //CHESSENGINE_MOVE_H
