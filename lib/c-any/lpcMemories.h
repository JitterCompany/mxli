/*
  lpcMemories.h - NXP LPCxxxx microcontroller memory descriptions. 
  Copyright 2011,2013,2017 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __lpcMemories_h
#define __lpcMemories_h

/** @file
 * @brief FLASH and RAM map of NXPs LPCxxxx devices, Code read protection (CRP).
 *
 * This module assists in writing UART ISP FLASH programmers and also supports in-application-programming (IAP) of LPC
 * devices. Functions are provided for identifying the device, find usable RAM for data transfer and for determining
 * FLASH sector layout and banking.
 * A LPC FLASH consists of multiple sectors of (typically) different sizes. However, only a few sizes are used for
 * a complete FLASH memory, so we store the segmentation as a list of pairs of (size,count).
 *
 * CRP levels 0..3 indicate increasing severity/security. Higher levels introduced to model the LPC800 NO_ISP
 * level are consideres as non-orderable levels that can be enabled or not. 
 *
 * Sectors numbers include the flash bank, where appropriate.
 */

#include <integers.h>
#include <stdbool.h>
#include <fifoPrint.h>
#include <int32Pair.h>

enum {
	LPC_BLOCK_SIZES		=5,	///< Copy RAM to Flash block sizes, used to be 4, 5 since LPC800
	LPC_SECTOR_ARRAYS	=4,	///< number of regions of changing sector size and count.
	LPC_IDS			=2,	///< Maximum number of 32-bit IDs
	LPC_BANKS		=3,	///< Maximum number of FLASH banks	// 3 is experimental .-)
	LPC_RAMS		=6,	///< Maximum number of (on-chip) RAM 
	LPC_ISP_RAMS		=4,	///< Maximum number of (on-chip) RAM regions the boot-loader uses, increased from 2 to 4 with LPC541xx
	LPC_ISP_BUFFERS		=3,	///< maximum number of ISP transfer RAM buffers. Must be >=2
};

/** The size of individual sectors within one family is the same - what varies is the total number of sectors
 * implemented in each device. This list of different sizes/counts is terminated by a (0,0) element.
 */
typedef struct __attribute__((packed)) {
	Int8	sizeK;
	Int8	n;
} LpcSectorArray;

typedef enum {
	CORE_UNKNOWN,
	CORE_ARMV4T,		///< ARM7TDMI,...
	CORE_ARMV6_M,		///< Cortex-M0, Cortex-M0+
	CORE_ARMV7_M,		///< Cortex-M3, Cortex-M4
} LpcCore;

/** This structure contains all properties of a whole LPC family necessary to use the ISP boot loader or the IAP
 * interface.
 * Current assumptions:
 * FLASH is organized in sectors without gaps.
 * 0, 1 or 2 FLASH banks of equal sector layout are possible.
 * FLASH may start at addresses different from 0.
 * Multiple regions of ram are available, but only the first one can be guaranteed to be available at boot time.
 * Valid code is determined by checksumming the first few vector table entries.
 */
typedef struct __attribute__((packed,aligned(4))) {
	Uint32		idMasks		[LPC_IDS];
	Uint32		addressFlashs	[LPC_BANKS];
	LpcSectorArray	sectorArrays	[LPC_SECTOR_ARRAYS];
	Uint16		blockSizes	[LPC_BLOCK_SIZES];	///< copy RAM-to-FLASH block sizes, ascending (if !=0)
	Uint16		pageSize;				///< some flashes can be accessed in pages < sector
	Uint8		banks;					///< how many flash banks? 0..2
	Uint8		checksumVectors;			///< how many vectors will be checksummed for valid code?
	Uint8		checksumVector;				///< where to put the checksum per flash bank.
	Uint8		core;					///< what kind of processor (at least).
} LpcFamily;

typedef struct __attribute__((packed)) {
	Uint32	address;	///< start of block or start of RAM region, if size<0 meaning: from top of region down
	Int32	size;		///< size in bytes. If <0 then it's counted from the end of the region (STACK).
} LpcIspRamUsage;

typedef enum {
	ISP_PROTOCOL_UUENCODE,		///< UUEncode was used for binary data transmission until LPC800 appeared
	ISP_PROTOCOL_BINARY,		///< NOT-encoded binary data transmission for LPC800.
} LpcIspDataProtocol;

/** This structure contains all properties of a whole LPC family necessary to use the ISP boot loader with the
 * exception of those properties used by both the IAP interface and the ISP boot loader.
 */
typedef struct __attribute__((packed,aligned(4))) {
	Uint32		addressRams	[LPC_RAMS];
	LpcIspRamUsage	ramUsage	[LPC_ISP_RAMS];
	Int8		protocol;			///< protocol used to transfer data bytes.
} LpcIspFamily;

