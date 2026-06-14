/** Goal: Convert a 64-bit board into the desired index between 0 - 2^(num of bits in mask)
 * Find 128 unique magic numbers(64 for Rooks, 64 for bishops)
 */

#include "include/magicNumbers.h"
#include "include/moveGeneration.h"
#include <iostream>
#include <bit>
#include <random>
// Randomly generate 64-bit integers
std::mt19937_64 rng(12345); // Generates the EXACT SAME sequence of random numbers every single time I run my program.

// random num getter
U64 random_U64() {
    return rng();
}

// Generates a random number with very few 1s
U64 random_sparse_U64() {
    return random_U64() & random_U64() & random_U64(); // dense random numbers can cause collisions
}

/* ---Collision Testing--- */

// Brute forces a Magic Number for a specific square and piece.
U64 findMagicNumber(int square, int bitsInMask, bool isBishop) {
    // init testing arrays (4096 is max permutations for a Rook)
    U64 occupancies[4096];
    U64 attacks[4096];
    U64 usedAttacks[4096];
    bool isFilled[4096];

    // grab the correct mask based on piece type
    U64 mask = isBishop ? bishopMasks[square] : rookMasks[square];

    // total permutations we need to test (2^bitsInMask)
    int numOccupancies = 1 << bitsInMask;

    // Pre-calculate the TRUE attack rays for every possible blocker arrangement
    for (int i = 0; i < numOccupancies; i++) {
        occupancies[i] = setOccupancyHelper(i, bitsInMask, mask);
        attacks[i] = isBishop ? simulateBishopAttacks(square, occupancies[i]) : simulateRookAttacks(square, occupancies[i]);
    }

    // Brute-Force Guessing Loop (Try 100 million times before giving up)
    for (int attempt = 0; attempt < 100000000; attempt++) {

        U64 magicGuess = random_sparse_U64();

        // Skip numbers that don't have enough bits
        if (std::popcount((mask * magicGuess) & 0xFF00000000000000ULL) < 6) continue;

        // Reset our tracker arrays for this guess
        for (int i = 0; i < 4096; i++) {
            usedAttacks[i] = 0ULL;
            isFilled[i] = false;
        }

        bool failed = false;

        // Test the guess against every single pre-calculated permutation
        for (int i = 0; i < numOccupancies; i++) {

            // Scan the board to get the scrambled index
            int magicIndex = (occupancies[i] * magicGuess) >> (64 - bitsInMask);

            if (!isFilled[magicIndex]) {
                usedAttacks[magicIndex] = attacks[i];
                isFilled[magicIndex] = true;
            }
            // If the box is full, but the attack inside doesn't match our attack -> Collision
            else if (usedAttacks[magicIndex] != attacks[i]) {
                failed = true;
                break;
            }
        }

        if (!failed) {
            return magicGuess;
        }
    }

    // If we hit 100 million attempts, return 0
    std::cout << "Failed to find Magic Number for square " << square << std::endl;
    return 0ULL;
}

/**
 * Runs the brute-force algorithm for all 64 squares and prints the
 * array code directly to the terminal
 *
 */
void initMagicNumbers() {
    std::cout << "--- BISHOP MAGIC NUMBERS ---" << std::endl;
    for (int square = 0; square < 64; square++) {
        int bits = std::popcount(bishopMasks[square]);
        U64 magic = findMagicNumber(square, bits, true);

        // Print it in perfect C++ hexadecimal format
        std::cout << "0x" << std::hex << magic << "ULL,\n";
    }
    std::cout << "\n\n";

    std::cout << "--- ROOK MAGIC NUMBERS ---" << std::endl;
    for (int square = 0; square < 64; square++) {
        int bits = std::popcount(rookMasks[square]);
        U64 magic = findMagicNumber(square, bits, false);

        std::cout << "0x" << std::hex << magic << "ULL,\n";
    }
    std::cout << "\n";
}