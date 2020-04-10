#include <mxli.h>

// This should not exist...
struct Patch patch = { 0 };			///< state variable required to patch broken ISP CR LF sequences.

bool lpcCommand(const LpcIspIo *io, char command, const Uint32 *params, int nParams, Uint32 *results, int nResults);

#include <fifoPrintFixedPoint.h>
#include <fifoParse.h>
#include <int32Math.h>
#include <int32Pair.h>
#include <uu.h>

#include <ansi.h>
#include <macros.h>

bool (*lpcIspFlowControlHook)(const LpcIspIo *io, char code) = 0;

// some convenience functions
//

Uint32 lpcIspTransmissionTimeUs (const LpcIspConfigCom *com, Uint32 nChars) {
	const int bitsPerChar = 1+8+com->stopBits;
	return MEGA/com->baud * nChars * bitsPerChar;
}

/** Always returns false, but outputs the message only, if not in silent mode.
 */
bool errorMessage (const LpcIspIo *io, const char *msg) {
	if (io->debugLevel>=LPC_ISP_NORMAL) {
		fifoPrintString (io->stderr, "ERROR: ");
		fifoPrintString (io->stderr, msg);
		pushStderr (io);
	}
	return false;
}

void warnMessage (const LpcIspIo *io, const char *msg) {
	if (io->debugLevel>=LPC_ISP_NORMAL) {
		fifoPrintString (io->stderr, "WARNING: ");
		fifoPrintString (io->stderr, msg);
		pushStderr (io);
	}
}

void progressMessage(const LpcIspIo *io, const char *msg) {
	if (io->debugLevel>=LPC_ISP_PROGRESS) {
		fifoPrintString (io->stderr, msg);
		pushStderr (io);
	}
}

void infoMessage(const LpcIspIo *io, const char *msg) {
	if (io->debugLevel>=LPC_ISP_INFO) {
		fifoPrintString (io->stderr, msg);
		pushStderr (io);
	}
}

/** Always returns true, but outputs the message only, if in debug mode.
 */
void debugMessage (const LpcIspIo *io, const char *msg) {
	if (io->debugLevel>=LPC_ISP_DEBUG) {
		fifoPrintString (io->stderr, msg);
		pushStderr (io);
	}
}

/** This function reads in the answer from the LPC into fifoInLine. Any previous contents of the line is discarded.
 * It quickly reads in all characters up to an CR or LF. CR and LF are both represented by LF in the Fifo.
 * Two different LF/CR in direct succession are avoided. If no CR or LF is found, this function runs into a timeout
 * that should terminate this simple application. The state is hidden in the global variable patch, because in a
 * perfect world, it would not exist at all.
 * @return true, if a line was read, false if timeout happened or buffer was too small.
 */
bool loadNextLine(const LpcIspIo *io) {
	// line starts at the same position as the input
	*io->lpcInLine = *io->lpcIn;
	fifoInvalidateWrites(io->lpcInLine);

	while (true) {
		while (fifoCanRead (io->lpcIn)) {
			const char c = fifoRead (io->lpcIn);
			if (io->debugLevel>=LPC_ISP_DEBUG) {
				fifoPrintString (io->stderr,BLUE);
				fifoPrintCharEscaped (io->stderr,c);
				fifoPrintString (io->stderr,NORMAL);
				pushStderr (io);
			}

			switch(c) {
				case 0x13:	// flow control: stop
				case 0x11:	// flow control: start
					if (io->debugLevel>=LPC_ISP_NORMAL) {
						fifoPrintString (io->stderr,"ERROR: Flow control not implemented :-(\n");
						pushStderr (io);
					}
					return false;
				case '\n':
				case '\r':
					if (!patch.lineChar && c!=patch.lineChar) {	// don't recognize different line chars in succession
						if (io->debugLevel>=LPC_ISP_DEBUG) {
							fifoPrintString (io->stderr,"LINE BREAK:\\");
							fifoPrintChar (io->stderr,c=='\n' ? 'n' : 'r');
							fifoPrintChar (io->stderr,'\n');
							pushStderr (io);
						}
						patch.lineChar = c;
						return true;
					}
					else {	// skip that useless character
						fifoValidateWrite (io->lpcInLine);	// add it to the line and...
						fifoRead (io->lpcInLine);		// ... destroy it.
						if (io->debugLevel>=LPC_ISP_DEBUG) {
							if (io->debugLevel>=LPC_ISP_DEBUG) {
								fifoPrintString (io->stderr,"DISCARD:\\");
								fifoPrintChar (io->stderr,c=='\n' ? 'n' : 'r'); // discard line separators in succession
								fifoPrintLn (io->stderr);
								pushStderr (io);
							}
						}
					}
					patch.lineChar = c;	// remember
					break;
				default:
					fifoValidateWrite (io->lpcInLine);
					patch.lineChar = 0;
			}
		}
		// reload for next loop
		if (!pullLpcIn (io)) {
			if (io->debugLevel>=LPC_ISP_NORMAL) {
				fifoPrintString (io->stderr, "ERROR: no response from lpc\n");
				pushStderr (io);
			}
			return false;	//  no characters available in time
		}
	}
}

/** Reads in another line, only if fifo does not already contain one.
 */
bool loadLineOnDemand(const LpcIspIo *io) {
	return loadNextLine (io);
}

/** Reads in a fixed number of bytes.
 * The result is stored in lpcInLine.
 */
bool loadN (const LpcIspIo *io, Uint32 n) {
	// line starts at the same position as the input
	*io->lpcInLine = *io->lpcIn;
	fifoInvalidateWrites(io->lpcInLine);

	while (fifoCanRead(io->lpcInLine)<n) {
		if (fifoCanRead (io->lpcIn)) {
			const char c = fifoRead (io->lpcIn);
			fifoValidateWrite (io->lpcInLine);
			if (io->debugLevel>=LPC_ISP_DEBUG) {
				fifoPrintString (io->stderr,BLUE);
				fifoPrintCharEscaped (io->stderr,c);
				fifoPrintString (io->stderr,NORMAL);
				pushStderr (io);
			}
		}
		else if (!pullLpcIn (io)) {
			errorMessage (io,"Data expected from LPC\n");
			return false;
		}
	}

	return true;
}

/** As LPC ISP programs are all bull-shit, we scan for the expected string within all the garbage instead of expecting
 * an EXACT answer. If it's found, everything up to (including) the pattern is removed.
 */
bool findStringInLine(const LpcIspIo *io, const char *word) {
	Fifo clone = * io->lpcInLine;
	if (fifoMatchUntilPattern(&clone,word)) {
		fifoCopyReadPosition(io->lpcInLine,&clone);
		return true;
	}
	else return false;
}

/** Tries to find a string and if it doesn't an error message is output.
 */
