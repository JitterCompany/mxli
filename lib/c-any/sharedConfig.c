#include <sharedConfig.h>

Uint32 sharedConfig8Read (Uint8 what) {
	typedef const Uint32* LongPointer;
	typedef const Uint16* ShortPointer;
	typedef const Uint* BytePointer;

	const Uint8 offset = what & SHARED_CONFIG8_OFFSET;
	switch(what>>_SHARED_CONFIG8_TYPE) {
		case SHARED_CONFIG8_LONG_:	return offset*4 < sharedConfig8.size ? ((LongPointer)sharedConfig8.values)[offset] : 0;
		case SHARED_CONFIG8_SHORT_:	return offset*2 < sharedConfig8.size ? ((ShortPointer)sharedConfig8.values)[offset] : 0;
		case SHARED_CONFIG8_BYTE_:	return offset < sharedConfig8.size ? ((BytePointer)sharedConfig8.values)[offset] : 0;
		case SHARED_CONFIG8_BOOL_:	return offset/8 < sharedConfig8.size ? ((BytePointer)sharedConfig8.values)[offset/8]>>(offset&7) : 0;
		default:			return 0;	// does not happen
	}
}

