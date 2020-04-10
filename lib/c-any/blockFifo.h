// (C) Marc Prager, 2007-2009
#ifndef __blockFifo_h
#define __blockFifo_h

/** @file 
 * @brief Inline-function implementation of a FIFO managing a set of constant size buffers.
 *
 * A BlockFifo manages buffers in a producer-consumer scenario. On the write-side, buffers are offered to the caller.
 * None of the buffers offered are used by the read-side. The read side is offered buffers, that were previously
 * released by the write side. None of the buffers offered is still used by the read side. BlockFifo also offers the
 * possibility to put a mark on one buffer at the write side and check for that mark on the read side. This is intended
 * for synchronization between producer and consumer on a granularity greater than one buffer. Only one mark can be set
 * at a time.
 * If the underlying processor will load/store int and unsigned atomically, then this Fifo is thread safe in the
 * following sense:
 *   1. concurrent read/write status, read and write, setting and reading mark are allowed.
 *   2. Concurrent accesses of more than one thread at the read side ist NOT allowed.
 *   3. Concurrent accesses of more than one thread at the write side ist NOT allowed.
 */

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

 
typedef struct {
	size_t			blocks;
	size_t			blockSize;
	char*			blockBuffer;

	volatile size_t		rTotal;
	volatile size_t		wTotal;

	volatile int		rPos;
	volatile int		wPos;

	volatile int		mPos;		///< marking position
} BlockFifo;

/** Reset blockFifo to empty state.
 * WARNING: Not thread safe.
 */
static inline void blockFifoReset(BlockFifo *blockFifo) {
	blockFifo->rTotal = 0;
	blockFifo->wTotal = 0;
	blockFifo->rPos = 0;
	blockFifo->wPos = 0;
	blockFifo->mPos = -1;
}

/** Initialize a with an blockFifo of given size.
 * WARNING: Not thread safe. Do not call if any other functions may be called concurrently.
 * @param blockFifo the BlockFifo object.
 * @param blockBuffer a properly aligned block buffer to store the blocks
 * @param blockBufferSize the size (bytes) of blockBuffer
 * @param blockSize the byte-size of one element.
 * @return the number of blocks
 */
static inline size_t blockFifoInit(BlockFifo *blockFifo, void *blockBuffer, size_t blockBufferSize, size_t blockSize) {
	blockFifoReset(blockFifo);
	blockFifo->blockBuffer = (char*) blockBuffer;
	blockFifo->blockSize = blockSize;
	return blockFifo->blocks = blockBufferSize/blockSize;
}

/** Check, how many indices are (at least) free at the write-side.
 * @param blockFifo the BlockFifo object.
 * @return the number of immediately releaseable indices. The actual number may be greater at a later time. 
 */
static inline size_t blockFifoCanWrite(BlockFifo const *blockFifo) {
	return blockFifo->blocks - blockFifo->wTotal + blockFifo->rTotal;	// yes, this is correct.
}

/** Tries to mark the current write position. The mark is a token, that is passed beween the write-side and
 * the read side. Must not be called unless blockFifoCanWrite().
 * @return true if marking was successful, false if there was no mark available. 
 */
static inline bool blockFifoWriteMark(BlockFifo *blockFifo) {
	if (blockFifo->mPos==-1) {
		blockFifo->mPos = blockFifo->wPos;
		return true;
	}
	else return false;
}

/** Lock the next write-side index. Make sure there is an index free before.
 * @param blockFifo the BlockFifo object.
 * @return an index not occupied by the read-side. In order to make the index available on the read-side it must be
 *   released on the write-side.
 * @see blockFifoCanWrite(), blockFifoWriteRelease()
 */
static inline void* blockFifoWriteLock(BlockFifo *blockFifo) {
	return & blockFifo->blockBuffer[blockFifo->wPos*blockFifo->blockSize];
}

/** Release the currently locked write-side index and make it available to the read-side.
 * @param blockFifo the BlockFifo object.
 */
static inline void blockFifoWriteRelease(BlockFifo *blockFifo) {
	blockFifo->wTotal++;
	blockFifo->wPos++; if (blockFifo->wPos==blockFifo->blocks) blockFifo->wPos = 0;	// short for: wPos = (wPos+1)%blocks
}

/** Check, how many indices are (at least) available at the read-side.
 * @param blockFifo the BlockFifo object.
 * @return the number of immediately readable indices. The actual number may be greater at a later time. 
 */
static inline size_t blockFifoCanRead(BlockFifo const *blockFifo) {
	return blockFifo->wTotal - blockFifo->rTotal;	// yes, this is correct, despite limited precision.
}

/** Checks if the currently locked read position is marked. If it is marked, the mark is removed.
 * Must not be called unless blockFifoCanRead().
 * @return true if the read-side position was marked false if not
 */
static inline bool blockFifoReadMark(BlockFifo *blockFifo) {
	if (blockFifo->mPos!=-1 && blockFifo->mPos==blockFifo->rPos) {
		blockFifo->mPos = -1;
		return true;
	}
	else return false;
}
 
/** Lock the next read-side block. Make sure there is a block available before.
 * @param blockFifo the BlockFifo object.
 * @return a block previously released on the write-side. In order to re-use the block on the write-side it must be
 *   released on the read-side.
 * @see blockFifoCanRead(), blockFifoReadRelease()
 */
static inline const void* blockFifoReadLock(BlockFifo *blockFifo) {
	return & blockFifo->blockBuffer[blockFifo->rPos*blockFifo->blockSize];
}

/** Release the currently locked read-side index and make it available to the write-side again.
 * @param blockFifo the BlockFifo object.
 */
static inline void blockFifoReadRelease(BlockFifo *blockFifo) {
	blockFifo->rTotal++;
	blockFifo->rPos++; if (blockFifo->rPos==blockFifo->blocks) blockFifo->rPos = 0;	// short for: rPos = (rPos+1)%blocks
}

#ifdef __cplusplus
}
#endif
#endif

