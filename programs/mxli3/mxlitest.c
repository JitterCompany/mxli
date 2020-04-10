/** @file
 * @brief mxli LPC ISP functions
 */
#include <module/mxli.h>
#include <c-linux/hexFile.h>
#include <hexImage.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Adaption Linux <-> Fifo

#include <c-linux/serial.h>

#include <time.h>
#include <unistd.h>
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

void escape (int fd, const char *msg) {
	for ( ; *msg ; msg++) write (fd,msg,1); 
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

bool adapterSetRts	(bool level)	{ return serialSetRts (fdLpc,level);	}
bool adapterSetDtr	(bool level)	{ return serialSetDtr (fdLpc,level);	}

static char bufferLpcIn	[4096], bufferLpcOut [4096], bufferStdin [64], bufferStdout [4096], bufferStderr [4096];
static Fifo
	fifoLpcIn	= { bufferLpcIn,	sizeof bufferLpcIn,	},
	fifoLpcInLine	= { },
	fifoLpcOut	= { bufferLpcOut,	sizeof bufferLpcOut,	},
	fifoStdin	= { bufferStdin,	sizeof bufferStdin,	},
	fifoStdout	= { bufferStdout,	sizeof bufferStdout,	},
	fifoStderr	= { bufferStderr,	sizeof bufferStderr,	};

static const LpcIspIo lpcIspIo = {
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

	.debugLevel	= LPC_ISP_DEBUG,
	//.debugLevel	= LPC_ISP_PROGRESS,
};

bool (*lpcIspFlowControlHook)(const LpcIspIo *io, char code) = 0;

int main(void) {
	const LpcIspConfigCom com = {
		.crystalHz = 12*MEGA,
		.baud = 115200,
		.resetUs = 100*KILO,
		.bootUs = 300*KILO,
		.timeoutUs = MEGA,
		.stopBits = 1,
		.ispProtocol = ISP_PROTOCOL_BINARY,
		//.useEcho = true,	// limits to 9600bd on LPC800, otherwise 115200
		.useEcho = false,
	};

	const LpcIspFlashOptions flash = {
		.eraseBeforeWrite = true,
		.eraseOnDemand = true,
		.crpAllow = 0,
		.crpDesired = 0,
		.crpOffsetBank = 0x2F0,
	};

	fdLpc = serialOpenBlockingTimeout("/dev/ttyUSB0",com.baud, com.timeoutUs/KILO/100);

	patch.lineChar = 0;

	lpcIspIo.setDtr (false);
	lpcIspIo.setRts (false);
	lpcIspIo.sleepUs (com.resetUs);
	lpcIspIo.setDtr (true);
	lpcIspIo.sleepUs (com.bootUs);
	if (lpcSync (&lpcIspIo,com.crystalHz)) {
		//fifoPrintString (lpcIspIo.stdout, "\033[1mSync OK\033[0m\n");
		fifoPrintString (lpcIspIo.stdout, CYAN "Sync OK\n" NORMAL);
		pushStdout (&lpcIspIo);
	}

	if (lpcBaud (&lpcIspIo,com.baud,1)) {
		fifoPrintString (lpcIspIo.stdout, CYAN "Baud OK\n" NORMAL);
		pushStdout (&lpcIspIo);
	}

	if (lpcEcho (&lpcIspIo,com.useEcho)) {
		fifoPrintString (lpcIspIo.stdout, CYAN "Echo OK\n" NORMAL);
		pushStdout (&lpcIspIo);
	}

	Uint32 version = 0;
	if (lpcReadBootCodeVersion (&lpcIspIo, &version)) {
		fifoPrintString (lpcIspIo.stdout, CYAN "Boot code version ");
		fifoPrintUint32 (lpcIspIo.stdout, version>>8,1);
		fifoPrintChar (lpcIspIo.stdout, '.');
		fifoPrintUint32 (lpcIspIo.stdout, version&255,1);
		fifoPrintString (lpcIspIo.stdout, NORMAL);
		fifoPrintLn (lpcIspIo.stdout);
		pushStdout (&lpcIspIo);
	}

	Uint32 partIds[LPC_IDS];
	const LpcMembers members = {
		.thePreferred = &lpcMember812_M101_FD20,
	};

	if (lpcReadPartId (&lpcIspIo, &members, partIds, 0)) {
		fifoPrintString (lpcIspIo.stdout, CYAN "Part ID(s): ");
		for (int i=0; i<LPC_IDS; i++) {
			if (i!=0) fifoPrintString (lpcIspIo.stdout, ", ");
			fifoPrintString (lpcIspIo.stdout, "0x");
			fifoPrintHex (lpcIspIo.stdout, partIds[i], 8,8);
		}
		fifoPrintString (lpcIspIo.stdout, "\n" NORMAL);
		pushStdout (&lpcIspIo);
	}

	Uint32 uids[4];
	if (lpcReadUid (&lpcIspIo, uids)) {
		fifoPrintString (lpcIspIo.stdout, CYAN "Part UID: ");
		for (int i=0; i<4; i++) {
			if (i!=0) fifoPrintString (lpcIspIo.stdout, ", ");
			fifoPrintString (lpcIspIo.stdout, "0x");
			fifoPrintHex (lpcIspIo.stdout, uids[i], 8,8);
		}
		fifoPrintString (lpcIspIo.stdout, "\n" NORMAL);
		pushStdout (&lpcIspIo);
	}

/*
	char imageBytes[128] = "This is no executable image, really!!!\n";
	Fifo image = { imageBytes, sizeof imageBytes, .wTotal = sizeof imageBytes };
	Uint32 address = 0x10000270;

	if (lpcWriteBinary (&lpcIspIo,&com,address, &image)) {
		fifoPrintString (lpcIspIo.stdout, CYAN "Image written\n" NORMAL);
		pushStdout (&lpcIspIo);
	};

	char readBackBytes[1024];
	Fifo readBack = { readBackBytes, sizeof readBackBytes, };

	if (lpcReadBinary (&lpcIspIo,&com,&readBack, address, 128)) {
		fifoPrintString (lpcIspIo.stdout, CYAN "Image read\n" NORMAL);
		fifoPutFifo (lpcIspIo.stdout, &readBack);
		pushStdout (&lpcIspIo);
	}
*/
	Uint8 hexImageBuffer [0x10000];
	HexImage hexImage = { hexImageBuffer, sizeof hexImageBuffer, };
	const char *fn = "main.bin";

	Executable32Segment executable[20];
	if (hexFileLoad(fn, &hexImage)
	&& executable32FromHexImage (executable,20, &hexImage)) {
		fifoPrintExecutable32Segments (lpcIspIo.stdout, executable);
		pushStdout (&lpcIspIo);

		fifoPrintString (lpcIspIo.stdout, CYAN "write image: ");
		if (lpcFlash (&lpcIspIo, &com, &flash, members.thePreferred, executable)) {
			fifoPrintString (lpcIspIo.stdout, "OK");
		}
		else {
			fifoPrintString (lpcIspIo.stdout, "Failed");
		}
		fifoPrintString (lpcIspIo.stdout, "\n" NORMAL);
		pushStdout (&lpcIspIo);
	}
	else {
		fifoPrintString (lpcIspIo.stderr, "Cannot load image file.\n");
		pushStderr (&lpcIspIo);
	}

	close(fdLpc);
}

