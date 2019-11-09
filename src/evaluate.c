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

#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "masks.h"
#include "transposition.h"
#include "types.h"

EvalTrace T, EmptyTrace;
int PSQT[32][SQUARE_NB];

#define S(mg, eg) (MakeScore((mg), (eg)))

/* Material Value Evaluation Terms */

const int PawnValue   = S( 210, 236);
const int KnightValue = S( 900, 810);
const int BishopValue = S( 946, 846);
const int RookValue   = S(1338,1390);
const int QueenValue  = S(2590,2760);
const int KingValue   = S(   0,   0);

/* Piece Square Evaluation Terms */

const int PawnPSQT32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
    S( -38,  18), S(  12,   8), S( -22,  14), S( -12,  -2), 
    S( -42,   8), S( -22,   6), S( -16, -10), S(  -4, -26), 
    S( -32,  24), S( -20,  22), S(  28, -26), S(  24, -48), 
    S(  -8,  32), S(   8,  22), S(   0,  -4), S(  28, -42), 
    S(  -8,  64), S(   2,  60), S(  20,  38), S(  76, -16), 
    S( -34, -80), S(-130, -18), S(   6, -46), S(  80, -74), 
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
};

const int KnightPSQT32[32] = {
    S(-100, -52), S( -16, -80), S( -32, -58), S(  -2, -38),
    S( -12, -44), S(   6, -26), S(   2, -62), S(  22, -40),
    S(   6, -50), S(  40, -50), S(  28, -36), S(  50, -10),
    S(  30,   8), S(  46,  12), S(  58,  34), S(  60,  44),
    S(  48,  34), S(  52,  22), S(  78,  52), S(  60,  78),
    S( -28,  30), S(  12,  26), S(  64,  54), S(  66,  58),
    S(  14, -22), S( -10,   4), S(  72, -40), S(  86,   0),
    S(-336, -34), S(-162,  -4), S(-220,  38), S( -60,   2),
};

const int BishopPSQT32[32] = {
    S(  36, -36), S(  30, -40), S( -20, -18), S(  18, -26),
    S(  62, -68), S(  48, -64), S(  46, -44), S(  22, -24),
    S(  32, -28), S(  58, -32), S(  34, -14), S(  42, -12),
    S(  32, -20), S(  36,  -6), S(  32,   8), S(  42,  16),
    S( -22,  20), S(  36,   8), S(  12,  24), S(  22,  38),
    S(   2,  12), S(   0,  30), S(  32,  24), S(  40,  20),
    S( -90,  30), S( -72,  24), S(  -8,  10), S( -40,  16),
    S( -80,   4), S( -96,  16), S(-174,  32), S(-182,  48),
};

const int RookPSQT32[32] = {
    S( -20, -56), S( -28, -38), S(  -4, -46), S(  14, -58),
    S(-106, -26), S( -26, -60), S( -18, -60), S(   2, -66),
    S( -56, -26), S( -14, -22), S( -32, -30), S(  -2, -46),
    S( -32,  -2), S( -14,  12), S( -12,   4), S(  12,  -8),
    S(  -2,  16), S(  28,  10), S(  48,   2), S(  76,  -6),
    S( -22,  36), S(  54,  10), S(   6,  34), S(  70,   2),
    S(   4,  14), S( -34,  32), S(  16,  12), S(  44,  14),
    S(  70,  44), S(  50,  48), S(  12,  58), S(  32,  50),
};

const int QueenPSQT32[32] = {
    S(  46,-104), S(   8, -78), S(  22, -98), S(  40, -82),
    S(  30, -76), S(  56,-112), S(  60,-144), S(  40, -50),
    S(  24, -44), S(  54, -38), S(  18,  10), S(  20,   0),
    S(  26,  -6), S(  30,  34), S(   2,  42), S( -26, 126),
    S( -12,  36), S( -10,  84), S( -28,  44), S( -58, 144),
    S( -46,  56), S( -28,  38), S( -46,  46), S( -24,  52),
    S( -10,  54), S(-120, 128), S( -18,  32), S( -82, 106),
    S( -14,  34), S(  36,  18), S(  18,  18), S( -12,  40),
};

