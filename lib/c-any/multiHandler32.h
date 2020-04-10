/*
  multiHandler32.h - helper module for splitting up interrupt handlers into multiple functions based on bit masks.
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef multiHandler32_h
#define multiHandler32_h

#include <integers.h>
#include <stdbool.h>

/** @file
 * @brief Implementation of a listener concept.
 *
 *
 * Multiple handlers are registered for handling interrupt requests (or
 * something similar). Every handler has its bit-mask and if that mask intersects with IR-bits, then the handler is called.
 * Multiple handlers may match or none at all. One bit stands for one request. A list is implemented as an array with
 * a final zero element (handler==0).
 */
typedef struct {
	Uint32		ir;			///< The bits, that cause a callback
	void		(*handler)(Uint32 ir);	///< callback; 0 for END OF LIST
} MultiHandler32;

typedef MultiHandler32 *MultiHandler32List;

/** The handler function type.
 */
typedef void Handler32 (Uint32 ir);

/** Initializes empty list.
 */
void multiHandler32ListInit(MultiHandler32List list);

/** Adds one element to the list, if possible.
 * @return true, if successfully added, false otherwise.
 */
bool multiHandler32ListAdd(MultiHandler32List list, void (*handler)(Uint32), Uint32 mask, int nList);

/** Executes all handlers with matching mask.
 */
void multiHandler32Dispatch(MultiHandler32List list, Uint32 ir);

#endif

