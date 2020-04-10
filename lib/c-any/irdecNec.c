#include <irdecNec.h>
#include <int32Math.h>

/** Decoder object state.
 */
enum {
	IRDEC_NEC_INIT,		///< nothing detected yet.
	IRDEC_NEC_DATA,		///< header detected, accumulating data bits.
};

const IrdecNecConfig irdecNecConfigDefault = {
	.headerUs = 13500,
	.oneUs = 2250,
	.zeroUs = 1125,
	.toleranceUs = 300
};

//SLICE

// helper function
static bool isApprox(int a, int b, int r) {
	return int32Abs(a-b) <= r;
}

// helper function
static int irdecNecHeader(IrdecNec *irdec, int tEdgeUs) {
	irdec->state = IRDEC_NEC_DATA;
	irdec->tPrevUs = tEdgeUs;
	irdec->bitIdx = 0;
	irdec->data = 0;
	return 0;
}

/** Feeds the time of the next edge into the decoder.
 * @param irdec the decoder
 * @param tEdgeUs the time stamp of the edge in microseconds.
 * @return a valid address/command code or 0 if nothing was decoded, yet. 0 is NOT a valid
 *   address/command code in this protocol. cmd is bits 16..23, addr is bits 0..15.
 */
int irdecNecFeed(IrdecNec *irdec, int tEdgeUs) {
	const int dtUs = tEdgeUs - irdec->tPrevUs;
	const bool isHeader = isApprox(irdec->config->headerUs,dtUs,irdec->config->toleranceUs);
	irdec->tPrevUs = tEdgeUs;

	switch (irdec->state) {
	case IRDEC_NEC_INIT: return isHeader ? irdecNecHeader(irdec,tEdgeUs) : 0;
	case IRDEC_NEC_DATA: {
			const bool isOne = isApprox(irdec->config->oneUs,dtUs,irdec->config->toleranceUs);
			const bool isZero = isApprox(irdec->config->zeroUs,dtUs,irdec->config->toleranceUs);
			if (isOne || isZero) {
				if (irdec->bitIdx<32) {
					irdec->data |= (int)isOne << irdec->bitIdx;
					irdec->bitIdx++;
				}

				if (irdec->bitIdx==32) {
					irdec->state = IRDEC_NEC_INIT;
					return irdec->data;
				}
				else return 0;
			}
			else if (isHeader) return irdecNecHeader(irdec,tEdgeUs);	// header match, unexpectedly
			else irdec->state = IRDEC_NEC_INIT;	// no match whatsoever
		}
		return 0;
	default: return 0;
	}
}

/** Checks, if datagram is plain old NEC protocol
 */
bool irdecNecCheck(int code) {
	return 0 == (0xFF00FF00 & ((code<<8) ^ ~code));
}

/** Checks, if datagram is extended NEC protocol
 */
bool irdecNecCheckExtended(int code) {
	return	0 == (0xFF000000 & ((code<<8) ^ ~code))
		&& !irdecNecCheck(code);		// and not a standard one!
}

