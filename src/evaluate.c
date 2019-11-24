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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "masks.h"
#include "transposition.h"
#include "types.h"

EvalTrace T, EmptyTrace;
EvalScore PSQT[32][SQUARE_NB];

const int EarlyPhaseArray[33] = {
    1000, 938, 875, 813, 766, 722, 679, 638,
     598, 558, 518, 478, 438, 398, 358, 318,
     278, 238, 202, 166, 130,  96,  66,  40,
      18,   0,   0,   0,   0,   0,   0,   0,
       0, 
};
const int MidPhaseArray[33] = {
       0,  62, 125, 187, 218, 244, 267, 286,
     304, 322, 339, 356, 374, 391, 409, 427,
     444, 461, 471, 481, 490, 496, 493, 483,
     464, 437, 375, 312, 250, 187, 125,  62,
       0, 
};
const int LatePhaseArray[33] = {
       0,   0,   0,   0,  16,  34,  54,  76,
      98, 120, 143, 166, 188, 211, 233, 255,
     278, 301, 327, 353, 380, 408, 441, 477,
     518, 563, 625, 688, 750, 813, 875, 938,
    1000, 
};
/* Material Value Evaluation Terms */

EvalScore PawnValue   = {  97,  94, 108};
EvalScore KnightValue = { 398, 431, 377};
EvalScore BishopValue = { 427, 436, 396};
EvalScore RookValue   = { 600, 645, 625};
EvalScore QueenValue  = {1242,1256,1270};
EvalScore KingValue   = {   0,   0,   0};

/* Piece Square Evaluation Terms */

EvalScore PawnPSQT32[32] = {
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
    { -14, -19,   6}, {   5, -10,   5}, { -12,  13,  -8}, {  -6,   5,  -5}, 
    { -15, -25,   5}, {  -8, -12,   4}, {  -7, -11,  -3}, {  -3,   3, -15}, 
    { -14, -12,   7}, { -15,   5,   2}, {  14, -22,   2}, {   9, -18, -12}, 
    {  -7,   0,   5}, {  -5,   8,   0}, {  -9,   7, -15}, {  10, -23, -13}, 
    { -15,  20,   9}, { -14,  18,   6}, {  -9,  12,   1}, {  17,  12, -19}, 
    { -18, -30, -54}, { -64, -41, -34}, {  -1, -13, -38}, {  37,   0, -46}, 
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
};

EvalScore KnightPSQT32[32] = {
    { -46, -37, -29}, {  -4, -26, -19}, { -10, -16, -24}, {   4,  -7, -12}, 
    {   0, -11, -19}, {   3,  -4, -10}, {   3, -11, -24}, {  10,   4, -19}, 
    {   7, -12, -18}, {  18,  -1, -19}, {  12,   4, -18}, {  21,  18,  -9}, 
    {  15,   0,  11}, {  18,  13,   6}, {  23,  22,  11}, {  25,  32,  14}, 
    {  20,  18,  15}, {  20,  21,   6}, {  29,  33,  20}, {  19,  45,  21}, 
    { -17,  -2,   9}, {  -6,   5,   8}, {  23,  26,  21}, {  18,  27,  22}, 
    {   5,  -3,  -9}, { -11,  -4,  -3}, {  21,   9, -18}, {  39,  21,  -1}, 
    {-174,-103, -35}, { -81, -45, -11}, {-110, -52,   3}, { -31, -18,  -5}, 
};

EvalScore BishopPSQT32[32] = {
    {  16,  -3, -13}, {  10,  -3, -16}, { -11,  -9,  -9}, {   9,  -5, -11}, 
    {  25,  -5, -27}, {  18, -16, -16}, {  16,  -3, -17}, {   6,   6, -18}, 
    {  12, -11,  -7}, {  21,   4, -15}, {   9,  14, -17}, {  16,   5,  -8}, 
    {  11,  -4,  -7}, {  10,   8,  -8}, {   8,  19, -11}, {  14,  17,  -3}, 
    { -16,  -3,   5}, {  10,  12,  -3}, {  -3,  10,   1}, {  -1,  19,   3}, 
    {  -4,   1,   1}, {  -8,   4,   7}, {   5,   8,   6}, {   7,   8,   4}, 
    { -47,   5,   1}, { -42, -15,   1}, { -14,  -4,  -2}, { -23, -11,   1}, 
    { -42, -23,  -5}, { -47, -24,  -1}, { -87, -41,   0}, { -90, -40,   8}, 
};

EvalScore RookPSQT32[32] = {
    { -13, -29, -17}, { -19,  -6, -26}, {  -5, -12, -21}, {   1,  -4, -27}, 
    { -51, -25, -21}, { -13, -18, -28}, {  -6, -22, -24}, {   2,  -9, -30}, 
    { -26, -26, -13}, {  -7, -13,  -7}, { -14, -15, -15}, {   1,  -8, -21}, 
    { -17, -16,  -3}, { -12,  -1,   2}, {  -7,  -4,  -2}, {   4,   3,  -6}, 
    {  -9,  -2,   3}, {   6,   7,   2}, {  16,  11,  -2}, {  32,  17,  -4}, 
    { -22,  -3,   9}, {  17,  13,  -1}, {  -6,   5,  11}, {  25,  15,  -1}, 
    {  -4,   1,  -2}, { -22,  -4,   5}, {   4,   3,  -2}, {  19,  12,   1}, 
    {  29,  24,  15}, {  21,  21,  16}, {   4,  14,  19}, {  15,  18,  17}, 
};

EvalScore QueenPSQT32[32] = {
    {  32,  -9, -44}, {  12, -14, -35}, {  17, -15, -42}, {  21, -11, -27}, 
    {  15, -10, -34}, {  26, -11, -47}, {  29, -18, -53}, {  18,   1, -25}, 
    {  10,  -7, -22}, {  25,  -4, -17}, {   6,   8,  -6}, {  10,   1,  -7}, 
    {  12,  -8,  -3}, {  10,  11,   8}, {  -1,   3,   7}, { -16,  25,  29}, 
    {  -9,  -8,  13}, {  -9,   9,  28}, { -20,  -6,   8}, { -36,  15,  42}, 
    { -31,  -6,  20}, { -24,  -5,   9}, { -32, -10,   8}, { -24,  -5,  11}, 
    {  -9,   7,  19}, { -64,  -7,  43}, { -16,  -5,   4}, { -49,  -7,  34}, 
    { -14,  -1,   8}, {  13,   8,   4}, {   4,   3,   2}, { -10,   0,  10}, 
};

EvalScore KingPSQT32[32] = {
    {  44, -24, -46}, {  38,   2, -34}, { -10, -13,  -6}, { -23, -52,   6}, 
    {  26,  14, -19}, {  -6,  -3, -13}, { -41,   6,  -2}, { -54, -13,   4}, 
    {   7,  -5, -21}, {  12,   3, -16}, {  15,   8,   5}, {  -8,   4,  15}, 
    {   4, -12, -28}, {  85,  32, -22}, {  43,  21,  12}, {   0,   4,  25}, 
    {   6,  -6, -16}, {  95,  40, -21}, {  48,  31,  12}, {   6,   7,  18}, 
    {  46,  12, -24}, { 120,  58, -14}, {  96,  54,   4}, {  40,  21,   0}, 
    {   6, -21, -49}, {  46,  23, -10}, {  32,  20,   2}, {   9,   3,  -8}, 
    {   9, -42, -92}, {  75,  16, -43}, { -19, -17, -19}, { -18, -16, -15}, 
};

/* Pawn Evaluation Terms */