const int KingPSQT32[32] = {
    S(  82,-160), S(  82,-102), S( -22, -26), S( -52, -44),
    S(  60, -60), S(  -6, -40), S( -72,  12), S(-100,  16),
    S(  16, -62), S(  34, -54), S(  34,  -6), S( -18,  24),
    S(   4, -72), S( 166, -74), S(  82,   4), S( -12,  44),
    S(   8, -40), S( 190, -62), S(  92,  20), S(   4,  38),
    S(  92, -44), S( 240, -38), S( 188,  12), S(  76,  10),
    S(  12, -84), S(  94, -10), S(  64,  20), S(  18,   4),
    S(  18,-196), S( 150,-102), S( -38, -22), S( -36, -20),
};

/* Pawn Evaluation Terms */

const int PawnCandidatePasser[2][RANK_NB] = {
   {S(   0,   0), S( -54, -20), S( -22,  18), S( -30,  56),
    S(  -2, 120), S(  88, 132), S(   0,   0), S(   0,   0)},
   {S(   0,   0), S( -28,  34), S( -10,  40), S(   8,  86),
    S(  34, 164), S(  64, 108), S(   0,   0), S(   0,   0)},
};

const int PawnIsolated = S( -14, -22);

const int PawnStacked[2] = { S( -18, -28), S( -18, -18) };

const int PawnBackwards[2] = { S(  14,   0), S( -14, -38) };

const int PawnConnected32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
    S(  -4, -16), S(  22,   2), S(   6,   2), S(  10,  32), 
    S(  28,  -2), S(  58,  -4), S(  42,  14), S(  48,  30), 
    S(  18,   0), S(  46,   8), S(  20,  22), S(  30,  40), 
    S(  26,  16), S(  44,  28), S(  54,  42), S(  64,  40), 
    S( 114,  50), S( 102,  96), S( 136, 108), S( 168, 116), 
    S( 224,   2), S( 408,  22), S( 456,  62), S( 480, 102), 
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
};

/* Knight Evaluation Terms */

const int KnightOutpost[2][2] = {
    { S(  14, -52), S(  62,  -8) },
    { S(   8, -52), S(  30,  -8) },
};

const int KnightBehindPawn = S(   8,  38);

const int KnightMobility[9] = {
    S(-154,-208), S( -64,-200), S( -36, -86), S( -10, -36),
    S(  12, -16), S(  24,  18), S(  42,  22), S(  60,  22),
    S(  84,  -6),
};

/* Bishop Evaluation Terms */

const int BishopPair = S(  44, 138);

const int BishopRammedPawns = S( -20, -30);

const int BishopOutpost[2][2] = {
    { S(  20, -24), S(  80,   0) },
    { S(  10, -24), S(  40,   0) },
};

const int BishopBehindPawn = S(   6,  36);

const int BishopMobility[14] = {
    S(-130,-294), S( -60,-190), S( -22,-112), S(  -2, -60),
    S(  18, -36), S(  34,  -8), S(  40,  12), S(  42,  22),
    S(  40,  34), S(  48,  36), S(  46,  38), S(  84,  16),
    S(  84,  36), S( 142, -28),
};

/* Rook Evaluation Terms */

const int RookFile[2] = { S(  30,   8), S(  70,   6) };

const int RookOnSeventh = S(  -4,  52);

const int RookMobility[15] = {
    S(-296,-226), S(-104,-226), S( -30,-122), S( -14, -42),
    S( -14,  -2), S( -16,  28), S( -14,  48), S(  -2,  54),
    S(  12,  60), S(  20,  68), S(  26,  80), S(  36,  86),
    S(  38,  94), S(  60,  78), S( 160,   8),
};

