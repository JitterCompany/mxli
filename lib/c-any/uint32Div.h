/*
  uint32Div.h 
  Copyright 2014-2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

/** @file
 * @brief Fast divide-by-constant functions evading the DIV instruction or even (worse) emulation.
 *
 * These functions work by multiply and shift right instead of some algorithmic approach. You can rely on gcc to do
 * this optimization for you, but I observed (2014) gcc to fail in many cases.
 */

#ifndef __uint32Div_h
#define __uint32Div_h

#include <integers.h>
#include <fixedPoint.h>
#include <print.h>

#ifdef PROVIDE_MX32_DIV10	// define this, if you want to substitute div by a mul+shift

// all functions provide the full 32 bit range.

// prototypes

__attribute__((const)) inline static Uint32 uint32Div10(Uint32 value) {
	enum { e=35, M=3435973837 };		return (Uint64)value*M >> e;
}

__attribute__((const)) inline static Uint32 uint32Mod10(Uint32 value) {
	return value - 10*uint32Div10(value);

/** Calculates both div (higher 16bits of return value and mod (lower 16bits of return value).
 */
__attribute__((const)) inline static Uint64 uint32DivMod10(Uint32 value) {
	return uint32Div10(value)<<32 | uint32Mod10(value);
}


// some more of these useful functions
__attribute__((const)) inline static Uint32 uint32Div100 (Uint32 value) {
	enum { e=37, M=1374389535 };		return (Uint64)value*M >> e;
}

__attribute__((const)) inline static Uint32 uint32Div1000 (Uint32 value) {
	enum { e=38, M=274877907 };		return (Uint64)value*M >> e;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
#else

// let's count on compiler optimizations or a divide instruction of the core...

// 32 bit functions
__attribute__((const)) inline static Uint32 uint32Div10(Uint32 value) {
	return value / 10;
}

__attribute__((const)) inline static Uint32 uint32Mod10(Uint32 value) {
	return value % 10;
}

/** Calculates both div (higher 16bits of return value and mod (lower 16bits of return value).
 */
__attribute__((const)) inline static Uint64 uint32DivMod10(Uint32 value) {
	return (Uint64)uint32Div10(value)<<32 | uint32Mod10(value) % 10;
}

// some more of these useful functions
__attribute__((const)) inline static Uint32 uint32Div100 (Uint32 value) {
	return value / 100;
}

__attribute__((const)) inline static Uint32 uint32Div1000 (Uint32 value) {
	return value / 1000;
}

#endif

#endif