bool findStringInLineOrError(const LpcIspIo *io, const char *word) {
	Fifo clone = * io->lpcInLine;
	if (fifoMatchUntilPattern(&clone,word)) {
		fifoCopyReadPosition(io->lpcInLine,&clone);
		return true;
	}
	else {
		if (io->debugLevel>=LPC_ISP_NORMAL) {
			fifoPrintString (io->stderr,"ERROR: pattern \"");
			fifoPrintString (io->stderr,word);
			fifoPrintString (io->stderr,"\" not found, Fifo was \"");
			fifoAppend (io->stderr,&clone);
			fifoPrintString (io->stderr,"\"\n");
			pushStderr (io);
		}
		return false;
	}
}

/** Reads in a line (if neccessary) that is not an echo line.
 * This function priorizes the echo!!
 */
bool getNonEchoAnswerLine(const LpcIspIo *io, const char *echo) {
	// get a new line.
	if (!loadLineOnDemand(io)) return false;

	Fifo clone = *io->lpcInLine;
	// check if it's an echo
	if (findStringInLine(io,echo)) {
		if (io->debugLevel>=LPC_ISP_DEBUG) {
			fifoPrintString (io->stderr," - skipping echo line. Pattern was \"");
			fifoPrintString (io->stderr,echo);
			fifoPrintString (io->stderr,"\", Fifo was \"");
			fifoAppend (io->stderr,&clone);
			fifoPrintString (io->stderr,"\"\n");
			pushStderr (io);
		}

		// load next line (following the echo) 
		return loadLineOnDemand(io);
	}
	else return true;	// not an echo
}

/** Consumes a line along with anything (left) in it.
 */
bool consumeLine(const LpcIspIo *io) {
	return true;	// maybe check for trailing chars...
}

/** Reads one Int32 from a line and consumes the whole line, discarding anything else.
 */
bool findInt(const LpcIspIo *io, Int32 *result) {
	return	fifoParseInt(io->lpcInLine,result)
		&& consumeLine(io);
}

/** Reads one Uint32 from a line and consumes the whole line, discarding anything else.
 */
bool findUnsigned(const LpcIspIo *io, Uint32 *result) {
	return	fifoParseUnsigned(io->lpcInLine,result)
		&& consumeLine(io);
}

/** Reads the result code from a (zero return parameters) command.
 * Optionally the echo of LPC is skipped.
 * @param echo a pattern of the sent command.
 * @param result the ISP result code is put there
 * @return true if the response was successfully parsed, false otherwise.
 */
bool readResult(const LpcIspIo *io, const char* echo, Uint32 *result) {
	Fifo cloneBefore = * io->lpcInLine;
	if (getNonEchoAnswerLine(io,echo)
	&& findUnsigned(io,result)) {
		if (io->debugLevel>=LPC_ISP_DEBUG) {
			fifoPrintString (io->stderr, " - return code ");
			fifoPrintString (io->stderr, lpcIspErrorMessage(*result));
			fifoPrintLn (io->stderr);
			pushStderr (io);
		}
		if (*result==0) return true;
		else {
			if (io->debugLevel>=LPC_ISP_NORMAL) {
				fifoPrintString (io->stderr, "ERROR: ");
				fifoPrintString (io->stderr, lpcIspErrorMessage(*result));
				fifoPrintLn (io->stderr);
				pushStderr (io);
			}
			return false;
		}
	}
	else {
		if (io->debugLevel>=LPC_ISP_NORMAL) {
			fifoPrintString (io->stderr, "ERROR: could not read return code.\n");
			fifoPrintString (io->stderr, "Input fifo before line load was \"");
			fifoAppend (io->stderr, &cloneBefore);
			fifoPrintString (io->stderr, "\"\nInput fifo after line load was \"");
			fifoAppend (io->stderr, io->lpcInLine);
			fifoPrintString (io->stderr, "\"\n");
			pushStderr (io);
		}
		return false;
	}
}

/** Reads n integer values after the result code was removed.
 */
bool readUnsignedValues(const LpcIspIo *io, Uint32 *values, int n) {
	for (int i=0; i<n; i++) {
		if (loadLineOnDemand(io)
		&& findUnsigned(io,&values[i])) { // fine
			if (io->debugLevel>=LPC_ISP_DEBUG) {
				fifoPrintString (io->stderr, " - return value[");
				fifoPrintUint32 (io->stderr, i, 1);
				fifoPrintString (io->stderr, "]\n");
				pushStderr (io);
			}
		}
		else {
			if (io->debugLevel>=LPC_ISP_NORMAL) {
				fifoPrintString (io->stderr, "ERROR: could not read return value[");
				fifoPrintUint32 (io->stderr, i, 1);
				fifoPrintString (io->stderr, "]\n");
				pushStderr (io);
			}
			return false;
		}
	}
	return true;
}

/** Maximum detailed diagnostics.
 */
bool lpcSync(const LpcIspIo *io, int crystalHz) {

	if (fifoPrintString(io->lpcOut,"?\r\n")
	&& pushLpcOut(io)) {
		// at least sending seems to work...
	}
	else {
		if (io->debugLevel>=LPC_ISP_NORMAL) {
			fifoPrintString (io->stderr,"ERROR: Communication problem\n");
			pushStderr (io);
		}
		return false;
	}

	if (loadLineOnDemand(io)
	&& findStringInLineOrError(io,"Synchronized")) {
		if (io->debugLevel>=LPC_ISP_DEBUG) {
			fifoPrintString (io->stderr," - Accepted: Synchronized\n");
			pushStderr (io);
		}
	}
	else {
		if (io->debugLevel>=LPC_ISP_NORMAL) {
			fifoPrintString (io->stderr,"ERROR: No or invalid response\n");
			pushStderr (io);
		}
		return false;
	}

	if (!fifoPrintString (io->lpcOut,"Synchronized\r\n")
	|| !pushLpcOut (io)
	|| !loadLineOnDemand (io)) return false;

	// check, if we get something like "bla Synchronized bla\n"
	if (findStringInLineOrError (io,"Synchronized")) {
		if (io->debugLevel>=LPC_ISP_DEBUG) {
			fifoPrintString (io->stderr," - Accepted, but needs one more line with OK\n");
			pushStderr (io);
		}
	}
	else return false;

	if (loadLineOnDemand(io)
	&& findStringInLineOrError(io,"OK")) {
		if (io->debugLevel>=LPC_ISP_DEBUG) {
			fifoPrintString (io->stderr," - Accepted: Synchronized OK\n");
			pushStderr (io);
		}
	}
	else return false;

	if (crystalHz > 0) {
		const Uint32 param = (crystalHz+500)/1000;
		Uint32 result;
		if (fifoPrintUint32 (io->lpcOut,param,1)
		&& fifoPrintString (io->lpcOut,"\r\n")
		&& pushLpcOut (io)
		&& loadLineOnDemand (io)
		&& findUnsigned (io,&result)) {
			if (io->debugLevel>=LPC_ISP_DEBUG) {
				fifoPrintString (io->stderr," - crystal frequency reply.\n");
				pushStderr (io);
			}
		}
		else return false;

		if (result!=param) {
			if (io->debugLevel>=LPC_ISP_NORMAL) {
				fifoPrintString (io->stderr,"ERROR: echo frequency mismatch.\n");
				pushStderr (io);
			}
			return false;	// consume line containing crystal value
		}

		if (loadLineOnDemand (io)
		&& findStringInLineOrError (io,"OK")) {
			if (io->debugLevel>=LPC_ISP_DEBUG) {
				fifoPrintString (io->stderr," - Accepted\n");
				pushStderr (io);
			}
		}
		else return false;
	}

	return true;
}


