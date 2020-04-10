# You have to define the variable TARGET (a library xxx.a) and this makefile
# defines the necessary targets and dependencies for you.
# In a subdirectory, create a makefile that defines TARGET=xxx.a and include
# this helper script. Then you can create the library easily

# CLEAN_AUTO_HEADERS may contain a target for cleaning generated headers.
# AUTO_HEADERS may contain a target for creating headers

SLICE_SOURCES:=$(wildcard *-sliced.?)

SOURCES:=$(filter-out %-sliced.s, $(wildcard *.c *.cpp *.C *.s))
SOURCES:=$(filter-out %-sliced.c, ${SOURCES})
SOURCES:=$(filter-out %-sliced.C, ${SOURCES})

OBJECTS=$(addsuffix .o, $(basename ${SOURCES}))

.PHONY: all clean new slices showobjs showsources

# new slice program generates better time stamps.
all: ${AUTO_HEADERS} slices
	${MAKE} ${TARGET}

new:
	${MAKE} clean
	${MAKE} all

${TARGET}: ${OBJECTS}
	-@${RM} $@
	${AR} Trcs $@ ${OBJECTS}
	@echo "FINISHED: ${TARGET}"

slices:
	@echo "make slices"
	${SLICE} ${SLICE_SOURCES}

showobjs:
	@echo ${OBJECTS}

showsources:
	@echo ${SOURCES}

showincludes:
	@echo "CFLAGS+=${CPPFLAGS_LIB_INC_DIRS}"

.PHONY: clean
clean: ${CLEAN_AUTO_HEADERS}
	-${RM} ${TARGET} *.o *.elf *.bin _slice* > /dev/null 2>&1

.PHONY: fast
fast:	${TARGET}

%.o: %.c
	${CC} -c ${CPPFLAGS} ${CFLAGS} $<

lib.dep:
	${CC} ${CPPFLAGS} -M -MF $@ *.c

