
/*
  lpcIsp.h 
  Copyright 2006-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __lpcIsp_h
#define __lpcIsp_h

#include <integers.h>

/** @file
 * @brief LPC ISP bootloader and IAP functions.
 *
 */

enum {
	LPC_ISP_CMD_SUCCESS			=0,
	LPC_ISP_INVALID_COMMAND			=1,
	LPC_ISP_SRC_ADDR_ERROR			=2,
	LPC_ISP_DST_ADDR_ERROR			=3,
	LPC_ISP_SRC_ADDR_NOT_MAPPED		=4,
	LPC_ISP_DST_ADDR_NOT_MAPPED		=5,
	LPC_ISP_COUNT_ERROR			=6,
	LPC_ISP_INVALID_SECTOR			=7,
	LPC_ISP_SECTOR_NOT_BLANK		=8,
	LPC_ISP_SECTOR_NOT_PREPARED		=9,
	LPC_ISP_COMPARE_ERROR			=10,
	LPC_ISP_BUSY				=11,
	LPC_ISP_PARAM_ERROR			=12,
	LPC_ISP_ADDR_ERROR			=13,
	LPC_ISP_ADDR_NOT_MAPPED			=14,
	LPC_ISP_CMD_LOCKED			=15,
	LPC_ISP_INVALID_CODE			=16,
	LPC_ISP_INVALID_BAUD_RATE		=17,
	LPC_ISP_INVALID_STOP_BIT		=18,
	LPC_ISP_CODE_READ_PROTECTION		=19,
	LPC_ISP_INVALID_FLASH_UNIT		=20,
	LPC_ISP_USER_CODE_CHECKSUM		=21,
	LPC_ISP_ERROR_SETTING_ACTIVE_PARTITION	=22,

	LPC_ISP_UNDEFINED			=100,
};

/** Provides a human-readable error message for a ISP command return code.
 * @param code the integer returned by the ISP handler
 * @return a simple error description.
 */
const char* lpcIspErrorMessage(int code);


#endif