EvalScore PawnCandidatePasser[2][8] = {
   {{   0,   0,   0}, { -25, -18, -11}, { -11,  -2,   8}, { -15,   3,  23}, {  -3,  26,  49}, {  37,  51,  60}, {   0,   0,   0}, {   0,   0,   0}},
   {{   0,   0,   0}, { -14,  -1,  15}, {  -4,   4,  20}, {   4,  21,  38}, {  14,  48,  69}, {  31,  41,  51}, {   0,   0,   0}, {   0,   0,   0}},
};

EvalScore PawnIsolated = {  -7,  -9, -10};

EvalScore PawnStacked[2] = {
    { -14,   2, -23}, { -11,   5, -13}, 
};

EvalScore PawnBackwards[2] = {
    {   6,  -3,  -2}, {  -5, -16, -18}, 
};

EvalScore PawnConnected32[32] = {
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
    {  -3,  -2,  -5}, {  12,  -5,   8}, {   3,   2,  -2}, {   5,   9,   9}, 
    {   8,  12,  -2}, {  23,  -1,   5}, {  16,  17,   2}, {  18,  21,   5}, 
    {   6,   8,  -3}, {  21,   3,   7}, {   7,  15,   3}, {  12,  21,   8}, 
    {   6,   7,   8}, {  19,  11,  13}, {  20,  23,  16}, {  29,   6,  25}, 
    {  50,  42,  22}, {  39,  48,  40}, {  59,  59,  47}, {  75,  67,  55}, 
    { 112,  62,  12}, { 204, 118,  29}, { 228, 140,  51}, { 240, 156,  68}, 
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
};

/* Knight Evaluation Terms */

EvalScore KnightOutpost[2][2] = {
   {{   7,   0, -24}, {  29,  22,  -6}},
   {{  -2,  -6, -21}, {  13,   3,  -3}},
};

EvalScore KnightBehindPawn = {   2,  18,   9};

EvalScore KnightMobility[9] = {
    { -74, -87,-101}, { -31, -59, -89}, { -16, -27, -38}, {  -5,  -9, -15}, 
    {   6,   0,  -6}, {  11,   9,   8}, {  19,  15,  12}, {  28,  20,  13}, 
    {  40,  20,   2}, 
};

/* Bishop Evaluation Terms */

EvalScore BishopPair = {  15,  45,  58};

EvalScore BishopRammedPawns = { -13,   0, -17};

EvalScore BishopOutpost[2][2] = {
   {{   8,   6, -12}, {  37,  26,   1}},
   {{   4,  -1,  -8}, {  10,   6,  -1}},
};

EvalScore BishopBehindPawn = {   3,   9,  14};

EvalScore BishopMobility[14] = {
    { -65,-101,-138}, { -30, -58, -88}, { -11, -31, -51}, {  -1, -13, -27}, 
    {   9,  -3, -15}, {  17,   7,  -2}, {  20,  14,   8}, {  21,  17,  12}, 
    {  20,  18,  17}, {  24,  21,  19}, {  23,  21,  19}, {  42,  27,  12}, 
    {  42,  31,  21}, {  71,  33,  -5}, 
};

/* Rook Evaluation Terms */

EvalScore RookFile[2] = {
    {  13,   1,   7}, {  23,  38,  -6}, 
};

EvalScore RookOnSeventh = {  -8,  10,  22};

EvalScore RookMobility[15] = {
    {-148,-132,-117}, { -52, -79,-106}, { -15, -35, -56}, {  -7, -13, -19}, 
    {  -7,  -4,  -2}, {  -8,   2,  12}, {  -7,   6,  21}, {  -1,  11,  24}, 
    {   6,  17,  27}, {  10,  21,  31}, {  13,  25,  37}, {  18,  29,  40}, 
    {  19,  31,  44}, {  30,  34,  38}, {  80,  46,  12}, 
};

/* Queen Evaluation Terms */

EvalScore QueenMobility[28] = {
    { -61,-151,-241}, {-210,-288,-367}, { -58,-121,-185}, { -16, -94,-172}, 
    {  -4, -64,-124}, {   0, -37, -75}, {   5, -20, -46}, {   5,  -7, -20}, 
    {   9,  -2, -13}, {  10,  11,   6}, {  13,  13,  13}, {  14,  20,  26}, 
    {  16,  20,  23}, {  17,  24,  31}, {  15,  25,  35}, {  12,  24,  38}, 
    {  11,  25,  40}, {   3,  22,  40}, {   4,  21,  38}, {  -1,  16,  33}, 
    {   7,  11,  15}, {  22,  18,  -3}, {  27,   0, -27}, {  30,  -6, -42}, 
    {  12, -24, -59}, {  24, -28, -82}, { -56, -48, -41}, { -31, -44, -58}, 
};

/* King Evaluation Terms */

EvalScore KingDefenders[12] = {
    { -23, -13,   1}, {  -2, -13,   1}, {   3,  -5,   5}, {   7,   8,   3}, 
    {  15,  19,   1}, {  24,  19,   4}, {  28,  16,   1}, {  13,   7,   1}, 
    {  12,   9,   7}, {  12,   9,   7}, {  12,   9,   7}, {  12,   9,   7}, 
};

EvalScore KingPawnFileProximity[8] = {
    {  33,  38,  29}, {  12,  26,  19}, {   6,   4,  13}, { -14, -19, -11}, 
    { -17, -33, -44}, { -16, -38, -59}, { -15, -39, -64}, { -11, -37, -63}, 
};

EvalScore KingShelter[2][8][8] = {
  {{{ -10,  -6,   0}, {  14,  -6, -20}, {  19,   8,  -6}, {  12,   9,   5}, {   7,   5,   4}, {   0,   1,   2}, {  -3, -16, -30}, { -49, -16,   9}},
   {{  17,   4,  -6}, {  17,   3, -12}, {   2,  -2,  -4}, { -14,  -6,   1}, { -30, -10,  10}, { -70,  -9,  52}, {  92,  88,  83}, { -22, -11,  -2}},
   {{  34,  16,  -2}, {  12,   5,  -8}, { -27, -10,   1}, {  -9,  -9,  -8}, { -19, -13,  -7}, { -12,  -6,   0}, {   0,  29,  59}, { -11,  -5,  -4}},
   {{   3,   8,   7}, {  21,   5,  -7}, {   5,  -9,  -7}, {  13,  -6, -12}, {  23,  -5, -26}, { -58, -30,  -2}, {-136, -52,  31}, {   9,  -3,  -5}},
   {{ -16,  -1,   3}, {   3,   2,  -4}, { -25, -10,  -4}, { -16,  -5,   0}, { -17, -13,  -8}, { -42, -24,  -6}, {  33,  11, -11}, {  -5,  -2,  -3}},
   {{  40,  18, -12}, {  21,  -3,  -8}, { -19, -11,   0}, { -12, -14, -13}, {   4,  -8, -22}, {  17,   0, -16}, {  41,   9, -22}, { -20, -16,   0}},
   {{  21,  19, -16}, {  -2,  -8, -13}, { -21, -14,  -2}, { -19, -15,  -7}, { -29, -19,  -9}, { -37,  -6,  23}, {   0,  20,  39}, {  -7,  -8,   0}},
   {{  -7, -10, -12}, {   6, -11, -12}, {   5,   6,   1}, {  -4,   8,   7}, { -12,  -1,  12}, { -10,  11,  31}, {-190, -66,  56}, { -18,   2,   7}}},
  {{{   0,   0,   0}, { -11, -15, -23}, {   5,  -5, -16}, { -40, -15,  10}, { -23, -11,  -1}, {   3,  20,  38}, {-167, -96, -26}, { -44, -19,   4}},
   {{   0,   0,   0}, {  20,   2, -17}, {   6,   0,  -6}, { -18, -10,  -3}, {  -1,  -5, -11}, {  26,  43,  61}, {-184,-103, -23}, { -36, -16,   0}},
   {{   0,   0,   0}, {  24,  10, -10}, {  -2,  -6,  -8}, {   5,  -5, -15}, {  15,   5,  -5}, { -87, -27,  32}, { -84, -79, -74}, { -14, -12,  -2}},
   {{   0,   0,   0}, {  -6,   2,   4}, {  -5,  -2,  -4}, { -19,  -8,  -2}, { -27, -15,  -3}, { -99, -41,  17}, {   7, -14, -36}, { -13, -11,  -2}},
   {{   0,   0,   0}, {  13,   0,   6}, {  12,  -6,   0}, {  12,   2,  -9}, {  12,  -2, -21}, { -57, -25,   7}, {-104, -84, -66}, {   4,  -4,   0}},
   {{   0,   0,   0}, {   2,   4, -10}, { -19, -11,  -4}, { -25, -17,  -9}, {  17,  -1, -19}, { -38, -19,  -2}, {  55,  47,  40}, { -15,  -8,  -6}},
   {{   0,   0,   0}, {  17,  13, -17}, {  10,  -1, -10}, {  -9, -12,  -5}, { -25, -13,   6}, {  -9,   2,  12}, { -56, -52, -50}, { -29,  -7,   5}},
   {{   0,   0,   0}, {  11, -13, -30}, {  17,  -3, -22}, { -18, -13,  -6}, { -18,  -3,  14}, {  -6,   6,  17}, {-228,-151, -74}, { -20, -11,   0}}},
};

