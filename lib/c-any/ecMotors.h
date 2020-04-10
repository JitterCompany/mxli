#ifndef __ecMotors_h
#define __ecMotors_h

/** @file
 * @brief Spin-up functions for some EC-motors from Maxon and Faulhaber.
 *
 */
#include <dds.h>

typedef void EcSpinUpFunction(DdsTuning256 *current, const DdsTuning256 *target, unsigned tune1Hz, unsigned fSample);

EcSpinUpFunction ecSpinUpMaxonEc14A;
EcSpinUpFunction ecSpinUpMaxonEc14B;

/** Provides amplitude and phase ramps.
 */
EcSpinUpFunction ecSpinUpMaxonEc14C;


/** Faulhaber series 1509-006B with 50mm filter wheel.
 */
EcSpinUpFunction ecSpinUpFaulhaber1509;

/** Faulhaber series 1226-012B unloaded.
 */
EcSpinUpFunction ecSpinUpFaulhaber1226;

/** Faulhaber series 1226-012B 44mm color wheel, soft mode.
 */
EcSpinUpFunction ecSpinUpFaulhaber1226Soft;

#endif

