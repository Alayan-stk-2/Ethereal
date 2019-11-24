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

const int EarlyPhaseArray[25] = {
    1000, 917, 833, 768, 709, 655, 602, 549,
     495, 442, 389, 335, 282, 230, 188, 150,
     114,  82,  52,  26,   6,   0,   0,   0,   0,
};
const int LatePhaseArray[25] = {
       0,   0,   0,  18,  42,  72, 102, 132,
     162, 192, 222, 252, 282, 313, 355, 400,
     447, 499, 552, 609, 673, 750, 833, 917, 1000, 
};

/* Material Value Evaluation Terms */

EvalScore PawnValue   = {  97,  95, 109};
EvalScore KnightValue = { 398, 430, 374};
EvalScore BishopValue = { 427, 434, 392};
EvalScore RookValue   = { 600, 647, 628};
EvalScore QueenValue  = {1242,1258,1274};
EvalScore KingValue   = {   0,   0,   0};

/* Piece Square Evaluation Terms */

EvalScore PawnPSQT32[32] = {
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
    { -14, -18,   9}, {   5, -10,   5}, { -12,  13,  -7}, {  -6,   5,  -5}, 
    { -15, -24,   8}, {  -8, -11,   5}, {  -7, -11,  -2}, {  -3,   2, -17}, 
    { -14, -11,  10}, { -15,   6,   4}, {  14, -23,   0}, {   9, -19, -15}, 
    {  -7,   1,   6}, {  -5,   8,   1}, {  -9,   7, -16}, {  10, -24, -16}, 
    { -15,  21,  12}, { -14,  19,   8}, {  -9,  13,   2}, {  17,  10, -24}, 
    { -18, -32, -58}, { -64, -39, -30}, {  -1, -15, -43}, {  37,  -5, -56}, 
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
};

EvalScore KnightPSQT32[32] = {
    { -46, -36, -27}, {  -4, -27, -21}, { -10, -17, -26}, {   4,  -8, -14}, 
    {   0, -12, -21}, {   3,  -5, -12}, {   3, -13, -27}, {  10,   2, -23}, 
    {   7, -14, -21}, {  18,  -3, -24}, {  12,   2, -22}, {  21,  16, -13}, 
    {  15,   0,  10}, {  18,  12,   5}, {  23,  21,  10}, {  25,  31,  13}, 
    {  20,  18,  14}, {  20,  20,   4}, {  29,  32,  19}, {  19,  45,  21}, 
    { -17,   0,  12}, {  -6,   6,  10}, {  23,  26,  21}, {  18,  27,  23}, 
    {   5,  -4, -11}, { -11,  -3,  -2}, {  21,   7, -23}, {  39,  18,  -6}, 
    {-174, -94, -18}, { -81, -41,  -2}, {-110, -45,  17}, { -31, -16,  -2}, 
};

EvalScore BishopPSQT32[32] = {
    {  16,  -5, -17}, {  10,  -5, -19}, { -11,  -9,  -9}, {   9,  -6, -13}, 
    {  25,  -8, -33}, {  18, -18, -20}, {  16,  -5, -21}, {   6,   4, -21}, 
    {  12, -12,  -9}, {  21,   2, -20}, {   9,  12, -20}, {  16,   3, -11}, 
    {  11,  -5,  -9}, {  10,   7, -10}, {   8,  18, -13}, {  14,  16,  -5}, 
    { -16,  -2,   8}, {  10,  11,  -5}, {  -3,  10,   2}, {  -1,  19,   3}, 
    {  -4,   1,   2}, {  -8,   5,   9}, {   5,   8,   6}, {   7,   8,   4}, 
    { -47,   8,   7}, { -42, -12,   6}, { -14,  -3,  -1}, { -23,  -9,   4}, 
    { -42, -21,   0}, { -47, -21,   5}, { -87, -36,  11}, { -90, -34,  20}, 
};

