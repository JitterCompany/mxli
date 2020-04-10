#ifndef __projectionDesignControl_h
#define __projectionDesignControl_h

#include <integers.h>
#include <stdbool.h>
#include <fifo.h>

typedef Uint16 CscMatrix[3][3];

////////////////////////////////////////////////////////////////////////////////
// Sending commands:
//
/** Binary datagram.
 */
struct OperationPacket {
	Uint8		header[7];
	Uint8		type;		///< see enum OperationPacketType
	Uint16		number;		///< operation number
	Uint8		validation;	///< return only: operation is valid
	Uint8		reserved1;
	Uint8		reserved[4];
	Int16		value;		///< value to SET / result of a GET
	Int16		value2;		///< unused
	Int32		lowerLimit;
	Int32		upperLimit;
	Int32		increment;
} __attribute__((packed));

typedef struct  OperationPacket OperationPacket;


enum OperationType {
	OP_TYPE_SET=1,
	OP_TYPE_GET=2,
	OP_TYPE_INCREMENT=3,
	OP_TYPE_DECREMENT=4,
	OP_TYPE_EXECUTE=5,
	OP_TYPE_DESCRIPTOR=7,
};

/** Binary Operation codes.
 */
enum OperationNumber {				// tested ?
	OP_POWER		= 0x029C,	// OK
	OP_VIDEO_SOURCE		= 0x4401,	//
	OP_SOURCE_SCAN		= 0x4423,	// value = 2 (!) on, value = 0 off.
	OP_SCALING		= 0x4416,	// OK
	OP_AUDIO_MUTE		= 0x0269,
	OP_GAMMA_COMPUTER	= 0x0290,
	OP_GAMMA_VIDEO		= 0x0291,
	OP_PICTURE_MUTE		= 0x023B,
	OP_OSD			= 0x029D,
	OP_OSD_WARNING		= 0x02C7,
	OP_FREEZE_FRAME		= 0x440B,
	OP_VIDEO_FORMAT		= 0x4413,
	OP_VIDEO_TYPE		= 0x4409,
	OP_ORIENTATION		= 0x0251,	// OK
	OP_TESTIMAGE		= 0x041D,	// OK
	OP_ECO_MODE		= 0x02D6,	// OK
	OP_CSC_COUNTER		= 0x065D,	///< OK, set counter
	OP_CSC_VALUE		= 0x065C,	///< OK, set value
	//OP_CSC_USE		= 0x0381,	
	//OP_FILTER		= 0x0BCD,	///< Optical Filter 0=off, 1=on
};

enum {
	SCALING_1_1			= 0x00,
	SCALING_16_9			= 0x03,
	SCALING_ANAMORPHOTIC		= 0x0E,
	SCALING_FILL_ALL		= 0x01,
	SCALING_FILL_ASPECT_RATIO	= 0x02,
};

enum {
	CSC_USE_OFF			= 0x00,
	CSC_USE_ON			= 0x02,
	CSC_USE_ON_FILTER		= 0x01,
};

enum {	// Video source
	VIDEO_SOURCE_VGA1		= 0x00,
	VIDEO_SOURCE_VGA2		= 0x01,
	VIDEO_SOURCE_DVI		= 0x02,
	VIDEO_SOURCE_SVIDEO		= 0x04,
	VIDEO_SOURCE_COMPOSITE		= 0x05,
	VIDEO_SOURCE_YPBPR1		= 0x06,
};

enum {	// Fixed gamma values
	GAMMA_FILM1			= 0x00,
	GAMMA_FILM2			= 0x01,
	GAMMA_VIDEO1			= 0x02,
	GAMMA_VIDEO2			= 0x03,
	GAMMA_COMPUTER1			= 0x07,
	GAMMA_COMPUTER2			= 0x08,
};

enum {	// image orientation
	ORIENTATION_DESKTOP_FRONT	= 0x00,
	ORIENTATION_DESKTOP_REAR	= 0x02,
	ORIENTATION_CEILING_FRONT	= 0x03,
	ORIENTATION_CEILING_REAR	= 0x01,
};

enum {	// video format
	VIDEO_FORMAT_AUTO		= 0x10,
	VIDEO_FORMAT_NTSC		= 0x01,
	VIDEO_FORMAT_PAL		= 0x06,
	VIDEO_FORMAT_SECAM		= 0x08,
};

////////////////////////////////////////////////////////////////////////////////
// Receiving datagrams

enum OppState {
	OPP_SCAN = 0,
	OPP_PAK_RECEIVED,	///< PAK received
	OPP_MAGIC1_RECEIVED,	///< HEADER, byte #1 received
	OPP_MAGIC2_RECEIVED,
};

typedef struct {
	char*		buffer;		///< pointer to operation packet
	enum OppState	state;
	int		index;		///< index in buffer
} OperationPacketParser;

////////////////////////////////////////////////////////////////////////////////
// Functions

/** Sends a command to the projector. The projector will send back an acknowledge message.
 * @param fifo output stream
 * @param type the type of the message: OP_TYPE_GET or OP_TYPE_SET in most cases.
 * @param command the operation number
 * @param value parameter to command
 * @return true if the command could be placed into fifo, false if fifo ran out of space.
 */
bool fifoWriteCommand(Fifo *fifo, int type, int command, int value);

/** Sends a command to the projector. The projector will send back an acknowledge message.
 * @param fifo output stream
 * @param command the operation number
 * @param value parameter to command
 * @return true if the command could be placed into fifo, false if fifo ran out of space.
 */
bool fifoWriteSetCommand(Fifo *fifo, int command, int value);

/** Requests a response message from the projector.
 * @param fifo output stream
 * @param command the operation number
 * @return true if the command could be placed into fifo, false if fifo ran out of space.
 */
bool fifoWriteGetCommand(Fifo *fifo, int command);

/** Writes a full CSC matrix (9 values 0..1024) to the projector. The projector will respond 10 messages.
 * @param fifo output stream
 * @param matrix the CSC matrix values
 * @return true if the command could be placed into fifo, false if fifo ran out of space.
 */
bool fifoWriteCscMatrix(Fifo *fifo, const CscMatrix matrix);

/** Feeds one more character into the operation packet parser.
 * @param opp the current state of the parser, including parsed data.
 * @param b the byte to feed.
 * @return true, if a valid packet ist finished, false if scanning or still parsing.
 */
bool operationPacketParserReceive(OperationPacketParser *opp, char b);

#endif

