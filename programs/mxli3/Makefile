include ../../lib/LibraryConfig.make
include ../../Project.make

LDLIBS+=-lrt
.PHONY: all
all: mxli

mxli: mxli.o

.PHONY: clean
clean:
	-rm mxli *.o

.PHONY: install
install:
	ln -sfrt ~/bin mxli
	sudo cp mxli.1 /usr/share/man/man1/

