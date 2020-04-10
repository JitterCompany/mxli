#ifndef __lpcMac_h
#define __lpcMac_h

#include <stdbool.h>
#include <integers.h>
#include <miim.h>

/** @file
 * @brief Register definitions of LPC17,LPC23 MAC.
 *
 */

/** LPC17 MAC access structure. Layout correct for LPC17 at least.
 */
typedef struct __attribute__((packed,aligned(4))) {
	// 0x00
	Uint32 volatile		mac1;			///< MAC configuration 1
	Uint32			mac2;			///< MAC configuration 2
	Uint32			ipgt;			///< back-to-back inter-packet-gap
	Uint32			ipgr;			///< non back-to-back inter-packet-gap
	// 0x10
	Uint32			clrt;			///< collision window / retry
	Uint32			maxf;			///< maximum frame
	Uint32			supp;			///< PHY support
	Uint32			test;			///< test
	// 0x20
	Uint32 volatile		mcfg;			///< MIIM configuration
	Uint32 volatile		mcmd;			///< MIIM command
	Uint32 volatile		madr;			///< MIIM address
	Uint32 volatile		mwtd;			///< MIIM write data
	// 0x30
	Uint32 volatile const	mrdd;			///< MIIM read data
	Uint32 volatile	const	mind;			///< MIIM indicators
	Uint32			_0[2];			// dummy
	// 0x40
	Uint32			sa[3];			///< station address
	Uint32			_1;			// dummy
	// 0x50
	Uint32			_2[(0x100-0x50)/4];	// dummy
	// 0x100
	Uint32 volatile		command;		///< PHY commands
	Uint32 volatile	const	status;			///< PHY status
	void const*		rxDescriptor;		///< receive descriptor(s) base address
	void*			rxStatus;		///< receive status base address
	Uint32			rxDescriptorNumber;	///< receive descriptor count
	Uint32 volatile const	rxProduceIndex;
	Uint32 volatile		rxConsumeIndex;		///< written by driver, HW on reset.
	void const*		txDescriptor;		///< transmit descriptor(s) base address
	void*			txStatus;
	Uint32			txDescriptorNumber;
	Uint32 volatile		txProduceIndex;		///< written by driver, HW on reset
	Uint32 volatile const	txConsumeIndex;
	Uint32			_3[(0x158-0x130)/4];
	Uint32 volatile const	tsv0;
	Uint32 volatile const	tsv1;
	Uint32 volatile const	rsv;
	Uint32			_4[(0x170-0x164)/4];	// dummy
	// 0x170
	Uint32			flowControlCounter;
	Uint32 volatile	const	flowControlStatus;
	// 0x178
	Uint32			_5[(0x200-0x178)/4];	// dummy
	// 0x200
	Uint32			rxFilterCtrl;
	Uint32			rxFilterWolStatus;
	Uint32			rxFilterWolClear;
	Uint32			_6;
	// 0x210
	Uint32			hashFilterL;
	Uint32			hashFilterH;
	// 0x218
	Uint32			_7[(0xFE0-0x218)/4];
	// 0xFE0
	Uint32 volatile const	intStatus;
	Uint32			intEnable;
	Uint32			intClear;
	Uint32			intSet;
	Uint32			_8;
	Uint32			powerDown;
} LpcMac;

/** Layout of MAC1 register
 */
typedef union {
	struct __attribute__((packed,aligned(4))) {
	Uint32	receiveEnable:1;	///< [0] allow incoming frames
	Uint32	passAllFrames:1;	///< [0] set to 1 to receive control frames in addition to data frames
	Uint32	rxFlowControl:1;	///< [0] 1 = act upon received PAUSE frames, 0 = ignore
	Uint32	txFlowControl:1;	///< [0] 1 = allow transmission of pause frames
	Uint32	loopBack:1;		///< [0] 1 = receive all frames sent
	Uint32	:3;
	Uint32	resetTx:1;		///< [0] 1 puts transmit logic in reset
	Uint32	resetMcsTx:1;		///< [0] 1 resets TX MAC control sublayer (flow control logic)
	Uint32	resetRx:1;		///< [0] 1 puts receive logic in reset
	Uint32	resetMcsRx:1;		///< [0] 1 resets RX MAC control sublayer (flow control logic)
	Uint32	:2;
	Uint32	simulationReset:1;	///< [0] resets the random number generator of TX
	Uint32	softReset:1;		///< [1] puts all interfaces in reset except the host interface
	};
	Uint32	uint32;
} Mac1;