typedef struct __attribute__((packed,aligned(4))) {
	const char		*name;		///< device name, like LPC2136/01
	Uint16			sizeFlashK;	///< per bank size
	Uint16			sizeRamKs[LPC_RAMS];
	Uint32			ids[LPC_IDS];		///< CPU ID
	const LpcFamily*	family;
	const LpcIspFamily*	ispFamily;
} LpcMember;


enum {
	BANK_Z		=0,	///< unbanked
	BANK_A		=1,
	BANK_B		=2,
	_SECTOR_BANK	=16,
	SECTOR_MASK	=(1<<_SECTOR_BANK)-1,
};

/** Prints a sector 'number'. Unbanked sectors are simple numbers. Banked sectors are prefixed by A or B.
 */
bool fifoPrintSector (Fifo *fifo, int sector);

bool fifoPrintBank (Fifo *fifo, int bank);

/** Allows iteration through a memory region and evade any memory used by the ISP handler. Positive return values indicate bytes available.
 * Negative return values indicate the number used by the ISP handler from the offset point on. After that number of bytes we can try
 * again. A value of 0 indicates, there are no more bytes free in this region, starting from offset.
 * @param ramUsage the RAM usage description of the RAM loader.
 * @param address the start of the RAM region
 * @param size the total size of the RAM region
 * @param offset an offset within the region that indicates the start of a potential sub-region.
 * @return the number of bytes usable (>0) or -the number of bytes used by the ISP handler from that point (<0) or 0 if the
 *   region cannot contain any more free bytes.
 */
Int32 lpcIspGetFreeBytes (const LpcIspRamUsage ramUsage [LPC_ISP_RAMS], Uint32 address, Uint32 offset, Int32 size);

/** Checks if the given ID matches the controller description.
 * Some controller families use two IDs - don't used this function for these.
 * @param list the list of controllers.
 * @param ids the 32-bit IDs to search. 
 * @return true if matched, false otherwise.
 */
bool lpcMatchByIds (LpcMember const* list, Uint32 ids[LPC_IDS]);

/** Checks, if the given name matches the controller description.
 * @param list the list of controllers.
 * @param namePrefix the name to search. This may be shortened to search for prefixes only.
 * @return true if matched, false otherwise.
 */
bool lpcMatchByName (LpcMember const* list, const char *namePrefix);

/** This function should not exist. However, NXP designed their ISP code so poorly, that I can't know without
 * timing out, if it delivers 1 or 2 IDs (and maybe more in the future). So I provide this function, that calculates
 * the number of IDs returned from the first ID of a *KNOWN* devices.
 * @param member the device descriptor
 * @param id the first of a device.
 * @return 0 if id not found, values >= 1 if found.
 */
int lpcMatchNumberOfIds (LpcMember const* member, Uint32 id);

/** This function should not exist. However, NXP designed their ISP code so poorly, that I can't know without
 * timing out, if it delivers 1 or 2 IDs (and maybe more in the future). So I provide this function, that calculates
 * the number of IDs returned from the first ID of a *KNOWN* devices.
 * @param list the device list.
 * @param id the first of a device.
 * @return 0 if id not found, values >= 1 if found.
 */
int lpcFindNumberOfIds (LpcMember const* const* list, Uint32 id);

/** Searches for a controller by its ID. Some controller families use two IDs - don't used this function for these.
 * @param list the list of controllers.
 * @param ids the 32-bit IDs to search. 
 * @return the description of the controller if found, or 0 if no match.
 */
const LpcMember* lpcFindByIds (LpcMember const * const* list, Uint32 ids[LPC_IDS]);

/** Searches a controller by its name.
 * @param list the list of controllers.
 * @param namePrefix the name to search. This may be shortened to search for prefixes only.
 * @return the description of the controller if found, or 0 if no match.
 */
const LpcMember* lpcFindByName (LpcMember const * const* list, const char *namePrefix);

////////////////////////////////////////////////////////////////////////////////////////////////////
// FLASH, Banks, Sectors and Addresses

/** A structure for iterating through the sectors of a LPC.
 */
typedef struct {
	const LpcMember*	member;
	int			bankIdx;
	int			groupIdx;
	int			sectorInGroup;
	int			sectorInBank;
	Uint32			offsetInBank;
} LpcSectorIterator;


inline static bool lpcSectorIteratorHasNext (const LpcSectorIterator *it) {
	return	it->member->sizeFlashK>0		// avoid strange definition of multiple banks of size 0.
		&& it->bankIdx < it->member->family->banks;
}

/** Advance one sector
 */
