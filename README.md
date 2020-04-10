# mxli - An open source ISP programmer for NXP LPC controllers

This is a fork with added supported for some specific microcontrollers. The original project by Marc Prager is hosted on [http://www.windscooting.com/softy/mxli.html](http://www.windscooting.com/softy/mxli.html)
 

[man pages of v3.0](http://www.windscooting.com/softy/mxli-3.0.pdf)


## How to compile

**Requirements**

* a reasonable recent GCC
* make
* perl
* optionally: Doxygen

```console
make
cd programs/mxli3
make
```

Note, if you don't have Doxygen installed, the first `make` command will end in an error. You can ignore thats and continue to build `mxli3`.
You can also remove `doc` from `lib/Makefile`

## How to run

If your controller is a LPC11U37 family member running on a 12MHz crystal and your serial device is connected to /dev/ttyS0 then your command line should look like this:

```console
mxli -d /dev/ttyS0 -b 115200 -c 12M -E yourBinaryImage.bin
```