/* Queen Evaluation Terms */

const int QueenMobility[28] = {
    S(-122,-526), S(-420,-774), S(-116,-402), S( -32,-384),
    S(  -8,-278), S(   0,-168), S(  10,-104), S(  10, -46),
    S(  18, -32), S(  20,  12), S(  26,  26), S(  28,  56),
    S(  32,  48), S(  34,  66), S(  30,  74), S(  24,  82),
    S(  22,  88), S(   6,  90), S(   8,  84), S(  -2,  74),
    S(  14,  32), S(  44, -12), S(  54, -68), S(  60,-102),
    S(  24,-136), S(  48,-190), S(-112, -78), S( -62,-122),
};

/* King Evaluation Terms */

const int KingDefenders[12] = {
    S( -52,   0), S( -14,  -6), S(   2,   4), S(  16,  10),
    S(  34,  12), S(  54,   8), S(  62,  -4), S(  26,   0),
    S(  24,  12), S(  24,  12), S(  24,  12), S(  24,  12),
};

const int KingPawnFileProximity[FILE_NB]  = {
    S(  54,  38), S(  30,  30), S(   6,  20), S( -26, -24),
    S( -30, -80), S( -28,-112), S( -28,-130), S( -22,-140),
};

const int KingShelter[2][FILE_NB][RANK_NB] = {
  {{S( -22,   6), S(  30, -52), S(  40, -18), S(  24,   8),
    S(  12,   8), S(   2,   4), S(  -6, -66), S( -98,  36)},
   {S(  34, -16), S(  40, -36), S(   2, -10), S( -28,   8),
    S( -60,  30), S(-140, 136), S( 184, 164), S( -50,   2)},
   {S(  70,  -8), S(  28, -18), S( -56,  14), S( -22, -16),
    S( -40,  -8), S( -22,   2), S(   0, 132), S( -24,  -4)},
   {S(   8,  22), S(  42, -22), S(   8, -22), S(  30, -44),
    S(  50, -72), S(-116,  10), S(-272, 104), S(  10, -14)},
   {S( -30,  14), S(   8,  -8), S( -52,   2), S( -36,  10),
    S( -40, -12), S( -82,  -2), S(  66, -34), S( -12,  -4)},
   {S(  92, -36), S(  44, -34), S( -42,   4), S( -24, -36),
    S(  10, -48), S(  34, -42), S(  82, -60), S( -48,   2)},
   {S(  50, -26), S(  -2, -32), S( -46,  -4), S( -38, -16),
    S( -60, -14), S( -72,  64), S(   0,  88), S( -20,   2)},
   {S( -18, -24), S(  12, -38), S(  14,   0), S(  -4,  22),
    S( -22,  30), S( -18,  74), S(-380, 174), S( -34,  28)}},
  {{S(   0,   0), S( -22, -46), S(   8, -36), S( -80,  32),
    S( -44,   4), S(   6,  84), S(-334, -16), S( -92,  14)},
   {S(   0,   0), S(  48, -42), S(  14, -14), S( -36,  -2),
    S(  -2, -24), S(  52, 132), S(-368,  -6), S( -78,   6)},
   {S(   0,   0), S(  60, -22), S(  -4, -14), S(  14, -34),
    S(  30, -14), S(-174,  94), S(-168,-146), S( -40, -10)},
   {S(   0,   0), S(  -6,  18), S(  -4,   0), S( -34,   4),
    S( -54,   1), S(-198,  62), S(  14, -82), S( -44, -14)},
   {S(   0,   0), S(  24,   4), S(  22, -10), S(  28, -22),
    S(  28, -52), S(-114,  30), S(-208,-122), S(  -2, -14)},
   {S(   0,   0), S(  12, -16), S( -40,   0), S( -54, -12),
    S(  34, -48), S( -76,   6), S( 110,  76), S( -36, -12)},
   {S(   0,   0), S(  44, -30), S(  22, -26), S( -18, -14),
    S( -54,  18), S( -18,  30), S(-112, -98), S( -62,  22)},
   {S(   0,   0), S(  24, -76), S(  38, -54), S( -36,  -8),
    S( -34,  36), S( -10,  40), S(-456,-110), S( -44,   2)}},
};

