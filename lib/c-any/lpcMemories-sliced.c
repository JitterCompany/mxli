//HEADER
/*
  lpcMemories-sliced.c 
  Copyright 2011,2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <lpcMemories.h>
#include <macros.h>
#include <fifoParseStructure.h>
#include <fifoPrint.h>
#include <int32Math.h>
#include <fifoPrintFixedPoint.h>	// new fifoPrintInt32,...

//SLICE
bool lpcMatchByIds(const LpcMember *const m, Uint32 id[LPC_IDS]) {
	bool match = true;
	for (int i=0; i<LPC_IDS && m->family->idMasks[i]; i++)
		match = match && (id[i] & m->family->idMasks[i]) == m->ids[i];

	return match;
}

//SLICE
static bool startsWith(const char *prefix, const char *name) {
	while (*prefix!=0) {
		if (*prefix!=*name) return false;
		prefix++; name++;
	}
	return true;
}

bool lpcMatchByName(const LpcMember *const m, const char *name) {
	return startsWith(m->name,name);
}

//SLICE
int lpcMatchNumberOfIds(LpcMember const* member, Uint32 id) {
	if ((member->ids[0] & member->family->idMasks[0])==id) {
		for (int i=1; i<LPC_IDS; i++) {
			if (member->family->idMasks[i]==0) return i;
		}
		return LPC_IDS;	// maximum value
	}	
	else return 0;	// no match
}

//SLICE
int lpcFindNumberOfIds(LpcMember const* const* list, Uint32 id) {
	if (list) for (;*list; list++) {
		const int n = lpcMatchNumberOfIds(*list,id);
		if (n>0) return n;
	}	
	return 0;	// not found
}

//SLICE
const LpcMember* lpcFindByIds(const LpcMember *const *list, Uint32 ids[LPC_IDS]) {
	if (list) for (;*list; list++) if (lpcMatchByIds(*list,ids)) return *list;
	return 0;
}

//SLICE
const LpcMember* lpcFindByName(const LpcMember *const *list, const char *name) {
	if (list) for (;*list; list++) if (lpcMatchByName(*list,name)) return *list;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FLASH iteration

//SLICE
void lpcSectorIteratorNext (LpcSectorIterator *it) {
	const LpcFamily *f = it->member->family;

	it->offsetInBank += lpcSectorIteratorSize (it);
	it->sectorInGroup++ ;
	it->sectorInBank++;

	if (it->offsetInBank >= it->member->sizeFlashK * 1024) {	// FLASH bank size exeeded
		it->offsetInBank = 0;
		it->sectorInGroup = 0;
		it->sectorInBank = 0;
		it->groupIdx = 0;
		it->bankIdx ++;
	}
	else if (it->sectorInGroup >= f->sectorArrays [it->groupIdx].n) {		// next group
		it->sectorInGroup = 0;
		it->groupIdx ++;

		if (it->groupIdx >= LPC_SECTOR_ARRAYS		// end of FLASH (should not happen).
		|| f->sectorArrays [it->groupIdx].sizeK == 0) {	// FLASH greater than sector capacity (SNH)
			it->groupIdx = 0;
			it->bankIdx ++;
		}
	}
}

//SLICE
/** Calculates what sectors are affected from operations within an address range.
 * @param sectorFrom the first sector affected
 * @param sectorTo the last sector affected
 * @param addressFrom the lowest address of the range
 * @param addressTo the highest address of the range.
 * @return true, if the addresses are within the members FLASH range
 */
bool lpcAddressRangeToSectorRange(LpcMember const *member, Int32Pair *sectorRange, const Uint32Pair *addressRange) {
	LpcSectorIterator it = { member };

	while (lpcSectorIteratorHasNext (&it)) {
		if (lpcSectorIteratorContainsAddress (&it,addressRange->fst)) {
			sectorRange->fst = lpcSectorIteratorSector (&it);
		}
		if (lpcSectorIteratorContainsAddress (&it,addressRange->snd)) {
			sectorRange->snd = lpcSectorIteratorSector (&it);
			return sectorRange->fst>>_SECTOR_BANK == sectorRange->snd>>_SECTOR_BANK; 
		}
			
		lpcSectorIteratorNext (&it);
	}
	return false;	// at least end not found
}

//SLICE
int lpcBankToLastSector (LpcMember const *member, int bank) {
	const LpcFamily *family = member->family;
	if (0 < member->sizeFlashK
	&& (	bank==BANK_Z && family->banks==1		// unbanked
		|| bank>BANK_Z && bank < BANK_A+family->banks	// banked, and band in valid range
	)) {
		int sectors = 0;
		Uint32 sizeK = 0;
		for (int ss=0; ss<ELEMENTS(family->sectorArrays); ++ss) {
			for (int s=0; s<family->sectorArrays[ss].n && sizeK < member->sizeFlashK; s++) {
				sizeK += family->sectorArrays[ss].sizeK;
				sectors ++;
			}
		}
		return bank << _SECTOR_BANK | (sectors-1);
	}
	else return -1;
}

//SLICE
int lpcAddressToSector (LpcMember const *member, Uint32 address) {
	Int32Pair sectorRange;
	const Uint32Pair addressRange = { address, address };
	if (lpcAddressRangeToSectorRange (member, &sectorRange, &addressRange)) return sectorRange.fst;
	else return -1;
}

//SLICE
Uint32 lpcAddressToBankAddress (LpcMember const *member, Uint32 address) {
	int bank = lpcAddressToBank (member, address);
	switch(bank) {
		case -1:
		case BANK_Z:	return 0;
		default:	return member->family->addressFlashs [bank-BANK_A];
	}
}

//SLICE
Uint32 lpcAddressToBank (LpcMember const *member, Uint32 address) {
	int sector = lpcAddressToSector (member, address);
	return sector!=-1 ? sector >> _SECTOR_BANK : -1;
}

//SLICE
Uint32 lpcFamilyBankSize (LpcFamily const *family) {
	Uint32 size = 0;
	for (int g=0; g<LPC_SECTOR_ARRAYS && family->sectorArrays[g].n>=0; g++)
		size += family->sectorArrays[g].sizeK * 1024 * family->sectorArrays[g].n;

	return size;
}

//SLICE
// positive result +n = @ address+offset are n bytes available
// negative result -n = @ address+offset are n byte (at least) used by ISP
Int32 lpcIspGetBufferSize(const LpcIspRamUsage ramUsage [LPC_ISP_RAMS], Uint32 address, Uint32 offset, Int32 size) {
	// the LPC RAM block starts at address and extends until <address+size=end
	const Uint32 end = address+size;
	Uint32 lower = address+offset;
	if (lower>end) return 0;

	Uint32 upper = end;
	for (int r=0; r<LPC_ISP_RAMS; r++) {
		const Uint32 iAddress = ramUsage[r].address;
		if (iAddress<address || end<=iAddress) continue;	// RAM not in this region
		const Int32 iSize = ramUsage[r].size;
		// negative sizes indicate sizes from the top of the current block downwards (STACK, typically)
		Uint32 iStart = iSize>=0 ? iAddress : end+iSize;
		Uint32 iEnd = iSize>=0 ? iStart + iSize : end;

		if (iStart<=lower) {
			if (lower<iEnd) return lower-iEnd;	// region covered at the lower end.
			else continue; 				// iEnd <= lower	: no hit
		}
		else if (iStart<upper) upper = iStart;		// limit of upper bound only
		else continue; 					// needs more iterations to decide
	}
	return upper-lower;
}

/** Returns all RAM functional at ISP time and not used by the ISP handler on the given LPC device. The ISP handler
 * splits RAM0 into 2 pieces on most devices. This function returns the left-overs plus additional RAM if available.
 * @param lpc the LPC device description
 * @param buffers an array for placement of the results at positions 0..number of non-empty regions-1
 * @return the number of non-zero length RAM regions
 */
int lpcIspGetBuffers(const LpcMember* lpc, LpcIspBuffer buffers [LPC_ISP_BUFFERS]) {
	int t=0;	// transfer ram index
	for (int r=0; t<LPC_ISP_BUFFERS && r<LPC_RAMS; r++) {	// forall possible RAMs at all
		const Uint32 size = lpc->sizeRamKs[r] * 1024;
		if (size>0) {	// ram available in this device
			Uint32 rStart = lpc->ispFamily->addressRams[r];
			Uint32 offset = 0;
			Int32 length;
			do {
				length = lpcIspGetBufferSize(lpc->ispFamily->ramUsage,rStart,offset,size);
				if (length>0) {	// block found
					//printf("Found: %d @ %d\n",length, rStart+offset);
					buffers[t].address = rStart+offset;
					buffers[t].size = length;
					t++;
					offset += length;
				}
				else if (length<0) offset -= length;
				else ; // done
			} while (t<LPC_ISP_BUFFERS && length!=0);
		}
	}
	return t;
}

//SLICE
Uint32 lpcCrpLevelToCode(int level) {
	switch(level) {
		case 1:	return 0x12345678u;
		case 2: return 0x87654321u;
		case 3: return 0x43218765u;
		case 4: return 0x4E697370;	// as ASCII: "Nisp" reversed
		default: return 0xFFFFFFFFu;	// blank FLASH value
	}
}

//SLICE
int lpcCrpCodeToLevel(Uint32 code) {
	switch(code) {
		case 0x12345678u: return 1;
		case 0x87654321u: return 2;
		case 0x43218765u: return 3;
		case 0x4E697370u: return 4;
		default: return 0;
	}
}

//SLICE
bool lpcCrpLevelAllowed(int levelAllow, int levelDesired) {
	return	levelAllow<=3 && levelDesired<=levelAllow
		|| levelAllow==4 && (levelDesired<=0 || levelDesired==4);
}

