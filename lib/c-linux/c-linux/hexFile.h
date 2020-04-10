/*
  hexFile.h - Reading of Intel-HEX files (for uC-flashing). 
  Copyright 2012 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef c_linux__hexFile_h
#define c_linux__hexFile_h

#include <hexImage.h>

/** Loads a bin-file into memory (same format as a hex-file).
 */
bool hexFileLoadBin(int fd, HexImage *hexImage);

/** Loads a hex-file into memory.
 */
bool hexFileLoadHex(int fd, HexImage *hexImage);

/** Convenience function; loads hex or bin file into memory.
 * The decision is made on the file extension.
 * @param fnImage the path of the file or NULL for standard input.
 * @param hexImage data destination.
 * @return true, if successfully loaded, false otherwise.
 */
bool hexFileLoad(const char *fnImage, HexImage *hexImage);

#endif

