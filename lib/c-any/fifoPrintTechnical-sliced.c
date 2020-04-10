//HEADER
#include <fifoPrintTechnical.h>
#include <uint16Div.h>
#include <uint32Div.h>
#include <fifoPrintFixedPoint.h>

enum {
	LIMIT_M3	=999500,
};

bool fifoPrintTechnicalUint32_m3 (Fifo *fifo, Uint32 millis);

//SLICE
bool fifoPrintTechnicalUint32_m3 (Fifo *fifo, Uint32 millis) {
	if (millis<1000) {	// format .123
		return fifoPrintChar (fifo,'.') && fifoPrintUint16Pad (fifo, (Uint16)millis,3,'0');
	}
	else if (millis<9995) {	// format 1.23 : drop one digit behind the decimal point
		millis += 5;		// round up for div and mod
		const Uint16 i = uint16Div10 (uint16Div10 (uint16Div10 ((Uint16)millis)));
		const Uint16 f = uint16Div10 (millis) - i*100;
		return fifoPrintUint16 (fifo,i,1) && fifoPrintChar (fifo,'.') && fifoPrintUint16Pad (fifo,f,2,'0');
	}
	else if (millis<99950) {	// format 12.3 : drop 2 digits behind the decimal point
		millis += 50;
		const Uint16 scaled = uint16Div10 ((Uint16)uint32Div10 (millis));	// <= 999
		const Uint16 i = uint16Div10 (scaled);			// <= 99
		const Uint16 f = scaled - i*10;
		return fifoPrintUint16 (fifo,i,2) && fifoPrintChar (fifo,'.') && fifoPrintUint16Pad (fifo,f,1,'0');
	}
	else if (millis<999500) {	// format 123. : drop all digits behind the decimal point
		millis += 500;
		const Uint16 scaled = uint32Div1000 (millis);	// <= 999
		return fifoPrintUint16 (fifo,scaled,3) && fifoPrintChar (fifo,'.');
	}
	else return fifoPrintString (fifo,"ERR.");
}

//SLICE
/** Prints a value including unit and SI prefixes.
 * @param fifo the output
 * @param value the number in units of 10^exponent. E.g. -3 means milli
 * @param the physical unit, like "s" for second.
 */
bool fifoPrintTechnicalUint32 (Fifo *fifo, Uint32 value, int exponent, const char *unit) {
	int shift = 0;
	while (value>LIMIT_M3) {
		value = uint32Div1000 (value+500);
		shift += 3;
	}
	if (value!=0) while (value*1000<LIMIT_M3) {
		value *= 1000;
		shift -= 3;
	}
	return	fifoPrintTechnicalUint32_m3 (fifo,value)
		&& fifoPrintSiPrefix (fifo,shift+exponent+3)
		&& fifoPrintString (fifo,unit);
}

