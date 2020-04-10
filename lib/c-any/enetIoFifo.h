#ifndef enetIoFifo_h
#define enetIoFifo_h

/** @file
 * @brief An abstract interface of Ethernet IO that can be used to implement an IP-stack.
 *
 * The Ethernet driver owns all Fifos - the user never has to deal with allocation. Writing to the Fifos always
 * results in writes to the driver's buffer directly.
 */

#include <stdbool.h>
#include <fifo.h>

/** Checks if another packet's buffer is available for writing by the user.
 * @return true if there is at least one buffer available, false otherwise.
 */
bool enetIoFifoCanWrite(void);

/** Returns a Fifo for preparing a packet's contents. It is allowed to call this function multiple times without
 * enetIoFifoWriteEnd() - each call will return the same Fifo in empty state.
 * @return a writeable Fifo - check for availability before.
 */
Fifo* enetIoFifoWriteBegin(void);

/** Commits the contents of the current write Fifo. This function must be called to send a packet.
 */
void enetIoFifoWriteEnd(void);


/** Checks, if another packet is available for reading.
 * @return true if at least one packet has been received and can be processed, false otherwise.
 */
bool enetIoFifoCanRead(void);

/** Returns a Fifo for reading a packet's contents. It is allowed to call this function multiple times without
 * enetIoFifoReadEnd() - each call will return the whole packet again.
 */
Fifo* enetIoFifoReadBegin(void);

/** Releases the current read Fifo and allows for the next packet to be read. This function must be called to
 * receive further packets.
 */
void enetIoFifoReadEnd(void);

#endif
