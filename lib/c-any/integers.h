/*
  integers.h - Finally my preferred FIXED size integers. 
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __integers_h
#define __integers_h

/** @file
 * @brief Easy-to-type fixed size types.
 *
 * These are no longer synonyms for stdint.h types.
 */

#include <stdbool.h>

#ifdef __GNUC__
//fixed size types
typedef signed char Int8;
typedef short Int16;
typedef int Int32;
typedef long long Int64;

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned Uint32;
typedef unsigned long long Uint64;

enum Int32Limits {
	INT32_MAX		=0x7FFFFFFF,
	INT32_MIN		=0x80000000,
	INT32_UNDEFINED		=INT32_MIN,	///< in case I need an 'undefined', this is the best candidate; just think of -INT32_MIN....
};

enum Uint32Limits {
	UINT32_MAX		=0xFFFFFFFF,
	UINT32_UNDEFINED	=INT32_MAX,	///< in case I need an 'undefined', this is the best candidate; 
};

inline static bool int32IsDefined (Int32 i) {
	return i!=INT32_UNDEFINED;
}

inline static Int32 int32Undefined (void) {
	return INT32_UNDEFINED;
}

// variable size types
// Int/Uint match the size of a pointer
#if __SIZEOF_POINTER__ == 8
typedef Int64 Int;
typedef Uint64 Uint;

#elif __SIZEOF_POINTER__ == 4
typedef Int32 Int;
typedef Uint32 Uint;

#else
#error "Invalid pointer size in the author's opinion."
#endif

#else
#error "This header works for GCC only."
#endif

#endif

