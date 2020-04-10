#include <projectionDesignControl.h>

bool operationPacketParserReceive(OperationPacketParser *opp, char b) {
	static const char PAK = 0x1E;
	static const char magic1 = 0xBE;
	static const char magic2 = 0xEF;

	switch(opp->state) {
		case OPP_SCAN:
			if (b==PAK) opp->state = OPP_PAK_RECEIVED;
			else ; // printChar('?');
			return false;
		case OPP_PAK_RECEIVED:
			if (b==magic1) {
				opp->state = OPP_MAGIC1_RECEIVED;
				opp->buffer[0] = b;
			}
			else opp->state = OPP_SCAN;
			return false;
		case OPP_MAGIC1_RECEIVED:
			if (b==magic2) {
				opp->state = OPP_MAGIC2_RECEIVED;
				opp->buffer[1] = b;
				opp->index = 2;
			}
			else opp->state = OPP_SCAN;
			return false;
		case OPP_MAGIC2_RECEIVED:
			opp->buffer[opp->index] = b;
			++opp->index;
			if (opp->index==sizeof(OperationPacket)) {
				opp->state = OPP_SCAN;
				return true;
			}
			else return false;

		default: return false;
	}
}


/** Writes the header of the packet and clears out all the rest.
 * @param op the operation packet buffer.
 */
static void operationPacketPrepare(OperationPacket *op) {
	static const Uint8 operationPacketHeader[7] = { 0xBE, 0xEF, 0x03, 0x19, 0x00, 0x00, 0x00 };
	memset(op,0,sizeof *op);
	memcpy(op,operationPacketHeader,sizeof operationPacketHeader);
}

static void operationPacketSet(OperationPacket *op, Uint16 number, Uint16 value) {
	operationPacketPrepare(op);
	op->type = OP_TYPE_SET;
	op->number = number;
	op->value = value;
}

/*
 static void operationPacketGet(OperationPacket *op, Uint16 number) {
	operationPacketPrepare(op);
	op->type = OP_TYPE_GET;
	op->number = number;
}
*/

static bool fifoWriteOperationPacket(Fifo *fifo, const OperationPacket op) {
	if (fifoCanWrite(fifo)>sizeof op) {
		fifoWriteN(fifo,&op,sizeof op);
		return true;
	}
	else return false;
}

bool fifoWriteCommand(Fifo *fifo, int type, int command, int value) {
	OperationPacket op;
	operationPacketPrepare(&op);
	op.type = type;
	op.number = command;
	op.value = value;
	return fifoWriteOperationPacket(fifo,op);
}

bool fifoWriteSetCommand(Fifo *fifo, int command, int value) {
	return fifoWriteCommand(fifo,OP_TYPE_SET,command,value);
}

bool fifoWriteGetCommand(Fifo *fifo, int command) {
	return fifoWriteCommand(fifo,OP_TYPE_GET,command,0);
}

static bool fifoWriteCscValue(Fifo *fifo, unsigned short value) {
	OperationPacket op;
	operationPacketSet(&op,OP_CSC_VALUE,value);
	return fifoWriteOperationPacket(fifo,op);
}

static bool fifoWriteCscCounterReset(Fifo *fifo) {
	OperationPacket op;
	operationPacketSet(&op,OP_CSC_COUNTER,0);
	return fifoWriteOperationPacket(fifo,op);
}

/** Writes Csc values. 9 values must be written. The new values are in effect immediately.
 */
bool fifoWriteCscMatrix(Fifo* fifo, const CscMatrix cscMatrix) {
	static const unsigned char perm[] = { 1,0,2 };

	if (fifoWriteCscCounterReset(fifo)) {
		bool success = true;
		for (int i=0; i<3; ++i) for (int j=0; j<3 && success; ++j) {
			success = success && fifoWriteCscValue(fifo,cscMatrix[perm[i]][perm[j]]);
		}
		return success;
	}
	else return false;
}

