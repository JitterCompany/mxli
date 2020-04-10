/*
  temperature.h 
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __temperature_h
#define __temperature_h

/** @file
 * @brief Temperature constants for Celsius, Kelvin and Farenheit
 *
 * In this library, temperatures are typically represented by 32-bit signed(!) 'E10' fixed point values of Kelvins.
 * The signedness is handy for calculations.
 * Celsius: 273.15 K = 0 C ; delta 1C = delta 1K ; C = K + 273.15
 * Fahrenheit: 273.15 K = 32 F; 373.15 K = 212 F ; delta 100K = delta 180F ; F = K * 1.8 + 32 - 273.15 * 1.8
 */

#include <stdbool.h>
#include <integers.h>
#include <fixedPoint.h>
#include <fifo.h>

enum {
	KELVIN_TO_CELSIUS_OFFSET_E10	= -27315*1024 / 100,
	KELVIN_TO_FAHRENHEIT_E10	= 18*1024 / 10,
	KELVIN_TO_FAHRENHEIT_OFFSET_E10	= 32*1024 - 27315*18*1024 / 100 / 10,
};

enum {
	//TEMPERATURE_CHAR=0xB0,		///< degree sign in 8859-1, doesn't work on my linux box.
	TEMPERATURE_CHAR='\'',		///< degree sign in 8859-1, doesn't work on my linux box.
};

inline static Int32 temperatureK2C_e10 (Int32 tK_e10) {
	return tK_e10 + KELVIN_TO_CELSIUS_OFFSET_E10;
}

Int32 temperatureK2F_e10 (Int32 tK_e10);

inline static Int32 temperatureC2K_e10 (Int32 tC_e10) {
	return tC_e10 - KELVIN_TO_CELSIUS_OFFSET_E10;
}

Int32 temperatureF2K_e10 (Int32 tF_e10);

/** Parses a temperature. Either a degree sign or a single quote or nothing may be used immediately after the number.
 * @param fifo the character input
 * @param tK_e10 result temperature in Kelvins.
 * @return true in case of success, false otherwise and fifo is unchanged.
 */
bool fifoParseTemperature (Fifo *fifo, Int32 *tK_e10);

/** Prints a temperature, using a
 */
bool fifoPrintTemperatureK (Fifo *fifo, Int32 tK_e10, int min, int max, int decimals, bool showSign);
bool fifoPrintTemperatureC (Fifo *fifo, Int32 tK_e10, int min, int max, int decimals, bool showSign);
bool fifoPrintTemperatureF (Fifo *fifo, Int32 tK_e10, int min, int max, int decimals, bool showSign);

#endif

