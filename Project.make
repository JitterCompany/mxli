# (C) Marc Prager, 2010
# Project: c-linux, i386
#
# tool configuration
PROJECT_MAKEFILE:=$(word $(words ${MAKEFILE_LIST}), ${MAKEFILE_LIST})
PROJECT_DIR:=$(subst /SUFFIX,,$(dir ${PROJECT_MAKEFILE})SUFFIX)

SLICE:=${PROJECT_DIR}/util/slice.pl

# preprocessor
CPPFLAGS+=${LIB_CPPFLAGS}

# C compiler, -Wstrict-aliasing=2 is quite strict
CFLAGS:= -Wall -std=gnu99 -Wno-parentheses -g -Wno-unused-variable

#	${LD} ${LIB_LDFLAGS} ${LDFLAGS} -T${LDSCRIPT} -o $@ $^ ${LIB_LOADLIBES} ${LOADLIBES} ${LIB_LD_LIBS} ${LDLIBS} -lgcc

LDLIBS:=${LIB_LOADLIBES}
LDFLAGS:=${LIB_LDFLAGS}
# archiver
AR:=ar

# source file slicer (Perl script)
SLICE:=${PROJECT_DIR}/util/slice.pl

# *.h *.s headers from *.def
DEF2HDR:=${PROJECT_DIR}/util/def2hdr.pl

%.h: %.def
	${DEF2HDR} $<

%.s: %.def
	${DEF2HDR} $<

