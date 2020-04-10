# Simple Makefile for single source projects
#

include ../../lib/LibraryConfig.make
include ../../Project.make
LDLIBS+=-lreadline -lpthread


SINGLE_SOURCE:=$(word 1,$(wildcard *.C *.cpp *.c))
TARGET:=$(basename ${SINGLE_SOURCE})

${TARGET}:

.PHONY: info
info:
	@echo "target: ${TARGET}"

.PHONY: clean
clean:
	-rm ${TARGET} *.o

.PHONY: install
	ln -srt ~/bin ${TARGET}

