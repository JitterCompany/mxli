/*
  fd.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <c-linux/fd.h>
#include <poll.h>
#include <unistd.h>

bool fdCanRead(int fd) {
	struct pollfd pollfd = {
		.fd = fd,
		.events = POLLIN | POLLPRI,
		.revents = 0
	};
	return 1==poll(&pollfd,1,0);
}

int fdRead(int fd) {
	char c;
	if (1==read(fd,&c,1)) return 0xFF & (int)c;
	else return -1;
}

bool fdCanWrite(int fd) {
	struct pollfd pollfd = {
		.fd = fd,
		.events = POLLOUT | POLLERR | POLLHUP,
		.revents = 0
	};
	return 1==poll(&pollfd,1,0);
}

void fdWrite(int fd, char c) {
	write(fd,&c,1);
}

