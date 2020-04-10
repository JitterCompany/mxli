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

	next, if (m/[ \t]*$unionized .*/);	# removes generated lines

	print;
	if ($state==1) {
		print "$1$unionized };\n";
		$state=0;
	}

	# address detected
	if (m/([ \t]*)\/\/\.(0x[0-9a-fA-F]{2,8})/) {
		print "$1$unionized struct $packed { char _${dummy}[$2]; // padding\n";
		$dummy++;
		$state = 1;
		next;
	}

}