EvalScore RookPSQT32[32] = {
    { -13, -29, -17}, { -19,  -6, -27}, {  -5, -13, -23}, {   1,  -6, -30}, 
    { -51, -23, -17}, { -13, -19, -30}, {  -6, -23, -26}, {   2, -11, -34}, 
    { -26, -25, -11}, {  -7, -13,  -7}, { -14, -15, -15}, {   1,  -9, -24}, 
    { -17, -15,  -1}, { -12,   0,   4}, {  -7,  -4,  -1}, {   4,   2,  -7}, 
    {  -9,  -1,   4}, {   6,   7,   1}, {  16,  10,  -4}, {  32,  15,  -8}, 
    { -22,  -1,  13}, {  17,  12,  -3}, {  -6,   6,  13}, {  25,  13,  -4}, 
    {  -4,   1,  -2}, { -22,  -2,   8}, {   4,   3,  -3}, {  19,  11,  -1}, 
    {  29,  23,  13}, {  21,  21,  15}, {   4,  15,  21}, {  15,  18,  17}, 
};

EvalScore QueenPSQT32[32] = {
    {  32, -14, -54}, {  12, -17, -41}, {  17, -19, -49}, {  21, -14, -33}, 
    {  15, -13, -40}, {  26, -16, -56}, {  29, -23, -63}, {  18,  -2, -30}, 
    {  10,  -9, -26}, {  25,  -7, -22}, {   6,   7,  -7}, {  10,   0,  -9}, 
    {  12,  -9,  -5}, {  10,  11,   8}, {  -1,   3,   8}, { -16,  28,  35}, 
    {  -9,  -7,  16}, {  -9,  11,  33}, { -20,  -4,  11}, { -36,  20,  52}, 
    { -31,  -3,  26}, { -24,  -3,  13}, { -32,  -7,  13}, { -24,  -3,  15}, 
    {  -9,   9,  23}, { -64,   0,  56}, { -16,  -4,   7}, { -49,  -2,  44}, 
    { -14,   0,  11}, {  13,   7,   3}, {   4,   3,   2}, { -10,   1,  12}, 
};

EvalScore KingPSQT32[32] = {
    {  44, -30, -57}, {  38,  -3, -43}, { -10, -13,  -5}, { -23, -50,  10}, 
    {  26,  11, -25}, {  -6,  -3, -14}, { -41,   8,   3}, { -54,  -9,  11}, 
    {   7,  -7, -24}, {  12,   1, -20}, {  15,   7,   4}, {  -8,   5,  18}, 
    {   4, -14, -32}, {  85,  25, -35}, {  43,  19,   8}, {   0,   6,  28}, 
    {   6,  -7, -19}, {  95,  33, -35}, {  48,  29,   7}, {   6,   8,  20}, 
    {  46,   8, -33}, { 120,  50, -31}, {  96,  48,  -7}, {  40,  18,  -5}, 
    {   6, -24, -56}, {  46,  19, -17}, {  32,  18,  -2}, {   9,   2, -10}, 
    {   9, -48,-105}, {  75,   9, -58}, { -19, -17, -19}, { -18, -16, -15}, 
};

/* Pawn Evaluation Terms */

EvalScore PawnCandidatePasser[2][8] = {
   {{   0,   0,   0}, { -25, -17,  -9}, { -11,  -1,  10}, { -15,   5,  28}, {  -3,  29,  56}, {  37,  52,  63}, {   0,   0,   0}, {   0,   0,   0}},
   {{   0,   0,   0}, { -14,   1,  19}, {  -4,   5,  23}, {   4,  23,  42}, {  14,  51,  76}, {  31,  42,  54}, {   0,   0,   0}, {   0,   0,   0}},
};

EvalScore PawnIsolated = {  -7,  -9, -10};

EvalScore PawnStacked[2] = {
    { -14,   1, -24}, { -11,   5, -13}, 
};

EvalScore PawnBackwards[2] = {
    {   6,  -3,  -3}, {  -5, -17, -20}, 
};