void lpcSectorIteratorNext (LpcSectorIterator *it);

inline static Uint32 lpcSectorIteratorAddress (const LpcSectorIterator *it) {
	return	it->member->family->addressFlashs [it->bankIdx] + it->offsetInBank;
}

inline static Uint32 lpcSectorIteratorSize (const LpcSectorIterator *it) {
	return it->member->family->sectorArrays [it->groupIdx].sizeK * 1024;
}

inline static int lpcSectorIteratorBank (const LpcSectorIterator *it) {
	return it->member->family->banks >= 2 ? BANK_A + it->bankIdx : BANK_Z;
}

inline static int lpcSectorIteratorSector (const LpcSectorIterator *it) {
	return lpcSectorIteratorBank (it) << _SECTOR_BANK | it->sectorInBank;
}

inline static bool lpcSectorIteratorContainsAddress (const LpcSectorIterator *it, Uint32 address) {
	const Uint32 start = lpcSectorIteratorAddress (it);
	const Uint32 end = start + lpcSectorIteratorSize (it);
	return start <= address && address < end; 
}

/** Calculates what sectors are affected from operations within an address range.
 * @param member LPC family member descriptor
 * @param sectorRange the result sector range
 * @param addressRange the lowest/highest address of the range
 * @return true, if the addresses are within the members FLASH range, false if not.
 */
bool lpcAddressRangeToSectorRange(LpcMember const *member, Int32Pair *sectorRange, const Uint32Pair *addressRange);

//bool lpcAddressRangeToSectorRange(LpcMember const *member,
//	int *sectorFrom, int *sectorTo, Uint32 addressFrom, Uint32 addressTo);

//bool lpcSectorRange(LpcMember const *member, Uint32 *addressFrom, Uint32 *addressTo, int sector);

/** Calculates the last sector of a bank.
 * @param member LPC family member descriptor
 * @param bank the bank number, BANK_A for the first, BANK_B for the second, 0 (BANK_Z) for an unbanked device.
 */
int lpcBankToLastSector (LpcMember const *member, int bank);

/** Finds the sector to a given address.
 * @param member LPC family member descriptor
 * @param address the address of a single byte.
 * @return the sector that contains the byte or -1 if no such sector exists (in that device).
 */
int lpcAddressToSector (LpcMember const *member, Uint32 address);

/** Gets the starting address of a bank.
 * @param member LPC family member descriptor
 * @param bank the FLASH bank: -1, BANK_Z, BANK_A, BANK_B, ... 
 * return the first valid address of a FLASH bank. -1 and BANK_Z return 0.
 */
Uint32 lpcBankToAddress (LpcMember const *member, int bank);

/** Finds the sector to a given address.
 * @param member LPC family member descriptor
 * @param address the address of a single byte.
 * @return the base address of the FLASH bank containing the address. If no bank matches, 0 is returned.
 */
Uint32 lpcAddressToBankAddress(LpcMember const *member, Uint32 address);

/** Finds the bank for a given address.
 * @param member the LPC family member descriptor
 * @param address the absolute address in the LPC's address space
 * @return BANK_A if address <= bank A base address, BANK_B if address <= bank B address, ...
 *   In case of 0 matching banks, 0 (=/= BANK_A!) is returned.
 */
Uint32 lpcAddressToBank (LpcMember const *member, Uint32 address);

/** Finds the number of the highest sector of the whole family.
 */
int lpcFamilySectorHighest(LpcFamily const *map);

/** Calculates the FLASH size from the FLASH sector definition.
 * @param family the LPC family descriptor
 * @return the total size of one FLASH bank.
 */
Uint32 lpcFamilyBankSize (LpcFamily const *family);

////////////////////////////////////////////////////////////////////////////////////////////////////
// RAM handling

/** Calculates the size of the RAM region.
 * @param member the LPC family member descriptor
 * @param address the RAM region starting address
 * @return the size of the region. If 0, then an invalid start address was given or the region is not available
 *   in that specific device.
 */
int lpcRamRegionSize(LpcMember const *member, Uint32 address);

/** Finds the offset from the beginning of the FLASH.
 * @param map the families memory map
 * @param sector any valid sector number within the map or even +1 to calculate FLASH size.
 * @return the starting offset of the given sector.
 */
int lpcFamilySectorOffset(const LpcFamily* map, int sector);

typedef struct {
	Uint32	address;
	Uint32	size;
} LpcIspBuffer;

/** Returns all RAM functional at ISP time and not used by the ISP handler on the given LPC device. The ISP handler splits RAM0 into
 * 2 pieces on some devices (2014). This function returns the left-overs plus additional RAM if available.
 * @param lpc the LPC device description
 * @param buffers an array for placement of the results at positions 0..number of non-empty regions-1
 * @return the number of non-zero length RAM regions
 */
