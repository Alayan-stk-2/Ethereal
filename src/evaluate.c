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

EvalScore PawnValue   = {  94, 106, 101};
EvalScore KnightValue = { 379, 474, 360};
EvalScore BishopValue = { 403, 473, 368};
EvalScore RookValue   = { 585, 691, 620};
EvalScore QueenValue  = {1299,1244,1265};
EvalScore KingValue   = {   0,   0,   0};

/* Piece Square Evaluation Terms */

EvalScore PawnPSQT32[32] = {
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
    { -16, -24,   7}, {   1, -17,   4}, { -14,  13, -13}, { -10,   8, -13}, 
    { -17, -31,   8}, { -11, -17,   5}, { -10, -18,  -2}, {  -6,  -1, -19}, 
    { -17, -17,   8}, { -19,   2,   0}, {  10, -33,   4}, {   5, -27, -12}, 
    { -11,  -2,   2}, { -11,   4,  -4}, { -14,   4, -20}, {   5, -31, -16}, 
    { -21,  30,   3}, { -22,  28,  -1}, { -19,  19,  -5}, {   8,  10, -27}, 
    { -17, -32, -62}, { -60, -38, -38}, {  -3, -17, -51}, {  34,  -9, -66}, 
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
};

EvalScore KnightPSQT32[32] = {
    { -43, -35, -29}, {  -6, -30, -17}, { -12, -18, -29}, {   2, -10, -17}, 
    {  -2, -13, -24}, {   0,  -7, -15}, {  -1, -14, -31}, {   6,   2, -30}, 
    {   4, -16, -23}, {  14,  -5, -29}, {   7,   1, -29}, {  16,  16, -22}, 
    {  11,  -6,   9}, {  13,   9,   1}, {  17,  20,   2}, {  19,  31,   4}, 
    {  16,  15,   8}, {  14,  19,  -4}, {  22,  31,  11}, {  12,  48,   7}, 
    { -20,  -1,   7}, { -12,   6,   3}, {  17,  24,  14}, {  10,  24,  16}, 
    {   2,  -6, -13}, { -16,  -6,  -7}, {  12,   5, -29}, {  35,  16, -13}, 
    {-179, -96, -20}, { -81, -41,  -4}, {-111, -45,  12}, { -33, -17,  -7}, 
};

EvalScore BishopPSQT32[32] = {
    {  11, -10, -17}, {   3,  -9, -19}, { -17,  -6, -15}, {   3,  -8, -15}, 
    {  18, -12, -33}, {  10, -21, -19}, {   8,  -7, -23}, {  -1,   6, -27}, 
    {   6, -17,  -8}, {  13,   3, -25}, {   1,  16, -29}, {   8,   4, -17}, 
    {   5,  -9, -10}, {   3,   8, -16}, {   0,  21, -23}, {   6,  19, -14}, 
    { -22,  -4,   3}, {   2,  14, -13}, { -12,  13,  -6}, { -10,  23,  -7}, 
    { -10,  -1,  -2}, { -16,   5,   3}, {  -5,   7,   1}, {  -2,   5,   0}, 
    { -51,   3,   1}, { -49, -12,   0}, { -22,  -5,  -6}, { -28, -13,   1}, 
    { -45, -24,  -3}, { -46, -23,   1}, { -88, -37,   6}, { -90, -34,  15}, 
};

EvalScore RookPSQT32[32] = {
    { -19, -38, -21}, { -25,  -9, -40}, { -12, -19, -31}, {  -5, -14, -37}, 
    { -56, -24, -29}, { -19, -24, -39}, { -11, -30, -31}, {  -3, -16, -42}, 
    { -30, -35, -15}, { -11, -24, -11}, { -19, -21, -23}, {  -3, -17, -31}, 
    { -22, -27,  -6}, { -18,  -6,  -5}, { -13, -11, -10}, {  -1,  -6, -15}, 
    { -16,  -8,  -7}, {  -2,   4, -11}, {   8,   5, -15}, {  26,   6, -16}, 
    { -31,  -5,   1}, {   6,  10, -17}, { -16,   3,   1}, {  15,   6, -14}, 
    { -12,  -1, -17}, { -31,  -1,  -9}, {  -3,  -2, -16}, {  11,   8, -15}, 
    {  23,  14,   2}, {  17,  14,   4}, {   1,   9,   9}, {  11,  12,   5}, 
};

