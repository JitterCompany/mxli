# build library
#
.PHONY: all clean

# the order of the subprojects DOES matter. The libs with least dependencies on generated header files first.
SUBPROJECTS=c-any c-linux doc
all:
	for p in ${SUBPROJECTS}; do (cd $$p || exit 1; ${MAKE} $@; ) || exit 1; done

clean:
	for p in ${SUBPROJECTS}; do (cd $$p || exit 1; ${MAKE} $@; ) || exit 1; done

