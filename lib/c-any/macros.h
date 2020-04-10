/*
  macros.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

// (C) Marc Prager, 2005..2016
#ifndef MACROS_H
#define MACROS_H

/** @file
 * @brief Basic macros: MIN,MAX,ABS,ELEMENTS and more.
 */

#define DIV_CEILING(x,y)	(((x)+(y)-1)/(y))
#define DIV_ROUND(x,y)		(((x)+(y)/2)/(y))

#define ALIGN(align,x)		(DIV_CEILING(x,align)*align)
#define ALIGN8(x)		ALIGN(x,8)

// number of elements of an array
#define ELEMENTS(x)		(sizeof(x)/sizeof(x[0]))
// last element of an array
#define LAST(a)			(a[sizeof (a) / sizeof (a)[0] -1])

#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))

#define MINMAX(a,b,x) ((x)<(a) ? (a) : ((b)<(x) ? (b) : (x)))
#define ABS(x) ((x)>=0 ? (x) : (-(x)))

//#define offsetof (type,member) __builtin__offsetof(type,member)

#endif