EvalScore KingStorm[2][4][8] = {
  {{{  -4,   8,  24}, { 115,  59,   2}, { -23,  -4,  19}, { -17,  -7,   4}, { -14,  -8,   1}, {  -9,  -5,  -5}, { -18,  -1,  -3}, { -23,  -9,  -1}},
   {{  -3,  19,  42}, {  56,  35,  11}, { -18,  -1,  16}, {  -5,   6,   7}, {  -8,   9,  -1}, {   4,  -2,  -2}, {  -2,  -4,   2}, { -14,   2,   0}},
   {{   7,  20,  35}, {  16,  18,  18}, { -22,  -2,  13}, { -13,   1,   3}, {   1,   2,   0}, {   5,   4,  -1}, {   9,  -3,   2}, {   1,   6,   1}},
   {{  -2,   8,  23}, {  14,  15,  16}, { -18,  -8,   4}, { -11, -13,   0}, { -12, -15,   2}, {   6,  -3,  -6}, {   0,  -2,  -5}, { -10,  -2,   1}}},
  {{{   0,   0,   0}, { -15, -15, -16}, { -15, -14,   0}, {  15,   2, -11}, {   5,   6,  -5}, {   4,  -6, -15}, {  -2,  -1,  -1}, {  17,  22,  28}},
   {{   0,   0,   0}, { -16, -24, -33}, {   1,  -7,  -5}, {  35,  17,  -5}, {   0,   0,  -2}, {  14,  -5, -16}, {  -7,  -8, -10}, { -17,  -8,   0}},
   {{   0,   0,   0}, { -28, -37, -48}, { -23, -16,  -7}, {  11,   3,  -9}, {   5,  -2,   0}, {  -6, -12, -12}, { -13, -14, -14}, { -11,  -3,   4}},
   {{   0,   0,   0}, {  -2,  -9, -16}, { -14, -15, -16}, {  -9,  -6,  -6}, {  -2,  -9,  -3}, {   2,  -8, -18}, {  72,  34,  -2}, {  13,  16,  19}}},
};

/* King Safety Evaluation Terms */

const int KSAttackWeight[]  = { 0, 16, 6, 10, 8, 0 };
const int KSAttackValue     =   44;
const int KSWeakSquares     =   38;
const int KSFriendlyPawns   =  -22;
const int KSNoEnemyQueens   = -276;
const int KSSafeQueenCheck  =   95;
const int KSSafeRookCheck   =   94;
const int KSSafeBishopCheck =   51;
const int KSSafeKnightCheck =  123;
const int KSAdjustment      =  -18;

/* Passed Pawn Evaluation Terms */

EvalScore PassedPawn[2][2][8] = {
  {{{   0,   0,   0}, { -38, -17,  -1}, { -54, -20,  13}, { -80, -29,  16}, {  -5,   5,  10}, {  72,  39,   2}, { 159, 114,  66}, {   0,   0,   0}},
   {{   0,   0,   0}, { -30, -13,   4}, { -52, -16,  17}, { -71, -25,  21}, { -11,   8,  30}, {  91,  62,  41}, { 182, 147, 107}, {   0,   0,   0}}},
  {{{   0,   0,   0}, { -24,  -6,  14}, { -51, -16,  14}, { -73, -22,  22}, {  -5,  21,  32}, {  85,  73,  44}, { 258, 197, 126}, {   0,   0,   0}},
   {{   0,   0,   0}, { -30, -12,  10}, { -45, -19,  15}, { -63, -22,  29}, {  -2,  23,  47}, {  93, 100, 106}, { 162, 206, 250}, {   0,   0,   0}}},
};

EvalScore PassedFriendlyDistance[8] = {
    {   0,   0,   0}, {   2,  -5,   3}, {   7,  -8,   2}, {  11, -10,  -2}, 
    {   7,  -5,  -8}, { -12,  -2, -13}, { -23,   3, -14}, {   0,   0,   0}, 
};

EvalScore PassedEnemyDistance[8] = {
    {   0,   0,   0}, {   2,   4,  -1}, {   4,   8,   0}, {   7,  14,   4}, 
    {  -2,  16,  12}, {   4,  19,  20}, {  18,  28,  17}, {   0,   0,   0}, 
};

EvalScore PassedSafePromotionPath = { -19,  -4,  26};

/* Threat Evaluation Terms */

EvalScore ThreatWeakPawn             = { -12, -13, -24};
EvalScore ThreatMinorAttackedByPawn  = { -45, -55, -49};
EvalScore ThreatMinorAttackedByMinor = { -21, -44, -22};
EvalScore ThreatMinorAttackedByMajor = { -20, -34, -40};
EvalScore ThreatRookAttackedByLesser = { -41, -39, -21};
EvalScore ThreatMinorAttackedByKing  = { -24, -21, -24};
EvalScore ThreatRookAttackedByKing   = { -17, -20, -26};
EvalScore ThreatQueenAttackedByOne   = { -32, -42, -35};
EvalScore ThreatOverloadedPieces     = {  -7, -11, -10};
EvalScore ThreatByPawnPush           = {  12,  21,  16};

/* Closedness Evaluation Terms */

EvalScore ClosednessKnightAdjustment[9] = {
    { -16,  -9, -20}, { -14,  -4,   0}, { -14,   0,   7}, {  -8,   4,   9}, 
    {  -5,   8,  15}, {  -1,   8,  13}, {   2,   7,  11}, { -10,  10,  24}, 
    { -10,  -9,  13}, 
};

EvalScore ClosednessRookAdjustment[9] = {
    {  49,  38, -27}, {   3,  11,  20}, {  -5,   1,   7}, { -13,  -9,  -2}, 
    { -19, -11,  -4}, { -20, -15, -13}, { -20, -17, -15}, { -25, -21, -18}, 
    { -29, -24, -18}, 
};

/* Complexity Evaluation Terms */

EvalScore ComplexityTotalPawns  = {   0,   0,   6};
EvalScore ComplexityPawnFlanks  = {   0,   0,  44};
EvalScore ComplexityPawnEndgame = {   0,   0,  34};
EvalScore ComplexityAdjustment  = {   0,   0, -98};

/* General Evaluation Terms */

const int Tempo = 20;

#undef S