int lpcIspGetBuffers(const LpcMember* lpc, LpcIspBuffer buffers [LPC_ISP_BUFFERS]);


/** Returns the number of buffers of a given size (and 4-byte alignment) that can be generated by dividing up the RAM
 * space of buffer into equally sized (smaller) chunks.
 * @param buffer the current available RAM
 * @param buffers the current number of RAM regions of buffer
 * @param chunkSize the block size of smaller buffers to be aligned into buffer.
 * @return the total number of resulting smaller buffers.
 */
int lpcIspBufferChunkCount (const LpcIspBuffer buffer[LPC_ISP_BUFFERS], int buffers, int chunkSize);

/** Partitions a buffer of multiple contiguous regions into smaller chunks. The chunks size must be a multiple of 4.
 * The chunks are 4-byte aligned.
 * @param chunks the destination array for the partitioning. Must be large enough.
 * @param chunkSize the size of the smaller pieces.
 * @param buffer the current available RAM
 * @param buffers the current number of RAM regions of buffer
 */
void lpcIspBufferChunkPartition (LpcIspBuffer *chunks, int chunkSize, const LpcIspBuffer buffer[LPC_ISP_BUFFERS], int buffers);

int lpcIspMaxTransferSize(const LpcFamily* f);
int lpcIspMinTransferSize(const LpcFamily* f);

/** Finds the largest contiguous block of RAM useable for ISP transfer, that does not clash with the ISP handler's
 * RAM usage. This function prefers total size to large block size.
 */
bool lpcIspFindTransferRamMax(const LpcFamily* map, Uint32 *address, Uint32 *size, Uint32 *blockSize);

/** Finds the largest contiguous block of RAM useable for ISP transfer, that does not clash with the ISP handler's
 * RAM usage. This function prefers large block size to total size.
 */
bool lpcIspFindTransferRamMaxBlock(const LpcFamily* map, Uint32 *address, Uint32 *size, Uint32 *blockSize);

/** Returns the code used to enable different levels of Code Read Protection (CRP).
 * @param level a protection level, 0..3,4. 4 is for NO_ISP-mode.
 * @return the values 0xFFFFFFFF, 0x12345678, 0x87654321, 0x43218765 and 0x4E697370 for 0..4.
 */
Uint32 lpcCrpLevelToCode(int level);

/** Translates CRP codes into CRP levels.
 * @param code either a valid CRP code for levels 1..3, 4 or any other value for level 0.
 * @return CRP level 0..3, 4 (NO_ISP). 
 */
int lpcCrpCodeToLevel(Uint32 code);

/** Checks if a desired level is within the limits.
 * Level 0 allows level 0 only.
 * Level 1 allows levels 0..1 .
 * Level 2 allows level 0..2.
 * Level 3 allows levels 0..4.
 * Level 4 allows level 0 or level 4 only. Levels 3 or 4 disable ISP. Levels 1..3 disable JTAG/SWD, but level 4
 * doesn't.
 * <p>
 * Desired levels below 0 are all allowed.
 * @param levelAllow the maximum CRP level allowed.
 * @param levelDesired the desired CRP level.
 * @return true, if the desired level is allowed by the allow level.
 */
bool lpcCrpLevelAllowed(Int32 levelAllow, Int32 levelDesired);

/** Print all information about a LPCxxxx family member.
 */
bool fifoPrintLpcMember(Fifo* fifo, LpcMember const* member);

/** Pretty-prints the IDs along with their masks as hex numbers
 */
bool fifoPrintLpcIds(Fifo* fifo, Uint32 ids[LPC_IDS], Uint32 idMasks[LPC_IDS]);

/** Parses a string like 0x10000000,0xXXXXCAFE
 * @param fifo the text input
 * @param ids the LPC IDs
 * @param idMasks the mask values to apply.
 * @return the number of elements parsed
 */
int fifoParseLpcIds(Fifo* fifo, Uint32 ids[LPC_IDS], Uint32 idMasks[LPC_IDS]);

extern LpcFamily const
	lpcFamily8xx,
	lpcFamily11xx,
	lpcFamily11Exx,
	lpcFamily12xx,
	lpcFamily13xx,
	lpcFamily13x5_6_7,
	lpcFamily15xx,
	lpcFamily17xx,
	//lpcFamily1788,		// LPC177x and LPC178x
	lpcFamily2101_2_3,
	lpcFamily2104_5_6,
	lpcFamily211x,
	lpcFamily212x,
	lpcFamily213x,
	lpcFamily214x,
	lpcFamily23xx,
	lpcFamily43x2,		// single FLASH bank versions
	lpcFamily43xx,
	lpcFamily5410x,
	lpcFamily5411x,
	lpcFamily546xx;

