/*
  int32Integrator.c 
  Copyright 2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <int32Integrator.h>

void int32IntegratorMove(Int32Integrator *i, Int32 dxdtMin, Int32 dxdtMax, Int32 xTo) {
	const Int64 deltaX_e32 = ((Int64)xTo<<32) - i->q.value_e32;

	if (deltaX_e32>=0) {
		const Q3232 ldx = int32ScaleValue(i->time, dxdtMax);
		i->q.value_e32 += deltaX_e32<=ldx.value_e32 ? deltaX_e32 : ldx.value_e32;
	}
	else {
		const Q3232 ldx = int32ScaleValue(i->time, dxdtMin);
		i->q.value_e32 += deltaX_e32>=ldx.value_e32 ? deltaX_e32 : ldx.value_e32;
	}
}

