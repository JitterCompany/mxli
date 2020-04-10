/*
  seqio.c 
  Copyright 2011-2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <seqio.h>
#include <fifoPrint.h>

void seqioInit(Seqio *seqio, SeqioElement* elements, int size) {
	seqio->elements = elements;
	seqio->size = (Uint16)size;
	seqioReset (seqio);
}


void seqioReset(Seqio *seqio) {
	seqio->position = 0;
	seqio->status = SEQIO_PREPARE;
}

/** Determines the free elements left.
 */
static int seqioCellsLeft(const Seqio *seqio) {
	return seqio->size / sizeof(SeqioElement) - seqio->position;
}

static int nextCode(const Seqio *seqio) {
	const int es = seqioCellsLeft(seqio);
	if (es>=2) {	// current AND next are required at least
		const int currentCode = seqio->elements[seqio->position];

		if (currentCode & SEQIO_P_BIT) // require 3 elements
			return es>=3 ? seqio->elements [seqio->position + 2] : SEQIO_END;
		else	return seqio->elements [seqio->position + 1];
	}
	else return SEQIO_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool seqioPrepareWriteEmpty (Seqio *seqio) {
	return seqioPrepareWrite (seqio,0,0,true);
}

bool seqioPrepareWrite(Seqio *seqio, int cs, int n, bool bound) {
	if (seqio->status==SEQIO_PREPARE && seqioCellsLeft(seqio)>=1) {
		seqio->elements[seqio->position++] =
			(bound?SEQIO_B_BIT:0)
			| SEQIO_W_BIT
			| (n&3)<<SEQIO_L_SHIFT
			| cs & 0xFFFFFF 
			;
		return true;
	}
	else return false;
}

bool seqioPrepareWriteBlock(Seqio *seqio, const void *data, int n, bool bound) {
	if (seqio->status==SEQIO_PREPARE && seqioCellsLeft(seqio)>=2) {
		seqio->elements[seqio->position++] = (bound?SEQIO_B_BIT:0) | SEQIO_WRITE_BUFFER | n & SEQIO_L_MASK;
		seqio->elements[seqio->position++] = (SeqioElement) data;
		return true;
	}
	else return false;
}

bool seqioPrepareReadEmpty (Seqio *seqio) {
	return seqioPrepareRead (seqio,0,true);
}

bool seqioPrepareRead(Seqio *seqio, int n, bool bound) {
	if (seqio->status==SEQIO_PREPARE && seqioCellsLeft(seqio)>=1) {
		seqio->elements[seqio->position++] = (bound?SEQIO_BOUND:0) | SEQIO_R_BIT | (n&3)<<24;
		return true;
	}
	else return false;
}

bool seqioPrepareReadBlock(Seqio *seqio, void *data, int n, bool bound) {
	if (seqio->status==SEQIO_PREPARE && seqioCellsLeft(seqio)>=2) {
		seqio->elements[seqio->position++] = (bound?SEQIO_BOUND:0) | SEQIO_READ_BUFFER | n & SEQIO_L_MASK;
		seqio->elements[seqio->position++] = (SeqioElement) data;
		return true;
	}
	else return false;
}

bool seqioPrepareEnd(Seqio *seqio) {
	if (seqio->status==SEQIO_PREPARE) seqio->status = SEQIO_ACTION;
	else return false;

	if (seqioCellsLeft(seqio)>=1) seqio->elements[seqio->position++] = SEQIO_END;

	seqio->position = 0;
	seqio->charPosition = 0;
	return true;
}

bool seqioPrepareReplay(Seqio *seqio) {
	if (seqio->status!=SEQIO_PREPARE) {
		seqio->position = 0;
		seqio->charPosition = 0;
		seqio->status = SEQIO_ACTION;
		return true;
	}
	else return false;
}

bool seqioIsDone(const Seqio *seqio) {
	switch(seqio->status) {
		case SEQIO_DONE:
		case SEQIO_DONE2:
		case SEQIO_ERROR:
		case SEQIO_ERROR2:	return true;
		default:		return false;
	}
}

bool seqioIsJustDone(Seqio *seqio) {
	switch(seqio->status) {
		case SEQIO_DONE:	seqio->status = SEQIO_DONE2; return true;
		case SEQIO_ERROR:	seqio->status = SEQIO_ERROR2; return true;
		default:		return false;
	}
}

bool seqioIsSuccessful(const Seqio *seqio) {
	switch(seqio->status) {
		case SEQIO_DONE:
		case SEQIO_DONE2:	return true;
		default:		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/* Action == SEQIO_WRITE or SEQIO_WRITE_BUFFER, BEGIN maybe, which corresponds to a SEQIO_R_BIT (read!).
 * Checks if more than 1 byte can be read in the current transfer.
 */
static bool seqioActionCanDoMoreThanOne(const Seqio *seqio) {
	const int code = seqio->elements[seqio->position];
	const int size = code & SEQIO_P_BIT ? code & SEQIO_L_MASK : code>>SEQIO_L_SHIFT & 3;
	const int nCode = nextCode(seqio);

	return	seqio->charPosition+1 < size				// current block has more bytes or...
		|| nCode!=SEQIO_END && (				// next exists
			!(nCode & SEQIO_BOUND) &&			// next has no explicit bound flag
			!((code^nCode) & (SEQIO_R_BIT | SEQIO_W_BIT))	// read/write does not change
		);
}

/** Changes the status to PENDING/DONE, depending on the remaining elements.
 * Used to adjust the status at the end of other (pointer advancing) functions.
 * @param safe true, if last byte read from Seqio is already processed, false, if we're unsure about this AND 
 *   seqioActionConfirm() is required to make sure.
 */
static void seqioActionCheckFinished(Seqio *seqio, bool safe) {
	if (seqioCellsLeft(seqio)>0 && seqio->elements[seqio->position]==SEQIO_END
	|| seqioCellsLeft(seqio)==0) {
		seqio->status = safe ? SEQIO_DONE : SEQIO_PENDING;
		seqio->position = 0;
		seqio->charPosition = 0;
	}
}

/** Returns true for the single element (!) empty transaction
 */
static bool seqioActionIsEmpty (const Seqio *seqio) {
	const int code = seqio->elements[seqio->position];
	const int size = code & SEQIO_P_BIT ? code & SEQIO_L_MASK : code>>SEQIO_L_SHIFT & 3;

	return size==0;				// current block has no byte at all.
}

SeqioAction seqioAction(const Seqio *seqio) {
	if (seqio->status==SEQIO_ACTION && seqioCellsLeft(seqio)>=1) {
		const bool empty	= seqioActionIsEmpty (seqio);
		const int code		= seqio->elements[seqio->position];

		// bounds occur at the beginning, at explicit bounds or at R/W changes
		const bool first	= (seqio->charPosition==0)
					&& (	seqio->position==0
						|| (code & SEQIO_BOUND)
						|| (seqio->elements[seqio->position-1] ^ code) & SEQIO_R_BIT
					) && !empty;		// exception: empty transaction.

		// last chars occur at the end
		const bool last = !seqioActionCanDoMoreThanOne(seqio)
				&& !empty;			// exception: empty transaction

		// BEWARE: R-bit -> write; W-bit -> read
		return	0
			| (code & SEQIO_R_BIT && !empty ? SEQIO_ACTION_WRITE : 0)
			| (code & SEQIO_W_BIT && !empty ? SEQIO_ACTION_READ : 0)
			| (code & SEQIO_R_BIT && empty ? SEQIO_ACTION_WRITE_EMPTY : 0)
			| (code & SEQIO_W_BIT && empty ? SEQIO_ACTION_READ_EMPTY : 0)
			| (first ? SEQIO_ACTION_FIRST_BYTE : 0)
			| (last ? SEQIO_ACTION_LAST_BYTE : 0)
			;
	}
	else return SEQIO_ACTION_END;	// SEQIO_PENDING yields this, too
}

Uint32 seqioBlockActionEstimate (const Seqio *seqio) {
	const int cells = seqioCellsLeft (seqio);
	if (cells!=2) return cells;
	else {	// 2 cells special case
		const int code = seqio->elements[seqio->position];
		return code & SEQIO_P_BIT ? 1 : 2;
	}
}

SeqioBlockAction seqioBlockAction (const Seqio *seqio) {
	// assertion: seqio->status==SEQIO_ACTION
	Uint32 estimate = seqioBlockActionEstimate (seqio);

	SeqioElement * word = & seqio->elements[seqio->position];
	const int code = *word;
	const bool empty = seqioActionIsEmpty (seqio);
	const bool first	= (seqio->charPosition==0)
				&& (	seqio->position==0
					|| (code & SEQIO_BOUND)
					|| (seqio->elements[seqio->position-1] ^ code) & SEQIO_R_BIT
				) && !empty;		// exception: empty transaction.
	const bool last = estimate < 2;

	SeqioBlockAction action = {
		.action = 0
			| (code & SEQIO_R_BIT && !empty ? SEQIO_ACTION_WRITE : 0)
			| (code & SEQIO_W_BIT && !empty ? SEQIO_ACTION_READ : 0)
			| (code & SEQIO_R_BIT && empty ? SEQIO_ACTION_WRITE_EMPTY : 0)
			| (code & SEQIO_W_BIT && empty ? SEQIO_ACTION_READ_EMPTY : 0)
			| (first ? SEQIO_ACTION_FIRST_BYTE : 0)
			| (last ? SEQIO_ACTION_LAST_BYTE : 0),
		{ .data = code & SEQIO_P_BIT ? (void*)word[1] : word },
		.n = code & SEQIO_P_BIT ? code & SEQIO_L_MASK : (code >> SEQIO_L_SHIFT) & 3
	};
	return action;
}

void seqioBlockActionConfirm (Seqio *seqio) {
	// assertion: seqio->status==SEQIO_ACTION
	seqio->position += seqio->elements[seqio->position] & SEQIO_P_BIT ? 2 : 1;
	seqioActionCheckFinished (seqio,true);
}

// Read bytes, previously written (W-bit)
char seqioActionRead(Seqio *seqio) {
	const int code = seqio->elements[seqio->position];
	if (code & SEQIO_P_BIT) {
		const char result = ((char*)seqio->elements[seqio->position + 1]) [ seqio->charPosition++ ];
		const int n = code & SEQIO_L_MASK;
		if (seqio->charPosition>=n) {
			seqio->position += 2;
			seqio->charPosition = 0;
			seqioActionCheckFinished(seqio,false);
		}
		return result;
	}
	else {	// immediate bytes
		// read one of the immediate bytes
		const char result = (code >> (8* seqio->charPosition++)) & 0xFF;
		const int n = code>>24 & 3;
		if (seqio->charPosition >= n) {
			seqio->position ++;
			seqio->charPosition = 0;
			seqioActionCheckFinished(seqio,false);
		}
		return result;
	}
}

// Write bytes, previously marked as readable (R-bit)
void seqioActionWrite(Seqio *seqio, char c) {
	const int code = seqio->elements[seqio->position];
	if (code & SEQIO_P_BIT) {
		( (char*) seqio->elements[seqio->position + 1] ) [ seqio->charPosition++ ] = c;
		const int n = code & SEQIO_L_MASK;
		if (seqio->charPosition >= n) {
			seqio->position += 2;
			seqio->charPosition = 0;
			seqioActionCheckFinished(seqio,true);
		}
	}
	else {	// write immediate bytes
		seqio->elements[seqio->position] = code & ~(0xFF<<8*seqio->charPosition) | c<<8*seqio->charPosition;
		seqio->charPosition++;
		const int n = code>>24 & 3;
		if (seqio->charPosition>=n) {
			seqio->position ++;
			seqio->charPosition = 0;
			seqioActionCheckFinished(seqio,true);
		}
	}
}

// Write bytes, previously marked as readable (R-bit)
void seqioActionWriteFake (Seqio *seqio) {
	const int code = seqio->elements[seqio->position];
	if (code & SEQIO_P_BIT) {
		//( (char*) seqio->elements[seqio->position + 1] ) [ seqio->charPosition++ ] = c;
		seqio->charPosition++;
		const int n = code & SEQIO_L_MASK;
		if (seqio->charPosition >= n) {
			seqio->position += 2;
			seqio->charPosition = 0;
			seqioActionCheckFinished(seqio,true);
		}
	}
	else {	// write immediate bytes
		//seqio->elements[seqio->position] = code & ~(0xFF<<8*seqio->charPosition) | c<<8*seqio->charPosition;
		seqio->charPosition++;
		const int n = code>>24 & 3;
		if (seqio->charPosition>=n) {
			seqio->position ++;
			seqio->charPosition = 0;
			seqioActionCheckFinished(seqio,true);
		}
	}
}

static void seqioActionEmpty (Seqio *seqio) {
	seqio->position++;
	seqioActionCheckFinished (seqio,true);
}

void seqioActionReadEmpty (Seqio *seqio) {
	seqioActionEmpty (seqio);
}

void seqioActionWriteEmpty (Seqio *seqio) {
	seqioActionEmpty (seqio);
}

void seqioActionError(Seqio *seqio) {
	seqio->status = SEQIO_ERROR;
}

void seqioActionConfirm(Seqio *seqio) {
	if (seqio->status==SEQIO_PENDING) seqio->status = SEQIO_DONE;
}

bool seqioCanRead(Seqio *seqio) {
	switch(seqio->status) {
		case SEQIO_DONE:
		case SEQIO_DONE2:
			while (seqioCellsLeft(seqio)>0 && (seqio->elements[seqio->position] & SEQIO_R_BIT) ==0) {
				seqio->position += seqio->elements[seqio->position] & SEQIO_P_BIT ? 2 : 1;
			}
			return seqioCellsLeft(seqio)!=0;
		default: return false;
	}
}

char seqioRead(Seqio *seqio) {
	const int code = seqio->elements[seqio->position];
	if (code & SEQIO_P_BIT) {
		const char result = ((char*)seqio->elements[seqio->position + 1]) [ seqio->charPosition++ ];
		const int n = code & SEQIO_L_MASK;
		if (seqio->charPosition>=n) {
			seqio->position += 2;
			seqio->charPosition = 0;
		}
		return result;
	}
	else {
		// read one of the immediate bytes
		const char result = (code >> (8*seqio->charPosition ++)) & 0xFF;
		const int n = code>>24 & 3;
		if (seqio->charPosition>=n) {
			seqio->position ++;
			seqio->charPosition = 0;
		}
		return result;
	}
}

bool fifoPrintSeqioStatus (Fifo *fifo, int status) {
	const char* msg = "INVALID";
	switch(status) {
	case SEQIO_PREPARE:	msg = "PREPARE"; break;
	case SEQIO_ACTION:	msg = "ACTION"; break;
	case SEQIO_PENDING:	msg = "PENDING"; break;
	case SEQIO_DONE:	msg = "DONE"; break;
	case SEQIO_DONE2:	msg = "DONE2"; break;
	case SEQIO_ERROR:	msg = "ERROR"; break;
	case SEQIO_ERROR2:	msg = "ERROR2"; break;
	default: ;
	}
	return fifoPrintString (fifo,"SEQIO_") && fifoPrintString (fifo,msg);
}

bool fifoPrintSeqio (Fifo *fifo, const Seqio *_seqio) {
	Seqio seqio = *_seqio;
	bool success	= fifoPrintString (fifo,"Seqio{status=") && fifoPrintSeqioStatus (fifo,seqio.status)
			&& fifoPrintString (fifo,"position=") && fifoPrintInt (fifo,seqio.position)
			&& fifoPrintString (fifo,"charPosition=") && fifoPrintInt (fifo,seqio.charPosition)
			&& fifoPrintChar (fifo,':');
	if (seqio.status==SEQIO_ACTION) for (bool run = true; run; ) {
		int action = seqioAction (&seqio);
		if (action & SEQIO_ACTION_READ_EMPTY) {
			success = success && fifoPrintString (fifo,"W[]");
			seqioActionReadEmpty (&seqio);
		}
		if (action & SEQIO_ACTION_WRITE_EMPTY) {
			success = success && fifoPrintString (fifo,"R[]");
			seqioActionWriteEmpty (&seqio);
		}
		if (action & SEQIO_ACTION_FIRST_BYTE) success = success && fifoPrintString (fifo,"[");
		if (action & SEQIO_ACTION_READ) {
			int c = seqioActionRead (&seqio);
			success = success && fifoPrintString (fifo,"W0x") && fifoPrintHex (fifo,c,2,2);
			seqioActionConfirm (&seqio);
		}
		if (action & SEQIO_ACTION_WRITE) {
			seqioActionWriteFake (&seqio);
			success = success && fifoPrintChar (fifo,'R');
		}
		if (action & (SEQIO_ACTION_WRITE|SEQIO_ACTION_READ)
		&& 0==(action &SEQIO_ACTION_LAST_BYTE)) fifoPrintChar (fifo,',');
		if (action & SEQIO_ACTION_LAST_BYTE) fifoPrintString(fifo,"]");
		if (action & SEQIO_ACTION_END) {
			success = success && fifoPrintChar (fifo,'}');
			run = false;
		}
	}
	return success;
}