int evaluateBoard(Board *board, PKTable *pktable) {

    EvalInfo ei;
    EvalScore eval = SCORE_ZERO, pkeval = SCORE_ZERO;;
    int interpolatedEval, phase, earlyPhase, midPhase, latePhase, factor;

    // Setup and perform all evaluations
    initEvalInfo(&ei, board, pktable);
    eval   = evaluatePieces(&ei, board);
    pkeval = ei.pkeval[WHITE];
    addTo(&pkeval, neg(&ei.pkeval[BLACK]));
    addTo(&eval, pkeval);
    addTo(&eval, board->psqtmat);
    addTo(&eval, evaluateClosedness(&ei, board));
    addTo(&eval, evaluateComplexity(&ei, board, eval));

    // Calculate the game phase based on remaining non-pawn material
    // If 2 minors or less remain, phase will not change
    phase = 32 - 6 * popcount(board->pieces[QUEEN ])
               - 3 * popcount(board->pieces[ROOK  ])
               - 2 * popcount(board->pieces[KNIGHT]
                             |board->pieces[BISHOP]);
    phase = MAX(0, MIN(32, phase));
    earlyPhase = EarlyPhaseArray[phase];
    midPhase   = MidPhaseArray[phase];
    latePhase  = LatePhaseArray[phase];

    // Scale evaluation based on remaining material
    factor = evaluateScaleFactor(board, eval);

    // Compute the interpolated and scaled evaluation
    interpolatedEval = eval.og * earlyPhase
                     + eval.mg * midPhase
                     + eval.eg * latePhase * factor / SCALE_NORMAL;
    interpolatedEval = interpolatedEval/1000;

    // Factor in the Tempo after interpolation and scaling, so that
    // in the search we can assume that if a null move is made, then
    // then `eval = last_eval + 2 * Tempo`
    interpolatedEval += board->turn == WHITE ? Tempo : -Tempo;

    // Store a new Pawn King Entry if we did not have one
    if (ei.pkentry == NULL && pktable != NULL)
        storePKEntry(pktable, board->pkhash, ei.passedPawns, pkeval);

    // Return the evaluation relative to the side to move
    return board->turn == WHITE ? interpolatedEval : -interpolatedEval;
}

EvalScore evaluatePieces(EvalInfo *ei, Board *board) {

    EvalScore eval = SCORE_ZERO;
    EvalScore temp;

    addTo(&eval,  evaluatePawns(ei, board, WHITE) ); //FIXME : evaluatePawns always return 0, this is useless
    temp =        evaluatePawns(ei, board, BLACK);
    addTo(&eval, neg(&temp));
    addTo(&eval, evaluateKnights(ei, board, WHITE) );
    temp =       evaluateKnights(ei, board, BLACK);
    addTo(&eval, neg(&temp));
    addTo(&eval,evaluateBishops(ei, board, WHITE) );
    temp =      evaluateBishops(ei, board, BLACK);
    addTo(&eval, neg(&temp));
    addTo(&eval,  evaluateRooks(ei, board, WHITE) );
    temp =        evaluateRooks(ei, board, BLACK);
    addTo(&eval, neg(&temp));
    addTo(&eval, evaluateQueens(ei, board, WHITE) );
    temp =       evaluateQueens(ei, board, BLACK);
    addTo(&eval, neg(&temp));
    addTo(&eval,  evaluateKings(ei, board, WHITE) );
    temp =        evaluateKings(ei, board, BLACK);
    addTo(&eval, neg(&temp));
    addTo(&eval, evaluatePassed(ei, board, WHITE) );
    temp =       evaluatePassed(ei, board, BLACK);
    addTo(&eval, neg(&temp));
    addTo(&eval,evaluateThreats(ei, board, WHITE) );
    temp =      evaluateThreats(ei, board, BLACK);
    addTo(&eval, neg(&temp));

    return eval;
}

EvalScore evaluatePawns(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const int Forward = (colour == WHITE) ? 8 : -8;

    EvalScore eval = SCORE_ZERO, pkeval = SCORE_ZERO;
    int sq, flag;
    uint64_t pawns, myPawns, tempPawns, enemyPawns, attacks;

    // Store off pawn attacks for king safety and threat computations
    ei->attackedBy2[US]      = ei->pawnAttacks[US] & ei->attacked[US];
    ei->attacked[US]        |= ei->pawnAttacks[US];
    ei->attackedBy[US][PAWN] = ei->pawnAttacks[US];

    // Update King Safety calculations
    attacks = ei->pawnAttacks[US] & ei->kingAreas[THEM];
    ei->kingAttacksCount[US] += popcount(attacks);

    // Pawn hash holds the rest of the pawn evaluation
    if (ei->pkentry != NULL) return eval;

    pawns = board->pieces[PAWN];
    myPawns = tempPawns = pawns & board->colours[US];
    enemyPawns = pawns & board->colours[THEM];

    // Evaluate each pawn (but not for being passed)
    while (tempPawns) {

        // Pop off the next pawn
        sq = poplsb(&tempPawns);
        if (TRACE) T.PawnValue[US]++;
        if (TRACE) T.PawnPSQT32[relativeSquare32(US, sq)][US]++;

        uint64_t neighbors   = myPawns    & adjacentFilesMasks(fileOf(sq));
        uint64_t backup      = myPawns    & passedPawnMasks(THEM, sq);
        uint64_t stoppers    = enemyPawns & passedPawnMasks(US, sq);
        uint64_t threats     = enemyPawns & pawnAttacks(US, sq);
        uint64_t support     = myPawns    & pawnAttacks(THEM, sq);
        uint64_t pushThreats = enemyPawns & pawnAttacks(US, sq + Forward);
        uint64_t pushSupport = myPawns    & pawnAttacks(THEM, sq + Forward);
        uint64_t leftovers   = stoppers ^ threats ^ pushThreats;

        // Save passed pawn information for later evaluation
        if (!stoppers) setBit(&ei->passedPawns, sq);

        // Apply a bonus for pawns which will become passers by advancing a
        // square then exchanging our supporters with the remaining stoppers
        else if (!leftovers && popcount(pushSupport) >= popcount(pushThreats)) {
            flag = popcount(support) >= popcount(threats);
            addTo(&pkeval, PawnCandidatePasser[flag][relativeRankOf(US, sq)]);
            if (TRACE) T.PawnCandidatePasser[flag][relativeRankOf(US, sq)][US]++;
        }

        // Apply a penalty if the pawn is isolated. We consider pawns that
        // are able to capture another pawn to not be isolated, as they may
        // have the potential to deisolate by capturing, or be traded away
        if (!threats && !neighbors) {
            addTo(&pkeval, PawnIsolated);
            if (TRACE) T.PawnIsolated[US]++;
        }

        // Apply a penalty if the pawn is stacked. We adjust the bonus for when
        // the pawn appears to be a candidate to unstack. This occurs when the
        // pawn is not passed but may capture or be recaptured by our own pawns,
        // and when the pawn may freely advance on a file and then be traded away
        if (several(Files[fileOf(sq)] & myPawns)) {
            flag = (stoppers && (threats || neighbors))
                || (stoppers & ~forwardFileMasks(US, sq));
            addTo(&pkeval, PawnStacked[flag]);
            if (TRACE) T.PawnStacked[flag][US]++;
        }

        // Apply a penalty if the pawn is backward. We follow the usual definition
        // of backwards, but also specify that the pawn is not both isolated and
        // backwards at the same time. We don't give backward pawns a connected bonus
        if (neighbors && pushThreats && !backup) {
            flag = !(Files[fileOf(sq)] & enemyPawns);
            addTo(&pkeval, PawnBackwards[flag]);
            if (TRACE) T.PawnBackwards[flag][US]++;
        }

        // Apply a bonus if the pawn is connected and not backwards. We consider a
        // pawn to be connected when there is a pawn lever or the pawn is supported
        else if (pawnConnectedMasks(US, sq) & myPawns) {
            addTo(&pkeval, PawnConnected32[relativeSquare32(US, sq)]);
            if (TRACE) T.PawnConnected32[relativeSquare32(US, sq)][US]++;
        }
    }

    ei->pkeval[US] = pkeval; // Save eval for the Pawn Hash

    return eval; //FIXME eval is always zero, why even bother returning it ?
}

