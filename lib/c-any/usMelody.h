/*
  usMelody.h - Fixed point sine tables.
  Copyright 2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef usMelody_h
#define usMelody_h
/** @file
 * @brief Simple melody player based on some us-timer.
 *
 * A sound is (here) has a cycle time (frequency) and a duration. Cycle time is measured in us, duration in ms.
 * This gives reasonable ranges on (RAM-)space constrained controllers. Such a sound can be packed into 4 bytes.
 *
 * A melody is a sequence of sounds and optionally pauses. For simplicity, we use 0-terminated arrays of sounds. So do
 * not forget to terminate every melody with an 0-record {}.
 *
 * A player manages playing of one melody. The proposal here is to use a single volatile pointer to a melody.
 * Incrementing the pointer yields the next sound. After the last sound, the hardware sound driver must set the pointer
 * to 0 (consumer side). The value 0 indicates the possibility to play another melody to the producer side - your
 * main application most probably. The producer side must not modify the pointer unless it's 0.
 * I suggest to set the pointer to 0 only after the last sound has finished.
 *
 * You have to implement both, the producer and the consumer side. However, on lpc8 there's the sctBeep module as a
 * potential hardware driver and other platforms may provide similar functionality.
 *
 */

#include <integers.h>
#include <stdbool.h>
#include <fixedPoint.h>	// MEGA

enum {
	CYCLE_RATIO_C_E10	=1024,		// exp 0
	CYCLE_RATIO_CIS_E10	=967,		//   exp 1
	CYCLE_RATIO_D_E10	=912,		// exp 2
	CYCLE_RATIO_DIS_E10	=861,		//   exp 3
	CYCLE_RATIO_E_E10	=813,		// exp 4
	CYCLE_RATIO_F_E10	=767,		// exp 5
	CYCLE_RATIO_FIS_E10	=724,		//   exp 6
	CYCLE_RATIO_G_E10	=683,		// exp 7
	CYCLE_RATIO_GIS_E10	=645,		//   exp 8
	CYCLE_RATIO_A_E10	=609,		// exp 9
	CYCLE_RATIO_AIS_E10	=575,		//   exp 10
	CYCLE_RATIO_H_E10	=542,		// exp 11
	CYCLE_RATIO_CC_E10	=512,		// exp 12

	CYCLEUS_A_BASE	=MEGA/440,
	CYCLEUS_C0	=CYCLEUS_A_BASE * CYCLE_RATIO_C_E10 / CYCLE_RATIO_A_E10,
	CYCLEUS_CIS0	=CYCLEUS_C0 * CYCLE_RATIO_CIS_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_D0	=CYCLEUS_C0 * CYCLE_RATIO_D_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_DIS0	=CYCLEUS_C0 * CYCLE_RATIO_DIS_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_E0	=CYCLEUS_C0 * CYCLE_RATIO_E_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_F0	=CYCLEUS_C0 * CYCLE_RATIO_F_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_FIS0	=CYCLEUS_C0 * CYCLE_RATIO_FIS_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_G0	=CYCLEUS_C0 * CYCLE_RATIO_G_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_GIS0	=CYCLEUS_C0 * CYCLE_RATIO_GIS_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_A0	=CYCLEUS_C0 * CYCLE_RATIO_A_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_AIS0	=CYCLEUS_C0 * CYCLE_RATIO_AIS_E10 / CYCLE_RATIO_C_E10	/ 1,
	CYCLEUS_H0	=CYCLEUS_C0 * CYCLE_RATIO_H_E10 / CYCLE_RATIO_C_E10	/ 1,

	CYCLEUS_C1	=CYCLEUS_C0 					/ 2,
	CYCLEUS_CIS1	=CYCLEUS_C0 * CYCLE_RATIO_CIS_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_D1	=CYCLEUS_C0 * CYCLE_RATIO_D_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_DIS1	=CYCLEUS_C0 * CYCLE_RATIO_DIS_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_E1	=CYCLEUS_C0 * CYCLE_RATIO_E_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_F1	=CYCLEUS_C0 * CYCLE_RATIO_F_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_FIS1	=CYCLEUS_C0 * CYCLE_RATIO_FIS_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_G1	=CYCLEUS_C0 * CYCLE_RATIO_G_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_GIS1	=CYCLEUS_C0 * CYCLE_RATIO_GIS_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_A1	=CYCLEUS_C0 * CYCLE_RATIO_A_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_AIS1	=CYCLEUS_C0 * CYCLE_RATIO_AIS_E10 / CYCLE_RATIO_C_E10	/ 2,
	CYCLEUS_H1	=CYCLEUS_C0 * CYCLE_RATIO_H_E10 / CYCLE_RATIO_C_E10	/ 2,

	CYCLEUS_C2	=CYCLEUS_C0 					/ 4,
};

/** A sound has a frequency and a duration.
 */
typedef struct {
	Uint16		cycleUs;	///< if 0 then delay only
	Uint16		timeMs;		///< if 0 then NO sound / end of list
} UsMelodySound;

/** A melody is just an array of sounds.
 */
typedef const UsMelodySound *UsMelody;

/** A player is just a (volatile) pointer to a melody (Melody is already a pointer).
 */
typedef volatile UsMelody UsMelodyPlayer;

/** Checks, if the melody (pointer) has reached the final NULL-sound.
 */
inline static bool usMelodyHasNext (const UsMelodySound *melody) {
	return melody->timeMs != 0;
}

#endif