//SLICE
int lpcRamRegionSize(LpcMember const *member, Uint32 address) {
	for (int r=0; r<LPC_RAMS; r++) if (member->ispFamily->addressRams[r]==address) return member->sizeRamKs[r]*1024;
	return 0;
}

//SLICE
bool fifoPrintSector (Fifo *f, int sector) {
	const int bank = sector >> _SECTOR_BANK;
	bool success = true;
	if (bank) success = success && fifoPrintChar (f,'A'+bank-BANK_A);
	return success && fifoPrintInt32 (f, sector & SECTOR_MASK,1);
}

//SLICE
bool fifoPrintBank (Fifo *f, int bank) {
	return fifoPrintChar (f, bank!=BANK_Z ? 'A'+bank-BANK_A : 'Z');
}

//SLICE
bool fifoPrintLpcMember(Fifo *f, LpcMember const *m) {
	const LpcFamily* fam = m->family; 
	const LpcIspFamily* isp = m->ispFamily;

	bool success = fifoPrintString(f,m->name) && fifoPrintString(f,", IDs=");
	
	for (int i=0; i<LPC_IDS; i++) {
		if (i!=0) success = success && fifoPrintString(f,", ");
		success = success
			&& fifoPrintString(f,"0x")
			&& fifoPrintHex(f,m->ids[i],8,8)
			&& fifoPrintString(f," & 0x")
			&& fifoPrintHex(f,fam->idMasks[i],8,8);
	}

	success = success && fifoPrintString(f,"\nFLASH: ");
	for (int b=0; b<fam->banks; b++) {
		if (b!=0) success = success && fifoPrintString(f,", ");
		success = success
			&& fifoPrintUDec(f,m->sizeFlashK,1,10)
			&& fifoPrintString(f,"kiB @ 0x")
			&& fifoPrintHex(f,fam->addressFlashs[b],8,8);
	};

	success = success
		&& fifoPrintString(f,"\nRAM:   ")
		;
	for (int r=0; r<ELEMENTS(m->sizeRamKs) && success; r++) if ( m->sizeRamKs[r]!=0) {
		if (r!=0) success = success && fifoPrintString(f,", ");
		success = success
		&& fifoPrintUDec(f,m->sizeRamKs[r],1,10)
		&& fifoPrintString(f,"kiB @ 0x")
		&& fifoPrintHex(f,isp->addressRams[r],8,8)
		;
	}
	success = success
		&& fifoPrintLn(f)
		&& fifoPrintString(f,"ISP data protocol:")
		&& fifoPrintString(f,isp->protocol==ISP_PROTOCOL_UUENCODE ? "UUENCODE" : (
			isp->protocol==ISP_PROTOCOL_BINARY ? "BINARY" : "UNKNOWN"))
		&& fifoPrintLn(f);

	success = success
		&& fifoPrintString(f,"ISP RAM usage:");
	for (int i=0; i<LPC_ISP_RAMS && isp->ramUsage[i].size!=0; i++) {
		if (i!=0) success = success && fifoPrintString(f,", ");
		const Int32 rawSize = isp->ramUsage[i].size;
		const Uint32 rawAddress = isp->ramUsage[i].address;
		const Uint32 start	= rawSize >= 0 ? rawAddress
					: rawAddress + lpcRamRegionSize (m, rawAddress) + rawSize;
		const Uint32 end	= rawSize >= 0 ? rawAddress + rawSize -1 : start -rawSize -1;
		success = success
			&& fifoPrintString(f,"0x")
			&& fifoPrintHex(f,start,8,8)
			&& fifoPrintString(f,"..0x")
			&& fifoPrintHex(f,end,8,8) ;
	}
	success = success && fifoPrintLn(f);
	LpcSectorIterator it = { m, };
	while (lpcSectorIteratorHasNext (&it)) {
		//const int bank = lpcSectorIteratorBank (&it);
		const int sector = lpcSectorIteratorSector (&it);
		const Uint32 sectorStart = lpcSectorIteratorAddress (&it);
		const Uint32 sectorSize = lpcSectorIteratorSize (&it);
		const Uint32 sectorEnd = sectorStart + sectorSize-1;

		success = success
			&& fifoPrintSector (f, sector)
			&& fifoPrintString (f, (sector & SECTOR_MASK) < 10 ? " " : "")
			&& fifoPrintString (f,": 0x")
			&& fifoPrintHex (f, sectorStart,8,8)
			&& fifoPrintString (f,"..0x")
			&& fifoPrintHex (f, sectorEnd,8,8)
			&& fifoPrintString (f," ")
			&& fifoPrintUint32 (f,sectorSize/1024,1)
			&& fifoPrintString (f,"kiB")
			&& fifoPrintLn (f);

		lpcSectorIteratorNext (&it);
	}

	success = success && fifoPrintString(f,"Copy RAM to FLASH block sizes: ");
	for (int b=0; b<ELEMENTS(fam->blockSizes); ++b) {
		if (b!=0) success = success && fifoPrintString(f,", ");
		success = success && fifoPrintUDec(f,fam->blockSizes[b],1,10);
	}
	return success && fifoPrintLn(f);
}

//SLICE
/** Pretty-prints the IDs along with their masks as hex numbers
 */
bool fifoPrintLpcIds(Fifo* fifo, Uint32 ids[LPC_IDS], Uint32 idMasks[LPC_IDS]) {
	for (int i=0; i<LPC_IDS; i++) {
		if (i!=0) if (!fifoPrintString(fifo,", ")) return false;
		if (!fifoPrintHex32Masked(fifo,ids[i],idMasks[i],'X')) return false;
	}
	return true;
}

//SLICE
int fifoParseLpcIds(Fifo* fifo, Uint32 ids[LPC_IDS], Uint32 idMasks[LPC_IDS]) {
	Fifo clone = *fifo;
	int i;
	for (i=0; i<LPC_IDS; i++) {
		if ( (i==0 || fifoParseExactChar(&clone,','))
		&& fifoParseHex32Masked(&clone,&ids[i],&idMasks[i],'X') ) ; // fine
		else break;
	}
	fifoCopyReadPosition(fifo,&clone);
	return i;
}


//SLICE
const LpcMember* const lpcMembers8xx[] = {
	// generic ones
	//&lpcMember8,
	//&lpcMember810,
	//&lpcMember811,
	//&lpcMember812,
	
	// real ones
	&lpcMember810_M021_FN8,
	&lpcMember811_M001_JDH16,
	&lpcMember812_M101_JDH16,
	&lpcMember812_M101_JD20,
	&lpcMember812_M101_JDH20,
	// &lpcMember812_M101_JTB16,	// same ID as above

	// LPC820
	&lpcMember822_M101_JDH20,
	&lpcMember822_M101_JHI33,
	&lpcMember824_M201_JDH20,
	&lpcMember824_M201_JHI33,

	// LPC830
	&lpcMember832_M101_FDH20,
	&lpcMember834_M201_FHI33,

	0	// KEEP this!
};

//SLICE
const LpcMember* const lpcMembers11xx[] = {
	&lpcMember1110_FD20_ALT1,
	&lpcMember1110_FD20_ALT2,

	&lpcMember1111_M002_FDH20_ALT1,
	&lpcMember1111_M002_FDH20_ALT2,
	&lpcMember1111_M101_FHN33,
	&lpcMember1111_M102_FHN33,
	&lpcMember1111_M103_FHN33,
	&lpcMember1111_M201_FHN33,	
	&lpcMember1111_M202_FHN33,
	&lpcMember1111_M203_FHN33,

	&lpcMember1112_M101_FHN33,
	&lpcMember1112_M102_FD20_FDH20_FDH28_ALT1,
	&lpcMember1112_M102_FD20_FDH20_FDH28_ALT2,
	&lpcMember1112_M102_FHN33,
	&lpcMember1112_M103_FHN33,
	&lpcMember1112_M201_FHN33,
	&lpcMember1112_M202_FHN24_FHI33_FHN33,
	&lpcMember1112_M203_FHI33_FHN33,

	&lpcMember1113_M201_FHN33,
	&lpcMember1113_M202_FHN33,
	&lpcMember1113_M203_FHN33,
	&lpcMember1113_M301_FHN33_FBD48,
	&lpcMember1113_M302_FHN33_FBD48,
	&lpcMember1113_M303_FHN33_FBD48,

	&lpcMember1114_M102_FDH28_FN28_ALT1,
	&lpcMember1114_M102_FDH28_FN28_ALT2,
	&lpcMember1114_M201_FHN33,
	&lpcMember1114_M202_FHN33,
	&lpcMember1114_M203_FHN33,
	&lpcMember1114_M301_FHN33_FBD48,
	&lpcMember1114_M302_FHI33_FHN33_FBD48_FBD100,
	&lpcMember1114_M303_FHI33_FHN33_FBD48,
	&lpcMember1114_M323_FBD48,
	&lpcMember1114_M333_FHN33_FBD48,

	&lpcMember1115_M303_FBD48,

	&lpcMember11C12_M301_FBD48,
	&lpcMember11C14_M301_FBD48,
	&lpcMember11C22_M301_FBD48,
	&lpcMember11C24_M301_FBD48,

	//&lpcMember11E,	// generic
	&lpcMember11E11_M101_FN33,
	&lpcMember11E12_M201_FBD48,
	&lpcMember11E13_M301_FBD48,
	&lpcMember11E14_M401_FN33_FBD48_FBD64,
	&lpcMember11E36_M501_FN33_FBD64,
	&lpcMember11E37_M401_FBD64,
	&lpcMember11E37_M501_FBD48_FBD64,
	0
};

