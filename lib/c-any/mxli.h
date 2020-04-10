/** @file
 * @brief mxli LPC ISP functions
 */

#ifndef module__mxli_h
#define module__mxli_h

#include <stdbool.h>
#include <integers.h>
#include <fifo.h>
#include <lpcMemories.h>
#include <lpcIsp.h>
#include <executable32.h>
#include <int32PairList.h>

enum {
	LPC_ISP_SILENT =-1,		///< don't show errors.
	LPC_ISP_NORMAL,			///< show errors and explicit output.
	LPC_ISP_PROGRESS,		///< show progress bars.
	LPC_ISP_INFO,			///< some more implementation details
	LPC_ISP_DEBUG,			///< most detailed output: dump.
} LpcIspDebugLevel;

/** Communication channel to/from LPC: fifo based + push/pull functions. The Fifos will be used across multiple
 * function calls. Two principles are used in mxli: 1) Most responses are text line responses and these can be read
 * without danger of blocking if reading until (including) the CR or LF. 2) binary data can be read without danger
 * of blocking, because the size is known in advance. That way, mxli NEVER uses polling.
 * Some ISP handlers seem to forget some CR or LF, so a simple heuristic is used: every CR or LF indicates another
 * line, if it is NOT following a different line separator LF or CR. LF LF is two newlines, CR LF is not. In order
 * to avoid blocking, the first CR or LF is taken as a newline and reading stops, the second MAY be dropped on
 * loading the next line.
 */
struct LpcIspIo;
typedef struct LpcIspIo LpcIspIo;

struct LpcIspIo {
	Fifo	*lpcIn;				///< data received from LPC
	Fifo	*lpcInLine;			///< data received and parsed into one line.
	Fifo	*lpcOut;			///< data to be transmitted to LPC
	Fifo	*stdin;				///< user input
	Fifo	*stdout;			///< regular user/file output
	Fifo	*stderr;			///< error/warning/info messages
	bool	(*pullLpcIn)(const LpcIspIo*);	///< function for reading bytes into the input fifo.
	bool	(*pushLpcOut)(const LpcIspIo*);	///< function for flushing contents of fifo to output
	bool	(*pullStdin)(const LpcIspIo*);
	bool	(*pushStdout)(const LpcIspIo*);
	bool	(*pushStderr)(const LpcIspIo*);
	bool	(*setDtr)(bool level);		///< serial DTR signal, used for /RESET (active low, typically)
	bool	(*setRts)(bool level);		///< serial RTS signal, used for /BOOT (active low, typically)
	void	(*sleepUs)(Int32 us);		///< busy delay for generating pulse widths.

	char	debugLevel;
};

struct Patch {
	char	lineChar;			///< state variable required to patch broken ISP CR LF sequences.
};
extern struct Patch patch;

/** Communication parameters, including MCU/boot loader/board specific settings.
 */
typedef struct {
	Uint32	crystalHz;
	Uint32	baud;
	Uint32	resetUs;		///< how long to apply /RESET (and /BOOT)
	Uint32	bootUs;			///< how long to wait after de-asserting /RESET
	Uint32	timeoutUs;		///< serial timeout
	Uint8	stopBits;
	Uint8	ispProtocol;		///< binary or uuencoded ?
	bool	useEcho;
} LpcIspConfigCom;

/** Calculates the approx. time required to send/receive a given number of bytes with the given communication
 * parameters.
 * @param com the serial port settings
 * @param nChars the number of characters to send/receive
 * @return the minimum transmission time (us) required without waiting.
 */
Uint32 lpcIspTransmissionTimeUs (const LpcIspConfigCom *com, Uint32 nChars);

/** mxli-style of member selection: one potential override/user-defined and the precompiled ones.
 */
typedef struct {
	const LpcMember		*thePreferred;	///< one preferred member, if != 0
	const LpcMember *const	*list;		///< a list of other members, if != 0 
} LpcMembers;

/** Set this to a function to intercept control flow. Default (0) is do nothing.
 * @param io the communication channels
 * @param code typically the ISP command letter for the command to intercept.
 * @return true for normal program flow, false, if flow should be stopped (user canceled command).
 */
extern bool (*lpcIspFlowControlHook)(const LpcIspIo *io, char code);

/** Calculates the sectors affected by a FLASH image.
 * @param list an empty list to hold the sector ranges (fst..snd).
 * @param member the LPC family member descriptor
 * @param image the executable to load into FLASH
 * @return the number of (non-empty) sector ranges. -1 if list was too small in capacity.
 */