extern LpcIspFamily const
	lpcIsp8xx,
	lpcIsp111x,
	lpcIsp11Cxx,
	lpcIsp11Exx,
	lpcIsp12xx,
	lpcIsp13xx,
	lpcIsp15xx,
	lpcIsp17xx,
	lpcIsp210x,
	lpcIsp211x,
	lpcIsp212x,
	lpcIsp213x,
	lpcIsp214x,
	lpcIsp23xx,
	lpcIsp43xx,
	lpcIsp5410x,
	lpcIsp5411x,
	lpcIsp546xx;

/* If we use IAP on the microcontroller itself, we need to be able to include into the code the member's information
 * only.
 */
extern const LpcMember
	//lpcMember8,	///< generic
	//lpcMember800,	///< generic
	//lpcMember81,	///< generic
	//lpcMember810,	///< generic
	//lpcMember811,	///< generic
	//lpcMember812,	///< generic

	// LPC810
	lpcMember810_M021_FN8,

	lpcMember811_M001_JDH16,
	lpcMember812_M101_JDH16,
	lpcMember812_M101_JD20,
	lpcMember812_M101_JDH20,
	lpcMember812_M101_JTB16,

	// LPC820
	lpcMember822_M101_JDH20,
	lpcMember822_M101_JHI33,
	lpcMember824_M201_JDH20,
	lpcMember824_M201_JHI33,

	// LPC830
	lpcMember832_M101_FDH20,
	lpcMember834_M201_FHI33,

	//lpcMember11,	///< generic
	//lpcMember11E,	///< generic

	lpcMember1110_FD20_ALT1,
	lpcMember1110_FD20_ALT2,

	lpcMember1111_M002_FDH20_ALT1,
	lpcMember1111_M002_FDH20_ALT2,
	lpcMember1111_M101_FHN33,
	lpcMember1111_M102_FHN33,
	lpcMember1111_M103_FHN33,
	lpcMember1111_M201_FHN33,	
	lpcMember1111_M202_FHN33,
	lpcMember1111_M203_FHN33,

	lpcMember1112_M101_FHN33,
	lpcMember1112_M102_FD20_FDH20_FDH28_ALT1,
	lpcMember1112_M102_FD20_FDH20_FDH28_ALT2,
	lpcMember1112_M102_FHN33,
	lpcMember1112_M103_FHN33,
	lpcMember1112_M201_FHN33,
	lpcMember1112_M202_FHN24_FHI33_FHN33,
	lpcMember1112_M203_FHI33_FHN33,

	lpcMember1113_M201_FHN33,
	lpcMember1113_M202_FHN33,
	lpcMember1113_M203_FHN33,
	lpcMember1113_M301_FHN33_FBD48,
	lpcMember1113_M302_FHN33_FBD48,
	lpcMember1113_M303_FHN33_FBD48,

	lpcMember1114_M102_FDH28_FN28_ALT1,
	lpcMember1114_M102_FDH28_FN28_ALT2,
	lpcMember1114_M201_FHN33,
	lpcMember1114_M202_FHN33,
	lpcMember1114_M203_FHN33,
	lpcMember1114_M301_FHN33_FBD48,
	lpcMember1114_M302_FHI33_FHN33_FBD48_FBD100,
	lpcMember1114_M303_FHI33_FHN33_FBD48,
	lpcMember1114_M323_FBD48,
	lpcMember1114_M333_FHN33_FBD48,

	lpcMember1115_M303_FBD48,

	lpcMember11C12_M301_FBD48,
	lpcMember11C14_M301_FBD48,
	lpcMember11C22_M301_FBD48,
	lpcMember11C24_M301_FBD48,


	lpcMember11E11_M101_FN33,
	lpcMember11E12_M201_FBD48,
	lpcMember11E13_M301_FBD48,
	lpcMember11E14_M401_FN33_FBD48_FBD64,
	lpcMember11E36_M501_FN33_FBD64,
	lpcMember11E37_M401_FBD64,
	lpcMember11E37_M501_FBD48_FBD64,


	// lpcMember12,
	lpcMember1224_M101,
	lpcMember1224_M121,
	lpcMember1225_M301,
	lpcMember1225_M321,
	lpcMember1226_M301,
	lpcMember1227_M301,


	//lpcMember13,
	lpcMember1311,
	lpcMember1311_01,
	lpcMember1313,
	lpcMember1313_01,
	lpcMember1342,
	lpcMember1343,

	lpcMember1315,
	lpcMember1316,
	lpcMember1317,
	lpcMember1345,
	lpcMember1346,
	lpcMember1347,

	//lpcMember15,	///< generic
	lpcMember1517,
	lpcMember1518,
	lpcMember1519,
	lpcMember1547,
	lpcMember1548,
	lpcMember1549,

	//lpcMember17,	///< generic
	//lpcMember1700,	///< generic

	lpcMember1751,
	lpcMember1752,
	lpcMember1754,
	lpcMember1756,
	lpcMember1758,
	lpcMember1759,

	lpcMember1764,
	lpcMember1765,
	lpcMember1766,
	lpcMember1767,
	lpcMember1768,
	lpcMember1769,

	
	//lpcMember177,
	lpcMember1774,
	lpcMember1776,
	lpcMember1777,
	lpcMember1778,

	//lpcMember178,
	lpcMember1785,
	lpcMember1786,
	lpcMember1787,
	lpcMember1788,


	lpcMember2101,
	lpcMember2102,
	lpcMember2103,
	lpcMember2104,
	lpcMember2105,
	lpcMember2106,


	lpcMember2114,
	lpcMember2124,
	lpcMember2129,		// thanks to Alexander Inyukhin :-) for this member


	//lpcMember213x,	// generic
	lpcMember2131,
	lpcMember2132,
	lpcMember2134,
	lpcMember2136,
	lpcMember2138,


	//lpcMember214x,	///< generic
	lpcMember2141,
	lpcMember2142,
	lpcMember2144,
	lpcMember2146,
	lpcMember2148,


	//lpcMember236x,	///< generic
	lpcMember2364,
	lpcMember2365,
	lpcMember2366,
	lpcMember2367,
	lpcMember2368,
	lpcMember2377,
	lpcMember2378,
	lpcMember2378IR,
	lpcMember2387,
	lpcMember2388,

	lpcMember4310,	///< flashless
	lpcMember4320,	///< flashless
	lpcMember4330,	///< flashless
	lpcMember4350,	///< flashless
	lpcMember4370,	///< flashless

	lpcMember4312,
	lpcMember4313,
	lpcMember4315,
	lpcMember4317,
	lpcMember4322,
	lpcMember4323,
	lpcMember4325,
	lpcMember4327,
	lpcMember4333,
	lpcMember4337,
	lpcMember4353,
	lpcMember4357,

	lpcMember54101_J256,
	lpcMember54101_J512,
	lpcMember54102_J256,
	lpcMember54102_J512,

	lpcMember54113_J128,
	lpcMember54113_J256,
	lpcMember54114_J256
	;

