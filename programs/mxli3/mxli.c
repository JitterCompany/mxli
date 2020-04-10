/** @file
 * @brief mxli LPC ISP functions
 */
#include <mxli.h>
#include <c-linux/hexFile.h>
#include <hexImage.h>

#define MXLI_VERSION "3.3 ($Rev: 175 $)"
// New in 3.3: Raspberry Pi GPIO support


////////////////////////////////////////////////////////////////////////////////////////////////////
// Adaption Linux <-> Fifo

#include <c-linux/serial.h>

#include <time.h>
#include <unistd.h>
#include <stdlib.h>		// getenv()
#include <fixedPoint.h>
#include <ansi.h>
#include <fifoPrintFixedPoint.h>

static void adapterSleepUs (Int32 us) {
	if (us<1000*1000) usleep((int)us);
	else {
		struct timespec ts = {
			.tv_sec = us / 1000000,
			.tv_nsec = (us % 1000000) * 1000
		};
		nanosleep(&ts,0);
	}
}

static int fdLpc = -1;

bool adapterPullIn (Fifo *fifo, int fd) {
	char c;
	if (fifoCanWrite(fifo) && 1==read(fd,&c,1)) {
		fifoWrite(fifo,c);
		return true;
	}
	else return false;
}

bool adapterPushOut (Fifo *fifo, int fd) {
	while (fifoCanRead(fifo)) {
		const char c = fifoRead(fifo);
		if (1!=write(fd,&c,1)) return false;
	}
	return true;
}

bool adapterPullLpcIn	(const LpcIspIo *io)	{ return adapterPullIn (io->lpcIn,fdLpc);	}
bool adapterPushLpcOut	(const LpcIspIo *io)	{
	if (io->debugLevel>=LPC_ISP_DEBUG) {
		Fifo clone = *io->lpcOut;
		//fifoPrintString (io->stderr,RED);
		fifoDumpFifoAscii (io->stderr,&clone);
		//fifoPrintString (io->stderr,NORMAL);
	}
	return adapterPushOut (io->lpcOut,fdLpc);
}

bool adapterPullStdin	(const LpcIspIo *io)	{ return adapterPullIn (io->stdin,0);	}
bool adapterPushStdout	(const LpcIspIo *io)	{ return adapterPushOut (io->stdout,1);	}
bool adapterPushStderr	(const LpcIspIo *io)	{ return adapterPushOut (io->stderr,2);	}

enum {	STDOUT_BUFFERSIZE=4096,
};

static char
	bufferLpcIn	[4096],
	bufferLpcOut	[4096],
	bufferStdin	[64],
	bufferStdout	[STDOUT_BUFFERSIZE],
	bufferStderr	[4096];

static Fifo
	fifoLpcIn	= { bufferLpcIn,	sizeof bufferLpcIn,	},
	fifoLpcInLine	= { },
	fifoLpcOut	= { bufferLpcOut,	sizeof bufferLpcOut,	},
	fifoStdin	= { bufferStdin,	sizeof bufferStdin,	},
	fifoStdout	= { bufferStdout,	sizeof bufferStdout,	},
	fifoStderr	= { bufferStderr,	sizeof bufferStderr,	};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Command-line handling.

#include <fifoParse.h>
#include <fifoPopt.h>

// Results...

static bool
	echoEnable			= false,
	commandShowDeviceList		= false,
	commandShowMemberInfo		= false,
	commandShowMemberInfoCmdLine	= false,	// output command-line for defining this member
	commandShowBootCodeVersion	= false,
	commandShowUcName		= false,
	commandShowUid			= false,
	commandProbe			= false,
	commandWrite			= false,
	commandExecuteByReset		= false,
	commandMxliVersion		= false,
	commandNoIo			= false,	// terminate before opening communication device
	raspiGpio			= false,	// false: RTS/DTR, true:GPIOs of Raspi
	virginMode			= false,	// do not use compiled-in table
	quickMode			= false		// prefer speed
	;

static Int32
	debugLevel			= LPC_ISP_NORMAL,
	baudRate			= 115200,
	crystalHz			= 12*MEGA,
	overrideFlashSize		= -1,
	commandJumpAddress		= -1,
	commandJumpAddressSymbol	= -1,
	lockLevelRequested		= -1,
	readByteCount			= -1,
	commandSetActiveFlashBank	= -1,	// 0=Z, 1=A, 2=B, ...
	bootupTimeMs			= 300,	// default, according to man page
	overrideDestinationFlashBank	= BANK_A,	// default according to man page
//	deviceDefinitionCrpOffset	= -1,
	lockLevelAllowed		= 0,
	deviceDefinitionProtocol	= -1,
	serialTimeoutMs			= 500,	// default, according to man page
	resetTimeMs			= 100,	// time needed for a /RST to take effect (short delay)
	raspiGpioBoot			= 18,	// port pin used for /BOOT
	raspiGpioReset			= 17	// port pin used for /RESET
	;

enum {
	MXLI_WRITE_SEGMENTS	= 4,	// max 4 segments
};

static Uint32
	bufferDestinationAddresses	[MXLI_WRITE_SEGMENTS+1],
	bufferFlashBankAddresses	[LPC_BANKS+1],
	bufferBlockSizes		[LPC_BLOCK_SIZES+1],
	bufferOverrideRamSizes		[LPC_RAMS+1],
	overrideCrpAddress		=0x2FC			// just for mxli testing/debugging purpose
	;
	
static Uint32List
	listDestinationAddresses	= { bufferDestinationAddresses, sizeof bufferDestinationAddresses, },
	listFlashBankAddresses		= { bufferFlashBankAddresses, sizeof bufferFlashBankAddresses, },
	listBlockSizes			= { bufferBlockSizes, sizeof bufferBlockSizes, },
	listOverrideRamSizes		= { bufferOverrideRamSizes, sizeof bufferOverrideRamSizes, }
	;

static Int32Pair
	commandEraseSectorRange		= { -1, },	// undefined
	commandEraseFlashBankRange	= { -1, },	// undefined
	checksumCountAndIndex		= { -1,	},	// undefined; vector table checksum calculation range and result index
	bufferSectorSizeAndCount	[LPC_SECTOR_ARRAYS+1],
	bufferRamSizeAndAddress		[LPC_RAMS+1],
	bufferIspRamSizeAndAddress	[LPC_ISP_RAMS+1],
	bufferIds			[LPC_IDS+1]	// value/mask pairs
	;

