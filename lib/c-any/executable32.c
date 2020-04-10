//HEADER
#include <executable32.h>
#include <int32Math.h>
#include <fifoPrint.h>

//SLICE
Uint32 executable32Segments (const Executable32Segment *segs) {
	Uint32 n = 0;
	while (segs[n].size) n++;

	return n;
}

//SLICE
Executable32Segment executable32NextChunk (
	const Executable32Segment *exe,
	Executable32SegmentIterator *iterator,
	Uint32 chunkSize) {

	// move on to next segment, if the current one is exhausted
	while (exe[iterator->segment].size > 0 && exe[iterator->segment].size <= iterator->offset) {
		iterator->segment++;
		iterator->offset = 0;
	}

	Executable32Segment segment = {
		.size		= exe [iterator->segment].size > 0 ?
					uint32Min (exe[iterator->segment].size, chunkSize)
					: 0,
		.address	= exe[iterator->segment].address + iterator->offset,
		.data		= exe[iterator->segment].data + iterator->offset/sizeof(Uint32),
	};
	iterator->offset += segment.size;

	return segment;
}

//SLICE

bool fifoPrintExecutable32Segments (Fifo *fifo, const Executable32Segment *segments) {
	bool success = fifoPrintString (fifo,"Executable32:");
	for (int s=0; success && segments[s].size!=0; ++s) {
		success = success
		&& fifoPrintString (fifo,"\n  segment: address=0x")
		&& fifoPrintHex (fifo,segments[s].address,8,8)
		&& fifoPrintString (fifo,", size=0x")
		&& fifoPrintHex (fifo,segments[s].size,8,8);
	}
	return success && fifoPrintLn (fifo);
}

