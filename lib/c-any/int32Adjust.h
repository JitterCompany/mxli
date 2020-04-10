/*
  int32Adjust.h - 'low pass filter' adjustment.
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

/** @file
 * @brief Adjust a value towards a new value, but only by a small fraction per step.
 * Fast operations only (no div).
 */

#ifndef __int32Adjust_h
#define __int32Adjust_h

#include <integers.h>
#include <fixedPoint.h>

inline static Int32 int32Adjust_e10(Int32 speed_e10, Int32 value, Int32 newValue) {
	return (Int64)(E10-speed_e10) * value + (Int64)speed_e10*newValue  >> 10;
}

inline static Int32 int32Adjust_e20(Int32 speed_e20, Int32 value, Int32 newValue) {
	return (Int64)(E20-speed_e20) * value + (Int64)speed_e20*newValue  >> 20;
}

#endif
