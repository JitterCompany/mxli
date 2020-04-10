#!/usr/bin/perl -w -i.bak
#

#my $packed = "__attribute__((packed))";
my $packed = "PACKED";
my $unionized = "unionized";
my $state = 0;		# 0 = scan, 1 = address detected
my $dummy = 0;

while (<>) {
	# get indent
	if (m/([ \t]*)/) {
		$indent = $1;
	}
	else {
		$indent = "";
	}

	print, unless (m/[ \t]*$unionized .*/);	# removes generated lines
}

