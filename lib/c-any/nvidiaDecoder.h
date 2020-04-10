#ifndef __nvidiaIrDecoder_h
#define __nvidiaIrDecoder_h

/** @file
 * @brief A decoder for the NVIDIA 3D IR protocol.
 *
 * This module implements one instance of a NVidia IR decoder. Decoding is done in an optimistic way.
 * NOT TESTED YET!!
 */

#include <stdbool.h>

/** State of NVIDIA shutter glasses.
 */
enum NvidiaPhase {
	NV_OFF,		///< both shutters closed or no sync.
	NV_L,		///< left shutter open.
	NV_R		///< right shutter open.
};

typedef enum NvidiaPhase NvidiaPhase;

/** Resets the NVidia Decoder.
  */
void nvidiaDecoderInit(void);

/** Feeds the NVidia Decoder with a new signal edge.
 * @param level the signal level of the IR receiver.
 * @param tUs the time of the edge in microseconds.
 * @return the state of the shutter.
 */
NvidiaPhase nvidiaDecoderEdge(bool level, int tUs);

/** Indicates the lack of signal transitions to the NVidia Decoder which can lead to a time-out error.
 * @param tUs the current time.
 * @return the state of the shutter.
 */
NvidiaPhase nvidiaDecoderIdle(int tUs);

/** Return the current state of the shutter. 
 * @return the state of the shutter.
 */
NvidiaPhase nvidiaDecoderGetPhase(void);

#endif