const int KingStorm[2][FILE_NB/2][RANK_NB] = {
  {{S(  -8,  56), S( 234, -16), S( -50,  52), S( -38,  16),
    S( -28,   4), S( -16,  -8), S( -34,  10), S( -44,  -4)},
   {S(  -6,  98), S( 114,  24), S( -38,  48), S( -10,  22),
    S(  -8,  10), S(  10,  -8), S(  -2,   0), S( -22,   0)},
   {S(  16,  76), S(  34,  46), S( -46,  42), S( -22,  18),
    S(   6,   4), S(  14,   0), S(  18, -14), S(   6,  -2)},
   {S(  -4,  50), S(  32,  42), S( -34,  18), S( -28,   4),
    S( -26,   4), S(  14, -22), S(   2, -16), S( -26,   4)}},
  {{S(   0,   0), S( -30, -32), S( -34,  -2), S(  36, -34),
    S(  18, -14), S(   6, -36), S(  -6,  -2), S(  34,  58)},
   {S(   0,   0), S( -32, -68), S(  -6, -16), S(  70, -24),
    S(  -2,  -4), S(  26, -46), S( -14, -20), S( -34,   4)},
   {S(   0,   0), S( -56, -98), S( -54, -12), S(  22, -22),
    S(   6,  -4), S( -16, -28), S( -26, -30), S( -22,  12)},
   {S(   0,   0), S(  -4, -36), S( -32, -36), S( -24,  -8),
    S(  -8, -14), S(   8, -52), S( 144, -22), S(  26,  40)}},
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

const int PassedPawn[2][2][RANK_NB] = {
  {{S(   0,   0), S( -76,   6), S(-110,  42), S(-164,  54),
    S( -12,  24), S( 140, -10), S( 314, 112), S(   0,   0)},
   {S(   0,   0), S( -58,  14), S(-102,  48), S(-146,  60),
    S( -26,  62), S( 178,  64), S( 364, 202), S(   0,   0)}},
  {{S(   0,   0), S( -48,  32), S(-100,  38), S(-146,  66),
    S(  -6,  72), S( 178,  80), S( 526, 228), S(   0,   0)},
   {S(   0,   0), S( -58,  24), S( -90,  34), S(-134,  76),
    S(  -2, 104), S( 184, 234), S( 322, 550), S(   0,   0)}},
};

const int PassedFriendlyDistance[8] = {
    S(   0,   0), S(   0,   0), S(   6,  -8), S(  14, -20), 
    S(  12, -28), S( -16, -26), S( -30, -18), S(   0,   0), 
};

const int PassedEnemyDistance[8] = {
    S(   0,   0), S(   6,   0), S(  10,   4), S(  18,  18), 
    S(   2,  42), S(  14,  60), S(  48,  56), S(   0,   0), 
};

const int PassedSafePromotionPath = S( -58,  74);

/* Threat Evaluation Terms */

const int ThreatWeakPawn             = S( -26, -52);
const int ThreatMinorAttackedByPawn  = S(-102,-106);
const int ThreatMinorAttackedByMinor = S( -52, -72);
const int ThreatMinorAttackedByMajor = S( -46, -88);
const int ThreatRookAttackedByLesser = S( -98, -38);
const int ThreatQueenAttackedByOne   = S( -78, -58);
const int ThreatOverloadedPieces     = S( -16, -26);
const int ThreatByPawnPush           = S(  30,  42);

/* Complexity Evaluation Terms */

const int ComplexityTotalPawns  = S(   0,  14);
const int ComplexityPawnFlanks  = S(   0,  98);
const int ComplexityPawnEndgame = S(   0,  68);
const int ComplexityAdjustment  = S(   0,-220);

/* General Evaluation Terms */

const int Tempo = 40;

#undef S

int evaluateBoard(Board *board, PKTable *pktable) {

    EvalInfo ei;
    int phase, factor, eval, pkeval;

    // Setup and perform all evaluations
    initEvalInfo(&ei, board, pktable);
    eval   = evaluatePieces(&ei, board);
    pkeval = ei.pkeval[WHITE] - ei.pkeval[BLACK];
    eval  += pkeval + board->psqtmat;
    eval  += evaluateComplexity(&ei, board, eval);

    // Calculate the game phase based on remaining material (Fruit Method)
    phase = 24 - 4 * popcount(board->pieces[QUEEN ])
               - 2 * popcount(board->pieces[ROOK  ])
               - 1 * popcount(board->pieces[KNIGHT]
                             |board->pieces[BISHOP]);
    phase = (phase * 256 + 12) / 24;

    // Scale evaluation based on remaining material
    factor = evaluateScaleFactor(board, eval);

    // Compute the interpolated and scaled evaluation
    eval = (ScoreMG(eval) * (256 - phase)
         +  ScoreEG(eval) * phase * factor / SCALE_NORMAL) / 256;

    // Factor in the Tempo after interpolation and scaling, so that
    // in the search we can assume that if a null move is made, then
    // then `eval = last_eval + 2 * Tempo`
    eval += board->turn == WHITE ? Tempo : -Tempo;

    // Store a new Pawn King Entry if we did not have one
    if (ei.pkentry == NULL && pktable != NULL)
        storePKEntry(pktable, board->pkhash, ei.passedPawns, pkeval);

    // Return the evaluation relative to the side to move
    return board->turn == WHITE ? eval : -eval;
}

int evaluatePieces(EvalInfo *ei, Board *board) {

    int eval;

    eval  =   evaluatePawns(ei, board, WHITE)   - evaluatePawns(ei, board, BLACK);
    eval += evaluateKnights(ei, board, WHITE) - evaluateKnights(ei, board, BLACK);
    eval += evaluateBishops(ei, board, WHITE) - evaluateBishops(ei, board, BLACK);
    eval +=   evaluateRooks(ei, board, WHITE)   - evaluateRooks(ei, board, BLACK);
    eval +=  evaluateQueens(ei, board, WHITE)  - evaluateQueens(ei, board, BLACK);
    eval +=   evaluateKings(ei, board, WHITE)   - evaluateKings(ei, board, BLACK);
    eval +=  evaluatePassed(ei, board, WHITE)  - evaluatePassed(ei, board, BLACK);
    eval += evaluateThreats(ei, board, WHITE) - evaluateThreats(ei, board, BLACK);

    return eval;
}

int evaluatePawns(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const int Forward = (colour == WHITE) ? 8 : -8;

    int sq, flag, eval = 0, pkeval = 0;
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
            pkeval += PawnCandidatePasser[flag][relativeRankOf(US, sq)];
            if (TRACE) T.PawnCandidatePasser[flag][relativeRankOf(US, sq)][US]++;
        }

        // Apply a penalty if the pawn is isolated. We consider pawns that
        // are able to capture another pawn to not be isolated, as they may
        // have the potential to deisolate by capturing, or be traded away
        if (!threats && !neighbors) {
            pkeval += PawnIsolated;
            if (TRACE) T.PawnIsolated[US]++;
        }

        // Apply a penalty if the pawn is stacked. We adjust the bonus for when
        // the pawn appears to be a candidate to unstack. This occurs when the
        // pawn is not passed but may capture or be recaptured by our own pawns,
        // and when the pawn may freely advance on a file and then be traded away
        if (several(Files[fileOf(sq)] & myPawns)) {
            flag = (stoppers && (threats || neighbors))
                || (stoppers & ~forwardFileMasks(US, sq));
            pkeval += PawnStacked[flag];
            if (TRACE) T.PawnStacked[flag][US]++;
        }

        // Apply a penalty if the pawn is backward. We follow the usual definition
        // of backwards, but also specify that the pawn is not both isolated and
        // backwards at the same time. We don't give backward pawns a connected bonus
        if (neighbors && pushThreats && !backup) {
            flag = !(Files[fileOf(sq)] & enemyPawns);
            pkeval += PawnBackwards[flag];
            if (TRACE) T.PawnBackwards[flag][US]++;
        }

        // Apply a bonus if the pawn is connected and not backwards. We consider a
        // pawn to be connected when there is a pawn lever or the pawn is supported
        else if (pawnConnectedMasks(US, sq) & myPawns) {
            pkeval += PawnConnected32[relativeSquare32(US, sq)];
            if (TRACE) T.PawnConnected32[relativeSquare32(US, sq)][US]++;
        }
    }

    ei->pkeval[US] = pkeval; // Save eval for the Pawn Hash

    return eval;
}

