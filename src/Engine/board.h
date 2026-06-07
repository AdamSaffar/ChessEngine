//
// Created by saffa on 6/7/2026.
//

#ifndef CHESSENGINE_BOARD_H
#define CHESSENGINE_BOARD_H
#endif //CHESSENGINE_BOARD_H

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

class Board {
    // 12 distinct bitboards(6 for white, 6 for black)
    pieceBitBoards[12];
};