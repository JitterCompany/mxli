/*
  uconsole.c - main program of uconsole, a line-buffering/readline/history terminal program for NXP LPC ARM controllers.
  Copyright 2011-2013 Marc Prager
 
  uconsole is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  uconsole is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with uconsole.
  If not see <http://www.gnu.org/licenses/>
 */

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <c-linux/serial.h>
#include <fifoPopt.h>

const char* histFn = "/tmp/uconsole.hist";

void showHelp(void) {
	fprintf(stderr,
	"Usage: uconsole [options]\n"
	"  -?,-h                    : show this help\n"
	"  -b <baud>                : baud rate [115200]\n"
	"  -d <device>              : serial device [/dev/ttyUSB0]\n"
	"  -s <wave>                : control RTS and DTR (GIMx BSL and RESET)\n"
	"  -t <timeout/ms>          : receiver timeout [100]\n"
	"  -c                       : show control chars as escape sequences\n"
	"  -C                       : show all control chars, even \\r and \\n\n"
	"  -l {CR|LF|CRLF}          : transmission line feed [LF]\n"
	"  -p <prompt>              : show a prompt\n"
	"  -o <outputDevice>        : send received data to <outputDevice> [stdout]\n"
	"\n"
	"<wave> is a sequence of the chars rRdDpP with the following meanings:\n"
	"  r : RTS=0, R : RTS=1\n"
	"  d : DTR=0, D : DTR=1\n"
	"  p : pause 100ms\n"
	);
}

static char fifoCmdLineBuffer[1024];
static Fifo fifoCmdLine = { fifoCmdLineBuffer, sizeof fifoCmdLineBuffer };

static bool helpShow = false;
static bool controlCharsShow = false;
static bool controlCharsShowAll = false;
static Int32 baud = 115200;
static Int32 timeoutMs = 100;
static int symbolLineFeed = 2;
static Fifo fifoDevice = {};
static Fifo fifoWave = {};
static Fifo fifoPrompt = {};
static Fifo fifoOutputDevice = {};

const FifoPoptBool optionBools[] = {
	{ .shortOption = '?', .value = &helpShow },
	{ .shortOption = 'h', .value = &helpShow },
	{ .shortOption = 'c', .value = &controlCharsShow },
	{ .shortOption = 'C', .value = &controlCharsShowAll },
	{}
};

const FifoPoptInt32 optionInt32s[] = {
	{ .shortOption = 'b', .value = &baud, },
	{ .shortOption = 't', .value = &timeoutMs, },
	{}
};

const char* lineFeedSymbols[] = {
	"CR","LF","CRLF",
	0
};

const FifoPoptSymbol optionSymbols[] = {
	{ .shortOption = 'l', .value = &symbolLineFeed, .symbols = lineFeedSymbols },
	{}
};

const FifoPoptString optionStrings[] = {
	{ .shortOption = 'd', .value = &fifoDevice },
	{ .shortOption = 's', .value = &fifoWave },
	{ .shortOption = 'p', .value = &fifoPrompt },
	{ .shortOption = 'o', .value = &fifoOutputDevice },
	{}
};

bool parseCmdLine(int argc, const char* const *argv) {
	bool minus = false;
	if (fifoPoptAccumulateCommandLine(&fifoCmdLine,argc,argv)) {
		while (fifoCanRead(&fifoCmdLine)) {
			if (fifoPoptBool(&fifoCmdLine,optionBools,&minus)
			|| fifoPoptInt32(&fifoCmdLine,optionInt32s,&minus)
			|| fifoPoptString(&fifoCmdLine,optionStrings,&minus)
			|| fifoPoptSymbol(&fifoCmdLine,optionSymbols,&minus)
			) ;	// all fine
			else {
				fprintf(stderr,"Invalid argument: %s", fifoReadPositionToString(&fifoCmdLine));
				return false;
			}
		}
		return true;
	}
	else return false;
}

typedef struct {
	int		readFd;
	int		writeFd;
	volatile bool	run;
} TControl;

void dumpInputCharacter(int fd, char c) {
	char buffer[6];
	int n = 0;
	if (c=='\n') {
		if (controlCharsShowAll) n = snprintf(buffer,sizeof buffer,"\\n\n");	// write symbol and char
		else n = snprintf(buffer,sizeof buffer,"\n");
	}
	else if (c=='\r') {
		if (controlCharsShowAll) n = snprintf(buffer,sizeof buffer,"\\r");	// write symbol only
		else n = snprintf(buffer,sizeof buffer,"\r");
	}
	else if (0<=c && c<0x20) {
		if (controlCharsShow) n = snprintf(buffer,sizeof buffer,"\\x%02X",c & 0xFF);	// write symbol only
		else n = snprintf(buffer,sizeof buffer,"%c",c);
	}
	else n = snprintf(buffer,sizeof buffer,"%c",c);

	write(fd,buffer,n);
}

void* dumpInput(TControl *tc) {
	char c;
	while (tc->run) {
		const int n=read(tc->readFd,&c,1);
		switch(n) {
			case 0: break;
			case 1: dumpInputCharacter(tc->writeFd,c); break;
			default: tc->run = false;
		}
	}
	return 0;
}


int main(int argc, const char* const*argv) {
	if (!parseCmdLine(argc,argv)) return 1;
	if (helpShow) {
		showHelp();
		return 0;
	}
	const char *envDefault = getenv("ARM_TTY");
	const char *device = fifoIsValid(&fifoDevice) ? fifoReadPositionToString(&fifoDevice)
		: ( envDefault!=0 ? envDefault : "/dev/ttyUSB0");
		
	const int fd = serialOpenBlockingTimeout(device,baud,timeoutMs/100);
	if (fd<0) return 1;

	int fdOut = 1;
	if (fifoIsValid(&fifoOutputDevice)) {
		const char *terminal = fifoReadPositionToString(&fifoOutputDevice);
		fdOut = open(terminal, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH );
		if (fdOut<0) {
			fprintf(stderr,"Error opening \"%s\"\n",terminal);
			return 1;
		}
	}

	TControl tc = { fd,fdOut,true };
	pthread_t t;
	if (0!=pthread_create(&t,0,(void*((*)(void*)))&dumpInput,&tc)) return 1;

	if (fifoIsValid(&fifoWave)) {
		while (fifoCanRead(&fifoWave)) {
			const char c = fifoRead(&fifoWave);
			switch(c) {
			case 'r': serialSetRts(fd,false); break;
			case 'R': serialSetRts(fd,true); break;
			case 'd': serialSetDtr(fd,false); break;
			case 'D': serialSetDtr(fd,true); break;
			case 'p': usleep(100*1000); break;
			default :	fprintf(stderr,"Invalid char '%c' in wave definition.\n",c);
					return 1;
			}
		}
	}

	using_history();
	history_truncate_file(histFn,10);
	read_history(histFn);

	const char *prompt = fifoIsValid(&fifoPrompt) ? fifoReadPositionToString(&fifoPrompt) : 0;
	const char* lfs[] = { "\r","\n","\r\n" };
	char*	line;
	do {
		line = readline(prompt);
		if (line) {
			write(fd,line,strlen(line));
			write(fd,lfs[symbolLineFeed],strlen(lfs[symbolLineFeed]));
			add_history(line);
			free(line);
		}
	} while (line);
	fprintf(stderr,"Connection closed.\n");
	tc.run = false;
	write_history(histFn);
	return 0;
}