static Int32PairList
	listSectorSizeAndCount		= { bufferSectorSizeAndCount, sizeof bufferSectorSizeAndCount, },
	listRamSizeAndAddress		= { bufferRamSizeAndAddress, sizeof bufferRamSizeAndAddress, },
	listIspRamSizeAndAddress	= { bufferIspRamSizeAndAddress, sizeof bufferIspRamSizeAndAddress, },
	listIds				= { bufferIds, sizeof bufferIds, }
	;

static Fifo
	fifoComDevice			= {},
	fifoUseUcName			= {},	// this value is 'undefined', which is different from 'empty'!
	fifoDeviceDefinitionName	= {},
	fifoWaveDefinition		= {};

static char bufferImageFiles	[400];
static Fifoq
	fifoqImageFiles			= { bufferImageFiles, sizeof bufferImageFiles, };

// Raspberry pi GPIO
#include <c-linux/raspiGpio.h>

bool adapterSetRts (bool level)	{
	return raspiGpio ? raspiLazyGpioWrite (raspiGpioBoot, level) : serialSetRts (fdLpc,level);
}

bool adapterSetDtr (bool level)	{
	return raspiGpio ? raspiLazyGpioWrite (raspiGpioReset, level) : serialSetDtr (fdLpc,level);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

LpcIspIo lpcIspIo = {
	.lpcIn		= &fifoLpcIn,
	.lpcInLine	= &fifoLpcInLine,
	.lpcOut		= &fifoLpcOut,
	.stdin		= &fifoStdin,
	.stdout		= &fifoStdout,
	.stderr		= &fifoStderr,
	.pullLpcIn	= &adapterPullLpcIn,
	.pushLpcOut	= &adapterPushLpcOut,
	.pullStdin	= &adapterPullStdin,
	.pushStdout	= &adapterPushStdout,
	.pushStderr	= &adapterPushStderr,
	.setRts		= &adapterSetRts,
	.setDtr		= &adapterSetDtr,
	.sleepUs	= &adapterSleepUs,

	//.debugLevel	= LPC_ISP_NORMAL,
	//.debugLevel	= LPC_ISP_DEBUG,
	//.debugLevel	= LPC_ISP_PROGRESS,
};
static WaveSet waveSet = {
	.waves = {
		{	.commands = {	// enter ISP: /RST with /BOOT=0
				WAVE_CMD_D0, WAVE_CMD_R0, WAVE_CMD_PAUSE_SHORT, WAVE_CMD_D1, WAVE_CMD_PAUSE_LONG,
				WAVE_CMD_R1, WAVE_CMD_END,
			}
		},{	.commands = {	// execute: /RST (DTR) with /BOOT=1 (RTS)
				WAVE_CMD_D0, WAVE_CMD_R1, WAVE_CMD_PAUSE_SHORT, WAVE_CMD_D1, WAVE_CMD_PAUSE_LONG,
				WAVE_CMD_R1, WAVE_CMD_END,
			}
		},{	.commands = { 	// jump: do nothing, except not assert /RST or /BOOT
				WAVE_CMD_D1, WAVE_CMD_R1, WAVE_CMD_PAUSE_SHORT, WAVE_CMD_END,
			}
		},{	.commands = {	// error: keep uC in /RST
				WAVE_CMD_D0, WAVE_CMD_R0, WAVE_CMD_PAUSE_SHORT, WAVE_CMD_END,
			}
		}
	}
};

////////////////////////////////////////////////////////////////////////////////

/** Parses the symbols Z, A, B, ... into the numbers 0, 1, 2, ...
 */
bool fifoParseFlashBank (Fifo *fifo, Int32* bank) {
	enum { min = 'A', max = 'Y', zero = 'Z' };
	if (fifoParseExactChar (fifo, zero)) {
		*bank = BANK_Z;
		return true;
	}
	else if (fifoCanRead (fifo)) {
		const char c = fifoLookAhead(fifo);
		if (min<=c && c<=max) {
			fifoRead (fifo);
			*bank = c-min + BANK_A;
			return true;
		}
		else return false;
	}
	else return false;
}

/** Parses a sector number (int) optionally prefixed by a flash bank indicator into the corresponding sector number.
 */
bool fifoParseSector (Fifo *fifo, Int32* sector) {
	Fifo clone = *fifo;
	Int32 flashBank = BANK_Z;
	fifoParseFlashBank (&clone, &flashBank);
	if (fifoParseIntCStyle (&clone, sector)) {
		*sector |= flashBank << _SECTOR_BANK;
		fifoCopyReadPosition (fifo,&clone);
		return true;
	}
	else return false;
}

static FifoPoptBool optionBools[] = {
	{ .shortOption = 'i',			.value = &commandShowMemberInfo,	},
	{ .shortOption = 'k',			.value = &commandShowBootCodeVersion,	},
	{ .shortOption = 'n',			.value = &commandShowUcName,		},
	{ .shortOption = 'p',			.value = &commandProbe,			},
	{ .shortOption = 'q',			.value = &quickMode,			},
	{ .shortOption = 'w',			.value = &commandWrite,			},
	{ .shortOption = 'x',			.value = &commandExecuteByReset,	},
	{ .shortOption = 'E',			.value = &echoEnable,			},
	{ .shortOption = 'Q',			.value = &commandNoIo,			},
	{ .longOption = "uid",			.value = &commandShowUid,		},
	{ .longOption = "deviceDefinition",	.value = &commandShowMemberInfoCmdLine,	},
	{ .longOption = "deviceList",		.value = &commandShowDeviceList,	},
	{ .longOption = "raspi-gpio", 		.value = &raspiGpio,			},
	{ .longOption = "version",		.value = &commandMxliVersion,		},
	{ .longOption = "virgin", 		.value = &virginMode,			},
	{}	// EOL
};

static FifoPoptInt32Flag optionInt32Flags[] = {
	{ .shortOption='g',	.value = &debugLevel,	.indicator = LPC_ISP_DEBUG,	},
	{ .shortOption='v',	.value = &debugLevel,	.indicator = LPC_ISP_PROGRESS,	},
	{ .shortOption='V',	.value = &debugLevel,	.indicator = LPC_ISP_INFO,	},
	{}	// EOL
};

static FifoPoptInt32 optionInt32s[] = {
	{ .shortOption = 'b',	.value = &baudRate,			.parseInt = &fifoParseIntEng,		},
	{ .shortOption = 'c',	.value = &crystalHz,			.parseInt = &fifoParseIntEng,		},
	{ .shortOption = 'f',	.value = &overrideFlashSize,		.parseInt = &fifoParseIntEng,		},
	{ .shortOption = 'l',	.value = &lockLevelRequested,							},
	{ .shortOption = 'r',	.value = &readByteCount,		.parseInt = &fifoParseIntEng,		},
	{ .shortOption = 's',	.value = &commandSetActiveFlashBank,	.parseInt = &fifoParseFlashBank,	},
	{ .shortOption = 't',	.value = &bootupTimeMs,			.parseInt = &fifoParseIntEng,		},
	{ .shortOption = 'y',	.value = &overrideDestinationFlashBank,	.parseInt = &fifoParseFlashBank,	},
//	{ .shortOption = 'C',	.value = &deviceDefinitionCrpOffset,						},
	{ .shortOption = 'G',	.value = &debugLevel,								},
	{ .shortOption = 'L',	.value = &lockLevelAllowed,							},
	{ .shortOption = 'T',	.value = &serialTimeoutMs,		.parseInt = &fifoParseIntEng,		},
	{ .longOption = "crpAddress", .value = (Int32*)&overrideCrpAddress, .parseInt = &fifoParseIntEng,	},
	{ .longOption = "raspi-boot", .value = &raspiGpioBoot,							},
	{ .longOption = "raspi-reset", .value = &raspiGpioReset,						},
	{}	// EOL
};

static FifoPoptInt32Tuple optionInt32Tuples[] = {
	{ .shortOption = 'S',	.values = (Int32*) &checksumCountAndIndex, .n = 2, .separator = "@", .parseInt = &fifoParseIntEng, },
	{}	// EOL
};

static FifoPoptInt32Range optionInt32Ranges[] = {
	{ .shortOption = 'e',	.value = &commandEraseSectorRange, 	.parseInt = fifoParseSector,	.separator = "..",	},
	{ .shortOption = 'e',	.value = &commandEraseFlashBankRange, 	.parseInt = fifoParseFlashBank,	.separator = "..",	},
	{}	// EOL
};

static FifoPoptInt32List optionInt32Lists[] = {
	{	.shortOption = 'a',		.value = (Int32List*)&listDestinationAddresses,
		.parseInt = &fifoParseIntEng,	.separator = ','	
	},
	{ 	.shortOption = 'm',		.value = (Int32List*)&listOverrideRamSizes,
		.parseInt = &fifoParseIntEng,	.separator = ','
	},
	{	.shortOption = 'A',		.value = (Int32List*)&listFlashBankAddresses,
		.parseInt = &fifoParseIntEng,	.separator = ','	
	},
	{	.shortOption = 'B',		.value = (Int32List*)&listBlockSizes,
		.parseInt = &fifoParseIntEng,	.separator = ','	
	},
	{}	// EOL
};

static FifoPoptInt32PairList optionInt32PairLists[] = {
	{	.shortOption = 'F',	.value = &listSectorSizeAndCount,
		.parseIntFst = &fifoParseIntEng, .separatorPair = 'x', .parseIntSnd = &fifoParseIntEng,	.separatorList = ',',
	},
	{	.shortOption = 'M',	.value = &listRamSizeAndAddress,
		.parseIntFst = &fifoParseIntEng, .separatorPair = '@', .parseIntSnd = &fifoParseIntEng, .separatorList = ',',
	},
	{	.shortOption = 'R',	.value = &listIspRamSizeAndAddress,
		.parseIntFst = &fifoParseIntEng, .separatorPair = '@', .parseIntSnd = &fifoParseIntEng, .separatorList = ',',
	},
	{}	// EOL
};

static FifoPoptInt32PairListWithInt32Parameter optionInt32PairWithInt32Parameters[] = {
	{	.shortOption = 'I',	.value = &listIds,
		.parsePair = &fifoParseInt32WithDontCares,	.parameter = 'X',	.separator = ',',
	},
	{}
};

static const char* const symbolsJump[] = { "LMA", "LMA+1", 0 };
static const char* const symbolsProtocol[] = { "UUENCODE", "BINARY", 0 };

static FifoPoptSymbol optionSymbols[] = {
	{ .shortOption = 'j',	.value = &commandJumpAddressSymbol, .symbols = symbolsJump, },
	{ .shortOption = 'P',	.value = &deviceDefinitionProtocol, .symbols = symbolsProtocol, },
	{}	// EOL
};

static FifoPoptString optionStrings[] = {
	{	.shortOption = 'd',	.value = &fifoComDevice,		},
	{	.shortOption = 'u',	.value = &fifoUseUcName,		},
	{	.shortOption = 'N',	.value = &fifoDeviceDefinitionName,	},
	{	.shortOption = 'W',	.value = &fifoWaveDefinition,		},
	{}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char* argv[]) {
	LpcIspIo *io = &lpcIspIo;

	char lineBuffer[1024];
	Fifo fifoqCmdLine = { lineBuffer, sizeof lineBuffer };

	const char * const environmentVariable = "MXLI_PARAMETERS";
	const char * const environmentParameters = getenv (environmentVariable);
	if (environmentParameters!=0) {
		const int n = strlen (environmentParameters);
		ReadFifo fifoE = { (char*)environmentParameters, .size = n, .wTotal = n, };
		Fifo word;
		while (fifoParseUntil(&fifoE,&word," ")		// parse space-separated word
		|| (fifoParseStringNonEmpty(&fifoE,&word)) ) {	// or last word = rest of string
			if (fifoPutFifo(&fifoqCmdLine,&word)
			&& fifoqPrintNext(&fifoqCmdLine)) {
				// fine
			}
			else {
				errorMessage (io, "too many environment variable parameters for mxli\n");
				goto failEarly;
			}
		}
	}

	if (!fifoPoptAccumulateCommandLine(&fifoqCmdLine,argc,argv)) {
		errorMessage (io, "command line too long.\n");
		goto failEarly;
	}

	if (fifoPoptScanOption(&fifoqCmdLine,'?',"help")) {
		fifoPrintString (io->stderr,
			"Short usage: mxli [-b baud] [-d comDevice] [-c crystal] [-a imageOffset] [-y bank] [--raspi-gpio] [..] image.bin\n"
			"mxli has a man page - look at this for (much) more options and functionality\n"
		);
		pushStderr (io);
		goto returnEarly;
	}

	bool minus = false;
	while (fifoCanRead(&fifoqCmdLine)) {
		if (false
		|| fifoPoptBool					(&fifoqCmdLine, optionBools,&minus)
		|| fifoPoptInt32Flag				(&fifoqCmdLine, optionInt32Flags,&minus)
		|| fifoPoptInt32				(&fifoqCmdLine, optionInt32s,&minus)
		|| fifoPoptInt32Tuple				(&fifoqCmdLine, optionInt32Tuples,&minus)
		|| fifoPoptInt32Range				(&fifoqCmdLine, optionInt32Ranges,&minus)
		|| fifoPoptInt32List				(&fifoqCmdLine, optionInt32Lists,&minus)
		|| fifoPoptInt32PairList			(&fifoqCmdLine, optionInt32PairLists,&minus)
		|| fifoPoptInt32PairListWithInt32Parameter	(&fifoqCmdLine, optionInt32PairWithInt32Parameters,&minus)
		|| fifoPoptSymbol				(&fifoqCmdLine, optionSymbols,&minus)
		|| fifoPoptString				(&fifoqCmdLine, optionStrings,&minus)
		|| fifoPoptNonOptionAccumulate			(&fifoqCmdLine, &fifoqImageFiles ,&minus)
		) {
			// fine
		}
		else {
			if (io->debugLevel>=LPC_ISP_NORMAL) {
				fifoPrintString (io->stderr, "ERROR: invalid option/parameter: \"");
				fifoPrintString (io->stderr, fifoReadPositionToString (&fifoqCmdLine));
				fifoPrintString (io->stderr, "\"\n");
				fifoPrintString (io->stderr, "RTFM ;-)\n");
				pushStderr (io);
			}
			goto failEarly;
		}
	}

	if (commandMxliVersion) {
		fifoPrintString (io->stdout, "mxli-" MXLI_VERSION "\n");
		pushStdout (io);
	}

	io->debugLevel = debugLevel;

	// we could not output that earlier, because -g was not active!
	if (environmentParameters!=0
	&& io->debugLevel>=LPC_ISP_DEBUG) {
		fifoPrintString (io->stderr, "Using parameters of ");
		fifoPrintString (io->stderr, environmentVariable);
		fifoPrintChar (io->stderr, '=');
		fifoPrintString (io->stderr, environmentParameters);
		fifoPrintLn (io->stderr);
		pushStderr (io);
	}

	// procedure:
	// early parsing - before opening the connection to LPC
	// waveform parsing
	// communication parameters for sync
	// manual device definition
	// manual device selection
	// probing parameters
	// extended communication params: echo / UUENCODE/BINARY / speed
	// identify device
	// manual modification using overrides
	//
	// device information output
		// calculate sectors to erase
		// decide between single/multi flashbank
	// read UID
	// read operation
	// erase operation
	// write operation
	// verify

	// procedure starts here:

	// early parsing - before opening the connection to LPC
	// waveform parsing
	if (fifoIsValid (&fifoWaveDefinition)) {
		if (waveCompile (io,&waveSet,&fifoWaveDefinition)) ;	// fine
		else {
			errorMessage (io,"invalid waveform definition\n");
			goto failEarly;
		}
	}

	// communication parameters for sync
	LpcIspConfigCom com = {
		.crystalHz = crystalHz,
		.baud = baudRate,
		.resetUs = resetTimeMs * KILO,
		.bootUs = bootupTimeMs * KILO,
		.timeoutUs = serialTimeoutMs * KILO,
		.stopBits = 1,
		.ispProtocol = ISP_PROTOCOL_UUENCODE,
		//.useEcho = true,	// limits to 9600bd on LPC800, otherwise 115200
		.useEcho = echoEnable,
	};

	LpcFamily lpcFamily = {};
	LpcIspFamily lpcIspFamily = {};
	LpcMember lpcMember = {	// undefined := .name == 0
		.family =&lpcFamily,
		.ispFamily = &lpcIspFamily,
	};

	// manual device definition
	if (false
	|| 0 != uint32ListLength (&listFlashBankAddresses)		// -A
	|| 0 != uint32ListLength (&listBlockSizes)			// -B
//	|| deviceDefinitionCrpOffset != -1				// -C
	|| 0 != int32PairListLength (&listSectorSizeAndCount)		// -F
	|| 0 != int32PairListLength (&listIds)				// -I
	|| 0 != int32PairListLength (&listIspRamSizeAndAddress)		// -M
	|| fifoIsValid (&fifoDeviceDefinitionName)			// -N
	|| deviceDefinitionProtocol != -1				// -P
	|| 0 != int32PairListLength (&listIspRamSizeAndAddress)		// -R
	|| checksumCountAndIndex.fst != -1				// -S
	) {
		bool success = true;	// we check all options and report errors and bail out at the end, if something is missing.
	
		// if one of these options is specified, then all important (required) options must be specified.
		// Defaults (not required options): -A 0, -C 0x2FC,  -P UUENCODE

		// -A
		const int banks = uint32ListLength (&listFlashBankAddresses);	// :o) design error: empty list same as unspecified.
		if (banks>0) {
			for (int b=0; b<banks; b++) lpcFamily.addressFlashs [b] = listFlashBankAddresses.elements [b];
			lpcFamily.banks = banks;
		}
		else {	// default
			lpcFamily.banks = 1;
			// lpcFamily.addressFlashs [0] = 0;	// already set by blanking variable
		}

		// -B
		const int blockSizes = uint32ListLength (&listBlockSizes);
		if (blockSizes>0) {
			for (int b=0; b<blockSizes; b++) lpcFamily.blockSizes [b] = listBlockSizes.elements [b];
		}
		else {	// default
			lpcFamily.blockSizes [0] = 1024;	// according to manual page.
			// success = errorMessage (io,"missing option -B\n");
		}

		// -C
		// if (deviceDefinitionCrpOffset == -1) deviceDefinitionCrpOffset = 0x2FC;	// use default if unspecified

		// -F
		const int sectorGroups = int32PairListLength (&listSectorSizeAndCount);
		if (sectorGroups>0) {
			for (int g=0; g<sectorGroups; g++) {
				if (listSectorSizeAndCount.elements[g].fst & 1023) success = errorMessage (io, "invalid sector size\n");
				lpcFamily.sectorArrays[g].sizeK	= listSectorSizeAndCount.elements[g].fst / 1024;
				lpcFamily.sectorArrays[g].n	= listSectorSizeAndCount.elements[g].snd;
			}
			lpcMember.sizeFlashK = lpcFamilyBankSize (&lpcFamily) / 1024;	// assume full size
		}
		else success = errorMessage (io,"missing option -F\n");
		
		// -I
		const int ids = int32PairListLength (&listIds);
		if (ids>0) for (int i=0; i<ids; i++) {
			lpcMember.ids[i]	= listIds.elements[i].fst;
			lpcFamily.idMasks[i]	= listIds.elements[i].snd;
		}
		else success = errorMessage (io,"missing option -I\n");

		// -M
		const int rams = int32PairListLength (&listRamSizeAndAddress);
		if (rams>0) for (int r=0; r<rams; r++) {
			const int size = listRamSizeAndAddress.elements[r].fst;
			const Uint32 address = (Uint32) listRamSizeAndAddress.elements[r].snd;
			if (size&1023) success = errorMessage (io,"invalid ram size (!= n*1kiB) for -M\n");
			if (address&3) success = errorMessage (io,"invalid RAM alignment (!=4) for -M\n");
			lpcMember.sizeRamKs[r] = (Uint16) (size/1024);
			lpcIspFamily.addressRams[r] = address;
		}
		else success = errorMessage (io,"missing option -M\n");

		// -N
		if (fifoIsValid (&fifoDeviceDefinitionName)) {
			lpcMember.name = fifoReadLinear (&fifoDeviceDefinitionName);	// pray, that the fifo lives long enough :o)
		}
		else success = errorMessage (io,"missing option -N\n");

		// -P (optional)
		if (deviceDefinitionProtocol!=-1) lpcIspFamily.protocol = deviceDefinitionProtocol;
		else lpcIspFamily.protocol = ISP_PROTOCOL_UUENCODE;	// default according to manual page.

		// -R
		const int ispRams = int32PairListLength (&listIspRamSizeAndAddress);
		if (ispRams>0) for (int i=0; i<ispRams; i++) {
			const Int32 size = listIspRamSizeAndAddress.elements[i].fst;
			const Uint32 address = (Uint32) listIspRamSizeAndAddress.elements[i].snd;
			lpcIspFamily.ramUsage[i].address = address;
			lpcIspFamily.ramUsage[i].size = size;
		}
		else {	// default: worst case for RAM0
			lpcIspFamily.ramUsage [0].address = lpcIspFamily.addressRams [0];
			lpcIspFamily.ramUsage [0].size = 0x270;		// LPC800 has this (worst) value.
			lpcIspFamily.ramUsage [1].address = lpcIspFamily.addressRams [0];
			lpcIspFamily.ramUsage [1].size = - (256 + 32);	// most LPCs use that much stack and top 32 bytes.
			// success = errorMessage (io,"missing option -R\n");
		}

		// -S (optional)
		if (checksumCountAndIndex.fst != -1) {
			lpcFamily.checksumVectors = checksumCountAndIndex.fst;
			lpcFamily.checksumVector = checksumCountAndIndex.snd;
		}
		else {	// default to Cortex-M
			lpcFamily.checksumVectors = 8;		// manual page default
			lpcFamily.checksumVector = 7;		// manual page default
		}

		if (!success) goto failEarly;
	}

	// manual device selection => don't detect.
	// identify device
	const LpcMembers members = {
		.thePreferred = lpcMember.name!=0 ? &lpcMember : 0,	// command-line
		.list = virginMode ? 0 : lpcMembersXxxx,	// compiled-in
	};
	// show device list, including command-line defined device

	if (commandShowDeviceList) {
		if (members.thePreferred!=0) fifoPrintStringLn (io->stdout, members.thePreferred->name);
		for (int m=0; members.list!=0 && members.list[m]!=0; m++) fifoPrintStringLn (io->stdout, members.list[m]->name);
		pushStdout (io);
	}

	const LpcMember *selectedMember = 0;

	if (fifoIsValid (&fifoUseUcName)) {	// use device named on command line
		if (0!=members.thePreferred
		&& lpcMatchByName (members.thePreferred, fifoReadLinear (&fifoUseUcName))) selectedMember = members.thePreferred;
		else if (0!=(selectedMember = lpcFindByName (members.list, fifoReadLinear (&fifoUseUcName))) ) ;	// fine
		else {
			errorMessage (io, "controller not found by name\n");
			goto failClose;
		}
	}

	const WaveConfiguration waveConfiguration = {
		.pauseShortUs = resetTimeMs * 1000,
		.pauseLongUs = bootupTimeMs * 1000,
	};

	if (!commandNoIo) {
		// open device
		fdLpc = serialOpenBlockingTimeout (
			fifoIsValid (&fifoComDevice) ? fifoReadLinear (&fifoComDevice) : "/dev/ttyUSB0",
			com.baud,
			com.timeoutUs/(100*1000)
		);
		if (fdLpc<0) {
			errorMessage (io, "cannot open serial device\n");
			goto failEarly;
		}

		if (lpcWavePlay (io,&waveConfiguration, & waveSet.waves[WAVE_ISP], "Enter ISP")
		&& lpcSync (io, crystalHz)	// fine
		&& lpcComReconfigure (io,&com) );
		else goto failClose;
	}

	// probing parameters
	if (commandProbe) {
		errorMessage (io, "probing not implemented, yet\n");
	}

	if (!fifoIsValid (&fifoUseUcName)) {	// use device detection
		Uint32 partIds [LPC_IDS];
		if (lpcReadPartId (io, &members, partIds, 0)) {
			if (io->debugLevel>=LPC_ISP_DEBUG) {
				fifoPrintString (io->stderr, CYAN "Part ID(s): ");
				for (int i=0; i<LPC_IDS; i++) {
					if (i!=0) fifoPrintString (io->stderr, ", ");
					fifoPrintString (io->stderr, "0x");
					fifoPrintHex (io->stderr, partIds[i], 8,8);
				}
				fifoPrintString (io->stderr, "\n" NORMAL);
				pushStderr (io);
			}
		}
		else goto failClose;

		if (0!=members.thePreferred
		&& lpcMatchByIds (members.thePreferred, partIds)) selectedMember = members.thePreferred;
		else if (0!=(selectedMember = lpcFindByIds (members.list, partIds))) ; // fine
		else {
			errorMessage (io, "controller not found by IDs\n");
			goto failClose;
		}
	}

	// selectedMember is the description to use from now on
	//
	if (selectedMember==0) {
		errorMessage (io, "controller not identified\n");
		goto failClose;
	}

	// modify 'selectedMember'
	// copy selectedMember back to local variables to be able to modify it.
	//
	if (&lpcMember!=selectedMember) {	// selectedMember is from ROM table
		memcpy (&lpcMember, selectedMember, sizeof lpcMember);
		// the following are not modified by overrides...:
		//memcpy (&lpcIspFamily, selectedMember->ispFamily, sizeof lpcIspFamily);
		//memcpy (&lpcFamily, selectedMember->family, sizeof lpcFamily);
		//lpcMember.family = &lpcFamily;
		//lpcMember.ispFamily = &lpcIspFamily;
	}

	if (commandShowUcName) {
		fifoPrintString (io->stdout,selectedMember->name);
		fifoPrintLn (io->stdout);
		pushStdout (io);
	}

	
	// extended communication params: echo / UUENCODE/BINARY / speed
	com.ispProtocol = lpcMember.ispFamily->protocol;

	// manual modification using overrides
	//
	if (overrideFlashSize!=-1) {
		lpcMember.sizeFlashK = overrideFlashSize / 1024;
	}

	if (0!=uint32ListLength (&listOverrideRamSizes)) {
		for (int s=0; s<LPC_RAMS; s++) lpcMember.sizeRamKs[s] = 0;	// clear all RAMS
		for (int s=0; s<uint32ListLength (&listOverrideRamSizes) /* && s<LPC_RAMS */; s++) {
			lpcMember.sizeRamKs[s] = uint32ListAt (&listOverrideRamSizes,s) / 1024;
		}
	}

	selectedMember = &lpcMember;
	// selectedMember is read-only beyond this line !!

	// device information output
	if (commandShowMemberInfo) {
		fifoPrintLpcMember (io->stdout,selectedMember);
		pushStdout (io);
	}

	if (commandShowMemberInfoCmdLine) {
		fifoPrintString (io->stdout,"-N'");
		fifoPrintString (io->stdout, selectedMember->name);
		fifoPrintChar (io->stdout,'\'');
		fifoPrintString (io->stdout," -A");
		for (int b=0; b<selectedMember->family->banks; b++) {
			if (b!=0) fifoPrintChar (io->stdout,',');
			fifoPrintString (io->stdout,"0x");
			fifoPrintHex (io->stdout,selectedMember->family->addressFlashs[b],8,8);
		}
		fifoPrintString (io->stdout," -B");
		for (int b=0; b<LPC_BLOCK_SIZES && selectedMember->family->blockSizes[b]!=0; b++) {
			if (b!=0) fifoPrintChar (io->stdout,',');
			fifoPrintUint32 (io->stdout,selectedMember->family->blockSizes[b],1);
		}
		fifoPrintString (io->stdout," -F");
		for (int g=0; g<LPC_SECTOR_ARRAYS && selectedMember->family->sectorArrays[g].n!=0; g++) {
			if (g!=0) fifoPrintChar (io->stdout,',');
			fifoPrintUint16 (io->stdout, selectedMember->family->sectorArrays[g].sizeK,1);
			fifoPrintString (io->stdout,"kix");
			fifoPrintUint16 (io->stdout, selectedMember->family->sectorArrays[g].n,1);
		}
		fifoPrintString (io->stdout," -I");
		for (int i=0; i<LPC_IDS && selectedMember->family->idMasks[i]!=0; i++) {
			if (i!=0) fifoPrintChar (io->stdout,',');
			Int32Pair pair = { selectedMember->ids[i], selectedMember->family->idMasks[i] };
			if (canPrintInt32HexWithDontCares (&pair)) {
				fifoPrintString (io->stdout,"0x");
				fifoPrintInt32HexWithDontCares (io->stdout,&pair,'X');
			}
			else {
				fifoPrintString (io->stdout,"0b");
				fifoPrintInt32BinWithDontCares (io->stdout,&pair,'X');
			}
		}
		fifoPrintString (io->stdout," -M");
		for (int r=0, nr=0; r<LPC_RAMS; r++) {
			if (selectedMember->sizeRamKs[r]>0) {
				if (nr!=0) fifoPrintChar (io->stdout,',');
				fifoPrintUint32 (io->stdout,selectedMember->sizeRamKs[r],1);
				fifoPrintString (io->stdout,"ki@0x");
				fifoPrintHex (io->stdout,selectedMember->ispFamily->addressRams[r],8,8);
				nr++;
			}
		}
		fifoPrintString (io->stdout," -P");
		fifoPrintString (io->stdout,selectedMember->ispFamily->protocol==ISP_PROTOCOL_UUENCODE ? "UUENCODE":"BINARY");
		fifoPrintString (io->stdout," -R");
		for (int i=0; i<LPC_ISP_RAMS; i++) {
			if (i!=0) fifoPrintChar (io->stdout,',');
			const Int32 size = selectedMember->ispFamily->ramUsage[i].size;
			const Int32 address = selectedMember->ispFamily->ramUsage[i].address;
			if (size>0) {
				fifoPrintString (io->stdout,"0x");
				fifoPrintHex (io->stdout,size,3,8);
			}
			else {
				fifoPrintChar (io->stdout,'-');
				fifoPrintString (io->stdout,"0x");
				fifoPrintHex (io->stdout,-size,3,8);
			}
			fifoPrintString (io->stdout,"@0x");
			fifoPrintHex (io->stdout,address,8,8);
		}
		fifoPrintString (io->stdout," -S");
		fifoPrintInt16 (io->stdout,selectedMember->family->checksumVectors,1);
		fifoPrintChar (io->stdout,'@');
		fifoPrintInt16 (io->stdout,selectedMember->family->checksumVector,1);

		// adjust flash size with override
		fifoPrintString (io->stdout," -f");
		fifoPrintInt32 (io->stdout,selectedMember->sizeFlashK,1);
		fifoPrintString (io->stdout,"ki");
		fifoPrintLn (io->stdout);

		pushStdout (io);
	}

	if (commandNoIo) return true;		// exit here, if no IO allowed

	// check, if we need banked commands...
	const bool bankedCommands = lpcFamily.banks >= 2;

	Uint32 overrideFlashBankOffset = 0;
	if (overrideDestinationFlashBank!=-1
	&&  overrideDestinationFlashBank!=BANK_Z) {
		const int bankIndex = overrideDestinationFlashBank-BANK_A;
		if (bankIndex >= selectedMember->family->banks) {
			errorMessage (io, "FLASH bank out of range\n");
			goto failClose;
		}
		overrideFlashBankOffset = selectedMember->family->addressFlashs[bankIndex];
	}


	if (commandShowBootCodeVersion) {
		Uint32 version;
		if (lpcReadBootCodeVersion (io,&version)) {
			fifoPrintUint16 (io->stdout, (version >> 8) & 0xFF,1);
			fifoPrintChar (io->stdout,'.');
			fifoPrintUint16 (io->stdout, version & 0xFF,1);
			fifoPrintLn (io->stdout);
			pushStdout (io);
		}
		else goto failClose;
	}

	// command: read UID
	if (commandShowUid) {
		Uint32 uids[4];
		if (lpcReadUid (io,uids)) {
			for (int i=0; i<4; i++) {
				if (i!=0) fifoPrintChar (io->stdout,' ');
				fifoPrintString (io->stdout, "0x");
				fifoPrintHex (io->stdout,uids[i],8,8);
			}
			fifoPrintLn (io->stdout);
			pushStdout (io);
		}
		else goto failClose;
	}

	// read operation
	if (readByteCount!=-1) {
		Uint32 sourceAddress = overrideFlashBankOffset + 0;
		if (listDestinationAddresses.length > 0) sourceAddress = overrideFlashBankOffset + listDestinationAddresses.elements[0];
		for (Int32 n=0; n<readByteCount; ) {
			Int32 nBlock = readByteCount-n < STDOUT_BUFFERSIZE ? readByteCount-n : STDOUT_BUFFERSIZE;
			if (io->debugLevel>=LPC_ISP_PROGRESS) {
				fifoPrintString (io->stderr, "[");
			}
			if (lpcRead (io, &com, io->stdout, sourceAddress+n, nBlock)) pushStdout (io);
			else {
				pushStdout (io);	// write what we've got so far...
				goto failClose;
			}
			n += nBlock;
			if (io->debugLevel>=LPC_ISP_PROGRESS) {
				fifoPrintInt32 (io->stderr, nBlock, 1);
				fifoPrintString (io->stderr, "] total ");
				fifoPrintInt32 (io->stderr, n, 1);
				fifoPrintLn (io->stderr);
				pushStderr (io);
			}
		}
	}

	// image creation
	enum { MAX_IMAGES = 2, MAX_SEGMENTS=2+1 };
	Executable32Segment executable [MAX_SEGMENTS] = {};
	Uint32 hexImageBuffers [MAX_IMAGES] [512*1024];
	HexImage hexImages [MAX_IMAGES] = {};
	for (int i=0; i<MAX_IMAGES; i++) {
		hexImages[i].ram = (Uint8*)hexImageBuffers[i];
		hexImages[i].ramSize = sizeof hexImageBuffers[i];
	}

	// extract non-empty filenames
	const char * fileNames[MAX_IMAGES];
	for (int f=0; fifoCanRead (&fifoqImageFiles); ) {
		fifoParseBlanks(&fifoqImageFiles);
		const char *fn = fifoReadLinear (&fifoqImageFiles);
		fifoqParseSkipToNext (&fifoqImageFiles);
		if (fn[0]!=0) {
			fileNames[f] = fn;
			f++;
		}
	}

	for (int i=0; fileNames[i]!=0; i++) {
		if (i==0 && io->debugLevel >= LPC_ISP_PROGRESS) {
			fifoPrintString (io->stderr, CYAN "Loading images.\n" NORMAL);
			pushStderr (io);
		}

		const Uint32 offset = overrideFlashBankOffset +
			(i<uint32ListLength (&listDestinationAddresses) ?  listDestinationAddresses.elements[i] : 0);

		if (i>=MAX_IMAGES) {
			errorMessage (io, "too many image files\n");
			goto failClose;
		}

		commandWrite = true;

		if (io->debugLevel >= LPC_ISP_PROGRESS) {
			fifoPrintString (io->stderr, "Image=");
			fifoPrintString (io->stderr, fileNames[i]);
			fifoPrintString (io->stderr, " (bank) offset 0x");
			fifoPrintHex (io->stderr, offset, 8,8);
			fifoPrintLn (io->stderr);
			pushStderr (io);
		}
		if (hexFileLoad (fileNames[i],&hexImages[i])) {
			if (io->debugLevel >= LPC_ISP_PROGRESS) {
				fifoPrintString (io->stderr, "Image details:");
				fifoPrintHexImage (io->stderr, &hexImages[i]);
				fifoPrintLn (io->stderr);
				pushStderr (io);
			}
			
			// re-arrange into executable
			if (executable32FromHexImage (executable, MAX_SEGMENTS, &hexImages[i], offset)) ;
			else {
				errorMessage (io,"cannot convert hexImage to executable32 - too many segments?\n");
				goto failClose;
			}
		}
		else {
			if (io->debugLevel>=LPC_ISP_NORMAL) {
				fifoPrintString (io->stderr,"cannot load image file: ");
				fifoPrintString (io->stderr,fileNames[i]);
				fifoPrintLn (io->stderr);
				pushStderr (io);
			}
			goto failClose;
		}
	}
	
	// calculate sectors to target
	Int32Pair targetSectorBuffer [10];
	Int32PairList targetSectorRanges = { targetSectorBuffer, sizeof targetSectorBuffer, };
	int sectorRangeCount = lpcSectorCoverage (&targetSectorRanges, selectedMember, executable);
	// :o) should loop the following 2 lines:
	sectorRangeCount += int32PairListTransitiveFusion (&targetSectorRanges);
	sectorRangeCount += int32PairListSequenceFusion (&targetSectorRanges);


	if (io->debugLevel >= LPC_ISP_INFO) {
		fifoPrintString (io->stderr, "Sectors targeted by image: ");
		for (int s=0; s < int32PairListLength (&targetSectorRanges); s++) {
			if (s!=0) fifoPrintString (io->stderr, ", ");
			fifoPrintSector (io->stderr, targetSectorRanges.elements[s].fst);
			fifoPrintString (io->stderr,"..");
			fifoPrintSector (io->stderr, targetSectorRanges.elements[s].snd);
		}
		fifoPrintLn (io->stderr);
		pushStderr (io);
	}

	if (	(commandEraseSectorRange.fst != -1
		|| commandEraseFlashBankRange.fst != -1
		|| commandWrite
		|| commandSetActiveFlashBank != -1
		) && !lpcUnlock (io)) {
			errorMessage (io, "cannot unlock device\n");
			goto failClose;
	}

	if (commandEraseSectorRange.fst != -1) {
		const int sectorFrom = commandEraseSectorRange.fst;
		const int sectorTo = commandEraseSectorRange.snd;
		if (io->debugLevel >= LPC_ISP_PROGRESS) {
			fifoPrintString (io->stderr, CYAN "Erasing sectors ");
			fifoPrintSector (io->stderr, sectorFrom);
			fifoPrintString (io->stderr, "..");
			fifoPrintSector (io->stderr, sectorTo);
			fifoPrintString (io->stderr, NORMAL);
			pushStderr (io);
		}
		if (!lpcPrepareForWrite (io,sectorFrom,sectorTo,bankedCommands)
		|| !lpcErase (io,sectorFrom,sectorTo,bankedCommands)) {
			errorMessage (io, "erase failed\n");
			goto failClose;
		}
		if (io->debugLevel >= LPC_ISP_PROGRESS) {
			fifoPrintLn (io->stderr);
			pushStderr (io);
		}
	}
	
	if (commandEraseFlashBankRange.fst != -1) {
		for (int bank=commandEraseFlashBankRange.fst; bank<=commandEraseFlashBankRange.snd; bank++) {
			const int sectorTo = lpcBankToLastSector (selectedMember,bank);
			const int sectorFrom = sectorTo & ~SECTOR_MASK;
			if (io->debugLevel >= LPC_ISP_PROGRESS) {
				fifoPrintString (io->stderr, CYAN "Erasing bank ");
				fifoPrintBank (io->stderr,bank);
				fifoPrintString (io->stderr, " sectors ");
				fifoPrintSector (io->stderr, sectorFrom);
				fifoPrintString (io->stderr, "..");
				fifoPrintSector (io->stderr, sectorTo);
				fifoPrintString (io->stderr, NORMAL);
				pushStderr (io);
			}
			if (!lpcPrepareForWrite (io,sectorFrom,sectorTo,bankedCommands)
			|| !lpcErase (io,sectorFrom,sectorTo,bankedCommands)) {
				errorMessage (io, "erase failed\n");
				goto failClose;
			}
			if (io->debugLevel >= LPC_ISP_PROGRESS) {
				fifoPrintLn (io->stderr);
				pushStderr (io);
			}
		}
	}

	if (quickMode) {	// erase all targeted sectors at once, don't blank check.
		for (int r=0; r < int32PairListLength (&targetSectorRanges); r++) {
			const int sectorFrom = targetSectorRanges.elements[r].fst;
			const int sectorTo = targetSectorRanges.elements[r].snd;
			if (io->debugLevel >= LPC_ISP_PROGRESS) {
				fifoPrintString (io->stderr, CYAN "Erasing sectors ");
				fifoPrintSector (io->stderr, sectorFrom);
				fifoPrintString (io->stderr, "..");
				fifoPrintSector (io->stderr, sectorTo);
				fifoPrintString (io->stderr, NORMAL);
				pushStderr (io);
			}
			if (!lpcPrepareForWrite (io,sectorFrom,sectorTo,bankedCommands)
			|| !lpcErase (io,sectorFrom,sectorTo,bankedCommands)) {
				errorMessage (io, "erase failed\n");
				goto failClose;
			}
			if (io->debugLevel >= LPC_ISP_PROGRESS) {
				fifoPrintLn (io->stderr);
				pushStderr (io);
			}
		}
	}

	if (commandWrite) {
		const LpcIspFlashOptions flash = {
			.eraseBeforeWrite = !quickMode,
			.eraseOnDemand = !quickMode,
			.crpAllow = lockLevelAllowed,
			.crpDesired = lockLevelRequested,	// lockLevelRequested==-1 ? 0 : lockLevelRequested,
			.crpOffsetBank = overrideCrpAddress,
			.banked = bankedCommands,
		};

		if (io->debugLevel >= LPC_ISP_PROGRESS) {
			fifoPrintExecutable32Segments (io->stderr, executable);
			fifoPrintString (io->stderr, CYAN "Writing image(s).\n" NORMAL);
			pushStderr (io);
		}

		if (lpcFlash (io, &com, &flash, selectedMember, executable)) progressMessage (io, CYAN "Write image(s) OK\n" NORMAL);
		else {
			errorMessage (io, "write FLASH failed\n");
			goto failClose;
		}
	}

	if (commandSetActiveFlashBank != -1) {
		if (io->debugLevel >= LPC_ISP_PROGRESS) {
			fifoPrintExecutable32Segments (io->stderr, executable);
			fifoPrintString (io->stderr, CYAN "Setting active bank to: ");
			fifoPrintBank (io->stderr, commandSetActiveFlashBank);
			fifoPrintString (io->stderr, NORMAL "\n");
			pushStderr (io);
		}

		if (lpcSetFlashBank (io, commandSetActiveFlashBank)) ;
		else {
			errorMessage (io,"cannot set active FLASH bank\n");
			goto failClose;
		}
	}

	if (commandExecuteByReset) {
		if (lpcWavePlay (io,&waveConfiguration, & waveSet.waves[WAVE_EXECUTE], "RESET and RUN")); // fine
		else {
			errorMessage (io,"program NOT started\n");
			goto failClose;
		}
	}

	// normal way out...
	close(fdLpc);
	fifoPrintString (io->stderr, NORMAL);	// reset any colors...
	pushStderr (io);
	return 0;


	failClose:
	close(fdLpc);
	failEarly:	return 1;
	returnEarly:	return 0;
}

