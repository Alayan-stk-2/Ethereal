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

#include "pin.h"

#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include "move.h"
#include "movegen.h"

// Default values, UCI options can set them to true (1) or false (0)
int PinDetection  = 1;

/* What is a pin?

   The following definition of a pin is used:
   A piece P1 is said to be pinned to a piece P2
   by an attacking piece A if:
   - A attacks P1
   - A doesn't attack P2, but would attack P2 if P1's square was empty
   - P2 is undefended or P2 is more valuable than A
                         (using naive material values : K>Q>R>B=N>P)
   - P1 is not more valuable than P2

   The attacking piece A must be a slider (rook, bishop, queen).

   Some edge cases involving an attacking battery and a defended P2
   are not considered a pin under this definition.
   For an example of such an edge case,
   see 1b5k/rp6/b7/8/8/8/R3PP2/R3K3 w Q - 0 1

   Some edge cases involving a pinned piece that, by moving, can
   defend an otherwise undefended P2 that is worth less than A,
   are still considered pins under this definition.
   e.g. Qa1 pinning Nd4 to Re5. Nc6 and Nf3 protect e5.

   Moves creating discovered pins are classified as pinning moves.

   Finally, we note that there is no requirement on the pinning piece
   being defended or not being attacked. While the pin may be trivial
   to defend against if the pinning piece is hanging, it still qualify
   as a pin.
*/


/* evaluatePins count all the existing pins in the given board position,
   for the side to move.
   The number of pins and the pinning moves are printed. 

   The need to fully compute attack tables makes the function bigger
   than it would need to be in an environment where it can just read the
   pre-computed tables. */