//SLICE
const LpcMember* const lpcMembers12xx[] = {
	//&lpcMember12,	// generic
	&lpcMember1224_M101,
	&lpcMember1224_M121,
	&lpcMember1225_M301,
	&lpcMember1225_M321,
	&lpcMember1226_M301,
	&lpcMember1227_M301,
	0
};

//SLICE
const LpcMember* const lpcMembers13xx[] = {
	&lpcMember1311,
	&lpcMember1311_01,
	&lpcMember1313,
	&lpcMember1313_01,
	&lpcMember1342,
	&lpcMember1343,

	&lpcMember1315,
	&lpcMember1316,
	&lpcMember1317,
	&lpcMember1345,
	&lpcMember1346,
	&lpcMember1347,
	0
};

//SLICE
const LpcMember* const lpcMembers15xx[] = {
	//&lpcMember15,	//generic
	&lpcMember1517,
	&lpcMember1518,
	&lpcMember1519,
	&lpcMember1547,
	&lpcMember1548,
	&lpcMember1549,
	0
};

//SLICE
const LpcMember* const lpcMembers17xx[] = {
	&lpcMember1751,
	&lpcMember1752,
	&lpcMember1754,
	&lpcMember1756,
	&lpcMember1758,
	&lpcMember1759,
	&lpcMember1764,
	&lpcMember1765,
	&lpcMember1766,
	&lpcMember1767,
	&lpcMember1768,
	&lpcMember1769,

	&lpcMember1774,
	&lpcMember1776,
	&lpcMember1777,
	&lpcMember1778,
	&lpcMember1785,
	&lpcMember1786,
	&lpcMember1787,
	&lpcMember1788,
	0
};

//SLICE
const LpcMember* const lpcMembers210x[] = {
	&lpcMember2101,
	&lpcMember2102,
	&lpcMember2103,
	&lpcMember2104,
	&lpcMember2105,
	&lpcMember2106,
	0
};

//SLICE
const LpcMember* const lpcMembers211x[] = {
	&lpcMember2114,
	&lpcMember2124,
	&lpcMember2129,
	0
};


//SLICE
const LpcMember* const lpcMembers213x[] = {
	&lpcMember2131,
	&lpcMember2132,
	&lpcMember2134,
	&lpcMember2136,
	&lpcMember2138,
	0
};

//SLICE
const LpcMember* const lpcMembers214x[] = {
	&lpcMember2141,
	&lpcMember2142,
	&lpcMember2144,
	&lpcMember2146,
	&lpcMember2148,
	0
};

//SLICE
const LpcMember* const lpcMembers23xx[] = {
	&lpcMember2364,
	&lpcMember2365,
	&lpcMember2366,
	&lpcMember2367,
	&lpcMember2368,
	&lpcMember2377,
	&lpcMember2378,
	&lpcMember2378IR,
	&lpcMember2387,
	&lpcMember2388,
	0
};
//SLICE
const LpcMember* const lpcMembersLpc43xx[] = {
	&lpcMember4310,	///< flashless
	&lpcMember4320,	///< flashless
	&lpcMember4330,	///< flashless
	&lpcMember4350,	///< flashless
	&lpcMember4370,	///< flashless

	&lpcMember4312,
	&lpcMember4313,
	&lpcMember4315,
	&lpcMember4317,
	&lpcMember4322,
	&lpcMember4323,
	&lpcMember4325,
	&lpcMember4327,
	&lpcMember4333,
	&lpcMember4337,
	&lpcMember4353,
	&lpcMember4357,
	0
};

//SLICE
const LpcMember* const lpcMembersLpc541xx[] = {
	&lpcMember54101_J256,
	&lpcMember54101_J512,
	&lpcMember54102_J256,
	&lpcMember54102_J512,

	&lpcMember54113_J128,
	&lpcMember54113_J256,
	&lpcMember54114_J256,
	0
};