EvalScore evaluateKnights(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    EvalScore eval = SCORE_ZERO;
    int sq, outside, defended, count;
    uint64_t attacks;

    uint64_t enemyPawns  = board->pieces[PAWN  ] & board->colours[THEM];
    uint64_t tempKnights = board->pieces[KNIGHT] & board->colours[US  ];

    ei->attackedBy[US][KNIGHT] = 0ull;

    // Evaluate each knight
    while (tempKnights) {

        // Pop off the next knight
        sq = poplsb(&tempKnights);
        if (TRACE) T.KnightValue[US]++;
        if (TRACE) T.KnightPSQT32[relativeSquare32(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = knightAttacks(sq);
        ei->attackedBy2[US]        |= attacks & ei->attacked[US];
        ei->attacked[US]           |= attacks;
        ei->attackedBy[US][KNIGHT] |= attacks;

        // Apply a bonus if the knight is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the knight
        if (     testBit(outpostRanksMasks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            outside  = testBit(FILE_A | FILE_H, sq);
            defended = testBit(ei->pawnAttacks[US], sq);
            addTo(&eval, KnightOutpost[outside][defended]);
            if (TRACE) T.KnightOutpost[outside][defended][US]++;
        }

        // Apply a bonus if the knight is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            addTo(&eval, KnightBehindPawn);
            if (TRACE) T.KnightBehindPawn[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the knight
        count = popcount(ei->mobilityAreas[US] & attacks);
        addTo(&eval, KnightMobility[count]);
        if (TRACE) T.KnightMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM])) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[KNIGHT];
        }
    }

    return eval;
}

EvalScore evaluateBishops(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    EvalScore eval = SCORE_ZERO;
    int sq, outside, defended, count;
    uint64_t attacks;

    uint64_t enemyPawns  = board->pieces[PAWN  ] & board->colours[THEM];
    uint64_t tempBishops = board->pieces[BISHOP] & board->colours[US  ];

    ei->attackedBy[US][BISHOP] = 0ull;

    // Apply a bonus for having a pair of bishops
    if ((tempBishops & WHITE_SQUARES) && (tempBishops & BLACK_SQUARES)) {
        addTo(&eval, BishopPair);
        if (TRACE) T.BishopPair[US]++;
    }

    // Evaluate each bishop
    while (tempBishops) {

        // Pop off the next Bishop
        sq = poplsb(&tempBishops);
        if (TRACE) T.BishopValue[US]++;
        if (TRACE) T.BishopPSQT32[relativeSquare32(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = bishopAttacks(sq, ei->occupiedMinusBishops[US]);
        ei->attackedBy2[US]        |= attacks & ei->attacked[US];
        ei->attacked[US]           |= attacks;
        ei->attackedBy[US][BISHOP] |= attacks;

        // Apply a penalty for the bishop based on number of rammed pawns
        // of our own colour, which reside on the same shade of square as the bishop
        count = popcount(ei->rammedPawns[US] & squaresOfMatchingColour(sq));
        addTo(&eval, intMult(&BishopRammedPawns, count));
        if (TRACE) T.BishopRammedPawns[US] += count;

        // Apply a bonus if the bishop is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the bishop.
        if (     testBit(outpostRanksMasks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            outside  = testBit(FILE_A | FILE_H, sq);
            defended = testBit(ei->pawnAttacks[US], sq);
            addTo(&eval, BishopOutpost[outside][defended]);
            if (TRACE) T.BishopOutpost[outside][defended][US]++;
        }

        // Apply a bonus if the bishop is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            addTo(&eval, BishopBehindPawn);
            if (TRACE) T.BishopBehindPawn[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the bishop
        count = popcount(ei->mobilityAreas[US] & attacks);
        addTo(&eval, BishopMobility[count]);
        if (TRACE) T.BishopMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM])) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[BISHOP];
        }
    }

    return eval;
}

EvalScore evaluateRooks(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    EvalScore eval = SCORE_ZERO;
    int sq, open, count;
    uint64_t attacks;

    uint64_t myPawns    = board->pieces[PAWN] & board->colours[  US];
    uint64_t enemyPawns = board->pieces[PAWN] & board->colours[THEM];
    uint64_t tempRooks  = board->pieces[ROOK] & board->colours[  US];

    ei->attackedBy[US][ROOK] = 0ull;

    // Evaluate each rook
    while (tempRooks) {

        // Pop off the next rook
        sq = poplsb(&tempRooks);
        if (TRACE) T.RookValue[US]++;
        if (TRACE) T.RookPSQT32[relativeSquare32(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = rookAttacks(sq, ei->occupiedMinusRooks[US]);
        ei->attackedBy2[US]      |= attacks & ei->attacked[US];
        ei->attacked[US]         |= attacks;
        ei->attackedBy[US][ROOK] |= attacks;

        // Rook is on a semi-open file if there are no pawns of the rook's
        // colour on the file. If there are no pawns at all, it is an open file
        if (!(myPawns & Files[fileOf(sq)])) {
            open = !(enemyPawns & Files[fileOf(sq)]);
            addTo(&eval, RookFile[open]);
            if (TRACE) T.RookFile[open][US]++;
        }

        // Rook gains a bonus for being located on seventh rank relative to its
        // colour so long as the enemy king is on the last two ranks of the board
        if (   relativeRankOf(US, sq) == 6
            && relativeRankOf(US, ei->kingSquare[THEM]) >= 6) {
            addTo(&eval, RookOnSeventh);
            if (TRACE) T.RookOnSeventh[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the rook
        count = popcount(ei->mobilityAreas[US] & attacks);
        addTo(&eval, RookMobility[count]);
        if (TRACE) T.RookMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM])) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[ROOK];
        }
    }

    return eval;
}

EvalScore evaluateQueens(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    EvalScore eval = SCORE_ZERO;
    int sq, count;
    uint64_t tempQueens, attacks;

    tempQueens = board->pieces[QUEEN] & board->colours[US];

    ei->attackedBy[US][QUEEN] = 0ull;

    // Evaluate each queen
    while (tempQueens) {

        // Pop off the next queen
        sq = poplsb(&tempQueens);
        if (TRACE) T.QueenValue[US]++;
        if (TRACE) T.QueenPSQT32[relativeSquare32(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = queenAttacks(sq, board->colours[WHITE] | board->colours[BLACK]);
        ei->attackedBy2[US]       |= attacks & ei->attacked[US];
        ei->attacked[US]          |= attacks;
        ei->attackedBy[US][QUEEN] |= attacks;

        // Apply a bonus (or penalty) based on the mobility of the queen
        count = popcount(ei->mobilityAreas[US] & attacks);
        addTo(&eval, QueenMobility[count]);
        if (TRACE) T.QueenMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM])) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[QUEEN];
        }
    }

    return eval;
}

