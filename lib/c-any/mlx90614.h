/*
  mlx90614.h 
  Copyright 2014 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef mlx90614_h
#define mlx90614_h

/** @file
 * @brief Melexix MLX90614 SMBUS (I2C variant) IR sensor definitions.
 *
 * MLX90614 supports read word / write word (3 data bytes) transfers, only. SCL is limited to 10kHz..100kHz.
 * Transfers include a checksum using the polynomial x^8+x^2+x^1+1 .
 * EEPROM cells must be erased by writing 0 before writing a value.
 */

/** Addresses of the 32x16bit EEPROM
 */
enum Mlx90614EepromAddresses {
	MLX90614_EEPROM_T0_MIN		=0x00,
	MLX90614_EEPROM_T0_MAX		=0x01,
	MLX90614_EEPROM_PWM_CTRL	=0x02,
	MLX90614_EEPROM_TA_RANGE	=0x03,
	MLX90614_EEPROM_EPSILON		=0x04,	///< value 0.1..1.0, 0xFFFF=1.0
	MLX90614_EEPROM_CONFIG_1	=0x05,
	MLX90614_EEPROM_I2C_ADDRESS	=0x0E,	///< LSByte only
	MLX90614_EEPROM_ID0		=0x1C,	///< RO
	MLX90614_EEPROM_ID1		=0x1D,	///< RO
	MLX90614_EEPROM_ID2		=0x1E,	///< RO
	MLX90614_EEPROM_ID3		=0x1F,	///< RO

};

/** Addresses of the 32x17bit RAM
 */
enum Mlx90614RamAddresses {
	MLX90614_RAM_IR_1		=0x4,
	MLX90614_RAM_IR_2		=0x5,
	MLX90614_RAM_TA			=0x6,
	MLX90614_RAM_TOBJ_1		=0x7,
	MLX90614_RAM_TOBJ_2		=0x8,
};

enum Mlx90614Commands {
	MLX90614_CMD_RAM		=0x00,	///< + address of register
	MLX90614_CMD_EEPROM		=0x20,	///< + address of register
	MLX90614_CMD_READ_FLAG		=0xF0,	///< diagnostic function, 1 byte only if desired (instead of 3)
	MLX90614_CMD_SLEEP		=0xFF,
};

enum Mlx90614FlagBits {
	_MLX90614_FLAG_EE_BUSY		=7,	///< EEPROM write/erase still in progress
	_MLX90614_FLAG_EE_DEAD		=5,	///< EEPROM double error
	_MLX90614_FLAG_INIT		=4,	///< POR initialization still in progress
};

enum Mlx90614I2cAddress {
	MLX90614_I2C_ADDRESS_DEFAULT	=0x5A,	///< factory default
	MLX90614_I2C_ADDRESS_BROADCAST	=0x00,	///< MLX responds, independend of address setting
};


#endif