EvalScore PawnConnected32[32] = {
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
    {  -3,  -2,  -5}, {  12,  -5,   7}, {   3,   2,  -3}, {   5,   9,   9}, 
    {   8,  11,  -3}, {  23,  -2,   3}, {  16,  16,   0}, {  18,  20,   3}, 
    {   6,   7,  -4}, {  21,   2,   5}, {   7,  15,   3}, {  12,  21,   8}, 
    {   6,   7,   8}, {  19,  11,  12}, {  20,  23,  16}, {  29,   6,  25}, 
    {  50,  40,  18}, {  39,  48,  40}, {  59,  58,  45}, {  75,  66,  52}, 
    { 112,  56,  -1}, { 204, 107,   7}, { 228, 129,  29}, { 240, 145,  47}, 
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
};

/* Knight Evaluation Terms */

EvalScore KnightOutpost[2][2] = {
   {{   7,  -2, -28}, {  29,  20, -10}},
   {{  -2,  -7, -23}, {  13,   2,  -5}},
};

EvalScore KnightBehindPawn = {   2,  18,  10};

EvalScore KnightMobility[9] = {
    { -74, -89,-104}, { -31, -63, -96}, { -16, -28, -41}, {  -5, -10, -16},
    {   6,  -1,  -8}, {  11,   9,   8}, {  19,  15,  11}, {  28,  19,  11},
    {  40,  18,  -3},
};

/* Bishop Evaluation Terms */

EvalScore BishopPair = {  15,  48,  63};

EvalScore BishopRammedPawns = { -13,   0, -17};

EvalScore BishopOutpost[2][2] = {
   {{   8,   5, -15}, {  37,  24,  -4}},
   {{   4,  -2, -10}, {  10,   5,  -2}},
};

EvalScore BishopBehindPawn = {   3,  10,  15};


EvalScore BishopMobility[14] = {
    { -65,-106,-147}, { -30, -62, -95}, { -11, -33, -56}, {  -1, -15, -30},
    {   9,  -4, -18}, {  17,   6,  -4}, {  20,  13,   6}, {  21,  16,  11},
    {  20,  18,  17}, {  24,  21,  18}, {  23,  21,  19}, {  42,  25,   8},
    {  42,  30,  18}, {  71,  28, -14},
};

/* Rook Evaluation Terms */

EvalScore RookFile[2] = { {  13,   1,   6}, {  23,  36, -10}, };

EvalScore RookOnSeventh = {  -8,  12,  26};

EvalScore RookMobility[15] = {
    {-148,-130,-113}, { -52, -82,-113}, { -15, -38, -61}, {  -7, -14, -21},
    {  -7,  -4,  -1}, {  -8,   3,  14}, {  -7,   8,  24}, {  -1,  13,  27},
    {   6,  18,  30}, {  10,  22,  34}, {  13,  26,  40}, {  18,  30,  43},
    {  19,  33,  47}, {  30,  34,  39}, {  80,  42,   4},
};

/* Queen Evaluation Terms */

EvalScore QueenMobility[28] = {
    { -61,-162,-263}, {-210,-298,-387}, { -58,-129,-201}, { -16,-104,-192},
    {  -4, -71,-139}, {   0, -42, -84}, {   5, -23, -52}, {   5,  -9, -23},
    {   9,  -3, -16}, {  10,  11,   6}, {  13,  13,  13}, {  14,  21,  28},
    {  16,  20,  24}, {  17,  25,  33}, {  15,  26,  37}, {  12,  26,  41},
    {  11,  27,  44}, {   3,  24,  45}, {   4,  23,  42}, {  -1,  18,  37},
    {   7,  11,  16}, {  22,  16,  -6}, {  27,  -3, -34}, {  30, -10, -51},
    {  12, -28, -68}, {  24, -35, -95}, { -56, -47, -39}, { -31, -46, -61},
};

/* King Evaluation Terms */

