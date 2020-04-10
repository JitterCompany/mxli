/*
  fd.h - simplified polling on file descriptors.
  Copyright 2011 Marc Prager
 
  This file is part of the c-linux library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __fd_h
#define __fd_h

#include <stdbool.h>

/** @file
 * @brief Basic polling IO on Linux.
 */

/** Checks if at least one character can be read.
 * @param fd An open file descriptor.
 * @return true if a character is available or some error happened, false otherwise.
 */
bool fdCanRead(int fd);

/** Reads one character.
 * @param fd An open file descriptor.
 * @return a positive value between 0 and 255 for successful read or -1 for error or EOF.
 */
int fdRead(int fd);

/** Checks if at least one character can be written.
 * @param fd An open file descriptor.
 * @return true if writing will not block or some error happened, false otherwise.
 */
bool fdCanWrite(int fd);

/** Writes a single character.
 */
void fdWrite(int fd, char c);

#endif

