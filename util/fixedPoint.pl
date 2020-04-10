#!/usr/bin/perl -w -i
# fixed point source code constants converter (in-place edit!)
# include /*1.00_e20*/ into your source code and get 0x00100000/*1.00_e20*/ .
# include /*1.00_q12.20*/ into your source code and get 0x00100000/*1.00_q12.20*/ .
# include /*1.00_E63*/ into your source code and get 0x8000000000000000llu/*1.00_E63*/ .
#
# Capital letters Q/E produce 64-bit integers, q/e produce 32-bit integers.
# The modifications are done by in-place substitution on your working file.
# A 64-bit host system (Perl) is assumed.
# For now, DON'T mix the styles - the script cannot handle more than one style per line.

scalar(@ARGV)==1 or die "Usage: fixedPoint.pl <file.c>\n";

while (<>) {
	chomp;
	while ($_) {
		# low greediness initial match .*?
		if (m/^(.*?)(0x[0-9-a-fA-F]+)?(\/\*([+-]?\d*\.?\d*)_q(\d*).(\d*)\*\/)(.*)/) {
			my $prefix=$1;
			my $f=$4;
			my $qf=$6;
			my $qi=$5;
			my $postfix=$7;
			my $source=$3;
			print "$prefix";
			printf "0x%08x",int($f*(1<<$qf) & (1<<$qi+$qf)-1);
			print $source;
			$_ = $postfix;
		}
		elsif (m/^(.*?)(0x[0-9-a-fA-F]+)?(\/\*([+-]?\d*\.?\d*)_e(\d+)\*\/)(.*)/) {
			my $prefix=$1;
			my $f=$4;
			my $qf=$5;
			my $postfix=$6;
			my $source=$3;
			print "$prefix";
			printf "0x%08x",int($f*(1<<$qf) & 0xFFFFFFFF);
			print $source;
			$_ = $postfix;
		}
# doesn't work for some God-damn reason...
#		elsif (m/^(.*?)(0x[0-9-a-fA-F]+)?(\/\*([+-]?\d*\.?\d*)_Q(\d*).(\d*)\*\/)(.*)/) {
#			my $prefix=$1;
#			my $f=$4;
#			my $qf=$6;
#			my $qi=$5;
#			my $postfix=$7;
#			my $source=$3;
#			print "$prefix";
#			printf "0x%016xll",int($f*(1<<$qf) & (1<<$qi+$qf)-1);
#			print $source;
#			$_ = $postfix;
#		}
#		elsif (m/^(.*?)(0x[0-9-a-fA-F]+)?(\/\*([+-]?\d*\.?\d*)_E(\d+)\*\/)(.*)/) {
#			my $prefix=$1;
#			my $f=$4;
#			my $qf=$5;
#			my $postfix=$6;
#			my $source=$3;
#			print "$prefix";
#			printf "0x%016xll",int($f*(1<<$qf));
#			print $source;
#			$_ = $postfix;
#		}
		else {
			print;
			$_ = "";
		}
	}
	print "\n";
}

