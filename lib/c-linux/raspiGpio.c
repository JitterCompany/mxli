#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <c-linux/raspiGpio.h>

static const char *const gpio = "/sys/class/gpio";

struct {
	unsigned long long	exported;	///< true, if we're sure, the pin is exported
	unsigned long long	configured;	///< true, if we're sure, the direction is configured
} raspiLazyGpio = {};

bool raspiLazyGpioSysExists (int port) {
	char buffer[4+2+1+9+1];
	char io[3];
	return	snprintf (buffer, sizeof buffer, "gpio%d/direction",port)
	&& raspiLazyGpioReadString (buffer,io,sizeof io);				// dummy read
}

bool raspiLazyGpioWriteString (const char *name, const char *msg) {
	char buffer[500];
	int n = snprintf (buffer,sizeof buffer, "%s/%s",gpio,name);
	if (n<sizeof buffer) {
		int fd = open (buffer,O_WRONLY);
		if (fd>=0) {
			int length = strlen (msg);
			int w = write (fd,msg,length);
			close(fd);
			return w==length;
		}
		else return false;	// cannot open file
	}
	else return false;	// file name too long
}

int raspiLazyGpioReadString (const char *name, char *msg, int length) {
	char buffer[500];
	int n = snprintf (buffer,sizeof buffer, "%s/%s",gpio,name);
	if (n<sizeof buffer) {
		int fd = open (buffer,O_RDONLY);
		if (fd>=0) {
			int w = read (fd,msg,length);
			close(fd);
			return w;
		}
		else return 0;	// cannot open file
	}
	else return 0;	// file name too long
}

bool raspiLazyGpioExport (int port) {
	char buffer[3];
	if (sizeof buffer > snprintf (buffer,sizeof buffer,"%d",port)) {
		if (raspiLazyGpioSysExists (port)) {
			raspiLazyGpio.exported |= 1<<port;		// no delay here
			return true;
		}
		else if (raspiLazyGpioWriteString ("export",buffer)) {
			raspiLazyGpio.exported |= 1<<port;
			usleep (1000*100);	// takes some time for the entries to show up in ../gpioXY
			return true;
		}
		else return false;
	}
	else return false;
}

bool raspiLazyGpioDirection (int port, bool out) {
	char buffer[4+2+1+9+1];
	if (snprintf (buffer, sizeof buffer, "gpio%d/direction",port)
	&& (raspiLazyGpioIsExported (port) || raspiLazyGpioExport (port))
	&& raspiLazyGpioWriteString (buffer,out?"out":"in")) {
		raspiLazyGpio.configured |= 1<<port;
		return true;
	} 
	else return false;
}

bool raspiLazyGpioIsExported (int port) {
	return 0!= (raspiLazyGpio.exported & 1<<port);
}

bool raspiLazyGpioIsConfigured (int port) {
	return 0!= (raspiLazyGpio.configured & 1<<port);
}

bool raspiLazyGpioWrite (int port, bool value) {
	char buffer[4+2+1+5+1];
	return
	snprintf (buffer, sizeof buffer, "gpio%d/value",port)
	&& (raspiLazyGpioIsExported (port) || raspiLazyGpioExport (port))
	&& (raspiLazyGpioIsConfigured (port) || raspiLazyGpioDirection (port,true))	// switch to output
	&& raspiLazyGpioWriteString (buffer,value?"1":"0");
}

bool raspiLazyGpioRead (int port, bool *value) {
	char buffer[4+2+1+5+1];
	char result;
	if (snprintf (buffer, sizeof buffer, "gpio%d/value",port)
	&& (raspiLazyGpioIsExported (port) || raspiLazyGpioExport (port))
	&& (raspiLazyGpioIsConfigured (port) || raspiLazyGpioDirection (port,false))	// switch to input
	&& raspiLazyGpioReadString (buffer,&result,1)) {
		*value = result=='1';
		return true;
	}
	else return false;
}


