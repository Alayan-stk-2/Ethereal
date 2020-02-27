/*
  Ethereal is a UCI chess playing engine authored by Andrew Grant.
  <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>

  Ethereal is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Ethereal is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

enum {
    RANK_1 = 0x00000000000000FFull,
    RANK_2 = 0x000000000000FF00ull,
    RANK_3 = 0x0000000000FF0000ull,
    RANK_4 = 0x00000000FF000000ull,
    RANK_5 = 0x000000FF00000000ull,
    RANK_6 = 0x0000FF0000000000ull,
    RANK_7 = 0x00FF000000000000ull,
    RANK_8 = 0xFF00000000000000ull,

    FILE_A = 0x0101010101010101ull,
    FILE_B = 0x0202020202020202ull,
    FILE_C = 0x0404040404040404ull,
    FILE_D = 0x0808080808080808ull,
    FILE_E = 0x1010101010101010ull,
    FILE_F = 0x2020202020202020ull,
    FILE_G = 0x4040404040404040ull,
    FILE_H = 0x8080808080808080ull,

    WHITE_SQUARES = 0x55AA55AA55AA55AAull,
    BLACK_SQUARES = 0xAA55AA55AA55AA55ull,

    LONG_DIAGONALS = 0x8142241818244281ull,
    CENTER_SQUARES = 0x0000001818000000ull,
    CENTER_BIG     = 0x00003C3C3C3C0000ull,

    LEFT_FLANK  = FILE_A | FILE_B | FILE_C | FILE_D,
    RIGHT_FLANK = FILE_E | FILE_F | FILE_G | FILE_H,

    PROMOTION_RANKS = RANK_1 | RANK_8
};

enum {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
};

extern const uint64_t Files[FILE_NB];
extern const uint64_t Ranks[RANK_NB];

int fileOf(int sq);
int mirrorFile(int file);
int rankOf(int sq);
int relativeRankOf(int colour, int sq);
int square(int rank, int file);
int relativeSquare32(int colour, int sq);
uint64_t squaresOfMatchingColour(int sq);

int frontmost(int colour, uint64_t bb);
int backmost(int colour, uint64_t bb);

int popcount(uint64_t bb);
int getlsb(uint64_t bb);
int getmsb(uint64_t bb);
int poplsb(uint64_t *bb);
int popmsb(uint64_t *bb);
bool several(uint64_t bb);
bool onlyOne(uint64_t bb);

void setBit(uint64_t *bb, int i);
void clearBit(uint64_t *bb, int i);
bool testBit(uint64_t bb, int i);

void printBitboard(uint64_t b);
