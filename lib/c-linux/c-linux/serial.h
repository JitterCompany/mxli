/*
  serial.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-linux library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef linux__serial_h
#define linux__serial_h

#include <stdbool.h>

/** Opens a terminal (or serial device) in blocking mode with timeout using RAW mode.
 * Normally blocking reads/writes are performed to minimize system load. However, if no character arrives in a blocking
 * IO then a timeout triggers return from the blocking call.
 * @param tty the name of the device
 * @param baud the baud rate in bits/s.
 * @param timeoutDeciSeconds the timeout measured in 1/10 of a second.
 * @return a valid file handle in case of success, -1 in case of error.
 */
int serialOpenBlockingTimeout(const char *tty, int baud, int timeoutDeciSeconds);


/** Manually switches modem control line DTR.
 * @param fd file descriptor of the line.
 * @param on true to set this line to 1 in logic levels (inverted compared to RS-232),
 *   false to set the line to 0.
 * @return true in case of success, false otherwise.
 */
bool serialSetDtr(int fd, bool on);

/** Manually switches modem control line RTS.
 * @param fd file descriptor of the line.
 * @param on true to set this line to 1 in logic levels (inverted compared to RS-232),
 *   false to set the line to 0.
 * @return true, in case of success, false otherwise.
 */
bool serialSetRts(int fd, bool on);

#endif