int lpcSectorCoverage (Int32PairList *list, LpcMember const* member, Executable32Segment const* image);

bool lpcSync(const LpcIspIo *io, int crystalHz);

/** Re-sets the ISP communication parameters(after successful hand-shake).
 * @param io the communication channels
 * @param baud the baud rate to use after successful command execution
 * @param stopBits the number of stop bits (1 or 2) to use.
 * @return true in case of successful execution, false if communication problem.
 */
bool lpcBaud(const LpcIspIo *io, int baud, int stopBits);

/** At least LPC17 answers A 0 CR 0 CR LF on A 0 CR LF. A LF is missing.
 * Therefore we use only CR in this command.
 * We turn echo off quickly to get rid of this shit.
 */
bool lpcEcho(const LpcIspIo *io, bool on);

/** Reconfigures serial settings after a successful hand-shake with the LPC handler. This can be used to increase
 * speed after detection.
 * @param io the communication channels
 * @param com the serial port settings
 * @return true, if successful, false otherwise (and communication must be considered lost, then).
 */
bool lpcComReconfigure (const LpcIspIo *io, const LpcIspConfigCom *com);

/** Reads the LPC boot loader version number.
 * @param io the communication channels
 * @param version the version with minor number in the lower 8bits and the major number in the upper
 *   24 bits.
 * @return true on successful execution, false otherwise.
 */
bool lpcReadBootCodeVersion(const LpcIspIo *io, Uint32 *version);

/** Reads a device's part ID(s). Some devices have more than 1 ID and the number of IDs is derived from the value of
 * the first ID. This requires the member info.
 * @param io the communication channels.
 * @param members the list of supported devices.
 * @param partIds the result array.
 * @param ids if !=0 then this is the exact number of IDs to read. Set to 0 for probing.
 * @return true on successful execution, false otherwise.
 */
bool lpcReadPartId(const LpcIspIo *io, LpcMembers const *members, Uint32 partIds[LPC_IDS], int ids);

/** Reads out the device's unique identification number.
 * @param io the communication channels.
 * @param uid 4 32-bit integers.
 * @return true on successful execution, false otherwise.
 */
bool lpcReadUid(const LpcIspIo *io, Uint32 uid[4]);

/** Starts downloaded code.
 * @param io the communication channels.
 * @param pc the entry point.
 * @param thumbMode true, if execution should start in THUMB mode, false for ARM mode.
 */
bool lpcGo(const LpcIspIo *io, Uint32 pc, bool thumbMode);

bool lpcWriteBinary (const LpcIspIo *io, const LpcIspConfigCom *com, Uint32 address, Fifo *data);
bool lpcWriteUuencode (const LpcIspIo *io, const LpcIspConfigCom *com, Uint32 address, Fifo *data);
// convenience function
bool lpcWrite (const LpcIspIo *io, const LpcIspConfigCom *com, Uint32 address, Fifo *data);

bool lpcReadBinary (const LpcIspIo *io, const LpcIspConfigCom *com, Fifo *data, Uint32 address, Uint32 n);
bool lpcReadUuencode (const LpcIspIo *io, const LpcIspConfigCom *com, Fifo *data, Uint32 address, Uint32 n);
bool lpcRead (const LpcIspIo *io, const LpcIspConfigCom *com, Fifo *data, Uint32 address, Uint32 n);

bool lpcCopyRamToFlash (const LpcIspIo *io, Uint32 addressFlash, Uint32 addressRam, int n);
bool lpcValidateSectors (const LpcIspIo *io, int sectorStart, int sectorEnd);
bool lpcPrepareForWrite (const LpcIspIo *io, int sectorStart, int sectorEnd, bool banked);
bool lpcErase (const LpcIspIo *io, int sectorStart, int sectorEnd, bool banked);
bool lpcUnlock (const LpcIspIo *io);
bool lpcCompare (const LpcIspIo *io, Uint32 addrA, Uint32 addrB, int n);
bool lpcReadCrc (const LpcIspIo *io, Uint32 addr, int n, Uint32 *crc);
bool lpcSetFlashBank (const LpcIspIo *io, int flashBank);

/** lpcBlankCheck will not end in CMD_SUCCESS in case of non-blank sector(s). That's why we can't use
 * lpcCommand.
 */
bool lpcBlankCheck (const LpcIspIo *io, int sectorStart, int sectorEnd, bool *blank, bool banked);