EvalScore KingDefenders[12] = {
    { -23, -11,   4}, {  -2, -13,   1}, {   3,  -5,   5}, {   7,   8,   3}, 
    {  15,  18,  -1}, {  24,  18,   2}, {  28,  14,  -2}, {  13,   6,   0}, 
    {  12,   9,   6}, {  12,   9,   6}, {  12,   9,   6}, {  12,   9,   6}, 
};

EvalScore KingPawnFileProximity[8] = {
    {  33,  38,  28}, {  12,  26,  20}, {   6,   4,  14}, { -14, -19, -11}, 
    { -17, -35, -47}, { -16, -41, -64}, { -15, -42, -70}, { -11, -40, -70}, 
};

EvalScore KingShelter[2][8][8] = {
  {{{ -10,  -5,   1}, {  14,  -8, -24}, {  19,   6,  -9}, {  12,   9,   4}, {   7,   5,   4}, {   0,   1,   2}, {  -3, -18, -33}, { -49, -12,  16}},
   {{  17,   3,  -9}, {  17,   1, -16}, {   2,  -2,  -5}, { -14,  -5,   3}, { -30,  -7,  15}, { -70,  -1,  67}, {  92,  87,  82}, { -22, -10,   0}},
   {{  34,  14,  -6}, {  12,   4, -10}, { -27,  -8,   5}, {  -9,  -9,  -8}, { -19, -12,  -5}, { -12,  -5,   1}, {   0,  33,  66}, { -11,  -5,  -3}},
   {{   3,   8,   8}, {  21,   3, -11}, {   5, -10,  -8}, {  13,  -8, -15}, {  23,  -8, -32}, { -58, -26,   5}, {-136, -42,  52}, {   9,  -4,  -7}},
   {{ -16,   0,   5}, {   3,   2,  -5}, { -25,  -9,  -1}, { -16,  -4,   2}, { -17, -12,  -7}, { -42, -22,  -2}, {  33,   8, -17}, {  -5,  -2,  -3}},
   {{  40,  15, -18}, {  21,  -5, -12}, { -19, -10,   2}, { -12, -14, -13}, {   4, -10, -25}, {  17,  -2, -20}, {  41,   5, -30}, { -20, -15,   2}},
   {{  21,  17, -21}, {  -2,  -9, -14}, { -21, -13,   0}, { -19, -14,  -6}, { -29, -18,  -6}, { -37,  -2,  31}, {   0,  22,  44}, {  -7,  -8,   1}},
   {{  -7, -10, -13}, {   6, -12, -14}, {   5,   6,   0}, {  -4,   9,   8}, { -12,   1,  15}, { -10,  14,  36}, {-190, -51,  87}, { -18,   4,  10}}},
  {{{   0,   0,   0}, { -11, -16, -24}, {   5,  -6, -19}, { -40, -12,  16}, { -23, -10,   2}, {   3,  22,  42}, {-167, -87,  -8}, { -44, -16,  10}},
   {{   0,   0,   0}, {  20,   0, -22}, {   6,  -1,  -8}, { -18,  -9,  -1}, {  -1,  -6, -12}, {  26,  45,  65}, {-184, -93,  -3}, { -36, -14,   4}},
   {{   0,   0,   0}, {  24,   8, -14}, {  -2,  -6,  -9}, {   5,  -6, -18}, {  15,   4,  -7}, { -87, -20,  47}, { -84, -78, -73}, { -14, -11,  -1}},
   {{   0,   0,   0}, {  -6,   3,   5}, {  -5,  -2,  -4}, { -19,  -7,   0}, { -27, -13,   0}, { -99, -34,  31}, {   7, -17, -41}, { -13, -10,  -1}},
   {{   0,   0,   0}, {  13,   0,   5}, {  12,  -7,  -1}, {  12,   1, -12}, {  12,  -4, -25}, { -57, -21,  15}, {-104, -82, -61}, {   4,  -4,  -1}},
   {{   0,   0,   0}, {   2,   3, -11}, { -19, -10,  -2}, { -25, -16,  -7}, {  17,  -3, -24}, { -38, -17,   3}, {  55,  46,  38}, { -15,  -7,  -5}},
   {{   0,   0,   0}, {  17,  11, -21}, {  10,  -2, -12}, {  -9, -12,  -5}, { -25, -11,  10}, {  -9,   3,  15}, { -56, -52, -49}, { -29,  -5,   9}},
   {{   0,   0,   0}, {  11, -16, -35}, {  17,  -5, -27}, { -18, -12,  -5}, { -18,  -1,  18}, {  -6,   7,  20}, {-228,-141, -55}, { -20, -10,   3}}},
};

