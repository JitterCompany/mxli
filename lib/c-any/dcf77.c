/*
  dcf77.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <dcf77.h>

/*
static void dcfHandleBit(bool bit) {
	static const char bcdTable[] = { 1,2,4,8,10,20,40,80 };

	dcf77.parity = dcf77.parity ^ bit;

	switch(dcf77.bitIndex) {
	case 0:		if (bit) dcf77.errors |= FRAME_ERROR;	// bit 0 always 0
			break;
	case 16:	break;			// am Ende dieser Stunde MEZ/MESZ Umschaltung
	case 17:	dcf77.mesz = bit;	// 1->MESZ
			break;
	case 18:	if (dcf77.mez==bit) dcf77.errors |= MEZ_ERROR;	// 1->MEZ
			break;
	case 19:	break;			// am Ende dieser Stunde wird eine Schaltsekunde eingef"ugt
	case 20:	dcf77.parity = false;	// Zeitinformation. Immer 1.
			break;	
	case 21:	// Minute 0..59
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:	dcf77.min += bit*bcdTable[dcf77.bitIndex-21];
			break;
	case 28:	if (dcf77.parity) dcf77.errors |= TIME_ERROR;
			dcf77.parity = false;
			break;

	case 29:	// Stunde 0..23
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:	dcf77.hour += bit*bcdTable[dcf77.bitIndex-29];
			break;
	case 35:	if (dcf77.parity) dcf77.errors |= TIME_ERROR;
			dcf77.parity = false;
			break;
	case 36:	// Tag des Monats 1..31
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:	dcf77.day += bit*bcdTable[dcf77.bitIndex-36];
			break;
	case 42:	// Wochentag 1..7
	case 43:
	case 44:	dcf77.dayOfWeek += (int)bit << dcf77.bitIndex-42;
			break;
	case 45:	// Monat
	case 46:
	case 47:
	case 48:
	case 49:	dcf77.month += bit*bcdTable[dcf77.bitIndex-45];
			break;
	case 50:
	case 51:
	case 52:
	case 53:
	case 54:
	case 55:
	case 56:
	case 57:	dcf77.year += bit*bcdTable[dcf77.bitIndex-50];
			break;
	case 58:	if (dcf77.parity) dcf77.errors |= FRAME_ERROR;
			break;
	case 59:	break;	// this one is left out for synchronization

	}
	if (dcf77.indicator!=0) dcf77.indicator(true);
	dcf77.bitIndex++;
}
*/

char bcdToBin(char bcd) {
	return (bcd & 0xF) + (bcd>>4 & 0xF)*10;
}

int parity(int value) {
	int p = 0;
	for (int b=0; b<8*sizeof value; ++b) p ^= value>>b;

	return p&1;
}

bool dcf77ExtractTime(Time* time, Dcf77Time dt, size_t validBits) {
	char timeZone = dt>>17 & 3;		// bit 17=MESZ, bit 18=MEZ
	char bitsMinute = dt>>21 & 0x7F;	// 7 bits
	char parityMinute = dt>>28 & 1;
	char bitsHour = dt>>29 & 0x3F;		// 6 bits
	char parityHour = dt>>35 & 1;

	time->second = 0;
	time->minute = bcdToBin(bitsMinute);
	time->hour = bcdToBin(bitsHour);
	time->timeZone = timeZone;
	return  validBits>=59-17
		&& (timeZone==1 || timeZone==2)
		&& 0==parity(bitsMinute ^ parityMinute ^ bitsHour ^ parityHour);
}

bool dcf77ExtractDate(Date* date, Dcf77Time dt, size_t validBits) {
	unsigned char bitsDay = dt>>36 & 0x3F;		// 6 bits
	unsigned char bitsDayOfWeek = dt>>42 & 0x7;	// 3 bits
	unsigned char bitsMonth = dt>>45 & 0x1F;	// 5 bits
	unsigned char bitsYear99 = dt>>50 & 0xFF;	// 8 bits
	unsigned char parityDate = dt>>58 & 1;

	date->day = bcdToBin(bitsDay);
	date->month = bcdToBin(bitsMonth);
	date->year = 2000+bcdToBin(bitsYear99);
	date->dayOfWeek = bitsDayOfWeek;

	return	(validBits>=59-36)
		&& (0==parity(bitsDay^bitsDayOfWeek^bitsMonth^bitsYear99^parityDate));
}