/** Tries to get at least on more byte from the LPC. No blocking unless timeout.
 * @param io The communication channels
 * @return true, if at least one byte was available, false otherwise (timeout).
 */
inline static bool pullLpcIn (const LpcIspIo *io) {
	return io->pullLpcIn (io);
}

inline static bool pushLpcOut (const LpcIspIo *io) {
	return io->pushLpcOut (io);
}

inline static bool pullStdin (const LpcIspIo *io) {
	return io->pullStdin (io);
}

inline static bool pushStdout (const LpcIspIo *io) {
	return io->pushStdout (io);
}

inline static bool pushStderr (const LpcIspIo *io) {
	return io->pushStderr (io);
}

inline static void debugOut(const LpcIspIo *io, const char *msg) {
	fifoPrintString (io->stderr,msg);
	pushStderr (io);
}

/** Always returns false, but outputs the message only, if not in silent mode.
 */
bool errorMessage (const LpcIspIo *io, const char *msg);

void progressMessage(const LpcIspIo *io, const char *msg);

/** Always returns true, but outputs the message only, if in debug mode.
 */
void debugMessage (const LpcIspIo *io, const char *msg);

typedef struct {
	Uint32	address;	///< offset of CRP word in FLASH bank
	int	max;		///< maximum allowed CRP level
	int	desired;	///< explicitelt requested CRP level, -1 for 'unset'.
} CrpSettings;

bool lpcHandleCrp (
	const LpcIspIo *io,
	const CrpSettings *crp,
	Uint32 address,
	Fifo *fifoChunk);

bool lpcHandleChecksum (const LpcIspIo *io, const LpcMember *member, Uint32 address, Fifo *fifoChunk);

typedef struct {
	bool	eraseBeforeWrite;	///< erase destination sectors before writing
	bool	eraseOnDemand;		///< blank check before erase.
	Int8	crpAllow;		///< maximum allowed CRP level
	Int8	crpDesired;		///< desired CRP level
	Uint32	crpOffsetBank;		///< location of the CRP word
	bool	banked;			///< use banked commands?
} LpcIspFlashOptions;

typedef struct {
	Uint32		size;		///< size==0 indicates end of list.
	Uint32		address;
	const Uint32*	contents;	///< word-aligned data
} LpcImageSegment;

bool lpcFlash (
	const LpcIspIo *io,
	const LpcIspConfigCom *com,
	const LpcIspFlashOptions *options,
	const LpcMember *member,
	const Executable32Segment *segments
	);

////////////////////////////////////////////////////////////////////////////////////////////////////
// RTS/DTR wave form definitions.
//
enum WaveId {
	WAVE_ISP,		///< wave form to get into ISP handler
	WAVE_EXECUTE,		///< wave form to RESET and start FLASH program
	WAVE_JUMP,		///< wave form to be able to start FLASH program by a jump
	WAVE_ERROR,		///< wave form to keep RESET on device
	WAVE_COUNT		///< number of wave forms
};

enum WaveCommand {
	WAVE_CMD_END,
	WAVE_CMD_D0,
	WAVE_CMD_D1,
	WAVE_CMD_R0,
	WAVE_CMD_R1,
	WAVE_CMD_PAUSE_SHORT,
	WAVE_CMD_PAUSE_LONG,
	WAVE_CMD_PROMPT,		///< using the flow control hook

	WAVE_CMDS_MAX = 10,
};

/** One single sequence of signals and delays.
 */
typedef struct {
	char commands [WAVE_CMDS_MAX+1];	// always terminating 0 required
} Wave;

/** A definition of all available waveforms.
 */
typedef struct {
	Wave waves [WAVE_COUNT];
} WaveSet;

typedef struct {
	int	pauseShortUs;
	int	pauseLongUs;
} WaveConfiguration;

/** Translates the input of the -W option of mxli into waves data structure.
 * @param io The communication channels
 * @param waveSet the destination buffer for the result.
 * @param input a Fifo containing the (cryptic) wave definitions.
 * @return true in case of success.
 */
bool waveCompile (const LpcIspIo *io, WaveSet *waveSet, Fifo* input);

/** Generates the signals on the serial channel as described in wave.
 * @param io the communication channels
 * @param conf the signal timing configuration
 * @param wave the signal definition
 * @param prompt a prompt string to the user, in case the signal definition includes a user confirmation.
 * @return true if played successfully, false if the user canceled at the prompt or an error occurred.
 */
bool lpcWavePlay (const LpcIspIo *io, const WaveConfiguration *conf, const Wave *wave, const char *prompt);

#endif