/** Layout of MAC2 register
 */
typedef union {
	struct __attribute__((packed,aligned(4))) { Uint32
	fullDuplex:1,		///< [0] 1 = MAC operates in full duplex, 0 = half duplex
	frameLengthChecking:1,	///< [0] 1 = if length/type is a length (<=1500), then check it; USELESS!
	hugeFrameEnable:1,	///< [0] 1 = transmit/receive frames of any length
	delayedCrc:1,		///< [0] 1 = exclude 4 extra bytes in front of the frame from CRC
	crcEnable:1,		///< [0] 1 = append CRC
	padCrcEnable:1,		///< [0] 1 = pad short frames to 60 or 64 bytes (VLAN difference) excluding CRC
	vlanPadEnable:1,	///< [0] 1 = pad short frames to 64 bytes excluding CRC (always)
	autoDetectPadEnable:1,	///< [0] 1 = pad short frames to 60/64 bytes depending on the VLAN tag
	purePreambleEnforcement:1,	///< [0] 1 = checks if preamble contains only 0x55s
	longPreambleEnforcement:1,	///< [0] 1 = allow preamble length of 12 bytes only (non-std)
	:2,
	noBackoff:1,		///< [0] 1 = immediate retransmission after collision (non-std)
	backPressureNoBackoff:1,	///< [0] 1 = see data sheet
	excessDefer:1;		///< [0] 1 = MAC will defer to carrier indefinitely (std); 0 = use limit
	};
	Uint32	uint32;
} Mac2;

/** Layout of IntStatus register.
 */
typedef union {
	struct __attribute__((packed,aligned(4))) { Uint32
	rxOverrun:1,		///< fatal overrun. Needs to be resolved by RX reset.
	rxError:1,		///< receive errors. Not too useful because of length.
	rxFinished:1,		///< triggered, when all RX descriptors are processed (consume=produce)
	rxDone:1,		///< triggered, when descriptor processed that has an interrupt flag set
	txUnderrun:1,		///< fatal underrun. Needs to be resolved by TX reset.
	txError:1,		///< transmit errors.
	txFinished:1,		///< triggered, when all TX descriptors are processed (consume=produce)
	txDone:1,		///< triggered. when descriptor processed that has an interrupt flag set
	:4,
	soft:1,			///< software triggered
	wakeup;			///< wakeup event received.
	};
	Uint32	uint32;
} MacIntStatus;

typedef union {
	struct __attribute__((packed,aligned(4))) { Uint32
	rxEnable:1,		///< 1 = enable receive
	txEnable:1,		///< 1 = enable transmit
	:1,
	regReset:1,		///< 1 = resets all datapath registers and host registers; MAC needs separate reset.
	txReset:1,		///< 1 = resets transmit datapath
	rxReset:1,		///< 1 = resets receive datapath
	passRuntFrame:1,	///< 1 = passes frames smaller than 64bytes unless the have CRC errors
	passRxFilter:1,		///< 1 = disable receive filtering
	txFlowControl:1,	///< 1 = enables flow control with PAUSE(full duplex) or continuous preamble (half d.)
	rmii:1,			///< 1 = uses RMII interface. Set this to 1, LPC17 does not support MII.
	fullDuplex:1;		///< 1 = full duplex operation
	};
	Uint32	uint32;		///< command as an integer
} MacCommand;

typedef union {
	struct __attribute__((packed,aligned(4))) { Uint32
	rxStatus:1,		///< 1 = enabled in MAC.command and receive queue not full
	txStatus:1;		///< 1 = enables in MAC.command and transmit queue not empty
	};
	Uint32	uint32;		///< status as an integer
} MacStatus;

