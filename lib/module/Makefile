# TARGET=../libc-any.a

include ../../Project.make
include ../LibraryConfig.make
#include ../Library.make

# You have to define the variable TARGET (a library xxx.a) and this makefile
# defines the necessary targets and dependencies for you.
# In a subdirectory, create a makefile that defines TARGET=xxx.a and include
# this helper script. Then you can create the library easily

SOURCES:=$(filter-out %-sliced.s, $(wildcard *.c *.s))
SOURCES:=$(filter-out %-sliced.c, ${SOURCES})
SOURCES:=$(filter-out %-sliced.C, ${SOURCES})

OBJECTS=$(addsuffix .o, $(basename ${SOURCES}))

.PHONY: all slices showobjs showsources
all: ${OBJECTS}

.PHONY: silent
silent:
	${MAKE} -j3 > /dev/null

showobjs:
	@echo ${OBJECTS}

showsources:
	@echo ${SOURCES}

showincludes:
	@echo "CFLAGS+=${CPPFLAGS_LIB_INC_DIRS}"

.PHONY: clean
clean:
	-${RM} *.o > /dev/null 2>&1


