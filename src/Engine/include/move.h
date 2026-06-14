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

    return static_cast<uint16_t>(flag) | (static_cast<uint16_t>(targetSquare) << 4) | (static_cast<uint16_t>(startSquare) << 10);
}

/* Decoding Logic */

// Extract Flag (Bits 0-3)
inline constexpr unsigned int getFlag(uint16_t move) {
    return move & 0x000F; // mask: 0000 0000 0000 1111
}
// Extract Target Square(Bits 4-9)
inline constexpr unsigned int getTarget(uint16_t move) {
    return (move >> 4) & 0x003F; // shift right 4, then mask: 0000 0000 0011 1111
}
// Extract Start Square(Bits 10-15)
inline constexpr unsigned int getStart(uint16_t move) {
    return (move >> 10) & 0x003F; // shift right 10, then mask: 0000 0000 0011 1111
}

struct MoveList {
    // max number of legal moves in any chess position is 218
    uint16_t moves[256];
    int count = 0; 
    // helper function to push move into moves array
    inline void addMove(uint16_t move) {
        moves[count] = move;
        count++;
    }
};
#endif //CHESSENGINE_MOVE_H
