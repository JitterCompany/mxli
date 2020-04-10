/*
  physics.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __physics_h
#define __physics_h

/** @file
 * @brief Collection of physical functions.
 */

/** Calculates H2O vapour pressure.
 * @param celsius temperature in the range 0..269 .
 * @return the pressure over smooth surface, unit hPa (mbar)
 */
int h2oCelsiusToHpa(int celsius);

#endif
