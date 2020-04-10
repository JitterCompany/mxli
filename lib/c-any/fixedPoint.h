/*
  fixedPoint.h 
  Copyright 2012 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef fixedPoint_h
#define fixedPoint_h

/** @file
 * @brief helpers for fixed-point math.
 *
 *
 */

enum {
	E3	=1<<3,
	E4	=1<<4,
	E5	=1<<5,
	E6	=1<<6,
	E7	=1<<7,
	E8	=1<<8,
	E9	=1<<9,
	E10	=1<<10,
	E11	=1<<11,
	E12	=1<<12,
	E13	=1<<13,
	E14	=1<<14,
	E15	=1<<15,
	E16	=1<<16,
	E17	=1<<17,
	E18	=1<<18,
	E19	=1<<19,
	E20	=1<<20,
	E21	=1<<21,
	E22	=1<<22,
	E23	=1<<23,
	E24	=1<<24,
	E25	=1<<25,
	E26	=1<<26,
	E27	=1<<27,
	E28	=1<<28,
	E29	=1<<29,
	E30	=1<<30,
	E31	=1<<31,

	KILO	=1000,
	MEGA	=KILO*KILO,
	GIGA	=KILO*MEGA,

	KIBI	=E10,
	MEBI	=E20,
	GIBI	=E30,
};

#endif