EvalScore QueenPSQT32[32] = {
    {  34, -18, -57}, {  12, -20, -44}, {  16, -21, -51}, {  19, -20, -29}, 
    {  14, -16, -42}, {  24, -19, -58}, {  27, -30, -58}, {  16,  -3, -37}, 
    {   8, -13, -29}, {  24, -14, -23}, {   5,   3, -15}, {   8,  -4, -15}, 
    {  11, -18,  -6}, {   8,   6,   1}, {  -2,  -4,   0}, { -17,  26,  18}, 
    { -12, -16,  15}, { -11,   4,  27}, { -22, -11,   3}, { -37,  15,  38}, 
    { -35,  -6,  23}, { -30,  -7,   9}, { -35, -13,   6}, { -28, -11,   7}, 
    { -14,   6,  20}, { -66,  -5,  49}, { -20, -10,   0}, { -53,  -9,  37}, 
    { -20,  -4,   7}, {   8,   2,  -2}, {   0,  -4,  -4}, { -13,  -5,   6}, 
};

EvalScore KingPSQT32[32] = {
    {  44, -37, -40}, {  40,  -9, -35}, {  -9, -13,  -2}, { -24, -55,  19}, 
    {  25,  14, -22}, {  -4,  -4,  -9}, { -43,  13,   3}, { -59,  -2,  12}, 
    {   9,  -6, -18}, {   6,   5, -17}, {  13,   5,  10}, { -10,   9,  21}, 
    {   6, -10, -29}, {  85,  29, -33}, {  44,  19,  12}, {   4,   6,  32}, 
    {   7,  -4, -18}, {  95,  36, -35}, {  49,  32,   7}, {   8,   8,  22}, 
    {  47,   7, -39}, { 120,  52, -36}, {  97,  49, -12}, {  41,  17,  -9}, 
    {   6, -28, -68}, {  45,  17, -24}, {  31,  16, -10}, {   9,   0, -18}, 
    {   9, -52,-113}, {  74,   6, -65}, { -20, -20, -26}, { -19, -18, -20}, 
};

/* Pawn Evaluation Terms */

EvalScore PawnCandidatePasser[2][8] = {
   {{   0,   0,   0}, { -23, -16,  -8}, { -10,  -2,  11}, { -14,   2,  28}, {  -4,  27,  51}, {  32,  47,  58}, {   0,   0,   0}, {   0,   0,   0}},
   {{   0,   0,   0}, { -13,   1,  22}, {  -3,   0,  27}, {   4,  20,  42}, {  12,  53,  69}, {  29,  41,  54}, {   0,   0,   0}, {   0,   0,   0}},
};

EvalScore PawnIsolated = {  -7,  -9, -10};

EvalScore PawnStacked[2] = {
    { -17,   8, -31}, { -11,  12, -16}, 
};

EvalScore PawnBackwards[2] = {
    {   5,  -4,  -5}, {  -5, -12, -26}, 
};

