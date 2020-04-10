#ifndef __miimKs8721_h
#define __miimKs8721_h

/** @file
 * @brief MICREL PHY KS8721 specific management commands.
 *
 * Provides functions to determine auto-negotiation results and more.
 */

#include <miim.h>
#include <fifo.h>

/** Extended register 0x1Fh tells about negotiation results.
 */
union Ks8721_1fh {
	struct __attribute__((packed)) { Uint16
		disableScrambler:1,		///< RW
		enableSqeTest:1,		///< RW
		operationMode:3,		///< RO
		phyIsolate:1,			///< RO
		enablePause:1,			///< RO
		autoNegotiationComplete:1,	///< RW
		enableJabber:1,			///< RW
		interruptLevel:1,		///< RW
		powerSaving:1,			///< RW
		forceLink:1,			///< RW
		energyDetect:1,			///< RO
		pairswapDisable:1,		///< RW
		:2;
		
	};
	Uint16	uint16;
};

typedef union Ks8721_1fh Ks8721_1fh;

enum {
	KS8721_1FH_STILL_IN_AUTO_NEGOTIATION	=0,
	KS8721_1FH_10BASET_HALF_DUPLEX		=1,
	KS8721_1FH_100BASETX_HALF_DUPLEX	=2,
	KS8721_1FH_10BASET_FULL_DUPLEX		=5,
	KS8721_1FH_100BASETX_FULL_DUPLEX	=6,
	KS8721_1FH_PHY_MII_isolate		=7,
};


bool fifoShowKs8721Configuration(Fifo *fifo, int addressPhy);

#endif
