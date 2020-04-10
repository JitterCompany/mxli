/*
  int32Integrator.h 
  Copyright 2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef int32Integrator_h
#define int32Integrator_h

#include <integers.h>
#include <int32Math.h>
#include <int64Math.h>

#include <int32Scale.h>

/** @file
 * @brief Toolkit for doing numerical integration in a sampled system with mostly 32bit operations.
 *
 * This module is intended for 32bit CPUs. 64bit divide is used in initialization functions only.
 * All other functions use 64bit shifts at most. This allows good performance on 32bit systems.
 *
 * Int32Integrator performs numerical integration. Every call to integrate(v) adds a small portion that adds up to v
 * after n steps, where scale(n) = 1.
 */

/** Integrated value in a sampled system. Values are considered for times t=0,1,... .
 * Sample frequency is chosen by the scale object. A scale object of 1/fsample is used to model a sampling
 * frequency of fsample.
 */
typedef struct {
	const Int32Scale *	time;		///< scales by 1/f = T = sample distance 
	Q3232			q;		///< the current value and fraction
} Int32Integrator;

/** Performs one single step of numerical integration (summing up).
 * @param i the integrator object.
 * @param dxdt the speed.
 */
inline static void int32IntegratorI(Int32Integrator *i, Int32 dxdt) {
	const Q3232 v = int32ScaleValue(i->time,dxdt);
	i->q.value_e32 += v.value_e32;
}

/** Movement with limited dx/dt. This is a handy method for calculating the position of mass that should finally end up
 * at the destination position, but is limited in speed to go there.
 * @param i the integrator object.
 * @param dxdtMin the minimum speed - meant to be the maximum speed backwards.
 * @param dxdtMax the maximum speed forward.
 * @param xTo the destination position, integral/Exx part.
 */
void int32IntegratorMove(Int32Integrator *i, Int32 dxdtMin, Int32 dxdtMax, Int32 xTo);

/** Returns the current integrator value.
 * @param i the integrator object.
 * @return the current position, integral/Exx part.
 */
inline static Int32 int32IntegratorValue(Int32Integrator *i) {
	return i->q.value;
}

/** Sets the current integrator value unconditionally.
 * @param i the integrator object.
 * @param value the value to set. Any integrator fractions are reset.
 */
inline static void int32IntegratorSetValue(Int32Integrator *i, Int32 value) {
	i->q.value = value;
	i->q.fraction = 0;
}

#endif