EvalScore evaluateKings(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    EvalScore eval = SCORE_ZERO;
    int count, dist, blocked;

    uint64_t myPawns     = board->pieces[PAWN ] & board->colours[  US];
    uint64_t enemyPawns  = board->pieces[PAWN ] & board->colours[THEM];
    uint64_t enemyQueens = board->pieces[QUEEN] & board->colours[THEM];

    uint64_t defenders  = (board->pieces[PAWN  ] & board->colours[US])
                        | (board->pieces[KNIGHT] & board->colours[US])
                        | (board->pieces[BISHOP] & board->colours[US]);

    int kingSq = ei->kingSquare[US];
    if (TRACE) T.KingValue[US]++;
    if (TRACE) T.KingPSQT32[relativeSquare32(US, kingSq)][US]++;

    // Bonus for our pawns and minors sitting within our king area
    count = popcount(defenders & ei->kingAreas[US]);
    addTo(&eval, KingDefenders[count]);
    if (TRACE) T.KingDefenders[count][US]++;

    // Perform King Safety when we have two attackers, or
    // one attacker with a potential for a Queen attacker
    if (ei->kingAttackersCount[THEM] > 1 - popcount(enemyQueens)) {

        // Weak squares are attacked by the enemy, defended no more
        // than once and only defended by our Queens or our King
        uint64_t weak =   ei->attacked[THEM]
                      &  ~ei->attackedBy2[US]
                      & (~ei->attacked[US] | ei->attackedBy[US][QUEEN] | ei->attackedBy[US][KING]);

        // Usually the King Area is 9 squares. Scale are attack counts to account for
        // when the king is in an open area and expects more attacks, or the opposite
        float scaledAttackCounts = 9.0 * ei->kingAttacksCount[THEM] / popcount(ei->kingAreas[US]);

        // Safe target squares are defended or are weak and attacked by two.
        // We exclude squares containing pieces which we cannot capture.
        uint64_t safe =  ~board->colours[THEM]
                      & (~ei->attacked[US] | (weak & ei->attackedBy2[THEM]));

        // Find square and piece combinations which would check our King
        uint64_t occupied      = board->colours[WHITE] | board->colours[BLACK];
        uint64_t knightThreats = knightAttacks(kingSq);
        uint64_t bishopThreats = bishopAttacks(kingSq, occupied);
        uint64_t rookThreats   = rookAttacks(kingSq, occupied);
        uint64_t queenThreats  = bishopThreats | rookThreats;

        // Identify if there are pieces which can move to the checking squares safely.
        // We consider forking a Queen to be a safe check, even with our own Queen.
        uint64_t knightChecks = knightThreats & safe & ei->attackedBy[THEM][KNIGHT];
        uint64_t bishopChecks = bishopThreats & safe & ei->attackedBy[THEM][BISHOP];
        uint64_t rookChecks   = rookThreats   & safe & ei->attackedBy[THEM][ROOK  ];
        uint64_t queenChecks  = queenThreats  & safe & ei->attackedBy[THEM][QUEEN ];

        count  = ei->kingAttackersCount[THEM] * ei->kingAttackersWeight[THEM];

        count += KSAttackValue     * scaledAttackCounts
               + KSWeakSquares     * popcount(weak & ei->kingAreas[US])
               + KSFriendlyPawns   * popcount(myPawns & ei->kingAreas[US] & ~weak)
               + KSNoEnemyQueens   * !enemyQueens
               + KSSafeQueenCheck  * popcount(queenChecks)
               + KSSafeRookCheck   * popcount(rookChecks)
               + KSSafeBishopCheck * popcount(bishopChecks)
               + KSSafeKnightCheck * popcount(knightChecks)
               + KSAdjustment;

        // Convert safety to an MG and EG score, if we are unsafe
        int KD_OG = (count*count)/720;
        int KD_EG = 9*count / 200;

        EvalScore KD = { -KD_OG, -((KD_OG + KD_EG)/2), -KD_EG };

        if (count > 0) addTo(&eval, KD);
    }

    // Everything else is stored in the Pawn King Table
    if (ei->pkentry != NULL) return eval;

    // Evaluate based on the number of files between our King and the nearest
    // file-wise pawn. If there is no pawn, kingPawnFileDistance() returns the
    // same distance for both sides causing this evaluation term to be neutral
    dist = kingPawnFileDistance(board->pieces[PAWN], kingSq);
    addTo(&ei->pkeval[US], KingPawnFileProximity[dist]);
    if (TRACE) T.KingPawnFileProximity[dist][US]++;

    // Evaluate King Shelter & King Storm threat by looking at the file of our King,
    // as well as the adjacent files. When looking at pawn distances, we will use a
    // distance of 7 to denote a missing pawn, since distance 7 is not possible otherwise.
    for (int file = MAX(0, fileOf(kingSq) - 1); file <= MIN(FILE_NB - 1, fileOf(kingSq) + 1); file++) {

        // Find closest friendly pawn at or above our King on a given file
        uint64_t ours = myPawns & Files[file] & forwardRanksMasks(US, rankOf(kingSq));
        int ourDist = !ours ? 7 : abs(rankOf(kingSq) - rankOf(backmost(US, ours)));

        // Find closest enemy pawn at or above our King on a given file
        uint64_t theirs = enemyPawns & Files[file] & forwardRanksMasks(US, rankOf(kingSq));
        int theirDist = !theirs ? 7 : abs(rankOf(kingSq) - rankOf(backmost(US, theirs)));

        // Evaluate King Shelter using pawn distance. Use separate evaluation
        // depending on the file, and if we are looking at the King's file
        addTo(&ei->pkeval[US], KingShelter[file == fileOf(kingSq)][file][ourDist]);
        if (TRACE) T.KingShelter[file == fileOf(kingSq)][file][ourDist][US]++;

        // Evaluate King Storm using enemy pawn distance. Use a separate evaluation
        // depending on the file, and if the opponent's pawn is blocked by our own
        blocked = (ourDist != 7 && (ourDist == theirDist - 1));
        addTo(&ei->pkeval[US], KingStorm[blocked][mirrorFile(file)][theirDist]);
        if (TRACE) T.KingStorm[blocked][mirrorFile(file)][theirDist][US]++;
    }

    return eval;
}

EvalScore evaluatePassed(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    EvalScore eval = SCORE_ZERO;
    int sq, rank, dist, flag, canAdvance, safeAdvance;

    uint64_t bitboard;
    uint64_t tempPawns = board->colours[US] & ei->passedPawns;
    uint64_t occupied  = board->colours[WHITE] | board->colours[BLACK];

    // Evaluate each passed pawn
    while (tempPawns) {

        // Pop off the next passed Pawn
        sq = poplsb(&tempPawns);
        rank = relativeRankOf(US, sq);
        bitboard = pawnAdvance(1ull << sq, 0ull, US);

        // Evaluate based on rank, ability to advance, and safety
        canAdvance = !(bitboard & occupied);
        safeAdvance = !(bitboard & ei->attacked[THEM]);
        addTo(&eval, PassedPawn[canAdvance][safeAdvance][rank]);
        if (TRACE) T.PassedPawn[canAdvance][safeAdvance][rank][US]++;

        // Evaluate based on distance from our king
        dist = distanceBetween(sq, ei->kingSquare[US]);
        addTo(&eval, intMult(&PassedFriendlyDistance[rank], dist));
        if (TRACE) T.PassedFriendlyDistance[rank][US] += dist;

        // Evaluate based on distance from their king
        dist = distanceBetween(sq, ei->kingSquare[THEM]);
        addTo(&eval, intMult(&PassedEnemyDistance[rank], dist));
        if (TRACE) T.PassedEnemyDistance[rank][US] += dist;

        // Apply a bonus when the path to promoting is uncontested
        bitboard = forwardRanksMasks(US, rankOf(sq)) & Files[fileOf(sq)];
        flag = !(bitboard & (board->colours[THEM] | ei->attacked[THEM]));
        addTo(&eval, intMult(&PassedSafePromotionPath, flag));
        if (TRACE) T.PassedSafePromotionPath[US] += flag;
    }

    return eval;
}