int evaluateKnights(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, outside, defended, count, eval = 0;
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
            eval += KnightOutpost[outside][defended];
            if (TRACE) T.KnightOutpost[outside][defended][US]++;
        }

        // Apply a bonus if the knight is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            eval += KnightBehindPawn;
            if (TRACE) T.KnightBehindPawn[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the knight
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += KnightMobility[count];
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

int evaluateBishops(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, outside, defended, count, eval = 0;
    uint64_t attacks;

    uint64_t enemyPawns  = board->pieces[PAWN  ] & board->colours[THEM];
    uint64_t tempBishops = board->pieces[BISHOP] & board->colours[US  ];

    ei->attackedBy[US][BISHOP] = 0ull;

    // Apply a bonus for having a pair of bishops
    if ((tempBishops & WHITE_SQUARES) && (tempBishops & BLACK_SQUARES)) {
        eval += BishopPair;
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
        eval += count * BishopRammedPawns;
        if (TRACE) T.BishopRammedPawns[US] += count;

        // Apply a bonus if the bishop is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the bishop.
        if (     testBit(outpostRanksMasks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            outside  = testBit(FILE_A | FILE_H, sq);
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += BishopOutpost[outside][defended];
            if (TRACE) T.BishopOutpost[outside][defended][US]++;
        }

        // Apply a bonus if the bishop is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            eval += BishopBehindPawn;
            if (TRACE) T.BishopBehindPawn[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the bishop
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += BishopMobility[count];
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

int evaluateRooks(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, open, count, eval = 0;
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
            eval += RookFile[open];
            if (TRACE) T.RookFile[open][US]++;
        }

        // Rook gains a bonus for being located on seventh rank relative to its
        // colour so long as the enemy king is on the last two ranks of the board
        if (   relativeRankOf(US, sq) == 6
            && relativeRankOf(US, ei->kingSquare[THEM]) >= 6) {
            eval += RookOnSeventh;
            if (TRACE) T.RookOnSeventh[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the rook
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += RookMobility[count];
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

int evaluateQueens(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, count, eval = 0;
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
        eval += QueenMobility[count];
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

int evaluateKings(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int count, dist, blocked, eval = 0;

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
    eval += KingDefenders[count];
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
        if (count > 0) eval -= MakeScore(count * count / 360, count / 10);
    }

    // Everything else is stored in the Pawn King Table
    if (ei->pkentry != NULL) return eval;

    // Evaluate based on the number of files between our King and the nearest
    // file-wise pawn. If there is no pawn, kingPawnFileDistance() returns the
    // same distance for both sides causing this evaluation term to be neutral
    dist = kingPawnFileDistance(board->pieces[PAWN], kingSq);
    ei->pkeval[US] += KingPawnFileProximity[dist];
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
        ei->pkeval[US] += KingShelter[file == fileOf(kingSq)][file][ourDist];
        if (TRACE) T.KingShelter[file == fileOf(kingSq)][file][ourDist][US]++;

        // Evaluate King Storm using enemy pawn distance. Use a separate evaluation
        // depending on the file, and if the opponent's pawn is blocked by our own
        blocked = (ourDist != 7 && (ourDist == theirDist - 1));
        ei->pkeval[US] += KingStorm[blocked][mirrorFile(file)][theirDist];
        if (TRACE) T.KingStorm[blocked][mirrorFile(file)][theirDist][US]++;
    }

    return eval;
}

int evaluatePassed(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, rank, dist, flag, canAdvance, safeAdvance, eval = 0;

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
        eval += PassedPawn[canAdvance][safeAdvance][rank];
        if (TRACE) T.PassedPawn[canAdvance][safeAdvance][rank][US]++;

        // Evaluate based on distance from our king
        dist = distanceBetween(sq, ei->kingSquare[US]);
        eval += dist * PassedFriendlyDistance[rank];
        if (TRACE) T.PassedFriendlyDistance[rank][US] += dist;

        // Evaluate based on distance from their king
        dist = distanceBetween(sq, ei->kingSquare[THEM]);
        eval += dist * PassedEnemyDistance[rank];
        if (TRACE) T.PassedEnemyDistance[rank][US] += dist;

        // Apply a bonus when the path to promoting is uncontested
        bitboard = forwardRanksMasks(US, rankOf(sq)) & Files[fileOf(sq)];
        flag = !(bitboard & (board->colours[THEM] | ei->attacked[THEM]));
        eval += flag * PassedSafePromotionPath;
        if (TRACE) T.PassedSafePromotionPath[US] += flag;
    }

    return eval;
}

int evaluateThreats(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const uint64_t Rank3Rel = US == WHITE ? RANK_3 : RANK_6;

    int count, eval = 0;

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
    eval += count * ThreatWeakPawn;
    if (TRACE) T.ThreatWeakPawn[US] += count;

    // Penalty for pawn threats against our minors
    count = popcount((knights | bishops) & attacksByPawns);
    eval += count * ThreatMinorAttackedByPawn;
    if (TRACE) T.ThreatMinorAttackedByPawn[US] += count;

    // Penalty for any minor threat against minor pieces
    count = popcount((knights | bishops) & attacksByMinors);
    eval += count * ThreatMinorAttackedByMinor;
    if (TRACE) T.ThreatMinorAttackedByMinor[US] += count;

    // Penalty for all major threats against poorly supported minors
    count = popcount((knights | bishops) & poorlyDefended & attacksByMajors);
    eval += count * ThreatMinorAttackedByMajor;
    if (TRACE) T.ThreatMinorAttackedByMajor[US] += count;

    // Penalty for pawn and minor threats against our rooks
    count = popcount(rooks & (attacksByPawns | attacksByMinors));
    eval += count * ThreatRookAttackedByLesser;
    if (TRACE) T.ThreatRookAttackedByLesser[US] += count;

    // Penalty for any threat against our queens
    count = popcount(queens & ei->attacked[THEM]);
    eval += count * ThreatQueenAttackedByOne;
    if (TRACE) T.ThreatQueenAttackedByOne[US] += count;

    // Penalty for any overloaded minors or majors
    count = popcount(overloaded);
    eval += count * ThreatOverloadedPieces;
    if (TRACE) T.ThreatOverloadedPieces[US] += count;

    // Bonus for giving threats by safe pawn pushes
    count = popcount(pushThreat);
    eval += count * ThreatByPawnPush;
    if (TRACE) T.ThreatByPawnPush[colour] += count;

    return eval;
}

int evaluateScaleFactor(Board *board, int eval) {

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

    int eg = ScoreEG(eval);

    // Lone minor vs king and pawns, never give the advantage to the side with the minor
    if ( (eg > 0) && popcount(white) == 2 && (white & (knights | bishops)))
        return SCALE_DRAW;
    else if ( (eg < 0) && popcount(black) == 2 && (black & (knights | bishops)))
        return SCALE_DRAW;

    return SCALE_NORMAL;
}

int evaluateComplexity(EvalInfo *ei, Board *board, int eval) {

    // Adjust endgame evaluation based on features related to how
    // likely the stronger side is to convert the position.
    // More often than not, this is a penalty for drawish positions.

    (void) ei; // Silence compiler warning

    int complexity;
    int eg = ScoreEG(eval);
    int sign = (eg > 0) - (eg < 0);

    int pawnsOnBothFlanks = (board->pieces[PAWN] & LEFT_FLANK )
                         && (board->pieces[PAWN] & RIGHT_FLANK);

    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t queens  = board->pieces[QUEEN ];

    // Compute the initiative bonus or malus for the attacking side
    complexity =  ComplexityTotalPawns  * popcount(board->pieces[PAWN])
               +  ComplexityPawnFlanks  * pawnsOnBothFlanks
               +  ComplexityPawnEndgame * !(knights | bishops | rooks | queens)
               +  ComplexityAdjustment;

    if (TRACE) T.ComplexityTotalPawns[WHITE]  += sign * popcount(board->pieces[PAWN]);
    if (TRACE) T.ComplexityPawnFlanks[WHITE]  += sign * pawnsOnBothFlanks;
    if (TRACE) T.ComplexityPawnEndgame[WHITE] += sign * !(knights | bishops | rooks | queens);
    if (TRACE) T.ComplexityAdjustment[WHITE]  += sign;

    // Avoid changing which side has the advantage
    int v = sign * MAX(ScoreEG(complexity), -abs(eg));

    return MakeScore(0, v);
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
    ei->pkentry       =     pktable == NULL ? NULL : getPKEntry(pktable, board->pkhash);
    ei->passedPawns   = ei->pkentry == NULL ? 0ull : ei->pkentry->passed;
    ei->pkeval[WHITE] = ei->pkentry == NULL ? 0    : ei->pkentry->eval;
    ei->pkeval[BLACK] = ei->pkentry == NULL ? 0    : 0;
}

void initEval() {

    // Init a normalized 64-length PSQT for the evaluation which
    // combines the Piece Values with the original PSQT Values

    for (int sq = 0; sq < SQUARE_NB; sq++) {

        const int w32 = relativeSquare32(WHITE, sq);
        const int b32 = relativeSquare32(BLACK, sq);

        PSQT[WHITE_PAWN  ][sq] = + PawnValue   +   PawnPSQT32[w32];
        PSQT[WHITE_KNIGHT][sq] = + KnightValue + KnightPSQT32[w32];
        PSQT[WHITE_BISHOP][sq] = + BishopValue + BishopPSQT32[w32];
        PSQT[WHITE_ROOK  ][sq] = + RookValue   +   RookPSQT32[w32];
        PSQT[WHITE_QUEEN ][sq] = + QueenValue  +  QueenPSQT32[w32];
        PSQT[WHITE_KING  ][sq] = + KingValue   +   KingPSQT32[w32];

        PSQT[BLACK_PAWN  ][sq] = - PawnValue   -   PawnPSQT32[b32];
        PSQT[BLACK_KNIGHT][sq] = - KnightValue - KnightPSQT32[b32];
        PSQT[BLACK_BISHOP][sq] = - BishopValue - BishopPSQT32[b32];
        PSQT[BLACK_ROOK  ][sq] = - RookValue   -   RookPSQT32[b32];
        PSQT[BLACK_QUEEN ][sq] = - QueenValue  -  QueenPSQT32[b32];
        PSQT[BLACK_KING  ][sq] = - KingValue   -   KingPSQT32[b32];
    }
}
