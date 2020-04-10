/*
  seqioMlx9061x.h 
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef seqioMlx9061x_h
#define seqioMlx9061x_h

/** @file
 * @brief Functions for accessing MLX90614/5 on top of seqio / seqI2c.
 *
 * MLX90614 supports read word / write word (3 data bytes) transfers, only. SCL is limited to 10kHz..100kHz.
 * Transfers include a checksum using the polynomial x^8+x^2+x^1+1 .
 * EEPROM cells must be erased by writing 0 before writing a value.
 */

/** Prepares a word (16-bit) read, initiated by a command (RAM/ROM/FLAGS/SLEEP)
 * @param seqio the IO transaction object
 * @param crc (output) the CRC code of the data to be sent.
 * @param i2cAddress the device address - this is included in the CRC
 * @param command see datasheet for possible values
 */
void mlx9061xPrepareRead (Seqio *seqio, Int32 *crc, Int32 i2cAddress, Int32 command);

/** Call whenever the read transaction finished successfully. Extracts the received data and converts it into
 * a 16-bit word. The CRC is checked.
 * @param seqio the IO transaction object
 * @param crc the checksum of the bytes sent to the device - these are included in the overall CRC.
 * @param result the destination for the word read from the device. It is calculated, even if the CRC is wrong.
 * @return true for successful transaction and correct CRC, false otherwise
 */
bool mlx9061xFinishRead (Seqio *seqio, Int32 crc, Int32 *result);

/** Writes one 16-bit word of data.
 * @param seqio the IO transaction object
 * @param i2cAddress the 7-bit I2C address of the destination device
 * @param command instruction + address for data word
 * @param data the data word to be written
 * @return true for successful transaction and correct CRC, false otherwise
 */
bool mlx9061xPrepareWrite (Seqio *seqio, Int32 i2cAddress, int command, int data);

#endif
