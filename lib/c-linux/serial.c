/*
  serial.c 
  Copyright 2011,2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <c-linux/serial.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

//#define DEBUG(x) x		// uncomment for debugging
#define DEBUG(x)

DEBUG(#include <stdio.h>)

static int baud2Termios(int baud) {
	switch(baud) {
		case 9600:	return B9600;
		case 19200:	return B19200;
		case 38400:	return B38400;
		case 57600:	return B57600;
		case 115200:	return B115200;
		case 230400:	return B230400;
		default:	return B0;
	}
}

int serialOpenBlockingTimeout(const char *tty, int baud, int timeoutDeciSeconds) {
	int fd = open(tty,O_RDWR | O_NOCTTY);
	if (fd==-1) {
		DEBUG( fprintf(stderr,"ERROR: cannot open %d\n",tty); )
		return -1;
	}

	struct termios terminalSettings, terminalSettingsSaved;

	if (-1 != tcgetattr(fd,&terminalSettingsSaved)) {
		terminalSettings = terminalSettingsSaved;
		terminalSettings.c_iflag = terminalSettings.c_iflag
			& ~	( BRKINT
				| PARMRK	// redundant
				| ISTRIP
				| INLCR		// map NL to CR
				| IGNCR		// ignore CR
				| ICRNL
				| IXON
			)
			| IGNBRK
			| IGNPAR
			;
		terminalSettings.c_oflag = terminalSettings.c_oflag
			& ~	( ONLCR		// map NL to NL-CR
				| OPOST		// enables all output processing
			)
			;
		terminalSettings.c_cflag = terminalSettings.c_cflag
			| CLOCAL		// ignore modem control lines
			| CREAD			// enable reading chars
			;
		terminalSettings.c_lflag = terminalSettings.c_lflag
			& ~	( ICANON	// line-by-line processing
				| ECHO
				| ECHONL
				| ISIG
				| IEXTEN
			)
			;
		if (-1 != cfsetospeed(&terminalSettings,baud2Termios(baud))
		&& -1 != cfsetispeed(&terminalSettings,baud2Termios(baud))) {
			// fine
		}
		else {
			DEBUG( fprintf(stderr,"Error setting baud rate\n"); )
			return -1;
		}

		// configure for blocking read, but with timeout.
		terminalSettings.c_cc[VTIME] = timeoutDeciSeconds;	// unit 1/10s timeout
		terminalSettings.c_cc[VMIN] = 0;	// 1 byte, not 0 bytes at least

		if (-1 != tcsetattr(fd,TCSANOW,&terminalSettings)) ;	// at least one setting is applied
		else {
			// none of the new settings could be applied.
			//fprintf(stderr,"cannot apply terminal settings\n");
			return -1;
		}
	}
	else {
		DEBUG( fprintf(stderr,"Not a terminal device\n"); )
		return -1;
	}

	return fd;
}


// used for reset, active 1
// These signals are inverted, we correct for that
bool serialSetDtr(int fd, bool on) {
	int status;
	if (0==ioctl(fd,TIOCMGET,&status)) {
		status = !on ? status | TIOCM_DTR : status &~(TIOCM_DTR);
		ioctl(fd,TIOCMSET,&status);
		return true;
	}
	else return false;
}

// used for bsl, active 1
bool serialSetRts(int fd, bool on) {
	int status;
	if (0==ioctl(fd,TIOCMGET,&status)) {
		status = !on ? status | TIOCM_RTS : status &~(TIOCM_RTS);
		ioctl(fd,TIOCMSET,&status);
		return true;
	}
	else return false;
}