bool lpcGo(const LpcIspIo *io, Uint32 pc, bool thumbMode) {
	if (lpcIspFlowControlHook!=0) {
		if (fifoPrintString (io->stdout, "G ")
		&& fifoPrintUint32 (io->stdout, pc, 1)
		&& fifoPrintString (io->stdout, thumbMode ? " T" : " A")
		&& fifoPrintString (io->stdout, "\r\n")
		&& lpcIspFlowControlHook (io,'G')) ;	// continue
		else {
			if (io->debugLevel>=LPC_ISP_NORMAL) {
				fifoPrintString (io->stderr, "User canceled command\n");
				pushStderr (io);
			}
			return false;
		}
	}

	Uint32 result;
	const char *pattern = "G ";
	if (fifoPrintString (io->lpcOut, pattern)
	&& fifoPrintUint32 (io->lpcOut, pc, 1)
	&& fifoPrintString (io->lpcOut, thumbMode ? " T" : " A")
	&& fifoPrintString (io->lpcOut, "\r\n")
	&& pushLpcOut (io)
	&& readResult (io,pattern,&result)
	&& result==LPC_ISP_CMD_SUCCESS)
		return true;
		else return errorMessage (io,"Code NOT launched\n");
}

bool lpcCommandWrite (const LpcIspIo *io, char command, const Uint32 *params, int nParams) {
	if (lpcIspFlowControlHook!=0) {
		bool success =	fifoPrintChar (io->stdout,command);
		for (int i=0; i<nParams; i++) {
			success = success
				&& fifoPrintChar (io->stdout,' ')
				&& fifoPrintUint32 (io->stdout,params[i],1);
		}
		if (success) {
			if (lpcIspFlowControlHook (io,command)) /* fine. Continue. */;
			else {
				if (io->debugLevel>=LPC_ISP_NORMAL) {
					fifoPrintString (io->stderr, "User canceled command\n");
					pushStderr (io);
					return false;
				}
			}
		}
		else {
			fifoPrintString (io->stderr, "INTERNAL ERROR #1\n");	// buffer overflown.
			pushStderr (io);
			return false;
		}
	}
	bool success = fifoPrintChar (io->lpcOut, command);
	for (int i=0; i<nParams; i++) {
		success = success
			&& fifoPrintChar (io->lpcOut,' ')
			&& fifoPrintUint32 (io->lpcOut,params[i],1);
	}

	return success && fifoPrintString (io->lpcOut,"\r\n") && pushLpcOut (io);

}

/** Removes an optional echo line, the return code and optionally result values.
 */
bool lpcCommandRead (const LpcIspIo *io, char command, Uint32 *result, Uint32 *results, int nResults) {
	const char pattern[2] = { command, 0 };
	*result = LPC_ISP_UNDEFINED;

	if (readResult (io, pattern, result)
	&& readUnsignedValues (io, results, nResults)) return true;
	else {
		fifoPrintString (io->stderr, "ERROR: ");
		fifoPrintString (io->stderr, lpcIspErrorMessage(*result));
		fifoPrintLn (io->stderr);
		pushStderr (io);
		return false;
	}
}

bool lpcCommand(const LpcIspIo *io, char command, const Uint32 *params, int nParams, Uint32 *results, int nResults) {
	Uint32 returnCode;
	return	lpcCommandWrite (io,command,params,nParams)
		&& lpcCommandRead (io,command,&returnCode,0,0)
		&& returnCode==LPC_ISP_CMD_SUCCESS		// need a successful execution
		&& readUnsignedValues (io, results,nResults);
}

bool lpcBaud(const LpcIspIo *io, int baud, int stopBits) {
	Uint32 params[2] = { baud, stopBits };
	return lpcCommand (io, 'B', params, 2, 0, 0);
}

// At least LPC17 answers A 0\r0\r\n on A 0\r\n. A \n is missing.
// Therefore we use only \r in this command.
// We turn echo off quickly to get rid of this shit.
bool lpcEcho(const LpcIspIo *io, bool on) {
	Uint32 params[1] = { on };
	return lpcCommand (io,'A',params,1,0,0);
}

bool lpcComReconfigure (const LpcIspIo *io, const LpcIspConfigCom *com) {
	return	lpcEcho (io, com->useEcho)
		&& lpcBaud (io, com->baud, com->stopBits);
}

bool lpcReadBootCodeVersion(const LpcIspIo *io, Uint32 *version) {
	Uint32 values[2];

	if (lpcCommand (io,'K', 0,0, values,2)) {
		*version = values[1]<<8 | values[0];
		return true;
	}
	else return false;
}

bool lpcReadPartId(const LpcIspIo *io, LpcMembers const *members, Uint32 partIds[LPC_IDS], int ids) {
	if (lpcCommand (io,'J', 0,0, partIds, 1)) {	// read one part ID first.
		int n = ids;
		if (n<=0) n = members->thePreferred ? lpcMatchNumberOfIds(members->thePreferred,partIds[0]) : 0;
		if (n<=0) n = lpcFindNumberOfIds(members->list,partIds[0]);
		for (int i=1; i<n; i++) {
			if (loadLineOnDemand (io)
			&& findUnsigned(io,&partIds[i])) {	// fine
				if (io->debugLevel>=LPC_ISP_INFO) {
					fifoPrintString (io->stderr, "got ID[");
					fifoPrintInt32 (io->stderr, i, 1);
					fifoPrintString (io->stderr, "]=0x");
					fifoPrintHex (io->stderr, partIds[i],8,8);
					fifoPrintString (io->stderr, "\n");
					pushStderr (io);
				}
			}
			else {
				if (io->debugLevel>=LPC_ISP_NORMAL) {
					fifoPrintString (io->stderr, "Failed to get ID[");
					fifoPrintInt32 (io->stderr,i,1);
					fifoPrintString (io->stderr, "\n");
					pushStderr (io);
				}
				return false;
			}
		}
		return true;
	}
	else return false;

}