void evaluatePins(Board *board) {
    int sq, sq2, sq3, pinCount = 0;
    int pins[3][64];
    char * sideToMove = (board->turn == WHITE) ? "white" : "black";
    const int US = board->turn, THEM = !board->turn;
    uint64_t attackedPieces, attacks, attacksBehind, defended, defendedBy2 = 0ull;
    uint64_t defendedStable, defendedStableBy2;

    // We store pins as square_id of pinning piece, pinned piece and piece pinned to
    // A move creating a new pin isn't guaranteed to increase the pin count,
    // so we need the ability to compare pins.
    // A square_id set to -1 means no valid pin is stored.
    // The limit of 64 stored pins is arbitrary, it is meant to be high enough
    // to avoid overflows. A list or vector implementation could avoid it.
    for(int i=0;i<64;i++) {
        pins[0][i] = -1;
        pins[1][i] = -1;
        pins[2][i] = -1;
    }

    // Initialize enemy attack tables to know which pieces are undefended
    // We need to know about twice defended pieces in case the pinned piece
    // is the only defender. In this case we have a partial pin, still counted as a pin
    uint64_t kings   = board->pieces[KING  ];
    uint64_t queens  = board->pieces[QUEEN ];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t bishops = board->pieces[BISHOP];

    defended = kingAttacks(getlsb(board->colours[THEM] & kings));
    defendedBy2 |= defended & pawnAttackSpan(board->colours[THEM] & board->pieces[PAWN], ~0ull, THEM);
    defended |= pawnAttackSpan(board->colours[THEM] & board->pieces[PAWN], ~0ull, THEM);

    uint64_t tempBishops = bishops               & board->colours[THEM];
    uint64_t tempRooks   = rooks                 & board->colours[THEM];
    uint64_t tempQueens  = queens                & board->colours[THEM];
    uint64_t tempKnights = board->pieces[KNIGHT] & board->colours[THEM];
    uint64_t occupied = board->colours[WHITE] | board->colours[BLACK];

    while (tempKnights) {
        sq = poplsb(&tempKnights);
        attacks = knightAttacks(sq);
        defendedBy2 |= defended & attacks;
        defended |= attacks;
    }

    // When we will try moves, the attacks from the enemy knights, pawns and king
    // cannot change. We save these intermediate results to avoid recomputing them.
    defendedStable    = defended;
    defendedStableBy2 = defendedBy2;

    while (tempBishops) {
        sq = poplsb(&tempBishops);
        attacks = bishopAttacks(sq, occupied);
        defendedBy2 |= defended & attacks;
        defended |= attacks;
    }
    while (tempRooks) {
        sq = poplsb(&tempRooks);
        attacks = rookAttacks(sq, occupied);
        defendedBy2 |= defended & attacks;
        defended |= attacks;
    }
    while (tempQueens) {
        sq = poplsb(&tempQueens);
        attacks = queenAttacks(sq, occupied);
        defendedBy2 |= defended & attacks;
        defended |= attacks;
    }

    defended    &= board->colours[THEM];
    defendedBy2 &= board->colours[THEM];

    // Count and store pins for the side to move
    // The definition of a pin given earlier is used.

    tempBishops = bishops & board->colours[US];
    tempRooks   = rooks   & board->colours[US];
    tempQueens  = queens  & board->colours[US];

    // TODO The code for bishops, rooks and queens is largely shared,
    //      it would be cleaner to avoid copy-pasting.

    // Evaluate each bishop for pins
    while (tempBishops) {
        sq = poplsb(&tempBishops);
        attacks = bishopAttacks(sq, occupied);
        attackedPieces = attacks & board->colours[THEM];
        attacksBehind = bishopAttacks(sq, occupied & ~attackedPieces) & ~attacks;
        attacksBehind &= board->colours[THEM];
        // attacksBehind now contain enemy pieces that others can be pinned to
        // attackedPieces contains enemy pieces that might be pinned
        while(attacksBehind) {
            sq2 = poplsb(&attacksBehind);

            // The pinned piece is the only piece that is directly bishop-attacked
            // from both the bishop's square and sq2.
            sq3 = getlsb(bishopAttacks(sq2, occupied) & attackedPieces);

            // We test if the piece behind is more valuable than the attacking piece
            int pin_target = testBit(rooks | queens | kings, sq2);
            int undefended = 0;
            // If not, we test if it is undefended
            if (  !pin_target
                && (   !testBit(defended, sq2)
                    || (    testBit(bishops | queens, sq3)
                        && !testBit(defendedBy2, sq2)))) {
                undefended = 1;
            }

            if (   (pin_target || undefended)
                && getPieceValue(sq3, board) < getPieceValue(sq2, board)) {
                    pins[0][pinCount] = sq;
                    pins[1][pinCount] = sq3;
                    pins[2][pinCount] = sq2;
                    pinCount++;
            }
        }
    }

    // Evaluate each rook for pins
    while (tempRooks) {
        sq = poplsb(&tempRooks);
        attacks = rookAttacks(sq, occupied);
        attackedPieces = attacks & board->colours[THEM];
        attacksBehind = rookAttacks(sq, occupied & ~attackedPieces) & ~attacks;
        attacksBehind &= board->colours[THEM];

        while(attacksBehind) {
            sq2 = poplsb(&attacksBehind);

            sq3 = getlsb(rookAttacks(sq2, occupied) & attackedPieces);

            int pin_target = testBit(queens | kings, sq2);
            int undefended = 0;
            if (  !pin_target
                && (   !testBit(defended, sq2)
                    || (    testBit(rooks | queens, sq3)
                        && !testBit(defendedBy2, sq2)))) {
                undefended = 1;
            }

            if (   (pin_target || undefended)
                && getPieceValue(sq3, board) < getPieceValue(sq2, board)) {
                    pins[0][pinCount] = sq;
                    pins[1][pinCount] = sq3;
                    pins[2][pinCount] = sq2;
                    pinCount++;
            }
        }
    }

    // Evaluate each queen for pins
    while (tempQueens) {
        sq = poplsb(&tempQueens);
        attacks = queenAttacks(sq, occupied);
        attackedPieces = attacks & board->colours[THEM];
        attacksBehind = queenAttacks(sq, occupied & ~attackedPieces) & ~attacks;
        attacksBehind &= board->colours[THEM];

        while(attacksBehind) {
            sq2 = poplsb(&attacksBehind);

            sq3 = getlsb(queenAttacks(sq2, occupied) & attackedPieces);

            // We want to know if the attack is along a diagonal or not
            int diagonal = testBit(bishopAttacks(sq2, occupied), sq3);
            uint64_t semiPin = (diagonal) ? bishops | queens : rooks | queens;

            int pin_target = testBit(kings, sq2);
            int undefended = 0;
            if (  !pin_target
                && (   !testBit(defended, sq2)
                    || (    testBit(semiPin, sq3)
                        && !testBit(defendedBy2, sq2)))) {
                undefended = 1;
            }

            if (   (pin_target || undefended)
                && getPieceValue(sq3, board) < getPieceValue(sq2, board)) {
                    pins[0][pinCount] = sq;
                    pins[1][pinCount] = sq3;
                    pins[2][pinCount] = sq2;
                    pinCount++;
            }
        }
    }

    // Now, we generate all legal moves in this position
    // They are stored in the moves variable
    Undo undo[1];
    uint16_t moves[MAX_MOVES];
    uint16_t pinningMoves[MAX_MOVES];
    int pinningMovesCount = 0;
    int movecount = genAllLegalMoves(board, moves);

    // We loop over all moves.
    // We check the new pin situations,
    // and compare with the former to get a list
    // of pinning moves
    for (int j=0; j<movecount; j++) {
        applyMove(board, moves[j], undo);

        kings   = board->pieces[KING  ];
        queens  = board->pieces[QUEEN ];
        rooks   = board->pieces[ROOK  ];
        bishops = board->pieces[BISHOP];

        int newPins[3][64];
        int newPinCount = 0;
        for(int i=0;i<64;i++) {
            newPins[0][i] = -1;
            newPins[1][i] = -1;
            newPins[2][i] = -1;
        }

        // We generate the defended table again
        // in case the move uncovered new slider attacks

        defended    = defendedStable;
        defendedBy2 = defendedStableBy2;

        tempBishops = bishops & board->colours[THEM];
        tempRooks   = rooks   & board->colours[THEM];
        tempQueens  = queens  & board->colours[THEM];

        while (tempBishops) {
            sq = poplsb(&tempBishops);
            attacks = bishopAttacks(sq, occupied);
            defendedBy2 |= defended & attacks;
            defended |= attacks;
        }
        while (tempRooks) {
            sq = poplsb(&tempRooks);
            attacks = rookAttacks(sq, occupied);
            defendedBy2 |= defended & attacks;
            defended |= attacks;
        }
        while (tempQueens) {
            sq = poplsb(&tempQueens);
            attacks = queenAttacks(sq, occupied);
            defendedBy2 |= defended & attacks;
            defended |= attacks;
        }

        defended    &= board->colours[THEM];
        defendedBy2 &= board->colours[THEM];

        // We compute the pins again, but for the new position
        // TODO : passing around data structures is the main
        //        impediment to cleanly avoid repeating code

        tempBishops = bishops & board->colours[US];
        tempRooks   = rooks   & board->colours[US];
        tempQueens  = queens  & board->colours[US];

        // Evaluate each bishop for pins
        while (tempBishops) {
            sq = poplsb(&tempBishops);
            attacks = bishopAttacks(sq, occupied);
            attackedPieces = attacks & board->colours[THEM];
            attacksBehind = bishopAttacks(sq, occupied & ~attackedPieces) & ~attacks;
            attacksBehind &= board->colours[THEM];
            // attacksBehind now contain enemy pieces that others can be pinned to
            // attackedPieces contains enemy pieces that might be pinned
            while(attacksBehind) {
                sq2 = poplsb(&attacksBehind);

                // The pinned piece is the only piece that is directly bishop-attacked
                // from both the bishop's square and sq2.
                sq3 = getlsb(bishopAttacks(sq2, occupied) & attackedPieces);

                // We test if the piece behind is more valuable than the attacking piece
                int pin_target = testBit(rooks | queens | kings, sq2);
                int undefended = 0;
                // If not, we test if it is undefended
                if (  !pin_target
                    && (   !testBit(defended, sq2)
                        || (    testBit(bishops | queens, sq3)
                            && !testBit(defendedBy2, sq2)))) {
                    undefended = 1;
                }

                if (   (pin_target || undefended)
                    && getPieceValue(sq3, board) < getPieceValue(sq2, board)) {
                        newPins[0][newPinCount] = sq;
                        newPins[1][newPinCount] = sq3;
                        newPins[2][newPinCount] = sq2;
                        newPinCount++;
                }
            }
        }

        // Evaluate each rook for pins
        while (tempRooks) {
            sq = poplsb(&tempRooks);
            attacks = rookAttacks(sq, occupied);
            attackedPieces = attacks & board->colours[THEM];
            attacksBehind = rookAttacks(sq, occupied & ~attackedPieces) & ~attacks;
            attacksBehind &= board->colours[THEM];

            while(attacksBehind) {
                sq2 = poplsb(&attacksBehind);

                sq3 = getlsb(rookAttacks(sq2, occupied) & attackedPieces);

                int pin_target = testBit(queens | kings, sq2);
                int undefended = 0;
                if (  !pin_target
                    && (   !testBit(defended, sq2)
                        || (    testBit(rooks | queens, sq3)
                            && !testBit(defendedBy2, sq2)))) {
                    undefended = 1;
                }

                if (   (pin_target || undefended)
                    && getPieceValue(sq3, board) < getPieceValue(sq2, board)) {
                        newPins[0][newPinCount] = sq;
                        newPins[1][newPinCount] = sq3;
                        newPins[2][newPinCount] = sq2;
                        newPinCount++;
                }
            }
        }

        // Evaluate each queen for pins
        while (tempQueens) {
            sq = poplsb(&tempQueens);
            attacks = queenAttacks(sq, occupied);
            attackedPieces = attacks & board->colours[THEM];
            attacksBehind = queenAttacks(sq, occupied & ~attackedPieces) & ~attacks;
            attacksBehind &= board->colours[THEM];

            while(attacksBehind) {
                sq2 = poplsb(&attacksBehind);

                sq3 = getlsb(queenAttacks(sq2, occupied) & attackedPieces);

                // We want to know if the attack is along a diagonal or not
                int diagonal = testBit(bishopAttacks(sq2, occupied), sq3);
                uint64_t semiPin = (diagonal) ? bishops | queens : rooks | queens;

                int pin_target = testBit(kings, sq2);
                int undefended = 0;
                if (  !pin_target
                    && (   !testBit(defended, sq2)
                        || (    testBit(semiPin, sq3)
                            && !testBit(defendedBy2, sq2)))) {
                    undefended = 1;
                }

                if (   (pin_target || undefended)
                    && getPieceValue(sq3, board) < getPieceValue(sq2, board)) {
                        newPins[0][newPinCount] = sq;
                        newPins[1][newPinCount] = sq3;
                        newPins[2][newPinCount] = sq2;
                        newPinCount++;
                }
            }
        }

        // We now compare the pins in this position
        // compared to the pins we had before.

        int newPin = 0;

        // More pins means there is at least one new pin
        if (newPinCount > pinCount)
            newPin = 1;

        // We want to know if there is a pin that
        // didn't exist before
        else {
            for (int i=0;i<newPinCount;i++) {
                int matchingPin = 0;
                for(int k=0;k<pinCount;k++) {
                    // A pin is considered identical
                    // if the pinned and pinned to pieces
                    // are identical, even if the attacker
                    // has moved or is now different
                    if(   newPins[1][i] == pins[1][k]
                       && newPins[2][i] == pins[2][k]) {
                        matchingPin = 1;
                        break;
                    }
                }
                if (!matchingPin) {
                    newPin = 1;
                    break;
                }
            }
        }

        if (newPin) {
            pinningMoves[pinningMovesCount] = moves[j];
            pinningMovesCount++;
        }

        // Now we're done, revert the board back
        revertMove(board, moves[j], undo);
    }

    // Third, we print the results
    char moveStr[6];

    printf("There are %i pins for %s\n", pinCount, sideToMove);
    for (int i=0; i < pinningMovesCount; i++) {
        moveToString(pinningMoves[i], moveStr, board->chess960);
        printf("Move %s is a pinning move.\n", moveStr);
    }
}

// This function assumes the square is not empty
int getPieceValue(int sq, Board *board) {
    int value = testBit(board->pieces[PAWN  ], sq) ? 1 :
                testBit(board->pieces[KNIGHT], sq) ? 2 :
                testBit(board->pieces[BISHOP], sq) ? 2 :
                testBit(board->pieces[ROOK  ], sq) ? 3 :
                testBit(board->pieces[QUEEN ], sq) ? 4 :
                                                     5;

    return value;
}