EvalScore PawnConnected32[32] = {
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
    {  -3,   1,  -6}, {  11,  -8,   9}, {   3,   2,  -3}, {   5,   8,   7}, 
    {   7,  14,  -4}, {  21,  -2,   3}, {  15,  17,  -3}, {  16,  24,  -3}, 
    {   5,   7,  -4}, {  20,   1,   6}, {   6,  17,   0}, {  11,  24,   3}, 
    {   5,   6,   8}, {  18,  10,  11}, {  19,  20,  16}, {  27,   3,  26}, 
    {  44,  41,  12}, {  32,  51,  34}, {  53,  58,  40}, {  69,  63,  50}, 
    { 112,  56,  -3}, { 204, 106,   3}, { 228, 129,  28}, { 239, 144,  44}, 
    {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, {   0,   0,   0}, 
};

/* Knight Evaluation Terms */

EvalScore KnightOutpost[2][2] = {
   {{   5,   3, -32}, {  27,  27, -20}},
   {{  -6,  -2, -22}, {  11,   0,  -5}},
};

EvalScore KnightBehindPawn = {   2,  20,   4};

EvalScore KnightMobility[9] = {
    { -74, -89,-104}, { -31, -63, -96}, { -16, -28, -41}, {  -5, -10, -16},
    {   6,  -1,  -8}, {  11,   9,   8}, {  19,  15,  11}, {  28,  19,  11},
    {  40,  18,  -3},
};

/* Bishop Evaluation Terms */

EvalScore BishopPair = {  12,  58,  48};

EvalScore BishopRammedPawns = { -15,  10, -21};

EvalScore BishopOutpost[2][2] = {
   {{   6,  10, -20}, {  34,  28, -10}},
   {{   3,   0,  -9}, {  -1,   0,  -4}},
};

EvalScore BishopBehindPawn = {   3,   9,  15};

EvalScore BishopMobility[14] = {
    { -65,-106,-147}, { -30, -62, -95}, { -11, -33, -56}, {  -1, -15, -30},
    {   9,  -4, -18}, {  17,   6,  -4}, {  20,  13,   6}, {  21,  16,  11},
    {  20,  18,  17}, {  24,  21,  18}, {  23,  21,  19}, {  42,  25,   8},
    {  42,  30,  18}, {  71,  28, -14},
};

/* Rook Evaluation Terms */

EvalScore RookFile[2] = {    {  11,   1,   6}, {  17,  48, -25}, };

EvalScore RookOnSeventh = { -16,  26,  12};

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
    { -22, -10,   9}, {   0, -15,   3}, {   4,  -8,   8}, {   8,   7,   4}, 
    {  14,  21,  -7}, {  23,  20,  -2}, {  26,  13,  -3}, {  13,   6,   0}, 
    {  12,   9,   6}, {  12,   9,   6}, {  12,   9,   6}, {  12,   9,   6}, 
};

EvalScore KingPawnFileProximity[8] = {
    {  37,  57,  33}, {  11,  44,  21}, {  10,   8,  19}, { -15, -31,  -4}, 
    { -19, -47, -50}, { -18, -51, -72}, { -16, -49, -77}, { -11, -41, -71}, 
};