EvalScore KingStorm[2][4][8] = {
  {{{  -4,  10,  28}, { 115,  52, -12}, { -23,  -1,  24}, { -17,  -6,   7}, { -14,  -7,   3}, {  -9,  -5,  -4}, { -18,   0,  -1}, { -23,  -8,   2}},
   {{  -3,  22,  48}, {  56,  32,   5}, { -18,   1,  20}, {  -5,   7,   8}, {  -8,   9,   0}, {   4,  -2,  -3}, {  -2,  -4,   3}, { -14,   3,   2}},
   {{   7,  22,  38}, {  16,  18,  18}, { -22,   0,  17}, { -13,   2,   5}, {   1,   2,   0}, {   5,   4,  -2}, {   9,  -3,   1}, {   1,   6,   1}},
   {{  -2,  10,  26}, {  14,  15,  16}, { -18,  -7,   7}, { -11, -12,   1}, { -12, -14,   4}, {   6,  -4,  -7}, {   0,  -2,  -6}, { -10,  -1,   2}}},
  {{{   0,   0,   0}, { -15, -15, -16}, { -15, -13,   2}, {  15,   0, -14}, {   5,   5,  -6}, {   4,  -7, -17}, {  -2,  -1,  -1}, {  17,  23,  29}},
   {{   0,   0,   0}, { -16, -25, -35}, {   1,  -7,  -6}, {  35,  14, -10}, {   0,   0,  -2}, {  14,  -7, -20}, {  -7,  -8, -10}, { -17,  -7,   2}},
   {{   0,   0,   0}, { -28, -38, -50}, { -23, -15,  -5}, {  11,   2, -11}, {   5,  -2,  -1}, {  -6, -12, -13}, { -13, -14, -14}, { -11,  -2,   6}},
   {{   0,   0,   0}, {  -2, -10, -18}, { -14, -15, -16}, {  -9,  -6,  -6}, {  -2,  -9,  -3}, {   2,  -9, -21}, {  72,  29, -11}, {  13,  16,  20}}},
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

EvalScore PassedPawn[2][2][RANK_NB] = {
  {{{   0,   0,   0}, { -38, -15,   4}, { -54, -16,  21}, { -80, -23,  28}, {  -5,   6,  12}, {  72,  35,  -7}, { 159, 108,  54}, {   0,   0,   0}},
   {{   0,   0,   0}, { -30, -11,   8}, { -52, -12,  26}, { -71, -19,  33}, { -11,  11,  35}, {  91,  59,  35}, { 182, 142,  98}, {   0,   0,   0}}},
  {{{   0,   0,   0}, { -24,  -4,  19}, { -51, -12,  22}, { -73, -16,  34}, {  -5,  23,  37}, {  85,  70,  39}, { 258, 189, 109}, {   0,   0,   0}},
   {{   0,   0,   0}, { -30,  -9,  15}, { -45, -15,  22}, { -63, -16,  41}, {  -2,  26,  53}, {  93, 101, 108}, { 162, 211, 261}, {   0,   0,   0}}},
};

EvalScore PassedFriendlyDistance[8] = {
    {   0,   0,   0}, {   2,  -5,   3}, {   7,  -8,   1}, {  11, -11,  -4}, 
    {   7,  -6, -10}, { -12,  -2, -13}, { -23,   4, -13}, {   0,   0,   0}, 
};

EvalScore PassedEnemyDistance[8] = {
    {   0,   0,   0}, {   2,   4,  -1}, {   4,   8,  -1}, {   7,  14,   4}, 
    {  -2,  17,  14}, {   4,  20,  22}, {  18,  28,  17}, {   0,   0,   0}, 
};

EvalScore PassedSafePromotionPath = { -19,  -1,  32};

/* Threat Evaluation Terms */

EvalScore ThreatWeakPawn             = { -12, -14, -26};
EvalScore ThreatMinorAttackedByPawn  = { -45, -55, -50};
EvalScore ThreatMinorAttackedByMinor = { -21, -44, -22};
EvalScore ThreatMinorAttackedByMajor = { -20, -35, -43};
EvalScore ThreatRookAttackedByLesser = { -41, -38, -19};
EvalScore ThreatMinorAttackedByKing  = { -24, -21, -24};
EvalScore ThreatRookAttackedByKing   = { -17, -21, -27};
EvalScore ThreatQueenAttackedByOne   = { -32, -42, -35};
EvalScore ThreatOverloadedPieces     = {  -7, -11, -10};
EvalScore ThreatByPawnPush           = {  12,  21,  17};


/* Closedness Evaluation Terms */

EvalScore ClosednessKnightAdjustment[9] = {
    { -16,  -9, -21}, { -14,  -3,   2}, { -14,   1,  10}, {  -8,   5,  11}, 
    {  -5,   9,  17}, {  -1,   9,  15}, {   2,   8,  12}, { -10,  12,  28}, 
    { -10,  -8,  16}, 
};

EvalScore ClosednessRookAdjustment[9] = {
    {  49,  33, -37}, {   3,  12,  22}, {  -5,   2,   9}, { -13,  -8,  -1}, 
    { -19, -10,  -2}, { -20, -15, -12}, { -20, -17, -14}, { -25, -21, -17}, 
    { -29, -23, -17}, 
};

/* Complexity Evaluation Terms */

EvalScore ComplexityTotalPawns  = {   0,   0,   7};
EvalScore ComplexityPawnFlanks  = {   0,   0,  49};
EvalScore ComplexityPawnEndgame = {   0,   0,  34};
EvalScore ComplexityAdjustment  = {   0,   0,-110};

/* General Evaluation Terms */

const int Tempo = 20;

#undef S

int evaluateBoard(Board *board, PKTable *pktable) {

    EvalInfo ei;
    EvalScore eval = SCORE_ZERO, pkeval = SCORE_ZERO;;
    int interpolatedEval, phase, earlyPhase, latePhase, factor;

    // Setup and perform all evaluations
    initEvalInfo(&ei, board, pktable);
    eval   = evaluatePieces(&ei, board);
    pkeval = ei.pkeval[WHITE];
    addTo(&pkeval, neg(&ei.pkeval[BLACK]));
    addTo(&eval, pkeval);
    addTo(&eval, board->psqtmat);
    addTo(&eval, evaluateClosedness(&ei, board));
    addTo(&eval, evaluateComplexity(&ei, board, eval));

    // Calculate the game phase based on remaining material (Fruit Method)
    phase = 24 - 4 * popcount(board->pieces[QUEEN ])
               - 2 * popcount(board->pieces[ROOK  ])
               - 1 * popcount(board->pieces[KNIGHT]
                             |board->pieces[BISHOP]);

    phase = MAX(0, MIN(24, phase));
    earlyPhase = EarlyPhaseArray[phase];
    latePhase  = LatePhaseArray[phase];

    // Scale evaluation based on remaining material
    factor = evaluateScaleFactor(board, eval);

    // Compute the interpolated and scaled evaluation
    interpolatedEval = eval.og * earlyPhase
                     + eval.mg * (1000 - earlyPhase - latePhase)
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
        int KD_EG = count / 20;

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
