# Defines the include paths for .c and .s header files.
# This works even if this Makefile is included from another.
#
LIB_MAKEFILE:=$(word $(words ${MAKEFILE_LIST}), ${MAKEFILE_LIST})
LIB_DIR:=$(dir ${LIB_MAKEFILE})

LIB_LIBS:=c-any c-linux

LIB_INC_DIRS:=$(addprefix ${LIB_DIR}, ${LIB_LIBS} module)
LIB_CPPFLAGS:=$(patsubst %,-I%,${LIB_INC_DIRS})

# exported variables
# duplication of c-any is intentional:
LIB_LOADLIBES:=$(patsubst %,-l%,${LIB_LIBS} c-any) -lm
LIB_LDFLAGS:=$(patsubst %,-L%, $(addprefix ${LIB_DIR}, ./ ))