EvalScore KingShelter[2][8][8] = {
  {{{  -7,  -7,   0}, {  12, -12, -19}, {  18,   7,  -8}, {  12,  10,   3}, {   8,   5,   4}, {  -2,   0,   1}, {  -3, -18, -33}, { -48,  -7,  13}},
   {{  20,  -2,  -7}, {  15,   0, -14}, {   2,  -1,  -4}, { -14,  -4,   2}, { -29,  -7,  14}, { -71,  -2,  66}, {  92,  87,  82}, { -21,  -4,  -3}},
   {{  34,  11,  -7}, {  10,   5, -11}, { -26,  -4,   1}, {  -8,  -8,  -8}, { -17, -11,  -7}, { -13,  -6,   0}, {   0,  33,  66}, { -13,  -1,  -4}},
   {{   2,  11,   4}, {  21,  -1, -10}, {   6, -15,  -5}, {  12, -11, -10}, {  22, -10, -27}, { -58, -26,   6}, {-136, -42,  52}, {  10,  -3,  -7}},
   {{ -17,   5,   2}, {   4,  -1,  -4}, { -25,  -5,  -4}, { -15,   1,  -3}, { -14, -11, -10}, { -44, -23,  -3}, {  33,   8, -17}, {  -7,   2,  -3}},
   {{  38,  11, -16}, {  19,  -6, -10}, { -18,  -8,   1}, { -12, -12, -10}, {   4, -11, -25}, {  17,  -2, -19}, {  41,   5, -30}, { -18, -17,   4}},
   {{  20,  21, -25}, {  -3, -10, -11}, { -20, -13,   2}, { -20, -12,  -5}, { -27, -18,  -5}, { -38,  -3,  30}, {   0,  22,  44}, {  -5, -12,   2}},
   {{  -5, -11, -11}, {   4, -14,  -9}, {   3,   7,   0}, {  -5,  13,   4}, { -13,   0,  16}, { -10,  13,  33}, {-189, -51,  87}, { -17,   4,   9}}},
  {{{   0,   0,   0}, { -11, -15, -25}, {   8,  -6, -21}, { -39, -13,  15}, { -24, -10,   2}, {   3,  22,  42}, {-167, -87,  -8}, { -42, -10,  11}},
   {{   0,   0,   0}, {  18,  -1, -20}, {   6,  -1,  -8}, { -18,  -9,  -1}, {  -1,  -7, -13}, {  25,  44,  64}, {-184, -93,  -3}, { -33, -10,   4}},
   {{   0,   0,   0}, {  20,   8, -16}, {  -2,  -8, -10}, {   3,  -6, -19}, {  14,   3,  -7}, { -86, -20,  47}, { -84, -78, -73}, { -10, -14,   4}},
   {{   0,   0,   0}, { -10,   4,   0}, {  -6,  -2,  -8}, { -20,  -5,  -3}, { -27, -13,  -1}, { -99, -34,  31}, {   7, -17, -41}, {  -9,  -8,   4}},
   {{   0,   0,   0}, {  13,  -5,   7}, {  12, -13,   3}, {  12,   3, -15}, {  11,  -2, -25}, { -57, -21,  15}, {-104, -82, -61}, {   6,  -2,   4}},
   {{   0,   0,   0}, {  -1,   7, -15}, { -17,  -9,  -4}, { -22, -15,  -9}, {  17,  -3, -24}, { -38, -17,   3}, {  55,  46,  38}, { -15,  -6,  -2}},
   {{   0,   0,   0}, {  14,  15, -25}, {   8,  -3, -10}, { -10, -14,  -2}, { -23, -14,  11}, {  -8,   3,  15}, { -56, -52, -49}, { -28,  -1,   6}},
   {{   0,   0,   0}, {  10, -20, -29}, {  15,  -6, -26}, { -17, -14,  -6}, { -19,  -3,  17}, {  -8,   7,  20}, {-228,-141, -55}, { -16, -11,   5}}},
};