EvalScore evaluateThreats(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const uint64_t Rank3Rel = US == WHITE ? RANK_3 : RANK_6;

    EvalScore eval = SCORE_ZERO;
    int count;

    uint64_t friendly = board->colours[  US];
    uint64_t enemy    = board->colours[THEM];
    uint64_t occupied = friendly | enemy;

    uint64_t pawns   = friendly & board->pieces[PAWN  ];
    uint64_t knights = friendly & board->pieces[KNIGHT];
    uint64_t bishops = friendly & board->pieces[BISHOP];
    uint64_t rooks   = friendly & board->pieces[ROOK  ];
    uint64_t queens  = friendly & board->pieces[QUEEN ];

    uint64_t attacksByPawns  = ei->attackedBy[THEM][PAWN  ];
    uint64_t attacksByMinors = ei->attackedBy[THEM][KNIGHT] | ei->attackedBy[THEM][BISHOP];
    uint64_t attacksByMajors = ei->attackedBy[THEM][ROOK  ] | ei->attackedBy[THEM][QUEEN ];

    // Squares with more attackers, few defenders, and no pawn support
    uint64_t poorlyDefended = (ei->attacked[THEM] & ~ei->attacked[US])
                            | (ei->attackedBy2[THEM] & ~ei->attackedBy2[US] & ~ei->attackedBy[US][PAWN]);


    uint64_t weakMinors = (knights | bishops) & poorlyDefended;

    // A friendly minor or major is overloaded if attacked and defended by exactly one
    uint64_t overloaded = (knights | bishops | rooks | queens)
                        & ei->attacked[  US] & ~ei->attackedBy2[  US]
                        & ei->attacked[THEM] & ~ei->attackedBy2[THEM];

    // Look for enemy non-pawn pieces which we may threaten with a pawn advance.
    // Don't consider pieces we already threaten, pawn moves which would be countered
    // by a pawn capture, and squares which are completely unprotected by our pieces.
    uint64_t pushThreat  = pawnAdvance(pawns, occupied, US);
    pushThreat |= pawnAdvance(pushThreat & ~attacksByPawns & Rank3Rel, occupied, US);
    pushThreat &= ~attacksByPawns & (ei->attacked[US] | ~ei->attacked[THEM]);
    pushThreat  = pawnAttackSpan(pushThreat, enemy & ~ei->attackedBy[US][PAWN], US);

    // Penalty for each of our poorly supported pawns
    count = popcount(pawns & ~attacksByPawns & poorlyDefended);
    addTo(&eval, intMult(&ThreatWeakPawn, count));
    if (TRACE) T.ThreatWeakPawn[US] += count;

    // Penalty for pawn threats against our minors
    count = popcount((knights | bishops) & attacksByPawns);
    addTo(&eval, intMult(&ThreatMinorAttackedByPawn, count));
    if (TRACE) T.ThreatMinorAttackedByPawn[US] += count;

    // Penalty for any minor threat against minor pieces
    count = popcount((knights | bishops) & attacksByMinors);
    addTo(&eval, intMult(&ThreatMinorAttackedByMinor, count));
    if (TRACE) T.ThreatMinorAttackedByMinor[US] += count;

    // Penalty for all major threats against poorly supported minors
    count = popcount(weakMinors & attacksByMajors);
    addTo(&eval, intMult(&ThreatMinorAttackedByMajor, count));
    if (TRACE) T.ThreatMinorAttackedByMajor[US] += count;

    // Penalty for pawn and minor threats against our rooks
    count = popcount(rooks & (attacksByPawns | attacksByMinors));
    addTo(&eval, intMult(&ThreatRookAttackedByLesser, count));
    if (TRACE) T.ThreatRookAttackedByLesser[US] += count;

    // Penalty for king threats against our poorly defended minors
    count = popcount(weakMinors & ei->attackedBy[THEM][KING]);
    addTo(&eval, intMult(&ThreatMinorAttackedByKing, count));
    if (TRACE) T.ThreatMinorAttackedByKing[US] += count;

    // Penalty for king threats against our poorly defended rooks
    count = popcount(rooks & poorlyDefended & ei->attackedBy[THEM][KING]);
    addTo(&eval, intMult(&ThreatRookAttackedByKing, count));
    if (TRACE) T.ThreatRookAttackedByKing[US] += count;

    // Penalty for any threat against our queens
    count = popcount(queens & ei->attacked[THEM]);
    addTo(&eval, intMult(&ThreatQueenAttackedByOne, count));
    if (TRACE) T.ThreatQueenAttackedByOne[US] += count;

    // Penalty for any overloaded minors or majors
    count = popcount(overloaded);
    addTo(&eval, intMult(&ThreatOverloadedPieces, count));
    if (TRACE) T.ThreatOverloadedPieces[US] += count;

    // Bonus for giving threats by safe pawn pushes
    count = popcount(pushThreat);
    addTo(&eval, intMult(&ThreatByPawnPush, count));
    if (TRACE) T.ThreatByPawnPush[colour] += count;

    return eval;
}

EvalScore evaluateClosedness(EvalInfo *ei, Board *board) {

    EvalScore eval = SCORE_ZERO;
    int closedness, count;

    uint64_t white = board->colours[WHITE];
    uint64_t black = board->colours[BLACK];

    uint64_t knights = board->pieces[KNIGHT];
    uint64_t rooks   = board->pieces[ROOK  ];

    // Compute Closedness factor for this position
    closedness = 1 * popcount(board->pieces[PAWN])
               + 3 * popcount(ei->rammedPawns[WHITE])
               - 4 * openFileCount(board->pieces[PAWN]);
    closedness = MAX(0, MIN(8, closedness / 3));

    // Evaluate Knights based on how Closed the position is
    count = popcount(white & knights) - popcount(black & knights);
    addTo(&eval, intMult(&ClosednessKnightAdjustment[closedness], count));
    if (TRACE) T.ClosednessKnightAdjustment[closedness][WHITE] += count;

    // Evaluate Rooks based on how Closed the position is
    count = popcount(white & rooks) - popcount(black & rooks);
    addTo(&eval, intMult(&ClosednessRookAdjustment[closedness], count));
    if (TRACE) T.ClosednessRookAdjustment[closedness][WHITE] += count;

    return eval;
}

EvalScore evaluateComplexity(EvalInfo *ei, Board *board, EvalScore eval) {

    // Adjust endgame evaluation based on features related to how
    // likely the stronger side is to convert the position.
    // More often than not, this is a penalty for drawish positions.

    (void) ei; // Silence compiler warning

    EvalScore complexity = SCORE_ZERO;
    int sign = (eval.eg > 0) - (eval.eg < 0);

    int pawnsOnBothFlanks = (board->pieces[PAWN] & LEFT_FLANK )
                         && (board->pieces[PAWN] & RIGHT_FLANK);

    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t queens  = board->pieces[QUEEN ];

    // Compute the initiative bonus or malus for the attacking side
    addTo(&complexity, intMult(&ComplexityTotalPawns, popcount(board->pieces[PAWN])));
    addTo(&complexity, intMult(&ComplexityPawnFlanks, pawnsOnBothFlanks));
    if(!(knights | bishops | rooks | queens))
        addTo(&complexity, ComplexityPawnEndgame); 
    addTo(&complexity, ComplexityAdjustment);

    if (TRACE) T.ComplexityTotalPawns[WHITE]  += sign * popcount(board->pieces[PAWN]);
    if (TRACE) T.ComplexityPawnFlanks[WHITE]  += sign * pawnsOnBothFlanks;
    if (TRACE) T.ComplexityPawnEndgame[WHITE] += sign * !(knights | bishops | rooks | queens);
    if (TRACE) T.ComplexityAdjustment[WHITE]  += sign;

    // Avoid changing which side has the advantage
    int v = sign * MAX(complexity.eg, -abs(eval.eg));

    EvalScore result = { 0, v/2, v };
    return result;
}

