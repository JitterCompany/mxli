/*
  uint16Div.h 
  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

/** @file
 * @brief Provides fast divide by 10
 *
 */

#ifndef __uint16Div_h
#define __uint16Div_h

#include <integers.h>
#include <fixedPoint.h>
#include <print.h>

#ifdef USE_MX16DIV10	// replace divide

// prototypes

__attribute__((const)) inline static Uint32 uint16Div10(Uint32 value) {
	// gcc 4.8.1 on CortexM0 does not optimize like this (but he should!):
	enum { e=19, M=52429 };
	return value*M >> e;
}

__attribute__((const)) inline static Uint32 uint16Mod10(Uint32 value) {
	return value - 10*uint16Div10(value);
}

/** Calculates both div (higher 16bits of return value and mod (lower 16bits of return value).
 */
__attribute__((const)) inline static Uint32 uint16DivMod10(Uint32 value) {
	return uint16Div10(value)<<16 | uint16Mod10(value);
}

#else

// let's count on compiler optimizations or a divide instruction of the core...

// 16 bit functions
__attribute__((const)) inline static Uint32 uint16Div10(Uint32 value) {
	return (Uint16)value / 10;
}

__attribute__((const)) inline static Uint32 uint16Mod10(Uint32 value) {
	return (Uint16)value % 10;
}

__attribute__((const)) inline static Uint32 uint16DivMod10(Uint32 value) {
	return uint16Div10(value)<<16 | uint16Mod10(value);
}

#endif

#endif

