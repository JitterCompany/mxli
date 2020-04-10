#!/usr/bin/perl -w
#
while (<>) {
	if (m/external item at '(.*)':/) {
		print "$1\n";
	}
}
