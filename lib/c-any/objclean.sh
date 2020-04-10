#!/bin/sh
# removes all objects, that have no source file

for o in *.o; do
	if [ "$o" != "*.o" ] ; then
		C=${o%.o}.c
		S=${o%.o}.s
		if [ -f "$c" -o -f "$s" ] ; then
			echo "keep it: $o"
		else
			echo "rm $o"
		fi
	fi
done

