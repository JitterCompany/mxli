#!/bin/sh

TARGET="../libc-any.a"

CPP="cpp"
CPPFLAGS="-I../c-linux -I../c-any -I../module"
SOURCES=""
OBJECTS=""

for f in *.c; do
	SOURCES="$SOURCES $f"
	O=${f%.c}.o
	OBJECTS="${OBJECTS} $O"
done

#echo $SOURCES
#echo $OBJECTS
for s in $SOURCES; do
	$CPP $CPPFLAGS -MT"$s" -MM $s | unslash.pl
done
#make -j4 ${OBJECTS}
#make fast
