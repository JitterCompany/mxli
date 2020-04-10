/*
  mlx90615.h 
  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef mlx90615_h
#define mlx90615_h

/** @file
 * @brief Melexis MLX90615 SMBUS (I2C variant) IR sensor definitions.
 *
 * Features: -40..+115C object temperature, -40..+85C sensor temperature, medical grade
 * MLX90615 supports read word / write word (3 data bytes) transfers, only. SCL is limited to 10kHz..100kHz.
 * MLX90615 differs considerably from MLX90614 in interface.
 * Transfers include a checksum using the polynomial x^8+x^2+x^1+1 .
 * EEPROM cells must be erased by writing 0 (5ms) before writing a value (5ms).
 */

/** Addresses of the 32x16bit EEPROM
 */
enum Mlx90615EepromAddresses {
	MLX90615_EEPROM_I2C_ADDRESS	=0x00,	///< bits 0..6 I2C slave address, default 0x5B; also PWM Tmin
	MLX90615_EEPROM_PWM_T_RANGE	=0x01,
	MLX90615_EEPROM_CONFIG		=0x02,
	MLX90615_EEPROM_EPSILON		=0x03,	///< value 0.1..1.0, 0x4000 = 1.0 (E16/4)

	MLX90615_EEPROM_ID2		=0x1E,	///< RO
	MLX90615_EEPROM_ID3		=0x1F,	///< RO
};

/** Addresses of the 32x17bit RAM
 */
enum Mlx90615RamAddresses {
	MLX90615_RAM_RAW_IR		=0x5,	///< 16bit signed integer
	MLX90615_RAM_TA			=0x6,	///< 1/50 K resolution absolute temperature; bit15 is error flag.
	MLX90615_RAM_TOBJ		=0x7,	///< 1/50 K resolution absolute temperature; bit15 is error flag.:
};

enum Mlx90615Commands {
	MLX90615_CMD_EEPROM		=0x10,	///< + address of register
	MLX90615_CMD_RAM		=0x20,	///< + address of register
	MLX90615_CMD_SLEEP		=0xD5,	///< power down to 1.1uA (<3uA); wake up by SCL low for >=39ms (8ms trigger)
};

enum Mlx90615I2cAddress {
	MLX90615_I2C_ADDRESS_DEFAULT	=0x5B,	///< factory default
};

#endif