//Reads out the device serial number. @param sn 4 32-bit integers.
bool lpcReadUid(const LpcIspIo *io, Uint32 uids[4]) {
	return lpcCommand (io, 'N', 0,0, uids, 4);
}

bool lpcCopyRamToFlash(const LpcIspIo *io, Uint32 addressFlash, Uint32 addressRam, int n) {
	Uint32 params[3] = { addressFlash, addressRam, n };
	return lpcCommand (io, 'C', params,3, 0,0);
}

bool lpcValidateSectors (const LpcIspIo *io, int sectorStart, int sectorEnd) {
	return
		sectorStart>>_SECTOR_BANK == sectorEnd>>_SECTOR_BANK	// same bank
		|| errorMessage (io, "sector range across bank boundaries\n");
}

// distinguish banked and non-banked commands from here on...

static int sectorToBankIdx (int sector) {
	const int bankP1 = sector >> _SECTOR_BANK;
	return bankP1 ? bankP1-BANK_A : 0;
}

bool lpcPrepareForWrite(const LpcIspIo *io, int sectorStart, int sectorEnd, bool banked) {
	if (lpcValidateSectors (io,sectorStart,sectorEnd)) {
		const Uint32 params[3] = {
			sectorStart & SECTOR_MASK,
			sectorEnd & SECTOR_MASK,
			sectorToBankIdx (sectorStart)
		};
		return lpcCommand (io,'P', params, banked?3:2, 0,0);
	}
	else return false;
}

bool lpcErase(const LpcIspIo *io, int sectorStart, int sectorEnd, bool banked) {
	if (lpcValidateSectors (io,sectorStart,sectorEnd)) {
		const Uint32 params[3] = {
			sectorStart & SECTOR_MASK,
			sectorEnd & SECTOR_MASK,
			sectorToBankIdx (sectorStart)
		};
		return lpcCommand (io,'E', params, banked?3:2, 0,0);
	}
	else return false;
}

bool lpcUnlock(const LpcIspIo *io) {
	const Uint32 params[1] = { 23130 };
	return lpcCommand (io,'U',params,1,0,0);
}

bool lpcCompare(const LpcIspIo *io, Uint32 addrA, Uint32 addrB, int n) {
	const Uint32 params[3] = { addrA, addrB, n };
	return lpcCommand (io,'M',params,3,0,0);
}

bool lpcReadCrc(const LpcIspIo *io, Uint32 addr, int n, Uint32 *crc) {
	const Uint32 params[2] = { addr, n };
	return lpcCommand (io,'S',params,2, crc,1);
}

bool lpcSetFlashBank(const LpcIspIo *io, int flashBank) {
	const Uint32 params[1] = { flashBank==BANK_Z ? 0 : flashBank-BANK_A };	// ISP uses 0 for A, 1 for B, ...
	return lpcCommand (io,'S', params,1, 0,0);
}

/** lpcBlankCheck will not end in CMD_SUCCESS in case of non-blank sector(s). That's why we can't use
 * lpcCommand.
 */
bool lpcBlankCheck(const LpcIspIo *io, int sectorStart, int sectorEnd, bool *blank, bool banked) {
	if (lpcValidateSectors (io,sectorStart,sectorEnd)) {
		const Uint32 params[3] = {
			sectorStart & SECTOR_MASK,
			sectorEnd & SECTOR_MASK,
			sectorToBankIdx (sectorStart)
		};
		Uint32 returnCode;

		Uint32 info[2];
		if (lpcCommandWrite (io,'I', params, banked?3:2)
		&& getNonEchoAnswerLine (io, "I")
		&& findUnsigned (io, &returnCode)) {
		//&& lpcCommandRead (io,'I',&returnCode,0,0)) {
			switch(returnCode) {
				case LPC_ISP_CMD_SUCCESS:
					*blank = true;
					return true;
				case LPC_ISP_SECTOR_NOT_BLANK:
					if (readUnsignedValues (io, info, 2)
					&& io->debugLevel>=LPC_ISP_INFO) {
						fifoPrintString (io->stderr, "Sector not blank at address/offset 0x");
						fifoPrintHex (io->stderr, info[0],8,8);
						fifoPrintString (io->stderr, "\r\n");
						pushStderr (io);
					}
					*blank = false;
					return true;
				default:
					fifoPrintString (io->stderr, "ERROR: blank check failed:");
					fifoPrintString (io->stderr, lpcIspErrorMessage(returnCode));
					fifoPrintLn (io->stderr);
					pushStderr (io);
					return false;
			}
		}
		else return false;
	}
	else return false;
}

bool fifoCompare (Fifo *a, Fifo *b) {
/*
	for (int i=0; i<fifoCanRead(a) && i<fifoCanRead(b); i++) {
		if (fifoLookAheadRelative(a,i)!=fifoLookAheadRelative(b,i)) return false;
	}
*/
	return true;	// :o)
}

bool lpcWrite (const LpcIspIo *io, const LpcIspConfigCom *com, Uint32 address, Fifo *data) {
	switch (com->ispProtocol) {
		case ISP_PROTOCOL_BINARY: return lpcWriteBinary (io,com,address,data);
		case ISP_PROTOCOL_UUENCODE: return lpcWriteUuencode (io,com,address,data);
		default: return errorMessage (io, "unknown ISP data protocol\n");
	}
}

/** Tested on: LPC812 Rev4C with and without echo.
 */
bool lpcWriteBinary(const LpcIspIo *io, const LpcIspConfigCom *com, Uint32 address, Fifo *data) {
	const Uint32 n = fifoCanRead(data);
	Uint32 params[2] = { address, n };

	if (lpcCommand (io, 'W', params, 2, 0,0)) {	// announce write

		// data per write command	[n]
		if (io->debugLevel>=LPC_ISP_PROGRESS) {
			fifoPrintString (io->stderr, "Download to RAM [");
			fifoPrintUint32 (io->stderr, n, 1);
			//if (io->debugLevel>=LPC_ISP_DEBUG) {
				fifoPrintString (io->stderr,"@0x");
				fifoPrintHex (io->stderr,address,8,8);
			//}
			pushStderr (io);
		}

		Fifo clone = *data;

		// WHAT ABOUT the LF after the CR of the echo line??
		// Send data in small blocks and receive answer
		// This is a primitive kind of flow control.
		if (!fifoPutFifo(io->lpcOut,data)) return errorMessage (io,"output buffer overflow\n");

		//if (!flowControlHook('w',"w (write block[%d]).",(int)fifoCanRead(&fifoOut))) return false;
		if (!pushLpcOut(io)) return errorMessage (io,"IO error.\n");

		//if (!flowControlHook('r',"W (write block, read answer block[%d]).",(int)cs)) return false;

		Fifo fifoReadBack;

		// There is a LF still in front of the data to receive, at least according to LPC800 UM.
		if (!loadN (io,1)) return false;	// remove that \n

		if (com->useEcho) {
			if (loadN(io,n)
			&& fifoCompare (&clone,&fifoReadBack)) {
				fifoSkipRead(io->lpcInLine,n);
			}
			else return errorMessage (io, "binary echo mismatch.\n");
		}
		progressMessage (io,"]\n");
		return true;
	}
	else return false;
}

