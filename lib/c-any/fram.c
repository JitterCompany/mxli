/*
  fram.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <fram.h>

bool fifoPrintFramId(Fifo *fifo, FramId id) {
	return  fifoPrintString(fifo,"FRAM manuf=")
		&& (id.manufacturer==0x04 ?
			fifoPrintString(fifo,"RAMTRON")
			:
			fifoPrintString(fifo,"0x") && fifoPrintHex(fifo,id.manufacturer,3,3)
		)
		&& fifoPrintString(fifo,", size=")
		&& fifoPrintUDec(fifo,framSizeKib(id)>>10,1,4)
		&& fifoPrintString(fifo,"kiB, var=0x")
		&& fifoPrintHex(fifo,id.variation,2,2)
		&& fifoPrintString(fifo,", rev=")
		&& fifoPrintUDec(fifo,id.revision,1,2)
		;
}