// families
extern const LpcMember * const lpcMembers8xx[];
extern const LpcMember * const lpcMembers11xx[];
extern const LpcMember * const lpcMembers11Exx[];
extern const LpcMember * const lpcMembers12xx[];
extern const LpcMember * const lpcMembers13xx[];
extern const LpcMember * const lpcMembers15xx[];
extern const LpcMember * const lpcMembers17xx[];
extern const LpcMember * const lpcMembers23xx[];
extern const LpcMember * const lpcMembers43xx[];
extern const LpcMember * const lpcMembers541xx[];

extern const LpcMember * const lpcMembersXxxx[];	///< All known members.

enum {
	// LPC810
	LPC_ID_810_M021_FN8		=0x00008100,		// unavailable as of 2017
	// replaced by the -J- versions of same IDs
	// LPC_ID_811_M001_FDH16	=0x00008110,
	// LPC_ID_812_M101_FDH16	=0x00008120,
	// LPC_ID_812_M101_FD20		=0x00008121,
	// LPC_ID_812_M101_FDH20	=0x00008122,
	LPC_ID_811_M001_JDH16		=0x00008110,
	LPC_ID_812_M101_JDH16		=0x00008120,
	LPC_ID_812_M101_JD20		=0x00008121,
	LPC_ID_812_M101_JDH20		=0x00008122,
	LPC_ID_812_M101_JTB16		=0x00008122,	// same as above!

	// LPC820
	LPC_ID_822_M101_JDH20		=0x00008222,
	LPC_ID_822_M101_JHI33		=0x00008221,
	LPC_ID_824_M201_JDH20		=0x00008242,
	LPC_ID_824_M201_JHI33		=0x00008241,

	// LPC830
	LPC_ID_832_M101_FDH20		=0x00008322,
	LPC_ID_834_M201_FHI33		=0x00008341,

	LPC_ID_1110_FD20_ALT1				=0x0A07102B,
	LPC_ID_1110_FD20_ALT2				=0x1A07102B,

