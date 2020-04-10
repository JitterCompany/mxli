/*
  seqioFram.h 
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef seqioFram_h
#define seqioFram_h

/** @file
 * @brief FRAM commands implementation using seqio.
 */

#include <fram.h>
#include <seqio.h>
#include <simpleMath.h>

/** Prepares writing data to the FRAM. Large FRAMs need page addressing through I2C address modifications. We cannot
 * handle this here - the user has to take care of this. However, we still allow more than 64kiB to be transfered.
 * More than one write can be enqueued into one seqio transaction. The transactions are merged for beeing able to
 * cross FRAM page bounds. Do not mix reads and writes, as this breaks the possibility to cross page bounds.
 * A seqioPrepareEnd() is required after this call!!
 * @param seqio the Seqio data transfer object
 * @param addressInFram a 64kiB range (page) address inside the FRAM.
 * @param data the data to transfer to FRAM.
 * @param n the number of bytes to transfer. This can be more than 64kiB.
 * @return true in case seqio was large enough, false otherwise
 */
inline static bool seqioFramPreparePageWrite (Seqio *seqio, Uint16 addressInFram, const void *data, Uint32 n) {
	return
		seqioPrepareWrite (seqio,changeEndian16 (addressInFram),2,false)
		&& seqioPrepareWriteBlock (seqio,data,n,false);
}

/** Prepares reading data from the FRAM. Large FRAMs need page addressing through I2C address modifications. We cannot
 * handle this here - the user has to take care of this. However, we still allow more than 64kiB to be transfered.
 * More than one read can be enqueued into one seqio transaction. The transactions are merged for beeing able to
 * cross FRAM page bounds. Do not mix reads and writes, as this breaks the possibility to cross page bounds.
 * A seqioPrepareEnd() is required after this call!!
 * @param seqio the Seqio data transfer object
 * @param addressInFram a 64kiB range (page) address inside the FRAM.
 * @param data the destination buffer for data from FRAM.
 * @param n the number of bytes to transfer. This can be more than 64kiB.
 * @return true in case seqio was large enough, false otherwise
 */
inline static bool seqioFramPreparePageRead (Seqio *seqio, void *data, Uint16 addressInFram, Uint32 n) {
	return
		seqioPrepareWrite (seqio,changeEndian16 (addressInFram),2,false)
		&& seqioPrepareReadBlock (seqio,data,n,false);
}

/** Prepares reading of the device ID of an FRAM. This transaction must be sent to a reserved I2C address on the same
 * I2C bus as the FRAM. Read in the FRAM manual about this.
 * @param seqio the Seqio data transfer object
 * @param i2cAddressFram the 7-bit I2C address of the FRAM to question. The transaction itself is sent to a 'different'
 *   virtual device (but the FRAM is still responding to it ;-).
 * @return true in case seqio was large enough, false otherwise
 */
inline static bool seqioFramPrepareReadId (Seqio *seqio, Uint8 i2cAddressFram) {
	return
		seqioPrepareWrite (seqio,i2cAddressFram<<1,1,false)
		&& seqioPrepareRead (seqio,3,false);
}

/** Extracts the device ID from a successful 'read FRAM id' transaction.
 * @param seqio the Seqio data transfer object
 * @return the ID or 0 in case of failure
 */
inline static Uint32 seqioFramFinishReadId (Seqio *seqio) {
	if (seqioIsSuccessful (seqio)) return changeEndian24 (seqio->elements[1]);
	else return 0;
}

#endif