//SLICE
const LpcMember* const lpcMembersXxxx[] = {
	//&lpcMember8,
	// real ones
	&lpcMember810_M021_FN8,
	&lpcMember811_M001_JDH16,
	&lpcMember812_M101_JDH16,
	&lpcMember812_M101_JD20,
	&lpcMember812_M101_JDH20,
	// &lpcMember812_M101_JTB16, // same as above

	// LPC820
	&lpcMember822_M101_JDH20,
	&lpcMember822_M101_JHI33,
	&lpcMember824_M201_JDH20,
	&lpcMember824_M201_JHI33,

	// LPC830
	&lpcMember832_M101_FDH20,
	&lpcMember834_M201_FHI33,

	&lpcMember1110_FD20_ALT1,
	&lpcMember1110_FD20_ALT2,

	&lpcMember1111_M002_FDH20_ALT1,
	&lpcMember1111_M002_FDH20_ALT2,
	&lpcMember1111_M101_FHN33,
	&lpcMember1111_M102_FHN33,
	&lpcMember1111_M103_FHN33,
	&lpcMember1111_M201_FHN33,	
	&lpcMember1111_M202_FHN33,
	&lpcMember1111_M203_FHN33,

	&lpcMember1112_M101_FHN33,
	&lpcMember1112_M102_FD20_FDH20_FDH28_ALT1,
	&lpcMember1112_M102_FD20_FDH20_FDH28_ALT2,
	&lpcMember1112_M102_FHN33,
	&lpcMember1112_M103_FHN33,
	&lpcMember1112_M201_FHN33,
	&lpcMember1112_M202_FHN24_FHI33_FHN33,
	&lpcMember1112_M203_FHI33_FHN33,

	&lpcMember1113_M201_FHN33,
	&lpcMember1113_M202_FHN33,
	&lpcMember1113_M203_FHN33,
	&lpcMember1113_M301_FHN33_FBD48,
	&lpcMember1113_M302_FHN33_FBD48,
	&lpcMember1113_M303_FHN33_FBD48,

	&lpcMember1114_M102_FDH28_FN28_ALT1,
	&lpcMember1114_M102_FDH28_FN28_ALT2,
	&lpcMember1114_M201_FHN33,
	&lpcMember1114_M202_FHN33,
	&lpcMember1114_M203_FHN33,
	&lpcMember1114_M301_FHN33_FBD48,
	&lpcMember1114_M302_FHI33_FHN33_FBD48_FBD100,
	&lpcMember1114_M303_FHI33_FHN33_FBD48,
	&lpcMember1114_M323_FBD48,
	&lpcMember1114_M333_FHN33_FBD48,

	&lpcMember1115_M303_FBD48,

	&lpcMember11C12_M301_FBD48,
	&lpcMember11C14_M301_FBD48,
	&lpcMember11C22_M301_FBD48,
	&lpcMember11C24_M301_FBD48,

	//&lpcMember11E,
	&lpcMember11E11_M101_FN33,
	&lpcMember11E12_M201_FBD48,
	&lpcMember11E13_M301_FBD48,
	&lpcMember11E14_M401_FN33_FBD48_FBD64,
	&lpcMember11E36_M501_FN33_FBD64,
	&lpcMember11E37_M401_FBD64,
	&lpcMember11E37_M501_FBD48_FBD64,

	//&lpcMember12,
	&lpcMember1224_M101,
	&lpcMember1224_M121,
	&lpcMember1225_M301,
	&lpcMember1225_M321,
	&lpcMember1226_M301,
	&lpcMember1227_M301,

	&lpcMember1311,
	&lpcMember1311_01,
	&lpcMember1313,
	&lpcMember1313_01,
	&lpcMember1342,
	&lpcMember1343,

	&lpcMember1315,
	&lpcMember1316,
	&lpcMember1317,
	&lpcMember1345,
	&lpcMember1346,
	&lpcMember1347,


	//&lpcMember15,
	&lpcMember1517,
	&lpcMember1518,
	&lpcMember1519,
	&lpcMember1547,
	&lpcMember1548,
	&lpcMember1549,


	//&lpcMember17,
	&lpcMember1751,
	&lpcMember1752,
	&lpcMember1754,
	&lpcMember1756,
	&lpcMember1758,
	&lpcMember1759,
	&lpcMember1764,
	&lpcMember1765,
	&lpcMember1766,
	&lpcMember1767,
	&lpcMember1768,
	&lpcMember1769,
	&lpcMember1774,
	//&lpcMember177,
	&lpcMember1776,
	&lpcMember1777,
	&lpcMember1778,
	//&lpcMember178,
	&lpcMember1785,
	&lpcMember1786,
	&lpcMember1787,
	&lpcMember1788,

	&lpcMember2101,
	&lpcMember2102,
	&lpcMember2103,
	&lpcMember2104,
	&lpcMember2105,
	&lpcMember2106,

	&lpcMember2114,
	&lpcMember2124,
	&lpcMember2129,

	&lpcMember2131,
	&lpcMember2132,
	&lpcMember2134,
	&lpcMember2136,
	&lpcMember2138,

	&lpcMember2141,
	&lpcMember2142,
	&lpcMember2144,
	&lpcMember2146,
	&lpcMember2148,

	&lpcMember2364,
	&lpcMember2365,
	&lpcMember2366,
	&lpcMember2367,
	&lpcMember2368,
	&lpcMember2377,
	&lpcMember2378,
	&lpcMember2378IR,
	&lpcMember2387,
	&lpcMember2388,

	&lpcMember4310,	///< flashless
	&lpcMember4320,	///< flashless
	&lpcMember4330,	///< flashless
	&lpcMember4350,	///< flashless
	&lpcMember4370,	///< flashless

	&lpcMember4312,
	&lpcMember4313,
	&lpcMember4315,
	&lpcMember4317,
	&lpcMember4322,
	&lpcMember4323,
	&lpcMember4325,
	&lpcMember4327,
	&lpcMember4333,
	&lpcMember4337,
	&lpcMember4353,
	&lpcMember4357,

	&lpcMember54101_J256,
	&lpcMember54101_J512,
	&lpcMember54102_J256,
	&lpcMember54102_J512,

	&lpcMember54113_J128,
	&lpcMember54113_J256,
	&lpcMember54114_J256,

	0	// terminator, required!
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// LPC800 family

//SLICE
const LpcFamily lpcFamily8xx = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=1, .n=32 }, { }, },	// LPC81x: 16 sectors, LPC82x/83x: 32 sectors
	.blockSizes = { 64, 128, 256, 512, 1024 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
const LpcIspFamily lpcIsp8xx = {
	.protocol = ISP_PROTOCOL_BINARY,
	.addressRams = { 0x10000000, },
	.ramUsage = { { 0x10000270-540, 540 }, },	// fixed stack usage @0x270, growing downwards <=540 (<0x220) bytes
							// and RAM up to 0x1000 0050 is untouched an preserved at boot.
};


//SLICE
/*
// generic type
const LpcMember lpcMember8 = {
	.name = "LPC8*",
	.sizeFlashK = 4,
	.sizeRamKs = { 1, },
	.ids = { 0 },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

// generic type
const LpcMember lpcMember800 = {
	.name = "LPC800*",
	.sizeFlashK = 4,
	.sizeRamKs = { 1, },
	.ids = { 0 },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

// generic type
const LpcMember lpcMember810 = {
	.name = "LPC810*",
	.sizeFlashK = 4,
	.sizeRamKs = { 1, },
	.ids = { 0 },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

// generic type
const LpcMember lpcMember811 = {
	.name = "LPC811*",
	.sizeFlashK = 4,
	.sizeRamKs = { 1, },
	.ids = { 0 },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

// generic type
const LpcMember lpcMember812 = {
	.name = "LPC812*",
	.sizeFlashK = 4,
	.sizeRamKs = { 1, },
	.ids = { 0 },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};
*/

//SLICE
const LpcMember lpcMember810_M021_FN8 = {
	.name = "LPC810 M021 FN8",
	.sizeFlashK = 4,
	.sizeRamKs = { 1, },
	.ids = { LPC_ID_810_M021_FN8, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember811_M001_JDH16 = {
	.name = "LPC811 M001 JDH16",
	.sizeFlashK = 8,
	.sizeRamKs = { 2, },
	.ids = { LPC_ID_811_M001_JDH16, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember812_M101_JDH16 = {
	.name = "LPC812 M101 JDH16",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_812_M101_JDH16, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember812_M101_JD20 = {
	.name = "LPC812 M101 JD20",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_812_M101_JD20, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember812_M101_JDH20 = {
	.name = "LPC812 M101 JDH20,JTB16",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_812_M101_JDH20, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember822_M101_JDH20 = {
	.name = "LPC822 M201 JDH20",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_822_M101_JDH20, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember822_M101_JHI33 = {
	.name = "LPC822 M101 JHI33",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_822_M101_JHI33, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember824_M201_JDH20 = {
	.name = "LPC824 M201 JDH20",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_824_M201_JDH20, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember824_M201_JHI33 = {
	.name = "LPC824 M201 JHI33",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_824_M201_JHI33, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember832_M101_FDH20 = {
	.name = "LPC832 M101 FDH20",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },			// yes, THAT little RAM :-(
	.ids = { LPC_ID_832_M101_FDH20, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};

//SLICE
const LpcMember lpcMember834_M201_FHI33 = {
	.name = "LPC834 M201 FHI33",
	.sizeFlashK = 32,
	.sizeRamKs = { 4, },			// yes, THAT little RAM :-(
	.ids = { LPC_ID_834_M201_FHI33, },
	.family = &lpcFamily8xx,
	.ispFamily = &lpcIsp8xx
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// LPC1100 family

//SLICE
const LpcFamily lpcFamily11xx = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=16 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
const LpcIspFamily lpcIsp11Cxx = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	// For at least LPC11xx and LPC11Exx the following holds:
	//
	// RAM up to 0x1000 0050 is untouched an preserved at boot.
	// ISP uses RAM from 0x1000 017C to 0x1000 025B
	// ISP command use top 32 bytes, the stack is located top-32 and may use up to 256 bytes
	.addressRams = { 0x10000000, },
	.ramUsage = {
		{ 0x1000017C, 0x25B-0x17C+1 },
		{ 0x10000000, -(256+32) },
	},
};

//SLICE
const LpcIspFamily lpcIsp111x = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	// For at least LPC11xx the following holds:
	//
	// RAM up to 0x1000 0050 is untouched an preserved at boot.
	// ISP uses RAM from 0x1000 0050 to 0x1000 017F
	// ISP command use top 32 bytes, the stack is located top-32 and may use up to 256 bytes
	.addressRams = { 0x10000000, },
	.ramUsage = {
		{ 0x10000050, 0x17F-0x050+1 },
		{ 0x10000000, -(256+32) },
	},
};


//SLICE
const LpcMember lpcMember1110_FD20_ALT1 = {
	.name = "LPC1110 FD20 (1)",
	.sizeFlashK = 4,
	.sizeRamKs = { 1, },
	.ids = { LPC_ID_1110_FD20_ALT1, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1110_FD20_ALT2 = {
	.name = "LPC1110 FD20 (2)",
	.sizeFlashK = 4,
	.sizeRamKs = { 1, },
	.ids = { LPC_ID_1110_FD20_ALT2, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};


//SLICE
const LpcMember lpcMember1111_M002_FDH20_ALT1 = {
	.name = "LPC1111 /002 FDH20 (1)",
	.sizeFlashK = 8,
	.sizeRamKs = { 2, },
	.ids = { LPC_ID_1111_M002_FDH20_ALT1, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1111_M002_FDH20_ALT2 = {
	.name = "LPC1111 /002 FDH20 (2)",
	.sizeFlashK = 8,
	.sizeRamKs = { 2, },
	.ids = { LPC_ID_1111_M002_FDH20_ALT2, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1111_M101_FHN33 = {
	.name = "LPC1111 /101 FHN33",
	.sizeFlashK = 8,
	.sizeRamKs = { 2, },
	.ids = { LPC_ID_1111_M101_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1111_M102_FHN33 = {
	.name = "LPC1111 /102 FHN33",
	.sizeFlashK = 8,
	.sizeRamKs = { 2, },
	.ids = { LPC_ID_1111_M102_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1111_M103_FHN33 = {
	.name = "LPC1111 /103 FHN33",
	.sizeFlashK = 8,
	.sizeRamKs = { 2, },
	.ids = { LPC_ID_1111_M103_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1111_M201_FHN33 = {
	.name = "LPC1111 /201 FHN33",
	.sizeFlashK = 8,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1111_M201_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};
	
//SLICE
const LpcMember lpcMember1111_M202_FHN33 = {
	.name = "LPC1111 /202 FHN33",
	.sizeFlashK = 8,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1111_M202_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};


//SLICE
const LpcMember lpcMember1111_M203_FHN33 = {
	.name = "LPC1111 /203 FHN33",
	.sizeFlashK = 8,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1111_M203_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};


//SLICE
const LpcMember lpcMember1112_M102_FD20_FDH20_FDH28_ALT1 = {
	.name = "LPC1112 /102 FD20,FDH20,FDH28 (1)",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1112_M102_FD20_FDH20_FDH28_ALT1, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1112_M102_FD20_FDH20_FDH28_ALT2 = {
	.name = "LPC1112 /102 FD20,FDH20,FDH28 (2)",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1112_M102_FD20_FDH20_FDH28_ALT2, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1112_M101_FHN33 = {
	.name = "LPC1112 /101 FHN33",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1112_M101_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1112_M102_FHN33 = {
	.name = "LPC1112 /102 FHN33",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1112_M102_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1112_M201_FHN33 = {
	.name = "LPC1112 /201 FHN33",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1112_M201_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1112_M202_FHN24_FHI33_FHN33 = {
	.name = "LPC1112 /202 FHN24,FHI33,FHN33",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1112_M202_FHN24_FHI33_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1112_M103_FHN33 = {
	.name = "LPC1112 /103 FHN33",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1112_M103_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1112_M203_FHI33_FHN33 = {
	.name = "LPC1112 /203 FHI33,FHN33",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1112_M203_FHI33_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};


//SLICE
const LpcMember lpcMember1113_M201_FHN33 = {
	.name = "LPC1113 /201 FHN33",
	.sizeFlashK = 24,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1113_M201_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1113_M202_FHN33 = {
	.name = "LPC1113 /202 FHN33",
	.sizeFlashK = 24,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1113_M202_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1113_M203_FHN33 = {
	.name = "LPC1113 /203 FHN33",
	.sizeFlashK = 24,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1113_M203_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1113_M301_FHN33_FBD48 = {
	.name = "LPC1113 /301 FHN33,FBD48",
	.sizeFlashK = 24,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1113_M301_FHN33_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1113_M302_FHN33_FBD48 = {
	.name = "LPC1113 /302 FHN33,FBD48",
	.sizeFlashK = 24,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1113_M302_FHN33_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1113_M303_FHN33_FBD48 = {
	.name = "LPC1113 /303 FHN33,FBD48",
	.sizeFlashK = 24,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1113_M303_FHN33_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};


//SLICE
const LpcMember lpcMember1114_M102_FDH28_FN28_ALT1 = {
	.name = "LPC1114 /102 FDH28,FN28 (1)",
	.sizeFlashK = 32,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1114_M102_FDH28_FN28_ALT1, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M102_FDH28_FN28_ALT2 = {
	.name = "LPC1114 /102 FDH28,FN28, (2)",
	.sizeFlashK = 32,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1114_M102_FDH28_FN28_ALT2, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M201_FHN33 = {
	.name = "LPC1114 /201 FHN33",
	.sizeFlashK = 32,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1114_M201_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M202_FHN33 = {
	.name = "LPC1114 /202 FHN33",
	.sizeFlashK = 32,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1114_M202_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M203_FHN33 = {
	.name = "LPC1114 /203 FHN33",
	.sizeFlashK = 32,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1114_M203_FHN33, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M301_FHN33_FBD48 = {
	.name = "LPC1114 /301 FHN33,FBD48",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1114_M301_FHN33_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M302_FHI33_FHN33_FBD48_FBD100 = {
	.name = "LPC1114 /302 FHI33,FHN33,FBD48,FBD100",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1114_M302_FHI33_FHN33_FBD48_FBD100, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M303_FHI33_FHN33_FBD48 = {
	.name = "LPC1114 /303 FHI33,FHN33,FBD48",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1114_M303_FHI33_FHN33_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M323_FBD48 = {
	.name = "LPC1114 /323 FBD48",
	.sizeFlashK = 48,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1114_M323_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};

//SLICE
const LpcMember lpcMember1114_M333_FHN33_FBD48 = {
	.name = "LPC1114 /333 FHN33,FBD48",
	.sizeFlashK = 56,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1114_M333_FHN33_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};


//SLICE
const LpcMember lpcMember1115_M303_FBD48 = {
	.name = "LPC1115 /303 FBD48",
	.sizeFlashK = 64,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1115_M303_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp111x,
};


//SLICE
const LpcMember lpcMember11C12_M301_FBD48 = {
	.name = "LPC11C12 /301 FBD48",
	.sizeFlashK = 16,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_11C12_M301_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp11Cxx,
};

//SLICE
const LpcMember lpcMember11C14_M301_FBD48 = {
	.name = "LPC11C14 /301 FBD48",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_11C14_M301_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp11Cxx,
};

//SLICE
const LpcMember lpcMember11C22_M301_FBD48 = {
	.name = "LPC11C22 /301 FBD48",
	.sizeFlashK = 16,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_11C22_M301_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp11Cxx,
};

//SLICE
const LpcMember lpcMember11C24_M301_FBD48 = {
	.name = "LPC11C24 /301 FBD48",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_11C24_M301_FBD48, },
	.family = &lpcFamily11xx,
	.ispFamily = &lpcIsp11Cxx,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// LPC11E00 family

//SLICE
const LpcFamily lpcFamily11Exx = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=32 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
const LpcIspFamily lpcIsp11Exx = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	// For LPC11Exx the following holds:
	//
	// RAM up to 0x1000 0050 is untouched an preserved at boot.
	// ISP uses RAM from 0x1000 017C to 0x1000 025B
	// ISP command use top 32 bytes, the stack is located top-32 and may use up to 256 bytes
	.addressRams = { 0x10000000, 0x20000000, 0x20004000 },
	.ramUsage = {
		{ 0x1000017C, 0x25B-0x17C+1 },
		{ 0x10000000, -(256+32) },
	},
};

// generic one
/*
const LpcMember lpcMember11E = {
	.name = "LPC11E*",
	.sizeFlashK = 8,
	.sizeRamKs = { 4, },
	.ids = { 0 },
	.family = &lpcFamily11Exx,
	.ispFamily = &lpcIsp11Exx,
};
*/

//SLICE
const LpcMember 	lpcMember11E11_M101_FN33 = {
	.name = "LPC11E11 /101 FN33",
	.sizeFlashK = 8,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_11E11_M101_FN33, },
	.family = &lpcFamily11Exx,
	.ispFamily = &lpcIsp11Exx,
};

//SLICE
const LpcMember 	lpcMember11E12_M201_FBD48 = {
	.name = "LPC11E12 /201 FBD48",
	.sizeFlashK = 16,
	.sizeRamKs = { 6, },
	.ids = { LPC_ID_11E12_M201_FBD48, },
	.family = &lpcFamily11Exx,
	.ispFamily = &lpcIsp11Exx,
};

//SLICE
const LpcMember 	lpcMember11E13_M301_FBD48 = {
	.name = "LPC11E13 /301 FBD48",
	.sizeFlashK = 24,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_11E13_M301_FBD48, },
	.family = &lpcFamily11Exx,
	.ispFamily = &lpcIsp11Exx,
};

//SLICE
const LpcMember 	lpcMember11E14_M401_FN33_FBD48_FBD64 = {
	.name = "LPC11E14 /401 FN33,FBD48,FBD64",
	.sizeFlashK = 32,
	.sizeRamKs = { 8,42, },
	.ids = { LPC_ID_11E14_M401_FN33_FBD48_FBD64, },
	.family = &lpcFamily11Exx,
	.ispFamily = &lpcIsp11Exx,
};

//SLICE
const LpcMember 	lpcMember11E36_M501_FN33_FBD64 = {
	.name = "LPC11E36 /501 FN33,FBD64",
	.sizeFlashK = 96,
	.sizeRamKs = { 8, 2, 2, },
	.ids = { LPC_ID_11E36_M501_FN33_FBD64, },
	.family = &lpcFamily11Exx,
	.ispFamily = &lpcIsp11Exx,
};

//SLICE
const LpcMember 	lpcMember11E37_M401_FBD64 = {
	.name = "LPC11E37 /401 FBD64",
	.sizeFlashK = 128,
	.sizeRamKs = { 8, 0, 2, },	// irregularity in RAM: for IO-handler only ??
	.ids = { LPC_ID_11E37_M401_FBD64, },
	.family = &lpcFamily11Exx,
	.ispFamily = &lpcIsp11Exx,
};

//SLICE
const LpcMember 	lpcMember11E37_M501_FBD48_FBD64 = {
	.name = "LPC11E37 /501 FBD48,FBD64",
	.sizeFlashK = 128,
	.sizeRamKs = { 8, 2, 2, },
	.ids = { LPC_ID_11E37_M501_FBD48_FBD64, },
	.family = &lpcFamily11Exx,
	.ispFamily = &lpcIsp11Exx,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// LPC1200 family

//SLICE
const LpcFamily lpcFamily12xx = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=32 }, { }, },
	.blockSizes = { 4, 512, 1024, 4096 },		// LPC1200 can write very small (4B) pages/blocks, 4096B max
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
const LpcIspFamily lpcIsp12xx = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	// For at least LPC11xx and LPC11Exx the following holds:
	//
	// RAM up to 0x1000 0050 is untouched an preserved at boot.
	// ISP uses RAM from 0x1000 017C to 0x1000 025B
	// ISP command use top 32 bytes, the stack is located top-32 and may use up to 256 bytes
	.addressRams = { 0x10000000, },
	.ramUsage = {
		{ 0x1000017C, 0x25B-0x17C+1 },
		{ 0x10000000, -(256+32) },
	},
};

/*
// generic type
const LpcMember lpcMember12 = {
	.name = "LPC12*",
	.sizeFlashK = 32,
	.sizeRamKs = { 4, },
	.ids = { 0 },
	.family = &lpcFamily12xx,
	.ispFamily = &lpcIsp12xx,
};
*/

//SLICE
const LpcMember lpcMember1224_M101 = {
	.name = "LPC1224 M101",
	.sizeFlashK = 32,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1224_M101, },
	.family = &lpcFamily12xx,
	.ispFamily = &lpcIsp12xx,
};

//SLICE
const LpcMember lpcMember1224_M121 = {
	.name = "LPC1224 M121",
	.sizeFlashK = 48,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1224_M121, },
	.family = &lpcFamily12xx,
	.ispFamily = &lpcIsp12xx,
};

//SLICE
const LpcMember lpcMember1225_M301 = {
	.name = "LPC1225 M301",
	.sizeFlashK = 64,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1225_M301, },
	.family = &lpcFamily12xx,
	.ispFamily = &lpcIsp12xx,
};

//SLICE
const LpcMember lpcMember1225_M321 = {
	.name = "LPC1225 M321",
	.sizeFlashK = 80,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1225_M321, },
	.family = &lpcFamily12xx,
	.ispFamily = &lpcIsp12xx,
};

//SLICE
const LpcMember lpcMember1226_M301 = {
	.name = "LPC1226 M301",
	.sizeFlashK = 96,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1226_M301, },
	.family = &lpcFamily12xx,
	.ispFamily = &lpcIsp12xx,
};

//SLICE
const LpcMember lpcMember1227_M301 = {
	.name = "LPC1227 M301",
	.sizeFlashK = 128,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1227_M301, },
	.family = &lpcFamily12xx,
	.ispFamily = &lpcIsp12xx,
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// LPC1300 family

//SLICE
const LpcFamily lpcFamily13xx = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=8 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
const LpcIspFamily lpcIsp13xx = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	// .addressRams = { 0x10000000, 0x20000000, 0x20004000 },	// for x5/x6/x7
	.addressRams = { 0x10000000, },	// RAM1/2 is powered off at boot time in extended versions => of no use
	.ramUsage = {
		{ 0x1000017C, 0x25B-0x17C+1 },
		{ 0x10000000, -(256+32) },
	},
};

//SLICE
// extended versions...
const LpcFamily lpcFamily13x5_6_7 = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=16 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
};

/*
// generic type
const LpcMember lpcMember13xx = {	// least common denominator
	.name = "LPC13*",
	.sizeFlashK = 8,
	.sizeRamKs = { 4, },
	.ids = { 0 },
	.family = &lpcFamily13xx,
	.ispFamily = &lpcIsp13xx,
};
*/

//SLICE
const LpcMember lpcMember1311 = {
	.name = "LPC1311",
	.sizeFlashK = 8,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1311, },
	.family = &lpcFamily13xx,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1311_01 = {
	.name = "LPC1311/01",
	.sizeFlashK = 8,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1311_01, },
	.family = &lpcFamily13xx,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1313 = {
	.name = "LPC1313",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1313, },
	.family = &lpcFamily13xx,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1313_01 = {
	.name = "LPC1313/01",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1313_01, },
	.family = &lpcFamily13xx,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1342 = {
	.name = "LPC1342",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_1342, },
	.family = &lpcFamily13xx,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1343 = {
	.name = "LPC1343",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1343, },
	.family = &lpcFamily13xx,
	.ispFamily = &lpcIsp13xx,
};


//SLICE
const LpcMember lpcMember1315 = {
	.name = "LPC1315",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1315, },
	.family = &lpcFamily13x5_6_7,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1316 = {
	.name = "LPC1316",
	.sizeFlashK = 48,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1316, },
	.family = &lpcFamily13x5_6_7,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1317 = {
	.name = "LPC1317",
	.sizeFlashK = 64,
	.sizeRamKs = { 8, 0, 2 },
	.ids = { LPC_ID_1317, },
	.family = &lpcFamily13x5_6_7,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1345 = {
	.name = "LPC1345",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, 2, },
	.ids = { LPC_ID_1345, },
	.family = &lpcFamily13x5_6_7,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1346 = {
	.name = "LPC1346",
	.sizeFlashK = 48,
	.sizeRamKs = { 8, 2, },
	.ids = { LPC_ID_1346, },
	.family = &lpcFamily13x5_6_7,
	.ispFamily = &lpcIsp13xx,
};

//SLICE
const LpcMember lpcMember1347 = {
	.name = "LPC1347",
	.sizeFlashK = 64,
	.sizeRamKs = { 8, 2, 2 },
	.ids = { LPC_ID_1347, },
	.family = &lpcFamily13x5_6_7,
	.ispFamily = &lpcIsp13xx,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// LPC1500 family
// LPC15 RAM is special, because it's contiguous for all devices and enabled at boot time, hence
// fully available to the ISP handler. Sizes = 12ki/20ki/32ki @ 0x0200 0000
//
//SLICE
const LpcFamily lpcFamily15xx = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=64 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
const LpcIspFamily lpcIsp15xx = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	.addressRams = { 0x02000000, },	// uncommon address and 3 parts, but still contiguous!
	.ramUsage = {
		// { 0x02000100, 0x9E4-0x10 },	// fixed area used by CAN and USB ISP
		{ 0x02000000, -(256+32) },	// Stack
						// cite: 34.3.7.3 (UM10736, Rev1, 13 Feb 2014):
						// Memory for the UART ISP commands is allocated dynamically/
						// What does this mean? From the above stack I guess...
	},
};

/*
// generic type
const LpcMember lpcMember15 = {
	.name = "LPC15",
	.sizeFlashK = 64,
	.sizeRamKs = { 12, },
	.ids = { 0 },
	.family = &lpcFamily15xx,
	.ispFamily = &lpcIsp15xx,
};
*/

//SLICE
const LpcMember lpcMember1517 = {
	.name = "LPC1517",
	.sizeFlashK = 64,
	.sizeRamKs = { 12, },
	.ids = { LPC_ID_1517, },
	.family = &lpcFamily15xx,
	.ispFamily = &lpcIsp15xx,
};

//SLICE
const LpcMember lpcMember1518 = {
	.name = "LPC1518",
	.sizeFlashK = 128,
	.sizeRamKs = { 20, },
	.ids = { LPC_ID_1518, },
	.family = &lpcFamily15xx,
	.ispFamily = &lpcIsp15xx,
};

//SLICE
const LpcMember lpcMember1519 = {
	.name = "LPC1519",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, },
	.ids = { LPC_ID_1519, },
	.family = &lpcFamily15xx,
	.ispFamily = &lpcIsp15xx,
};

//SLICE
const LpcMember lpcMember1547 = {
	.name = "LPC1547",
	.sizeFlashK = 64,
	.sizeRamKs = { 12, },
	.ids = { LPC_ID_1547, },
	.family = &lpcFamily15xx,
	.ispFamily = &lpcIsp15xx,
};

//SLICE
const LpcMember lpcMember1548 = {
	.name = "LPC1548",
	.sizeFlashK = 128,
	.sizeRamKs = { 20, },
	.ids = { LPC_ID_1548, },
	.family = &lpcFamily15xx,
	.ispFamily = &lpcIsp15xx,
};

//SLICE
const LpcMember lpcMember1549 = {
	.name = "LPC1549",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, },
	.ids = { LPC_ID_1549, },
	.family = &lpcFamily15xx,
	.ispFamily = &lpcIsp15xx,
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// LPC1700 family
// I keep the RAM layouts unified here.

//SLICE
const LpcFamily lpcFamily17xx = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=16 }, { .sizeK=32, .n=14 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

/*
const LpcFamily lpcFamily1788 = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=16 }, { .sizeK=32, .n=14 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
};
*/

//SLICE
const LpcIspFamily lpcIsp17xx = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	.addressRams = { 0x10000000, 0x2007C000, 0x20080000 },	// LPC175x/176x 
	.ramUsage = {
		{ 0x1000017C, 0x25B-0x17C+1 },
		{ 0x10000000, -(256+32) },
	},
};

/*
const LpcIspFamily lpcIsp1788 = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	.addressRams = { 0x10000000, 0x20000000, },	// LPC177x/178x
	.ramUsage = {
		{ 0x1000017C, 0x25B-0x17C },
		{ 0x10000000, -(256+32) },
	},
};
*/

/*
// generic type
const LpcMember lpcMember17 = {
	.name = "LPC17*",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { 0 },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

const LpcMember lpcMember175 = {
	.name = "LPC175*",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { 0 },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};
*/

//SLICE
const LpcMember lpcMember1751 = {
	.name = "LPC1751",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_1751, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1752 = {
	.name = "LPC1752",
	.sizeFlashK = 64,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_1752, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1754 = {
	.name = "LPC1754",
	.sizeFlashK = 128,
	.sizeRamKs = { 16, 16, },
	.ids = { LPC_ID_1754, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1756 = {
	.name = "LPC1756",
	.sizeFlashK = 256,
	.sizeRamKs = { 16, 16, },
	.ids = { LPC_ID_1756, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1758 = {
	.name = "LPC1758",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 16, 16 },
	.ids = { LPC_ID_1758, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1759 = {
	.name = "LPC1759",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 16, 16 },
	.ids = { LPC_ID_1759, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};


/*
// generic type
const LpcMember lpcMember176 = {
	.name = "LPC176*",
	.sizeFlashK = 128,
	.sizeRamKs = { 16, 16, },
	.ids = { LPC_ID_1764, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};
*/

//SLICE
const LpcMember lpcMember1764 = {
	.name = "LPC1764",
	.sizeFlashK = 128,
	.sizeRamKs = { 16, 16, },
	.ids = { LPC_ID_1764, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1765 = {
	.name = "LPC1765",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, 16, 16 },
	.ids = { LPC_ID_1765, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1766 = {
	.name = "LPC1766",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, 16, 16 },
	.ids = { LPC_ID_1766, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1767 = {
	.name = "LPC1767",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 16, 16 },
	.ids = { LPC_ID_1767, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1768 = {
	.name = "LPC1768",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 16, 16 },
	.ids = { LPC_ID_1768, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1769 = {
	.name = "LPC1769",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 16, 16 },
	.ids = { LPC_ID_1769, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

/*
// generic type
const LpcMember lpcMember177 = {
	.name = "LPC177*",
	.sizeFlashK = 128,
	.sizeRamKs = { 32, 8, },
	.ids = { 0 },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};
*/

//SLICE
const LpcMember lpcMember1774 = {
	.name = "LPC1774",
	.sizeFlashK = 128,
	.sizeRamKs = { 32, 8, },
	.ids = { LPC_ID_1774, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1776 = {
	.name = "LPC1776",
	.sizeFlashK = 256,
	.sizeRamKs = { 64, 16, },
	.ids = { LPC_ID_1776, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1777 = {
	.name = "LPC1777",
	.sizeFlashK = 512,
	.sizeRamKs = { 64, 32, },
	.ids = { LPC_ID_1777, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1778 = {
	.name = "LPC1778",
	.sizeFlashK = 512,
	.sizeRamKs = { 64, 32, },
	.ids = { LPC_ID_1778, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

/*
// generic type
const LpcMember lpcMember178 = {
	.name = "LPC178*",
	.sizeFlashK = 256,
	.sizeRamKs = { 64, 16, },
	.ids = { 0 },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};
*/

//SLICE
const LpcMember lpcMember1785 = {
	.name = "LPC1785",
	.sizeFlashK = 256,
	.sizeRamKs = { 64, 16, },
	.ids = { LPC_ID_1785, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1786 = {
	.name = "LPC1786",
	.sizeFlashK = 256,
	.sizeRamKs = { 64, 16, },
	.ids = { LPC_ID_1786, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1787 = {
	.name = "LPC1787",
	.sizeFlashK = 512,
	.sizeRamKs = { 64, 16, },
	.ids = { LPC_ID_1787, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};

//SLICE
const LpcMember lpcMember1788 = {
	.name = "LPC1788",
	.sizeFlashK = 512,
	.sizeRamKs = { 64, 16, },
	.ids = { LPC_ID_1788, },
	.family = &lpcFamily17xx,
	.ispFamily = &lpcIsp17xx,
};


////////////////////////////////////////////////////////////////////////////////////////////////////
//LPC210x family

//SLICE
const LpcFamily lpcFamily2101_2_3 = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=8 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 5, 
};

//SLICE
const LpcIspFamily lpcIsp210x = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	.addressRams = { 0x40000000, },
	.ramUsage = { // verified for 2101..2106
		{ 0x40000120, 0x200-0x120 },
		{ 0x40000000, -(256+32) },
	},
};

//SLICE
const LpcMember lpcMember2101 = {
	.name = "LPC2101",
	.sizeFlashK = 8,
	.sizeRamKs = { 2, },
	.ids = { LPC_ID_2101, },
	.family = &lpcFamily2101_2_3,
	.ispFamily = &lpcIsp210x,
};

//SLICE
const LpcMember lpcMember2102 = {
	.name = "LPC2102",
	.sizeFlashK = 16,
	.sizeRamKs = { 4, },
	.ids = { LPC_ID_2102, },
	.family = &lpcFamily2101_2_3,
	.ispFamily = &lpcIsp210x,
};

//SLICE
const LpcMember lpcMember2103 = {
	.name = "LPC2103",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_2103, },
	.family = &lpcFamily2101_2_3,
	.ispFamily = &lpcIsp210x,
};


////////////////////////////////////////
// LPC2104_5_6 sub-family

//SLICE
const LpcFamily lpcFamily2104_5_6 = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=8, .n=16 }, { }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 5, 
};

//SLICE
const LpcMember lpcMember2104 = {
	.name = "LPC2104",
	.sizeFlashK = 120,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_2104, },
	.family = &lpcFamily2104_5_6,
	.ispFamily = &lpcIsp210x,
};

//SLICE
const LpcMember lpcMember2105 = {
	.name = "LPC2105",
	.sizeFlashK = 120,
	.sizeRamKs = { 32, },
	.ids = { LPC_ID_2105, },
	.family = &lpcFamily2104_5_6,
	.ispFamily = &lpcIsp210x,
};

//SLICE
const LpcMember lpcMember2106 = {
	.name = "LPC2106",
	.sizeFlashK = 120,
	.sizeRamKs = { 64, },
	.ids = { LPC_ID_2106, },
	.family = &lpcFamily2104_5_6,
	.ispFamily = &lpcIsp210x,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//LPC211x family

//SLICE
const LpcFamily lpcFamily211x = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=8, .n=16 }, {}, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 5, 
};

//SLICE
const LpcFamily lpcFamily212x = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=8, .n=8 }, { .sizeK=64, .n=2 }, { .sizeK=8, .n=8},  {} },
	.blockSizes = { 256, 512, 1024, 4096 },	// unchecked
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 5, 
};


//SLICE
const LpcIspFamily lpcIsp211x = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	.addressRams = { 0x40000000, },
	.ramUsage = { // verified for LPC211x, LPC212x, LPC221x, LPC229x
		{ 0x40000120, 0x200-0x120 },
		{ 0x40000000, -(256+32) },
	},
};

//SLICE
const LpcMember lpcMember2114 = {
	.name = "LPC2114",
	.sizeFlashK = 120,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_2114, },
	.family = &lpcFamily211x,
	.ispFamily = &lpcIsp211x,
};


//SLICE
const LpcMember lpcMember2124 = {
	.name = "LPC2124",
	.sizeFlashK = 248,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_2124, },
	.family = &lpcFamily212x,
	.ispFamily = &lpcIsp211x,
};


//SLICE
const LpcMember lpcMember2129 = {
	.name = "LPC2129",
	.sizeFlashK = 248,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_2129, },
	.family = &lpcFamily212x,
	.ispFamily = &lpcIsp211x,
};



////////////////////////////////////////////////////////////////////////////////////////////////////
//LPC213x family

//SLICE
const LpcFamily lpcFamily213x = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=8}, { .sizeK=32, .n=14 }, { .sizeK=4, .n=5 }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 5, 
};

//SLICE
const LpcIspFamily lpcIsp213x = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	.addressRams = { 0x40000000, },
	.ramUsage = { // verified
		{ 0x40000120, 0x200-0x120 },
		{ 0x40000000, -(256+32) },
	},
};

//SLICE
const LpcMember lpcMember2131 = {
	.name = "LPC2131",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_2131, },
	.family = &lpcFamily213x,
	.ispFamily = &lpcIsp213x,
};

//SLICE
const LpcMember lpcMember2132 = {
	.name = "LPC2132",
	.sizeFlashK = 64,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_2132, },
	.family = &lpcFamily213x,
	.ispFamily = &lpcIsp213x,
};

//SLICE
const LpcMember lpcMember2134 = {
	.name = "LPC2134",
	.sizeFlashK = 128,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_2134, },
	.family = &lpcFamily213x,
	.ispFamily = &lpcIsp213x,
};

//SLICE
const LpcMember lpcMember2136 = {
	.name = "LPC2136",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, },
	.ids = { LPC_ID_2136, },
	.family = &lpcFamily213x,
	.ispFamily = &lpcIsp213x,
};

//SLICE
const LpcMember lpcMember2138 = {
	.name = "LPC2138",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, },
	.ids = { LPC_ID_2138, },
	.family = &lpcFamily213x,
	.ispFamily = &lpcIsp213x,
};


////////////////////////////////////////////////////////////////////////////////////////////////////
//LPC214x family

//SLICE
const LpcFamily lpcFamily214x = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=8}, { .sizeK=32, .n=14 }, { .sizeK=4, .n=5 }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 5, 
};

//SLICE
const LpcIspFamily lpcIsp214x = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	.addressRams = { 0x40000000, /* 0x7FD00000, */ },	// upper RAM not powered at boot time :-(
	.ramUsage = { // verified
		{ 0x40000120, 0x200-0x120 },
		{ 0x40000000, -(256+32) },
	},
};

//SLICE
const LpcMember lpcMember2141 = {
	.name = "LPC2141",
	.sizeFlashK = 32,
	.sizeRamKs = { 8, },
	.ids = { LPC_ID_2141, },
	.family = &lpcFamily214x,
	.ispFamily = &lpcIsp214x,
};

//SLICE
const LpcMember lpcMember2142 = {
	.name = "LPC2142",
	.sizeFlashK = 64,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_2142, },
	.family = &lpcFamily214x,
	.ispFamily = &lpcIsp214x,
};

//SLICE
const LpcMember lpcMember2144 = {
	.name = "LPC2144",
	.sizeFlashK = 128,
	.sizeRamKs = { 16, },
	.ids = { LPC_ID_2144, },
	.family = &lpcFamily214x,
	.ispFamily = &lpcIsp214x,
};

//SLICE
const LpcMember lpcMember2146 = {
	.name = "LPC2146",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, 8, },
	.ids = { LPC_ID_2146, },
	.family = &lpcFamily214x,
	.ispFamily = &lpcIsp214x,
};

//SLICE
const LpcMember lpcMember2148 = {
	.name = "LPC2148",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 8, },
	.ids = { LPC_ID_2148, },
	.family = &lpcFamily214x,
	.ispFamily = &lpcIsp214x,
};


////////////////////////////////////////////////////////////////////////////////////////////////////
//LPC23 family

//SLICE
const LpcFamily lpcFamily23xx = {
	.banks = 1,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=4, .n=8 }, { .sizeK=32, .n=14 }, { .sizeK=4, .n=6 }, },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 5, 
};

//SLICE
const LpcIspFamily lpcIsp23xx = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	// .addressRams = { 0x40000000, 0x7FD0C000, 0x7FE00000 },
	.addressRams = { 0x40000000, },	// unchecked: upper RAM not powered at boot time :-(
	.ramUsage = { // verified
		{ 0x40000120, 0x200-0x120 },
		{ 0x40000000, -(256+32) },
	},
};

//SLICE
const LpcMember lpcMember2364 = {
	.name = "LPC2364",
	.sizeFlashK = 128,
	.sizeRamKs = { 8, /* 8, 16 */ },
	.ids = { LPC_ID_2364, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2365 = {
	.name = "LPC2365",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, /* 8, 16 */ },
	.ids = { LPC_ID_2365, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2366 = {
	.name = "LPC2366",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, /* 8, 16 */ },
	.ids = { LPC_ID_2366, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2367 = {
	.name = "LPC2367",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, /* 8, 16 */ },
	.ids = { LPC_ID_2367, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2368 = {
	.name = "LPC2368",
	.sizeFlashK = 504,
	.sizeRamKs = { 32, /* 8, 16 */ },
	.ids = { LPC_ID_2368, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2377 = {
	.name = "LPC2377",
	.sizeFlashK = 504,
	.sizeRamKs = { 32, /* 8, 16 */ },
	.ids = { LPC_ID_2377, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2378 = {
	.name = "LPC2378",
	.sizeFlashK = 504,
	.sizeRamKs = { 32, /* 8, 16, */ },
	.ids = { LPC_ID_2378, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2378IR = {
	.name = "LPC2378",
	.sizeFlashK = 504,
	.sizeRamKs = { 32, /* 8, 16, */ },
	.ids = { LPC_ID_2378IR, },		// initial revision
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2387 = {
	.name = "LPC2387",
	.sizeFlashK = 504,
	.sizeRamKs = { 64, /* 16, 16, */ },
	.ids = { LPC_ID_2387, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

//SLICE
const LpcMember lpcMember2388 = {
	.name = "LPC2388",
	.sizeFlashK = 504,
	.sizeRamKs = { 64, /* 16, 16, */ },
	.ids = { LPC_ID_2388, },
	.family = &lpcFamily23xx,
	.ispFamily = &lpcIsp23xx,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//LPC43 family

//SLICE
const LpcFamily lpcFamily43xx = {
	.addressFlashs = { 0x1A000000, 0x1B000000, },
	.banks = 2,	//.addressFlash = 0,
	.sectorArrays = { { .sizeK=8, .n=8 }, { .sizeK=64, .n=7 }, },
	.blockSizes = { 512, 1024, 4096 },
	.idMasks = { -1, 0x000000FF, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
const LpcFamily lpcFamily43x2 = {
	.addressFlashs = { 0x1A000000, },
	.banks = 1,
	.sectorArrays = { { .sizeK=8, .n=8 }, { .sizeK=64, .n=7 }, },
	.blockSizes = { 512, 1024, 4096 },
	.idMasks = { -1, 0x000000FF, },
};

//SLICE
const LpcIspFamily lpcIsp43xx = {
	.protocol = ISP_PROTOCOL_UUENCODE,
	.addressRams = {
		0x10000000,
		0x10080000,
		0x18000000,
	},
	.ramUsage = {
		{ 0x10080000, 0x200 },		// 0x200 bytes from bottom
		{ 0x10080000, -0x120 },		// 32 + 256 bytes from top. :o) Not sure about this
						// UM specifies in 6.4.5.7 'top of on-chip RAM'. WTF?
	},
};

//SLICE
///< flashless

const LpcMember lpcMember4310 = {
	.name = "LPC4310",
	.sizeRamKs = { 96, 40, 0 },
	.ids = { LPC_ID_4310, },
	.family = &lpcFamily43x2,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
///< flashless
const LpcMember lpcMember4320 = {
	.name = "LPC4320",
	.sizeRamKs = { 96, 40, 0 },
	.ids = { LPC_ID_4320, },
	.family = &lpcFamily43x2,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
///< flashless
const LpcMember lpcMember4330 = {
	.name = "LPC4330",
	.sizeRamKs = { 128, 72, 0 },
	.ids = { LPC_ID_4330, },
	.family = &lpcFamily43x2,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
///< flashless
const LpcMember lpcMember4350 = {
	.name = "LPC4350",
	.sizeRamKs = { 128, 72, 0 },
	.ids = { LPC_ID_4350, },
	.family = &lpcFamily43x2,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
///< flashless
const LpcMember lpcMember4370 = {
	.name = "LPC4370",
	.sizeRamKs = { 128, 72, 16 },
	.ids = { LPC_ID_4370, },
	.family = &lpcFamily43x2,
	.ispFamily = &lpcIsp43xx,
};


//SLICE
const LpcMember lpcMember4312 = {
	.name = "LPC4312",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4312_3, LPC_ID2_4312, },
	.family = &lpcFamily43x2,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
const LpcMember lpcMember4313 = {
	.name = "LPC4313",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4312_3, LPC_ID2_4313, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};


//SLICE
const LpcMember lpcMember4315 = {
	.name = "LPC4315",
	.sizeFlashK = 384,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4315_7, LPC_ID2_4315, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
const LpcMember lpcMember4317 = {
	.name = "LPC4317",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4315_7, LPC_ID2_4317, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};


//SLICE
const LpcMember lpcMember4322 = {
	.name = "LPC4322",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4322_3, LPC_ID2_4322, },
	.family = &lpcFamily43x2,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
const LpcMember lpcMember4323 = {
	.name = "LPC4323",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4322_3, LPC_ID2_4323, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};


//SLICE
const LpcMember lpcMember4325 = {
	.name = "LPC4325",
	.sizeFlashK = 384,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4325_7, LPC_ID2_4325, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
const LpcMember lpcMember4327 = {
	.name = "LPC4327",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4325_7, LPC_ID2_4325, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};


//SLICE
const LpcMember lpcMember4333 = {
	.name = "LPC4333",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4333_7, LPC_ID2_4333, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
const LpcMember lpcMember4337 = {
	.name = "LPC4337",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4333_7, LPC_ID2_4337, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};


//SLICE
const LpcMember lpcMember4353 = {
	.name = "LPC4353",
	.sizeFlashK = 256,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4353_7, LPC_ID2_4353, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};

//SLICE
const LpcMember lpcMember4357 = {
	.name = "LPC4357",
	.sizeFlashK = 512,
	.sizeRamKs = { 32, 40, 0 },
	.ids = { LPC_ID_4353_7, LPC_ID2_4357, },
	.family = &lpcFamily43xx,
	.ispFamily = &lpcIsp43xx,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//LPC54100 family

//SLICE
// from UM Rev 2.4
const LpcFamily lpcFamily5410x = {
	.addressFlashs = { 0x00000000, },
	.banks = 1,
	.sectorArrays = { { .sizeK=32, .n=32 },  },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
// from UM Rev 2.4
const LpcIspFamily lpcIsp5410x = {
	.protocol = ISP_PROTOCOL_BINARY,
	.addressRams = {		// all devices have 104 kiB total RAM
		0x02000000,		// 64 kiB SRAM0
		0x02010000,		// 32 kiB SRAM1, right after SRAM0
		0x03400000,		// 8 kiB SRAM2
	},
	// Manual Rev 2.4 says: IAP uses top 32 bytes of 64 kiB SRAM0 (0x02000000..0x02010000 - 32) and at most 128B of
	// the user stack.
	// ISP allocates RAM dynamically
	// all RAM is powered at startup
	.ramUsage = {
		{ 0x02000000, -0x120 },		// 64 kiB - 0x100 bytes stack usage + 32 bytes top usage.
		{ 0x02010000, 0x8000 },		// 64 kiB - 0x100 bytes stack usage + 32 bytes top usage.
		{ 0x03400000, 0x2000 },		// Hopefully, we can use this entirely ?
	},
};

//SLICE
// from UM Rev 2.4
const LpcMember lpcMember54101_J256 = {
	.name = "LPC54101 J256",
	.sizeFlashK = 256,
	.sizeRamKs = { 64, 32, 8,  },
	.ids = { LPC_ID_54101_J256, },
	.family = &lpcFamily5410x,
	.ispFamily = &lpcIsp5410x,
};

//SLICE
// from UM Rev 2.4
const LpcMember lpcMember54101_J512 = {
	.name = "LPC54101 J512",
	.sizeFlashK = 512,
	.sizeRamKs = { 64, 32, 8,  },
	.ids = { LPC_ID_54101_J512, },
	.family = &lpcFamily5410x,
	.ispFamily = &lpcIsp5410x,
};

//SLICE
// from UM Rev 2.4
const LpcMember lpcMember54102_J256 = {
	.name = "LPC54102 J256",
	.sizeFlashK = 256,
	.ids = { LPC_ID_54102_J256, },
	.sizeRamKs = { 64, 32, 8,  },
	.family = &lpcFamily5410x,
	.ispFamily = &lpcIsp5410x,
};

//SLICE
// from UM Rev 2.4
const LpcMember lpcMember54102_J512 = {
	.name = "LPC54102 J512",
	.sizeFlashK = 512,
	.ids = { LPC_ID_54102_J512, },
	.sizeRamKs = { 64, 32, 8,  },
	.family = &lpcFamily5410x,
	.ispFamily = &lpcIsp5410x,
};


// 5411x family
//SLICE
// from UM Rev 1.6
const LpcFamily lpcFamily5411x = {
	.addressFlashs = { 0x00000000, },
	.banks = 1,
	.sectorArrays = { { .sizeK=32, .n=16 },  },
	.blockSizes = { 256, 512, 1024, 4096 },
	.idMasks = { -1, },
	.checksumVectors = 8,
	.checksumVector = 7, 
};

//SLICE
const LpcIspFamily lpcIsp5411x = {
	.protocol = ISP_PROTOCOL_BINARY,
	.addressRams = {
		0x20000000,		///< SRAM0 64 kiB ; clock active after boot ; powered by default
		0x20010000,		///< SRAM1 64 kiB ;  not available on ... J128 ; clock active after boot ; powered by default
		0x20020000,		///< SRAM2 32 kiB ;  not available on ... J128 ; clock disabled after boot ; powered by default
		0x04000000,		///< SRAMX 32 kiB ; clock active after boot ; powered by default
	},
	// Manual Rev 1.6 says: IAP uses top 32 bytes of 64 kiB SRAM0 and at most 128B of the user stack.
	// ISP allocates RAM dynamically
	// all RAM is powered at startup, but clock is disabled for SRAM2
	.ramUsage = {
		{ 0x20000000, -0x120 },		// 32 + 256 bytes from top.
		{ 0x20010000, 0x10000 },
		{ 0x20020000, 0x8000 },
		{ 0x04000000, 0x8000 },
	},
};

//SLICE
// from UM Rev 1.6
const LpcMember lpcMember54113_J128 = {
	.name = "LPC54113 J128",
	.sizeFlashK = 128,
	.ids = { LPC_ID_54113_J128, },
	.sizeRamKs = { 64, 0, 0, 32  },
	.family = &lpcFamily5411x,
	.ispFamily = &lpcIsp5411x,
};

//SLICE
// from UM Rev 1.6
const LpcMember lpcMember54113_J256 = {
	.name = "LPC54113 J256",
	.sizeFlashK = 256,
	.ids = { LPC_ID_54113_J256, },
	.sizeRamKs = { 64, 64, 32, 32 },
	.family = &lpcFamily5411x,
	.ispFamily = &lpcIsp5411x,
};

//SLICE
// from UM Rev 1.6
const LpcMember lpcMember54114_J256 = {
	.name = "LPC54114 J256",
	.sizeFlashK = 256,
	.ids = { LPC_ID_54114_J256, },
	.sizeRamKs = { 64, 64, 32, 32 },
	.family = &lpcFamily5411x,
	.ispFamily = &lpcIsp5411x,
};

////////////////////////////////////////////////////////////////////////////////////////////////////
