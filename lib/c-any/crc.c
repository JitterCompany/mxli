#include <crc.h>
#include <stdbool.h>

/** Processes another byte.
 * @param shiftRegister the current value of the shift register. This is updated.
 * @param data the data to add to the stream.
 * @return the current division remainder - the CRC sum, finally.
 */
Uint32 crc8Feed (Uint32 polynom, Uint32 shiftRegister, Uint8 data) {
	enum { HIGH_BIT=8 };

	shiftRegister = shiftRegister << 8 | data;
	for (int b=7; b>=0; b--) {	// for new data bits
		const bool apply = (shiftRegister & 1<<b+HIGH_BIT) != 0;
		if (apply) shiftRegister ^= polynom<<b;
	}
	return shiftRegister;
}


Uint32 crc8FeedN (Uint32 polynom, Uint32 shiftRegister, const Uint8 *data, Uint32 n) {
	for (Uint32 i=0; i<n; i++) shiftRegister = crc8Feed (polynom, shiftRegister, data[i]);

	return shiftRegister;
}

