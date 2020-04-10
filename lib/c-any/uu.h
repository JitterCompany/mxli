/*
  uu.h - UUEncode/UUDecode. 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __uu_h
#define __uu_h

#include <fifoPrint.h>
#include <integers.h>

/** UUencodes one line of output (45 input bytes max).
 * @return true, if output FIFO was large enough.
 */
bool fifoUuEncodeLine(Fifo *uu, Fifo *data, Uint32 *checksum);

/** UUDecodes one line of input (61 chars + CRLF max).
 * @return true, if output Fifo was large enough and everything went well.
 */
bool fifoUuDecodeLine(Fifo *data, Fifo *uu, Uint32 *checksum);

#endif