	LPC_ID_1111_M002_FDH20_ALT1			=0x0A16D02B,
	LPC_ID_1111_M002_FDH20_ALT2			=0x1A16D02B,
	LPC_ID_1111_M101_FHN33				=0x041E502B,
	LPC_ID_1111_M102_FHN33				=0x2516D02B,
	LPC_ID_1111_M103_FHN33				=0x00010013,
	LPC_ID_1111_M201_FHN33				=0x0416502B,	
	LPC_ID_1111_M202_FHN33				=0x2516902B,
	LPC_ID_1111_M203_FHN33				=0x00010012,

	LPC_ID_1112_M101_FHN33				=0x042D502B,
	LPC_ID_1112_M102_FD20_FDH20_FDH28_ALT1		=0x0A24902B,
	LPC_ID_1112_M102_FD20_FDH20_FDH28_ALT2		=0x1A24902B,
	LPC_ID_1112_M102_FHN33				=0x2524D02B,
	LPC_ID_1112_M201_FHN33				=0x0425502B,
	LPC_ID_1112_M202_FHN24_FHI33_FHN33		=0x2524902B,
	LPC_ID_1112_M103_FHN33				=0x00020023,
	LPC_ID_1112_M203_FHI33_FHN33			=0x00020022,

	LPC_ID_1113_M201_FHN33				=0x0434502B,
	LPC_ID_1113_M202_FHN33				=0x2532902B,
	LPC_ID_1113_M301_FHN33_FBD48			=0x0434102B,
	LPC_ID_1113_M302_FHN33_FBD48			=0x2532102B,
	LPC_ID_1113_M303_FHN33_FBD48			=0x00030030,
	LPC_ID_1113_M203_FHN33				=0x00030032,

	LPC_ID_1114_M102_FDH28_FN28_ALT1		=0x0A40902B,
	LPC_ID_1114_M102_FDH28_FN28_ALT2		=0x1A40902B,
	LPC_ID_1114_M201_FHN33				=0x0444502B,
	LPC_ID_1114_M202_FHN33				=0x2540902B,
	LPC_ID_1114_M203_FHN33				=0x00040042,
	LPC_ID_1114_M301_FHN33_FBD48			=0x0444102B,
	LPC_ID_1114_M302_FHI33_FHN33_FBD48_FBD100	=0x2540102B,
	LPC_ID_1114_M303_FHI33_FHN33_FBD48		=0x00040040,
	LPC_ID_1114_M323_FBD48				=0x00040060,
	LPC_ID_1114_M333_FHN33_FBD48			=0x00040070,

	LPC_ID_1115_M303_FBD48				=0x00050080,

	LPC_ID_11C12_M301_FBD48				=0x1421102B,
	LPC_ID_11C14_M301_FBD48				=0x1440102B,
	LPC_ID_11C22_M301_FBD48				=0x1431102B,
	LPC_ID_11C24_M301_FBD48				=0x1430102B,

	LPC_ID_11E11_M101_FN33				=0x293E902B,
	LPC_ID_11E12_M201_FBD48				=0x2954502B,
	LPC_ID_11E13_M301_FBD48				=0x296A102B,
	LPC_ID_11E14_M401_FN33_FBD48_FBD64		=0x2980102B,
	LPC_ID_11E36_M501_FN33_FBD64			=0x00009C41,
	LPC_ID_11E37_M401_FBD64				=0x00007C45,
	LPC_ID_11E37_M501_FBD48_FBD64			=0x00007C41,


	LPC_ID_1224_M101		= 0x3640C02B,
	LPC_ID_1224_M121		= 0x3642C02B,
	LPC_ID_1225_M301		= 0x3650C02B,
	LPC_ID_1225_M321		= 0x3652C02B,
	LPC_ID_1226_M301		= 0x3660C02B,
	LPC_ID_1227_M301		= 0x3670C02B,


	LPC_ID_1311	= 0x2C42502B,
	LPC_ID_1311_01	= 0x1816902B,
	LPC_ID_1313	= 0x2C40102B,
	LPC_ID_1313_01	= 0x1830102B,
	LPC_ID_1342	= 0x3D01402B,
	LPC_ID_1343	= 0x3D00002B,

	LPC_ID_1315	= 0x3A010523,
	LPC_ID_1316	= 0x1A018524,
	LPC_ID_1317	= 0x1A020525,
	LPC_ID_1345	= 0x28010541,
	LPC_ID_1346	= 0x08018542,
	LPC_ID_1347	= 0x08020543,

	LPC_ID_1517	= 0x00001517,
	LPC_ID_1518	= 0x00001518,
	LPC_ID_1519	= 0x00001519,
	LPC_ID_1547	= 0x00001547,
	LPC_ID_1548	= 0x00001548,
	LPC_ID_1549	= 0x00001549,