int evaluateScaleFactor(Board *board, EvalScore eval) {

    // Scale endgames based on remaining material. Currently, we only
    // look for OCB endgames that include only one Knight or one Rook

    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];
    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t queens  = board->pieces[QUEEN ];

    if (   onlyOne(white & bishops)
        && onlyOne(black & bishops)
        && onlyOne(bishops & WHITE_SQUARES)) {

        if (!(knights | rooks | queens))
            return SCALE_OCB_BISHOPS_ONLY;

        if (   !(rooks | queens)
            &&  onlyOne(white & knights)
            &&  onlyOne(black & knights))
            return SCALE_OCB_ONE_KNIGHT;

        if (   !(knights | queens)
            && onlyOne(white & rooks)
            && onlyOne(black & rooks))
            return SCALE_OCB_ONE_ROOK;
    }

    int eg = eval.eg;

    // Lone minor vs king and pawns, never give the advantage to the side with the minor
    if ( (eg > 0) && popcount(white) == 2 && (white & (knights | bishops)))
        return SCALE_DRAW;
    else if ( (eg < 0) && popcount(black) == 2 && (black & (knights | bishops)))
        return SCALE_DRAW;

    return SCALE_NORMAL;
}

void initEvalInfo(EvalInfo *ei, Board *board, PKTable *pktable) {

    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];

    uint64_t pawns   = board->pieces[PAWN  ];
    uint64_t bishops = board->pieces[BISHOP] | board->pieces[QUEEN];
    uint64_t rooks   = board->pieces[ROOK  ] | board->pieces[QUEEN];
    uint64_t kings   = board->pieces[KING  ];

    // Save some general information about the pawn structure for later
    ei->pawnAttacks[WHITE]  = pawnAttackSpan(white & pawns, ~0ull, WHITE);
    ei->pawnAttacks[BLACK]  = pawnAttackSpan(black & pawns, ~0ull, BLACK);
    ei->rammedPawns[WHITE]  = pawnAdvance(black & pawns, ~(white & pawns), BLACK);
    ei->rammedPawns[BLACK]  = pawnAdvance(white & pawns, ~(black & pawns), WHITE);
    ei->blockedPawns[WHITE] = pawnAdvance(white | black, ~(white & pawns), BLACK);
    ei->blockedPawns[BLACK] = pawnAdvance(white | black, ~(black & pawns), WHITE);

    // Compute an area for evaluating our King's safety.
    // The definition of the King Area can be found in masks.c
    ei->kingSquare[WHITE] = getlsb(white & kings);
    ei->kingSquare[BLACK] = getlsb(black & kings);
    ei->kingAreas[WHITE] = kingAreaMasks(WHITE, ei->kingSquare[WHITE]);
    ei->kingAreas[BLACK] = kingAreaMasks(BLACK, ei->kingSquare[BLACK]);

    // Exclude squares attacked by our opponents, our blocked pawns, and our own King
    ei->mobilityAreas[WHITE] = ~(ei->pawnAttacks[BLACK] | (white & kings) | ei->blockedPawns[WHITE]);
    ei->mobilityAreas[BLACK] = ~(ei->pawnAttacks[WHITE] | (black & kings) | ei->blockedPawns[BLACK]);

    // Init part of the attack tables. By doing this step here, evaluatePawns()
    // can start by setting up the attackedBy2 table, since King attacks are resolved
    ei->attacked[WHITE] = ei->attackedBy[WHITE][KING] = kingAttacks(ei->kingSquare[WHITE]);
    ei->attacked[BLACK] = ei->attackedBy[BLACK][KING] = kingAttacks(ei->kingSquare[BLACK]);

    // For mobility, we allow bishops to attack through each other
    ei->occupiedMinusBishops[WHITE] = (white | black) ^ (white & bishops);
    ei->occupiedMinusBishops[BLACK] = (white | black) ^ (black & bishops);

    // For mobility, we allow rooks to attack through each other
    ei->occupiedMinusRooks[WHITE] = (white | black) ^ (white & rooks);
    ei->occupiedMinusRooks[BLACK] = (white | black) ^ (black & rooks);

    // Init all of the King Safety information
    ei->kingAttacksCount[WHITE]    = ei->kingAttacksCount[BLACK]    = 0;
    ei->kingAttackersCount[WHITE]  = ei->kingAttackersCount[BLACK]  = 0;
    ei->kingAttackersWeight[WHITE] = ei->kingAttackersWeight[BLACK] = 0;

    // Try to read a hashed Pawn King Eval. Otherwise, start from scratch
    ei->pkentry       =     pktable == NULL ? NULL       : getPKEntry(pktable, board->pkhash);
    ei->passedPawns   = ei->pkentry == NULL ? 0ull       : ei->pkentry->passed;
    ei->pkeval[WHITE] = ei->pkentry == NULL ? SCORE_ZERO : ei->pkentry->eval;
    ei->pkeval[BLACK] = ei->pkentry == NULL ? SCORE_ZERO : SCORE_ZERO;
}

void initEval() {

    // Init a normalized 64-length PSQT for the evaluation which
    // combines the Piece Values with the original PSQT Values

    for (int sq = 0; sq < SQUARE_NB; sq++) {

        const int w32 = relativeSquare32(WHITE, sq);
        const int b32 = relativeSquare32(BLACK, sq);

        PSQT[WHITE_PAWN  ][sq] = PawnValue;
        PSQT[WHITE_KNIGHT][sq] = KnightValue;
        PSQT[WHITE_BISHOP][sq] = BishopValue;
        PSQT[WHITE_ROOK  ][sq] = RookValue;
        PSQT[WHITE_QUEEN ][sq] = QueenValue;
        PSQT[WHITE_KING  ][sq] = KingValue;

        PSQT[BLACK_PAWN  ][sq] = neg(&PawnValue);
        PSQT[BLACK_KNIGHT][sq] = neg(&KnightValue);
        PSQT[BLACK_BISHOP][sq] = neg(&BishopValue);
        PSQT[BLACK_ROOK  ][sq] = neg(&RookValue);
        PSQT[BLACK_QUEEN ][sq] = neg(&QueenValue);
        PSQT[BLACK_KING  ][sq] = neg(&KingValue);

        addTo(&PSQT[WHITE_PAWN  ][sq],   PawnPSQT32[w32]);
        addTo(&PSQT[WHITE_KNIGHT][sq], KnightPSQT32[w32]);
        addTo(&PSQT[WHITE_BISHOP][sq], BishopPSQT32[w32]);
        addTo(&PSQT[WHITE_ROOK  ][sq],   RookPSQT32[w32]);
        addTo(&PSQT[WHITE_QUEEN ][sq],  QueenPSQT32[w32]);
        addTo(&PSQT[WHITE_KING  ][sq],   KingPSQT32[w32]);

        addTo(&PSQT[BLACK_PAWN  ][sq],   neg(&PawnPSQT32[b32]));
        addTo(&PSQT[BLACK_KNIGHT][sq], neg(&KnightPSQT32[b32]));
        addTo(&PSQT[BLACK_BISHOP][sq], neg(&BishopPSQT32[b32]));
        addTo(&PSQT[BLACK_ROOK  ][sq],   neg(&RookPSQT32[b32]));
        addTo(&PSQT[BLACK_QUEEN ][sq],  neg(&QueenPSQT32[b32]));
        addTo(&PSQT[BLACK_KING  ][sq],   neg(&KingPSQT32[b32]));
    }
}
