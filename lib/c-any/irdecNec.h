#ifndef __irdecNec_h
#define __irdecNec_h

#include <stdbool.h>

/** @file
 * @brief NEC IR protocol decoder.
 *
 * This module assumes, that IR decoder edges are sampled with us-resolution and forwarded to this decoder.
 * For this protocol, the rising edges of light intensity (=falling edges of TSOP6238-IR-receiver output)
 * are used.
 */

/** Configuration parameters for NEC protocol.
 */
typedef struct {
	int	headerUs;	///< time from header edge to first bit edge (13.5ms).
	int	oneUs;		///< time between two edges of a '1' (2.25ms).
	int	zeroUs;		///< time between two edges of a '0'(1.12ms).
	int	toleranceUs;	///< allowable edge jitter (0.3ms?).
} IrdecNecConfig;

/** Decoder object.
 */
typedef struct {
	const IrdecNecConfig*	config;
	int			state;		///< set to 0 initially
	int			tPrevUs;	///< time of previous edge.
	int			bitIdx;		///< current bit index, LSB comes first.
	int			data;		///< accumulated data bits
} IrdecNec;

/** Default settings of NEC protocol.
 */
const IrdecNecConfig irdecNecConfigDefault;

/** Feeds the time of the next edge into the decoder.
 * @param irdec the decoder
 * @param tEdgeUs the time stamp of the edge in microseconds.
 * @return a valid address/command code or 0 if nothing was decoded, yet. 0 is NOT a valid
 *   address/command code in this protocol. cmd is bits 16..23, addr is bits 0..15.
 */
int irdecNecFeed(IrdecNec *irdec, int tEdgeUs);

/** Checks, if datagram is plain old NEC protocol
 */
bool irdecNecCheck(int code);

/** Checks, if datagram is extended NEC protocol
 */
bool irdecNecCheckExtended(int code);

#endif