EvalScore KingStorm[2][4][8] = {
  {{{  -4,   8,  29}, { 114,  49, -17}, { -17,  -5,  24}, { -15,  -9,   9}, { -15,  -7,   4}, { -10,  -4,  -3}, { -19,   8,  -9}, { -25,  -2,   3}},
   {{  -3,  21,  47}, {  55,  33,  -1}, { -12,  -2,  19}, {  -6,  10,   6}, { -11,  14,  -4}, {   2,  -3,  -2}, {  -4,  -4,   5}, { -14,   6,   3}},
   {{   6,  20,  38}, {  14,  17,  13}, { -19,   0,  15}, { -14,   4,   3}, {  -1,   5,  -3}, {   3,   7,  -4}, {   8,  -5,   6}, {   0,   8,   2}},
   {{  -2,   8,  26}, {  11,   9,  11}, { -17, -11,   5}, { -10, -16,   1}, { -11, -16,   4}, {   5,  -4,  -5}, {  -2,   2,  -6}, { -10,   7,  -1}}},
  {{{   0,   0,   0}, { -16, -16, -16}, { -12, -20,   8}, {  13,  -3,  -9}, {   0,   9,  -7}, {   4,  -7, -15}, {   0,   0,  -2}, {  18,  23,  29}},
   {{   0,   0,   0}, { -16, -26, -36}, {   9, -16,   0}, {  33,  17,  -9}, {  -1,   4,  -3}, {  13,  -9, -16}, {  -6,  -8, -10}, { -17,  -8,   2}},
   {{   0,   0,   0}, { -28, -39, -51}, { -18, -17,  -3}, {   8,   5, -11}, {   5,  -3,   0}, {  -5, -13, -11}, { -12, -13, -13}, { -11,  -2,   5}},
   {{   0,   0,   0}, {  -2, -10, -19}, { -11, -15, -14}, {  -9,   0,  -8}, {  -2,  -9,  -1}, {   0,  -4, -17}, {  71,  27, -12}, {  13,  16,  20}}},
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
  {{{   0,   0,   0}, { -38, -14,   4}, { -52, -15,  21}, { -76, -17,  27}, {  -5,  14,   9}, {  76,  42, -15}, { 167, 110,  46}, {   0,   0,   0}},
   {{   0,   0,   0}, { -31, -11,   9}, { -53, -11,  28}, { -66, -16,  35}, { -11,  14,  39}, {  94,  54,  40}, { 180, 141,  96}, {   0,   0,   0}}},
  {{{   0,   0,   0}, { -24,  -5,  23}, { -52,  -9,  23}, { -70,  -7,  30}, {  -8,  34,  35}, {  79,  80,  35}, { 250, 192, 102}, {   0,   0,   0}},
   {{   0,   0,   0}, { -30,  -9,  17}, { -45, -17,  28}, { -56, -16,  43}, {  -5,  30,  56}, {  93, 101, 104}, { 164, 203, 247}, {   0,   0,   0}}},
};

EvalScore PassedFriendlyDistance[8] = {
    {   0,   0,   0}, {   2,  -6,   5}, {   8, -11,   4}, {  13, -15,   0}, 
    {  10,  -9,  -7}, { -11,  -2, -13}, { -27,  12, -15}, {   0,   0,   0}, 
};

EvalScore PassedEnemyDistance[8] = {
    {   0,   0,   0}, {   2,   5,  -1}, {   3,  11,  -4}, {   4,  18,   0}, 
    {  -5,  21,   8}, {   1,  20,  17}, {  12,  30,  11}, {   0,   0,   0}, 
};

EvalScore PassedSafePromotionPath = {  -4,  -2,  27};

/* Threat Evaluation Terms */

EvalScore ThreatWeakPawn             = { -11, -10, -27};
EvalScore ThreatMinorAttackedByPawn  = { -42, -57, -46};
EvalScore ThreatMinorAttackedByMinor = { -19, -50, -11};
EvalScore ThreatMinorAttackedByMajor = { -18, -37, -41};
EvalScore ThreatRookAttackedByLesser = { -36, -41, -17};
EvalScore ThreatMinorAttackedByKing  = { -33, -24, -29};
EvalScore ThreatRookAttackedByKing   = { -22, -27, -32};
EvalScore ThreatQueenAttackedByOne   = { -27, -49, -39};
EvalScore ThreatOverloadedPieces     = {  -7, -12,  -8};
EvalScore ThreatByPawnPush           = {  11,  22,  15};


/* Closedness Evaluation Terms */

EvalScore ClosednessKnightAdjustment[9] = {
    { -23, -10, -33}, { -20,  -5,  -6}, { -20,   0,   0}, { -14,   4,   2}, 
    { -12,  10,   9}, {  -8,   9,   9}, {  -6,   8,  10}, { -19,  14,  28}, 
    { -19,  -5,  18}, 
};

EvalScore ClosednessRookAdjustment[9] = {
    {  30,  68, -80}, {  -1,   4,  19}, { -10,  -7,   5}, { -20, -17,  -6}, 
    { -29, -17,  -6}, { -32, -23, -17}, { -30, -23, -18}, { -32, -26, -20}, 
    { -35, -27, -19}, 
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