typedef union {
	struct __attribute__((packed,aligned(4))) { Uint32
	acceptUnicast:1,	///< [0] 1 = accepts all unicast frames
	acceptBroadcast:1,	///< [0] 1 = accepts all broadcast frames
	acceptMulticast:1,	///< [0] 1 = accepts all multicast frames
	acceptUnicastHash:1,	///< [0] 1 = accepts all unicast frames that pass the imperfect hash filter
	acceptMulticastHash:1,	///< [0] 1 = accepts all multicast frames that pass the imperfect hash filter
	acceptPerfect:1,	///< [0] 1 = accepts frames with destination address equal to station address
	:6,
	magicPacketWol:1,	///< [0] 1 = magic packet filter will generate WoL interrupt
	rxFilterWoL:1;		///< [0] 1 = perfect or imperfect filters will generate WoL.
	};
	Uint32	uint32;
} MacRxFilterControl;

/** LPC17 receive descriptor
 */
typedef struct __attribute__((packed,aligned(4))) {
	char*		packet;		///< data buffer
	union {
		struct __attribute__((packed)) {
			unsigned	sizeMinus1:11;
			unsigned	:20;
			unsigned	interrupt:1;	///< generate interrupt if frame fragment has arrived.
		};
		unsigned		control;	///< control bits as a single 32-bit word
	};
} RxDescriptor;

/** LPC17 receive status. Must be aligned on a 8 byte boundary. The structure is 8 bytes in size.
 */
typedef struct __attribute__((packed,aligned(8))) {
	union {
		struct __attribute__((packed, aligned(4))) { Uint32
			sizeMinus1:11,		///< actual Ethernet frame size including CRC.
			:7,
			controlFrame:1,		///< flow control frame or unknown opcode
			vlan:1,			///< indicates a VLAN frame
			failFilter:1,		///< frame has failed RX filter
			multicast:1,		///< multicast frame
			broadcast:1,		///< broadcast frame
			crcError:1,		///< frame has CRC error
			symbolError:1,		///< bit error from PHY
			lengthError:1,		///< Ethernet-I packet length does not match real size
			rangeError:1,		///< useless for Ethernet-II packets (the standard).
			alignmentError:1,	///< dribble bits and CRC error
			overrun:1,		///< receive overrun
			noDescriptor:1,		///< no new RX-descriptor available but needed
			lastFlag:1,		///< 1=this is the last fragment of a frame, 0=more follow
			error:1;		///< useless because of rangeError
		};
		Uint32	statusInfo;
	};
	Uint32		statusHashCrc;
} RxStatus;

typedef struct __attribute__((packed,aligned(4))) {
	char*	packet;		///< data buffer
	union {
		struct __attribute__((packed,aligned(4))) { Uint32
			sizeMinus1:11,
			:15,
			override:1,	///< 1 to use the following fields to override defaults of MAC
			huge:1,		///< enable frames of unlimited size.
			pad:1,		///< pad frames < 64 bytes
			crc:1,		///< append hardware-CRC to the frame
			last:1,		///< last fragment of a frame
			interrupt:1;	///< generator interrupt if frame sent and queue updated.
		};
		Uint32	control;	// same as above as a single 32-bit word.
	};
} TxDescriptor;

typedef union {
	struct __attribute__((packed,aligned(4))) {
		unsigned
			:21,
			collisionCount:4,	///< number of collisions up to retransmission maximum
			defer:1,		///< medium was occupied and defer resulted
			excessiveDefer:1,	///< packet was aborted due to excessive defer
			excessiveCollision:1,	///< packet was aborted due to excessive collisions
			lateCollision:1,	///< out of window collision resulting in package abort
			underrun:1,		///< ?
			noDescriptor:1,		///< no TX descriptor is available for finishing packet.
			error:1;		///< transmission error as logical OR of above errors.
	};
	unsigned	statusInfo;
} TxStatus;

enum MadrBits {
	MADR_REGISTER_ADDRESS	=0,	// 5 bits
	MADR_PHY_ADDRESS	=8,	// 5 bits
};

