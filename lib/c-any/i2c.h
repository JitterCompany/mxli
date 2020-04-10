/*
  i2c.h 
  Copyright 2011-2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __i2c_h
#define __i2c_h

/** @file
 * @brief Marc's most frequently used I2C slave devices.
 */

enum I2cAddresses {
	I2C_ADDRESS_FM24VXX		=0xA0>>1,	///< RAMTRON FRAM BASE ADDRESS. 8 addresses configurable.
	I2C_ADDRESS_FM24VXX_ID		=0xF8>>1,	///< 'reserved' device address for getting device ID.

	I2C_ADDRESS_LM73_BASE		=0x48,		///< lowest address, floating address pin.
	I2C_ADDRESS_LM73_FLOATING 	=0x48,		///< addr pin floating
	I2C_ADDRESS_LM73_GND		=0x49,		///< addr pin tied to ground
	I2C_ADDRESS_LM73_VCC		=0x4A,		///< addr pin tied to Vcc (untested)
	I2C_ADDRESS_LM73_GTS03		=I2C_ADDRESS_LM73_GND,	///< LM73 as used in AEGMIS GTS03 sensor.

	I2C_ADDRESS_LM75_BASE		=0x48,		///< A0..A2 all zero; 8 addresses; same as LM73

	I2C_ADDRESS_24AA01		=0x50,		///< 24AAxx serial EEPROM in SOT-23
	I2C_ADDRESS_ADJDS371		=0x74,		///< Avago I2C RGBW color sensor.
	I2C_ADDRESS_MLX90614		=0x5A,		///< Melexis IR thermometer, factory default value.
	I2C_ADDRESS_MLX90615		=0x5B,		///< Melexis IR thermometer, factory default value.
	I2C_ADDRESS_PCA9555_BASE	=0x20,		///< NXP I2C 16-bit IO-port, 
};

#endif

