/*
  bezier2Command.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */
#ifndef __bezier2Command_h
#define __bezier2Command_h

/** @file
 * @brief FC190 bstep curve definition format.
 *
 * One very special thing about 'bstep' or 'stepper4/5' is their separation of X and Y coordinates, because in theory
 * not only 2 axes are supported, but n*2 axes can run independently.
 */

#include <bezierN.h>

enum {
	BEZIER_COMMAND_STOP,		// stop point
	BEZIER_COMMAND_CURVE,		// non-trivial bezier interpolation, controlled speed
	BEZIER_COMMAND_LINE,		// straigth line, controlled speed (needs only 2 points)
	BEZIER_COMMAND_PPLINE,		// point-to-point, straigth line, controlled speed (2 points, one speed (the max))
	//BEZIER_COMMAND_G0LINE,		// point-to-point, accel, max speed, slow down; 3 segments.
	BEZIER_COMMAND_POINT,		// single point, two PWM-cycles, no movement.
	BEZIER_COMMAND_RESTART,		// start again
};

struct Bezier2Command {
	Int32Bezier2	bezierX;
	Int32Bezier2	bezierY;
	Int32		speed0;		///< absolute speed point t=0
	Int32		speed1;		///< absolute speed point t=1
	int		command;
	int		label;		///< user-defined symbolic label
};
typedef struct Bezier2Command Bezier2Command;

#endif