enum McmdBits {
	MCMD_READ		=0,
	MCMD_SCAN		=1,
};

enum MindBits {
	MIND_BUSY		=0,
	MIND_SCANNING		=1,
	MIND_NOT_VALID		=2,
	MIND_MII_LINK_FAIL	=3,
};

enum McfgBits {
	MCFG_SCAN_INCREMENT	=0,
	MCFG_SUPPRESS_PREAMBLE	=1,
	MCFG_CLOCK_SELECT	=2,	// 4 bits
	MCFG_RESET_MII_MGMT	=15,
	
};

enum CommandBits {
	COMMAND_RX_ENABLE=0,
	COMMAND_TX_ENABLE=1,
	COMMAND_REG_RESET=3,
	COMMAND_TX_RESET=4,
	COMMAND_RX_RESET=5,
	COMMAND_PASS_RUNT_FRAME=6,
	COMMAND_PASS_RX_FILTER=7,
	COMMAND_TX_FLOW_CONTROL=8,
	COMMAND_RMII=9,
	COMMAND_FULL_DUPLEX=10,
};

enum Mac1Bits {
	MAC1_RECEIVE_ENABLE=0,			///< receive incoming frames
	MAC1_PASS_ALL_RECEIVE_FRAMES=1,		///< also pass control frames
	MAC1_RX_FLOW_CONTROL=2,			///< act upon received PAUSE frames
	MAC1_TX_FLOW_CONTROL=3,
	MAC1_LOOPBACK=4,
	// reserved
	MAC1_RESET_TX=8,			///< reset transmit function logic
	MAC1_RESET_MCS_TX=9,			///< reset flow control of transmit function logic
	MAC1_RESET_RX=10,			///< reset receive function logic
	MAC1_RESET_MCS_RX=11,			///< reset flow control of receive function logic
	MAC1_SIMULATION_RESET=14,		///< reset random number generator of transmit function
	MAC1_SOFT_RESET=15,			///< reset all MAC modules except host interface
};

enum Mac2Bits {
	MAC2_FULL_DUPLEX=0,			///< enable full duplex mode
	MAC2_FRAME_LENGTH_CHECKING=1,		///< check length/type field of incoming/outgoing frames
	MAC2_HUGE_FRAME_ENABLE=2,		///< enable frames of any length
	MAC2_DELAYED_CRC=3,			///< unimportant
	MAC2_CRC_ENABLE=4,
	MAC2_PAD_CRC_ENABLE=5,
	MAC2_VLAN_PAD_ENABLE=6,
	MAC2_AUTO_DETECT_PAD_ENABLE=7,
	MAC2_PURE_PREAMBLE_ENFORCEMENT=8,
	MAC2_LONG_PREAMBLE_ENFORCEMENT=9,
	// reserved
	MAC2_NO_BACKOFF=12,
	MAC2_BACKPRESSURE_NO_BACKOFF=13,
	MAC2_EXCESS_DEFER=14,
};

enum SuppBits {
	SUPP_SPEED=8,				///< 1 for 100Mbit, 0 for 10Mbit
};

enum IntStatusBits {
	INT_STATUS_RX_OVERRUN	=0,	///< fatal overrun. Needs to be resolved by RX reset.
	INT_STATUS_RX_ERROR	=1,	///< receive errors. Not too useful because of length.
	INT_STATUS_RX_FINISHED	=2,	///< triggered, when all RX descriptors are processed (consume=produce)
	INT_STATUS_RX_DONE	=3,	///< triggered, when descriptor processed that has an interrupt flag set
	INT_STATUS_TX_UNDERRUN	=4,	///< fatal underrun. Needs to be resolved by TX reset.
	INT_STATUS_TX_ERROR	=5,	///< transmit errors.
	INT_STATUS_TX_FINISHED	=6,	///< triggered, when all TX descriptors are processed (consume=produce)
	INT_STATUS_TX_DONE	=7,	///< triggered. when descriptor processed that has an interrupt flag set
	INT_STATUS_SOFT		=12,	///< software triggered
	INT_STATUS_WAKEUP	=13,	///< wakeup event received.
};

#endif

