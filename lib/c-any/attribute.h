/*
  attribute.h - macros useful when using LTO with this lib. 
  Copyright 2014,2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __attribute_h
#define __attribute_h

// use this to keep functions in a compilation unit from beeing garbage collected.
#define KEEP __attribute__((used))
#define PURE_FUNCTION __attribute__((pure))	///< a function with no side effects
#define CONST_FUNCTION __attribute__((const))	///< a function with no side effects and no RAM dependencies

#endif

