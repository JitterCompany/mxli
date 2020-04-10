/*
  raspiGpio.h - GPIO on Raspberry Pi.
  Copyright 2017 Marc Prager
 
  This file is part of the c-linux library.
  c-linux is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-linux is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

/** @file
 * @brief Very slow implementation of Raspberry Pi GPIO using /sys/class/gpio interface.
 *
 * Special care is taken to facilitate the usage of pin output: The 'laziness' of the function here refers to both the
 * user's (programmer's) style and the logic behind the output functions, which ressembles somehow to lazy evaluation
 * in functional programming languages. Calculations are performed no sooner than needed. In case of raspiLazyGpioWrite
 * this means: the programmer need not care about initialization, just write a value v to pin p. The function then checks,
 * whether this pin is initialized and if not it initializes the pin before outputting the value v. This comes at the cost
 * of a considerable delay at the first access, however.
 */

#ifndef __raspiGpio_h
#define __raspiGpio_h

#include <stdbool.h>

/** Basic string access to /sys/class/gpio/.. 
 * @param file the file to access, relative to /sys/class/gpio/ like "export"
 * @param msg the string to write into the file, like "17"
 */
bool raspiLazyGpioWriteString (const char *file, const char *msg);

/** Basic string access to /sys/class/gpio/.. 
 * @param file the file to access, relative to /sys/class/gpio/ like "gpio17/direction"
 * @param msg the string to read from the file, like "17"
 * @param n the buffer size
 */
int raspiLazyGpioReadString (const char *file, char *buffer, int n);

/** Determines, whether a port is exported - according to this modules flags.
 * @param port the IO port number.
 * @return true if the port is exported, false if the module doesn't know for sure.
 */
bool raspiLazyGpioIsExported (int port);

/** Determines, whether a port is configured as input or output - according to this modules flags.
 * @param port the IO port number.
 * @return true if the port is configured, false if the module doesn't know for sure.
 */
bool raspiLazyGpioIsConfigured (int port);

/** Checks sysfs whether a port is exported. Doesn't consider the module's flags.
 * @param port the IO port number.
 * @return true, if the port is exported, false if it is not.
 */
bool raspiLazyGpioSysExists (int port);

/** Performs a 'lazy' export of a port.
 * @param port the IO port number.
 * @return true, if the port is successfully exported, false if the export failed.
 */
bool raspiLazyGpioExport (int port);

bool raspiLazyGpioUnexport (int port);

/** Performs a 'lazy' direction configuration of a port. If it is not yet exported, then it is exported
 * before configuring the direction.
 * @param port the IO port number.
 * @return true, if the port is successfully configured, false if anything failed.
 */
bool raspiLazyGpioDirection (int port, bool out);

/** Performs a 'lazy' write to a port.
 * If if is not yet configured, then it is configured before.
 * If it is not yet exported, then it is exported before configuring the direction.
 * Only the first access may be slowed down by preparations.
 * @param port the IO port number.
 * @return true, if the port is successfully exported, false if the export failed.
 */
bool raspiLazyGpioWrite (int port, bool value);

/** Performs a 'lazy' read from a port.
 * If if is not yet configured, then it is configured before.
 * If it is not yet exported, then it is exported before configuring the direction.
 * Only the first access may be slowed down by preparations.
 * @param port the IO port number.
 * @return true, if the port is successfully exported, false if the export failed.
 */
bool raspiLazyGpioRead (int port, bool *value);

#endif

