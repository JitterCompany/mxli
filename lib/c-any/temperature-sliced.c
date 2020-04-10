//HEADER
#include <temperature.h>
#include <fifoPrint.h>
#include <fifoParse.h>

//SLICE
Int32 temperatureK2F_e10 (Int32 tK_e10) {
	return tK_e10 * 9 / 5 + KELVIN_TO_FAHRENHEIT_OFFSET_E10;
}

//SLICE
Int32 temperatureF2K_e10 (Int32 tF_e10) {
	return (tF_e10 - KELVIN_TO_FAHRENHEIT_OFFSET_E10) * 5 / 9;
}

bool fifoParseTemperature (Fifo *fifo, Int32 *tK_e10) {
	Fifo clone = *fifo;
	Int32 t_e10;
	if (fifoParseFixedPointInt (&clone,&t_e10, E10)
	&& (fifoParseExactChar (&clone,TEMPERATURE_CHAR) || fifoParseExactChar (&clone,'\'') || true)) {
		if (fifoParseExactChar (&clone,'K')) *tK_e10 = t_e10;
		else if (fifoParseExactChar (&clone,'C')) *tK_e10 = temperatureC2K_e10 (t_e10);
		else if (fifoParseExactChar (&clone,'F')) *tK_e10 = temperatureF2K_e10 (t_e10);
		else return false;

		fifoCopyReadPosition (fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPrintTemperatureK (Fifo *fifo, Int32 tK_e10, int min, int max, int decimals, bool showSign) {
	return  fifoPrintFixedPointInt (fifo,tK_e10,E10,min,max,decimals,showSign)
		&& fifoPrintChar (fifo,TEMPERATURE_CHAR)
		&& fifoPrintChar (fifo,'K');
}

//SLICE
bool fifoPrintTemperatureC (Fifo *fifo, Int32 tK_e10, int min, int max, int decimals, bool showSign) {
	return  fifoPrintFixedPointInt (fifo,temperatureK2C_e10 (tK_e10),E10, min,max,decimals,showSign)
		&& fifoPrintChar (fifo,TEMPERATURE_CHAR)
		&& fifoPrintChar (fifo,'C');
}

//SLICE
bool fifoPrintTemperatureF (Fifo *fifo, Int32 tK_e10, int min, int max, int decimals, bool showSign) {
	return  fifoPrintFixedPointInt (fifo,temperatureK2F_e10 (tK_e10),E10, min,max,decimals,showSign)
		&& fifoPrintChar (fifo,TEMPERATURE_CHAR)
		&& fifoPrintChar (fifo,'F');
}

