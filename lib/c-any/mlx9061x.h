/*
  mlx9061x.h 
  Copyright 2016 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef mlx9061x_h
#define mlx9061x_h

#include <stdbool.h>
#include <integers.h>

/** @file
 * @brief Common functionality of MLX-90614 / 90615 / 90616.
 *
 * MLX9061x supports read word / write word (3 data bytes) transfers, only. SCL is limited to 10kHz..100kHz.
 * Transfers include a checksum using the polynomial x^8+x^2+x^1+1 .
 * EEPROM cells must be erased by writing 0 before writing a value.
 * MLX90614 and MLX90616 are quite similar, MLX90615 differs considerably.
 */

/** Addresses of the 32x17bit RAM
 */
enum Mlx9061xRamAddresses {
	MLX9061X_RAM_TA			=0x6,
	MLX9061X_RAM_TOBJ_1		=0x7,
	MLX9061X_RAM_TOBJ_2		=0x8,
};

enum Mlx9061xCommands {
	MLX9061X_CMD_RAM		=0x00,	///< + address of register
	MLX9061X_CMD_EEPROM		=0x20,	///< + address of register
};

enum Mlx9061xDeviceAddress {
	MLX9061X_I2C_ADDRESS_BROADCAST	=0x00,	///< MLX responds on I2C, independend of address setting
};

enum Mlx9061xTiming {
	MLX9061X_EEPROM_WRITE_TIME_US	=5000,	///< 5ms required at least.
	MLX9061X_I2C_REQUEST_US		=1440,	///< 1.44ms SCL low switches to I2C mode (from PWM or thermal relay mode) after POR/wakeup
};

/** Checks if a given command operates on an EEPROM register.
 * @param command a command byte for MLX9061x devices.
 * @return true, if the source/destination is in EEPROM, false if it's in RAM. The result is undefined for all other destinations.
 */
inline static bool mlx9061xIsEepromRegister (Uint8 command) {
	return command & MLX9061X_CMD_EEPROM != 0;
}

#endif