	LPC_ID_1751	= 0x25001118,
	LPC_ID_1752	= 0x25001121,
	LPC_ID_1754	= 0x25011722,
	LPC_ID_1756	= 0x25011723,
	LPC_ID_1758	= 0x25013F37,
	LPC_ID_1759	= 0x25113737,

	LPC_ID_1764	= 0x26011922,
	LPC_ID_1765	= 0x26013733,
	LPC_ID_1766	= 0x26013F33,
	LPC_ID_1767	= 0x26012837,
	LPC_ID_1768	= 0x26013F37,
	LPC_ID_1769	= 0x26113F37,

	LPC_ID_1774	= 0x27011132,
	LPC_ID_1776	= 0x27191F43,
	LPC_ID_1777	= 0x27193747,
	LPC_ID_1778	= 0x27193F47,

	LPC_ID_1785	= 0x281D1743,
	LPC_ID_1786	= 0x281D1F43,
	LPC_ID_1787	= 0x281D3747,
	LPC_ID_1788	= 0x281D3F47,


	LPC_ID_2101	= 0,	// same as LPC2103
	LPC_ID_2102	= 0,	// same as PLC2103
	LPC_ID_2103	= 0x0004FF11,

	LPC_ID_2104	= 0xFFF0FF12,
	LPC_ID_2105	= 0xFFF0FF22,
	LPC_ID_2106	= 0xFFF0FF32,


	LPC_ID_2114	= 0x0101FF12,
	LPC_ID_2124	= 0x0101FF13,
	LPC_ID_2129	= 0x0201FF13,


	LPC_ID_2131	= 0x0002FF01,
	LPC_ID_2132	= 0x0002FF11,
	LPC_ID_2134	= 0x0002FF12,
	LPC_ID_2136	= 0x0002FF23,
	LPC_ID_2138	= 0x0002FF25,


	LPC_ID_2141	= 0x0402FF01,
	LPC_ID_2142	= 0x0402FF11,
	LPC_ID_2144	= 0x0402FF12,
	LPC_ID_2146	= 0x0402FF23,
	LPC_ID_2148	= 0x0402FF25,


	LPC_ID_2364	= 0x1600F902,
	LPC_ID_2365	= 0x1600E823,
	LPC_ID_2366	= 0x1600F923,
	LPC_ID_2367	= 0x1600E825,
	LPC_ID_2368	= 0x1600F925,
	LPC_ID_2377	= 0x1700E825,
	LPC_ID_2378	= 0x1700FD25,
	LPC_ID_2378IR	= 0x0703FF25,	///< initial revision devices
	LPC_ID_2387	= 0x1800F935,
	LPC_ID_2388	= 0x1800FF35,

	// LPC43xx: only lowest 8 bits of ID2s must be used, other bits: mask out!
	LPC_ID_4310	=0xA00A3B3F,	///< flashless
	LPC_ID_4320	=0xA0008B3C,	///< flashless
	LPC_ID_4330	=0xA0000A30,	///< flashless
	LPC_ID_4350	=0xA0000830,	///< flashless
	LPC_ID_4370	=0x00000230,	///< flashless

	LPC_ID_4353_7		=0xA001C830,
		LPC_ID2_4353	=0x00000044,
		LPC_ID2_4357	=0x00000000,
	LPC_ID_4333_7		=0xA001CA30,
		LPC_ID2_4333	=0x00000044,
		LPC_ID2_4337	=0x00000000,
	LPC_ID_4322_3		=0xA00BCB3C,
		LPC_ID2_4322	=0x00000080,
		LPC_ID2_4323	=0x00000044,
	LPC_ID_4325_7		=0xA001CB3C,
		LPC_ID2_4325	=0x00000022,
		LPC_ID2_4327	=0x00000000,
	LPC_ID_4312_3		=0xA00BCB3F,
		LPC_ID2_4312	=0x00000080,
		LPC_ID2_4313	=0x00000044,
	LPC_ID_4315_7		=0xA001CB3F,
		LPC_ID2_4315	=0x00000022,
		LPC_ID2_4317	=0x00000000,


	// LPC5410x : single flash bank, simple IDs (1 word), all sectors same size; binary transfer;  UM Rev-2.4
	LPC_ID_54101_J256	=0x88454101,
	LPC_ID_54101_J512	=0x88854101,
	LPC_ID_54102_J256	=0x88454102,
	LPC_ID_54102_J512	=0x88854102,

	// LPC5411x : single flash bank, simple IDs (1 word), all sectors same size; binary transfer; UM Rev-1.6
	LPC_ID_54113_J128	=0x06254113,
	LPC_ID_54113_J256	=0x36454113,
	LPC_ID_54114_J256	=0x36454114,
};

#endif