/** Checked on LPC2136/01 -E and not -E.
 * Untested since change to Fifo.
 */
bool lpcWriteUuencode(const LpcIspIo *io, const LpcIspConfigCom *com, Uint32 address, Fifo *data) {
	const Uint32 n = fifoCanRead (data);
	Uint32 params[2] = { address, n };

	if (lpcCommand (io, 'W', params, 2, 0,0)) {	// announce write

		// data per write command	[n]
		if (io->debugLevel>=LPC_ISP_PROGRESS) {
			fifoPrintString (io->stderr, "Download to RAM [");
			fifoPrintUint32 (io->stderr, n, 1);
			fifoPrintString (io->stderr,"@0x");
			fifoPrintHex (io->stderr,address,8,8);
			pushStderr (io);
		}

		Uint32 checksum=0;
		for (int lineNo=0; fifoCanRead(data); lineNo++) {
			//if (!flowControlHook('w',"w (write uuencoded line).")) return false;
			if (fifoUuEncodeLine(io->lpcOut,data,&checksum) && pushLpcOut (io)) {
				if (com->useEcho) {
					if (!loadNextLine (io)) {
						return errorMessage (io,"Uuencoded echo line missing.\n");
					}
				}
				// else: all fine!
			}
			else return errorMessage (io, "Uuencode error\n");

			if ((lineNo%20)==19 || !fifoCanRead(data)) {	// time to send a checksum
				Uint32 checksumReturn;
				if (fifoPrintUint32 (io->lpcOut,checksum,1)
				&& fifoPrintString (io->lpcOut,"\r\n")
				&& pushLpcOut (io)
				&& (!com->useEcho || readUnsignedValues(io,&checksumReturn,1)
					&& checksum==checksumReturn)	// this checks communication only!
				&& loadLineOnDemand (io)
				&& findStringInLineOrError (io,"OK")) {
					progressMessage (io, ".");	// show progress, checksum confirmed
				}
				else {
					return errorMessage (io, "checksum invalid.\n");
				}

				checksum = 0;
			}
		}
		progressMessage (io,"]\n");
		return true;
	}
	else return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Read/Write functions

bool lpcRead (const LpcIspIo *io, const LpcIspConfigCom *com, Fifo *data, Uint32 address, Uint32 n) {
	switch (com->ispProtocol) {
		case ISP_PROTOCOL_BINARY: return lpcReadBinary (io,com,data,address,n);
		case ISP_PROTOCOL_UUENCODE: return lpcReadUuencode (io,com,data,address,n);
		default: return errorMessage (io, "unknown ISP data protocol\n");
	}
}

/** Reads in binary data.
 * @param io the communication channels.
 * @param address starting address to read
 * @param n the number of bytes to read.
 * @return true in case of success, false otherwise.
 */
bool lpcReadBinary(const LpcIspIo *io, const LpcIspConfigCom *com, Fifo *data, Uint32 address, Uint32 n) {
	const Uint32 params[2] = { address, n };

	if (lpcCommand (io,'R',params,2, 0,0)) {
		// :o) There's always a \n in the output stream
		// It seems, LPC800 always prefixes the binary data with a \n
		if (loadN (io,1)	// skip \n
		&& loadN (io, n)) {
			if (fifoPutFifo (data,io->lpcInLine)) ;	// OK
			else return errorMessage (io,"image Fifo overflown\n");

			progressMessage (io,".");
			return true;
		}
		else return false;
	}
	else return false;
}

bool lpcReadUuencode(const LpcIspIo *io, const LpcIspConfigCom *com, Fifo *data, Uint32 address, Uint32 n) {
	const Uint32 params[2] = { address, n };
	const Uint32 r0 = fifoCanRead (data);	// initial read position - data may not be empty!

	if (lpcCommand (io,'R',params,2, 0,0)) {

		Uint32 checksum=0;
		for (int lineNo=0; fifoCanRead (data)-r0 <n; lineNo++) {
			if (loadLineOnDemand (io)
			&& fifoUuDecodeLine (data,io->lpcInLine,&checksum)) ;	// fine
			else return errorMessage (io,"invalid/missing UUEncoded data\n");

			if ((lineNo % 20)==19 || fifoCanRead(data)-r0 ==n) {
				Uint32 checksumLpc = 0;
				if (readUnsignedValues (io,&checksumLpc,1)) {
					if (checksum==checksumLpc) {
						progressMessage (io,".");
						if (fifoPrintString (io->lpcOut, "OK\r\n")
						&& pushLpcOut (io)) {
							if (io->debugLevel>=LPC_ISP_INFO) {
								fifoPrintString (io->stderr," total ");
								fifoPrintInt32 (io->stderr, fifoCanRead(data)-r0,1);
								fifoPrintString (io->stderr," B - checksum OK\n");
								pushStderr (io);
							}
							// all fine.
						}
						else return errorMessage (io, "acknowledge problem\n");
					}
					else {
						fifoPrintString (io->stderr,"ERROR: checksum invalid. 0x");
						fifoPrintHex (io->stderr,fifoCanRead(data)-r0,8,8);
						fifoPrintString (io->stderr, "lpc:0x");
						fifoPrintHex (io->stderr,checksumLpc,8,8);
						fifoPrintString (io->stderr, "mxli:0x");
						fifoPrintHex (io->stderr,checksum,8,8);
						fifoPrintLn (io->stderr);
						pushStderr (io);
						return false;
					}
					checksum = 0;
				}
				else return errorMessage (io,"could not read checksum from lpc.\n");
			}
		}
		return true;
	}
	else return false;
}


/** Returns the number of buffers of a given size (and 4-byte alignment) that can be generated by dividing up the RAM
 * space of buffer into equally sized (smaller) chunks.
 * @param buffer the current available RAM
 * @param buffers the current number of RAM regions of buffer
 * @param chunkSize the block size of smaller buffers to be aligned into buffer.
 * @return the total number of resulting smaller buffers.
 */
int lpcIspBufferChunkCount (const LpcIspBuffer buffer[LPC_ISP_BUFFERS], int buffers, int chunkSize) {
	enum { align = 4, };

	int chunks = 0;
	for (int b=0; b<buffers; b++) {
		const Uint32 addressAligned = (buffer[b].address + align-1) & ~(align-1);
		const Uint32 sizeAligned = buffer[b].size - (addressAligned-buffer[b].address);	// alignment loss
		chunks += sizeAligned / chunkSize;
	}
	return chunks;
}

/** Partitions a buffer of multiple contiguous regions into smaller chunks. The chunks size must be a multiple of 4.
 * The chunks are 4-byte aligned.
 * @param chunks the destination array for the partitioning. Must be large enough.
 * @param buffer the current available RAM
 * @param buffers the current number of RAM regions of buffer
 * @param chunkSize the size of the smaller pieces.
 */
void lpcIspBufferChunkPartition (LpcIspBuffer *chunks, int chunkSize, const LpcIspBuffer buffer[LPC_ISP_BUFFERS], int buffers) {
	enum { align = 4, };

	int chunk = 0;
	for (int b=0; b<buffers; b++) {
		Uint32 addressAligned = (buffer[b].address + align-1) & ~(align-1);
		Uint32 sizeAligned = buffer[b].size - (addressAligned-buffer[b].address);	// alignment loss
		while (sizeAligned>=chunkSize) {
			chunks[chunk].address = addressAligned;
			chunks[chunk].size = chunkSize;
			addressAligned += chunkSize;
			sizeAligned -= chunkSize;
			chunk ++;
		}
	}
}

int lpcChooseTransferSize (const LpcMember *member, int chunkSize, int realSize) {
	if (realSize<chunkSize) for (int bs=0; bs<LPC_BLOCK_SIZES; bs++) {
		if (member->family->blockSizes[bs]>=realSize) return member->family->blockSizes[bs];
	}
	else return chunkSize;	// bigger pieces must be of chunk size

	return 0;	// cannot happen, if chunk size is one of the block sizes.
}

/** Calculates the checksum of the first vector table entries and writes the negation of it into the checksum
 * vector.
 */
bool lpcHandleChecksum (const LpcIspIo *io, const LpcMember *member, Uint32 absoluteAddress, Fifo *fifoChunk) {
	if (member->family->checksumVectors*4 <= fifoCanRead (fifoChunk)
	&& member->family->checksumVector*4 <= fifoCanRead (fifoChunk) ) {
		Fifo rewriter;
		Fifo clone = *fifoChunk;

		Uint32 sum = 0;
		Uint32 original = 0;
		for (int v=0; v<member->family->checksumVectors; v++) {
			Uint32 vector;
			fifoGetUint32 (&clone,&vector);
			if (v!=member->family->checksumVector) sum += vector;
			else original = vector;
		}
		fifoInitRewrite (&rewriter, fifoChunk);
		fifoSkipWrite (&rewriter, member->family->checksumVector*4);
		fifoPutInt32 (&rewriter, (Int32) - sum);

		if (io->debugLevel>=LPC_ISP_INFO) {
			fifoPrintString (io->stderr, "Fixed checksum @ 0x");
			fifoPrintHex (io->stderr, absoluteAddress+member->family->checksumVector*4, 8,8);
			fifoPrintString (io->stderr, " from 0x");
			fifoPrintHex (io->stderr, original, 8,8);
			fifoPrintString (io->stderr, " to 0x");
			fifoPrintHex (io->stderr, -sum, 8,8);
			fifoPrintLn (io->stderr);
			pushStderr (io);
		}
		return true;
	}
	else return errorMessage (io, "chunk too small for calculating vector table checksum\n");
}

/** Changes the code read protection word, depending of an allowed maximum CRP level and a desired CRP level.
 * @param image the FLASH image contents.
 * @param addressCrp the address of the word encoding the CRP level, typically 0x2FC.
 * @param address the destination base address of the image.
 * @param crpMax maximum (allowed) CRP level. Must be an exect match for negative levels.
 * @param crpDesired the desired CRP level. -1 if desired level is to be considered unset.
 */
//bool fifoLpcCrp(Fifo *image, Uint32 addressCrp, Uint32 address, int crpMax, int crpDesired) {

bool lpcHandleCrp (
	const LpcIspIo *io,
	const CrpSettings *crp,
	Uint32 offsetInBank,
	Fifo *fifoChunk) {

	if (crp->address&3) return errorMessage (io,"Alignment error of CRP word\n");
	if (crp->max>4 || crp->max<0) return errorMessage (io,"Maximum CRP level (-L) out of range. Allowed: 0..4\n");
	if (crp->desired>4 || crp->desired<-1) return errorMessage (io,"CRP level (-l) out of range. Allowed: 0..4\n");
	if (!lpcCrpLevelAllowed(crp->max,crp->desired)) return errorMessage (io,"CRP level (-l) not allowed\n");

	infoMessage (io,"INFO: Checking CRP\n");
	Fifo clone = *fifoChunk;
	if (fifoCanRead(&clone)>crp->address-offsetInBank) {	// don't worry: there is no partial write of the CRP word
		fifoSkipRead(&clone,crp->address-offsetInBank);
		Fifo writer;
		fifoInitRewrite(&writer,&clone);
		Uint32 crpWord = 0;
		if (!fifoGetUint32(&clone,&crpWord)) return errorMessage (io, "could not read CRP word\n");
		const int crpExe = lpcCrpCodeToLevel(crpWord);
		if (io->debugLevel>=LPC_ISP_DEBUG) {
			fifoPrintString (io->stderr, "CRP allowed =");
			fifoPrintInt32 (io->stderr, crp->max,1);
			fifoPrintString (io->stderr, "\nCRP desired =");
			fifoPrintInt32 (io->stderr, crp->desired,1);
			fifoPrintString (io->stderr, "\nCRP image =");
			fifoPrintInt32 (io->stderr, crpExe,1);
			fifoPrintLn (io->stderr);
			pushStderr (io);
		}
		if (crpExe>crp->max) {
			if (io->debugLevel>=LPC_ISP_PROGRESS) {
				fifoPrintString (io->stderr, "Downgrading image CRP level ");
				fifoPrintInt32 (io->stderr, crpExe, 1);
				fifoPrintLn (io->stderr);
				pushStderr (io);
			}
		}

		// checking formal conditions
		const int severeLevels[] = { 3,4 };
		for (int i=0; i<ELEMENTS(severeLevels); i++) {
			const int crpSev = severeLevels[i];
			if ((crp->desired==crpSev || crp->max==crpSev /*|| crpExe==crpSev*/)	// attempt for CRP3,4
			&& !(crp->desired==crpSev && crp->max==crpSev && crpExe==crpSev)) {	// requirement for CRP3,4 
				return errorMessage (io, "severe CRP level requested, but formal conditions unmet\n");
			}
		}

		// keep image's CRP level, if allowed
		if (crp->desired==-1 && crpExe>crp->max) {
			return errorMessage (io,"image CRP not allowed\n");
		}
		// setting crp level
		if (crp->desired!=-1	// forced CRP level
		&& crpExe!=crp->desired) {
			if (crpWord==~0u	// erased memory pattern
			|| crpExe>0) {		// any CRP level
				if (io->debugLevel>=LPC_ISP_NORMAL) {
					fifoPrintString (io->stderr, "WARNING: image location 0x");
					fifoPrintHex (io->stderr, crp->address, 3,8);
					fifoPrintString (io->stderr, "(CRP) modified.\n");
					pushStderr (io);
				}
				if (io->debugLevel>=LPC_ISP_PROGRESS) {
					fifoPrintString (io->stderr, "Forcing CRP level to ");
					fifoPrintInt32 (io->stderr, crp->desired,1);
					fifoPrintLn (io->stderr);
					pushStderr (io);
				}
				fifoPutInt32(&writer,lpcCrpLevelToCode(crp->desired));
			}
			else {
				errorMessage (io,"image CRP location must be blank (-1) or CRP level 1..4\n");
			}
		}

		return true;
	}
	else return errorMessage (io, "CRP not contained in this chunk\n");	// CRP not accessible in this chunk!
}

bool lpcFlash (
	const LpcIspIo *io,
	const LpcIspConfigCom *com,
	const LpcIspFlashOptions *options,
	const LpcMember *member,
	const Executable32Segment *segments
	) {

	// first calculate affected sectors
	const Uint32 nSegments = executable32Segments (segments);

	// try unlock. It's needed for every non-empty image.
	if (nSegments>0 && !lpcUnlock (io)) return false;

	Int32Pair sectorRanges [nSegments];
	for (int s=0; s<nSegments; s++) {
		sectorRanges[s].fst = lpcAddressToSector (member, segments[s].address);
		sectorRanges[s].snd = lpcAddressToSector (member, segments[s].address + segments[s].size-1);
		if (sectorRanges[s].fst==-1 || sectorRanges[s].snd==-1) {
			fifoPrintString (io->stderr, "ERROR: Address range not within FLASH: 0x");
			fifoPrintHex (io->stderr,segments[s].address,8,8);
			fifoPrintString (io->stderr, "..");
			fifoPrintHex (io->stderr,segments[s].address+segments[s].size-1,8,8);
			fifoPrintLn (io->stderr);
			pushStderr (io);
			return false;
		}
	}

	// then erase them, if requested
	if (options->eraseBeforeWrite) for (int s=0; s<nSegments; s++) {
		for (int sector=sectorRanges[s].fst; sector<=sectorRanges[s].snd; sector++) {
			bool blank;
			if (!options->eraseOnDemand	// erase always, not on demand
			|| lpcBlankCheck (io,sector,sector,&blank,options->banked) && !blank) {	// erase only if needed
				if (io->debugLevel>=LPC_ISP_PROGRESS) {
					fifoPrintString (io->stderr, "Erase sector ");
					fifoPrintSector (io->stderr, sector);
					fifoPrintString (io->stderr, " before write\n");
					pushStderr (io);
				}
				if (lpcPrepareForWrite (io,sector,sector,options->banked)
				&& lpcErase (io,sector,sector,options->banked)) ;	// fine
				else  {
					errorMessage (io,"erase on demand failed\n");
					return false;
				}
			}
			else ;	// already blank
		}
	}
	// else assume, they're already blanked
	

	// find RAM and block size
	LpcIspBuffer buffers [LPC_ISP_BUFFERS];
	const int nBuffers = lpcIspGetBuffers (member,buffers);

	if (io->debugLevel>=LPC_ISP_INFO) {
		fifoPrintString (io->stderr, "Transfer RAM buffers (unpartitioned):\n");
		for (int b=0; b<nBuffers; b++) {
			fifoPrintString (io->stderr,"  #");
			fifoPrintInt32 (io->stderr,b,2);
			fifoPrintString (io->stderr," @ 0x");
			fifoPrintHex (io->stderr,buffers[b].address,8,8);
			fifoPrintString (io->stderr,", size=");
			fifoPrintInt32 (io->stderr,buffers[b].size,1);
			fifoPrintLn (io->stderr);
			pushStderr (io);
		}
	}

	// Copy RAM-to-FLASH block sizes are always (as of 2014-03) smaller or equal to sectors sizes.

	// find a strategy to transfer the image to RAM
	// divide RAM into biggest possible transfer size chunks, such that no more 50% RAM is unusable.
	// Use these chunks to transfer data. Actual size may be reduced at the end of the segment (to better fit
	// the actual size).

	// Default: smallest chunk size. 
	int chunkSize = member->family->blockSizes[0];	// smallest size
	if (chunkSize==0) return errorMessage (io,"member->family->blockSizes[0] == 0  (lpcMemories.c invalid data)\n");
	const int maxRam = chunkSize * lpcIspBufferChunkCount (buffers,nBuffers,chunkSize);

	// now try better (bigger chunks at the expense of up to 50% total size) ...
	for (int bs=1; bs<LPC_BLOCK_SIZES && member->family->blockSizes[bs]!=0; bs++) {
		const int blockSize = member->family->blockSizes[bs];
		if (blockSize * lpcIspBufferChunkCount (buffers,nBuffers,blockSize) >= maxRam / 2) {
			chunkSize = blockSize;
		}
	}

	if (maxRam==0) {
		return errorMessage (io,"No transfer RAM found\n");
	}

	const int nChunks = lpcIspBufferChunkCount (buffers,nBuffers,chunkSize);
	LpcIspBuffer chunks [nChunks];
	Uint32 chunkFlashAddresses [nChunks];
	lpcIspBufferChunkPartition (chunks,chunkSize, buffers,nBuffers);

	if (io->debugLevel>=LPC_ISP_INFO) {
		fifoPrintString (io->stderr, "Transfer RAM chunks:\n");
		for (int c=0; c<nChunks; c++) {
			fifoPrintString (io->stderr,"  #");
			fifoPrintInt32 (io->stderr,c,4);
			fifoPrintString (io->stderr," @ 0x");
			fifoPrintHex (io->stderr,chunks[c].address,8,8);
			fifoPrintString (io->stderr,", size=");
			fifoPrintInt32 (io->stderr,chunks[c].size,1);
			fifoPrintLn (io->stderr);
			pushStderr (io);
		}
	}

	// copy RAM to FLASH.
	// we have different granularities: sectors, chunks, segments.
	// every sector needs a prepare for write before transfering RAM to FLASH.
	Executable32SegmentIterator iterator = { };	// all zeros

	char fifoBuffer [chunkSize];
	Fifo fifoChunk = { fifoBuffer, sizeof fifoBuffer, };

	bool finished = false;
	while (!finished) {
		int c = 0;
		for (; c<nChunks; c++) {
			const Executable32Segment segment = executable32NextChunk (segments,&iterator,chunkSize);
			if (segment.size>0) {
				chunkFlashAddresses [c] = segment.address;
				chunks [c].size = segment.size;
				//chunks [c].address = unchanged: RAM address
				// write chunk to LPC
				fifoReset (&fifoChunk);
				fifoWriteN (&fifoChunk, segment.data, segment.size);
				while (fifoCanRead(&fifoChunk)<chunkSize) fifoPrintChar(&fifoChunk,0xFF);		// padding

				const Uint32 bankAddress = lpcAddressToBankAddress(member, segment.address);
				const Uint32 offsetInBank = segment.address - bankAddress;

				// fix CRP, if neccessary.
				if (offsetInBank <= options->crpOffsetBank
				&& options->crpOffsetBank < offsetInBank + chunkSize) {
					const CrpSettings crp = {
						.address = options->crpOffsetBank,
						.max = options->crpAllow,
						.desired = options->crpDesired,
					};
					if (!lpcHandleCrp (io, &crp, offsetInBank, &fifoChunk)) return false;	// failed to fix CRP
				}

				// fix checksum, if neccessary
				if (offsetInBank==0 && member->family->checksumVectors>0) {
					if (lpcHandleChecksum (io, member, segment.address, &fifoChunk));	// all fine
					else return false;
				}

				if (!lpcWrite (io,com, chunks[c].address,&fifoChunk)) return false;
			}
			else {
				finished = c==0;
				break;
			}
		}

		// copy RAM-to-FLASH for all chunks
		for (int tc=0; tc<c; tc++) {
			const Uint32 flashAddress = chunkFlashAddresses[tc];
			const Uint32 ramAddress = chunks[tc].address;
			const Uint32 sector = lpcAddressToSector (member,flashAddress);
			// what will we do ? ...
			if (io->debugLevel >= LPC_ISP_PROGRESS) {
				fifoPrintString (io->stderr, "RAM 0x");
				fifoPrintHex (io->stderr,ramAddress,8,8);
				fifoPrintString (io->stderr," -> FLASH 0x");
				fifoPrintHex (io->stderr,flashAddress,8,8);
				fifoPrintLn (io->stderr);
				pushStderr (io);
			}
			// ...and do it:
			if (lpcPrepareForWrite (io, sector,sector,options->banked)
			&& lpcCopyRamToFlash (io, flashAddress ,ramAddress , chunkSize)) {
			}
			else return false;
		}
	}
	return true;
	
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// RTS/DTR wave form definitions.
//

bool lpcWavePlay (const LpcIspIo *io, const WaveConfiguration *conf, const Wave *wave, const char* prompt) {
	const char *cmds = wave->commands;

	while (*cmds!=WAVE_CMD_END) {
		switch(*cmds) {
		case WAVE_CMD_D0: io->setDtr (false); break;
		case WAVE_CMD_D1: io->setDtr (true); break;
		case WAVE_CMD_R0: io->setRts (false); break;
		case WAVE_CMD_R1: io->setRts (true); break;
		case WAVE_CMD_PAUSE_SHORT: io->sleepUs (conf->pauseShortUs); break;
		case WAVE_CMD_PAUSE_LONG: io->sleepUs (conf->pauseLongUs); break;
		case WAVE_CMD_PROMPT:
			fifoPrintString (io->stderr, prompt);
			pushStderr (io);
			lpcIspFlowControlHook (io, '?');
			break;
		default:
			errorMessage (io, "wavePlay() #1.\n");
		}
		cmds++;
	}
	return true;
}

/** Translates the input of the -W option into waves data structure.
 */
bool waveCompile (const LpcIspIo *io, WaveSet *waveSet, Fifo* input) {
	Fifo clone = *input;
	memset(waveSet->waves,0,sizeof waveSet->waves);
	int no=-1;
	int seq = 0;
	while (fifoCanRead(&clone)) {
		const char c = fifoRead(&clone);
		switch(c) {	// handle wave selection
			case 'i': no = WAVE_ISP;	seq = 0; continue;
			case 'x': no = WAVE_EXECUTE;	seq = 0; continue;
			case 'j': no = WAVE_JUMP;	seq = 0; continue;
			case 'e': no = WAVE_ERROR;	seq = 0; continue;
			case '=':
			case '.': continue;
		}

		// now comes a command (or error)

		// make sure, the wave number is selected already.
		if (no==-1) return errorMessage (io, "parsing wave definition: missing i,x,j or e.\n");

		// make sure there's space for the command
		if (seq<WAVE_CMDS_MAX) ; // fine: we can put in another and still not lose the final 0
		else return errorMessage (io,"too many commands in single waveform definition\n");

		// store the command
		switch(c) {
			case 'D': waveSet->waves[no].commands[seq++] = WAVE_CMD_D1; break;
			case 'd': waveSet->waves[no].commands[seq++] = WAVE_CMD_D0; break;
			case 'R': waveSet->waves[no].commands[seq++] = WAVE_CMD_R1; break;
			case 'r': waveSet->waves[no].commands[seq++] = WAVE_CMD_R0; break;
			case 'y': waveSet->waves[no].commands[seq++] = WAVE_CMD_PROMPT; break;
			case 'p': waveSet->waves[no].commands[seq++] = WAVE_CMD_PAUSE_SHORT; break;
			case 'P': waveSet->waves[no].commands[seq++] = WAVE_CMD_PAUSE_LONG; break;
			default:
				if (io->debugLevel>=LPC_ISP_NORMAL) {
					fifoPrintString (io->stderr, "ERROR: invalid character '");
					fifoPrintChar (io->stderr, c);
					fifoPrintString (io->stderr, "' in -W waveform definition.\n");
					pushStderr (io);
				}
				return false;
		}
	}

	fifoCopyReadPosition(input,&clone);
	return true;
}

int lpcSectorCoverage (Int32PairList *list, LpcMember const* member, Executable32Segment const* image) {
	Int32Pair latestRange = { -1, };
	Int32 ranges = 0;

	for (int s=0; image[s].size !=0; s++) {
		const Uint32Pair addressRange = {
			image[s].address,
			image[s].address + image[s].size -1
		};
		Int32Pair sectorRange;
		if (lpcAddressRangeToSectorRange (member, &sectorRange, &addressRange)) {
			if (latestRange.fst==-1) {
				latestRange = sectorRange;
				ranges = 1;
			}
			else {	// valid latestRange
				if (latestRange.snd!=sectorRange.fst) {	// disjoint range. Store previous one
					if (int32PairListAdd (list, &latestRange));		// all fine
					else return -1;	// list capacity exceeded

					latestRange.fst = sectorRange.fst;
					ranges ++;
				}
				latestRange.snd = sectorRange.snd;
			}
		}
		else ; 	// not a FLASH target. I don't consider it an error.
	}
	if (latestRange.fst != -1 && !int32PairListAdd (list, &latestRange)) return -1;
	else return ranges;
}

