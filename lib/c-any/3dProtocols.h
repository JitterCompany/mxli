#ifndef d3Protocols_h
#define d3Protocols_h

/**
 * Epson IR transmitter protocol of Epson TW-9000 projector.
 *
 * The IR transmitter protocol is as follows:
 * Every bit is 20us in time. 4 distinguishable events were identified (I assume opening and closing of shutter):
 * (a) Open channel A, pattern  0100010001010
 * (b) Close channel A, pattern 0100010100010
 * (c) Open channel B, pattern  0101000001010
 * (d) Close channel B, pattern 0101000100010
 * Pattern (c) is interesting, as it contains 2 0-1 transitions with a distance of 6*20us. No such transition can occur
 * elsewhere in the data stream. A pulse-width decoder will be used to find that pattern only.
 *
 */
enum D3_Epson {
	D3_EPSON_CH_PULSE_WIDTH_US		= 120,		///< pattern (c) above
	D3_EPSON_CH_PULSE_WIDTH_MIN_US		= 120-10,	///< pattern (c) above
	D3_EPSON_CH_PULSE_WIDTH_MAX_US		= 120+10,	///< pattern (c) above
	D3_EPSON_CH_PULSE_EDGE			= 1,		///< rising edge
	D3_EPSON_CH_PULSE_PHASE_E20		= (1<<20) / 2,	///< B active = 180deg.
};

/** NVIDIA shutter glasses protocol.
 * 4 pulse patterns are used:
 *   - left open   (LO) : L,H(44us),L
 *   - left close  (LC) : L,H(24us),L(20us),H(25us),L
 *   - right open  (RO) : L,H(24us),L(45us),H(32us),L
 *   - right close (RC) : L,H(24us),L(78us),H(40us),L
 * Normal sequence: ... -> LO -> LC -> RO -> RC -> ...
 * Easiest to detect is RC: 2 rising edges in 102us distance.
 */
enum D3_Nvidia {
	D3_NVIDIA_CH_PULSE_WIDTH_US		= 102,		///< right eye close pulse
	D3_NVIDIA_CH_PULSE_WIDTH_MIN_US		= 102-5,	///< right eye close pulse
	D3_NVIDIA_CH_PULSE_WIDTH_MAX_US		= 102+5,	///< right eye close pulse
	D3_NVIDIA_CH_PULSE_EDGE			= 1,		///< rising edge
	D3_NVIDIA_CH_PULSE_PHASE_E20		= (1<<20) / 10,	///< B inactive = approx 0deg
};

#endif

