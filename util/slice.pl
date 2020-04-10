#!/usr/bin/perl -w
#
# (C) Marc Prager, 2006, 2016
# 2016: output file is overwritten only if it's different.

use File::Compare;
use File::Compare 'cmp';	# get Unix name
#use File::Copy;
#use File::Copy qw/cp mv/;	# get normal Unix names

if (scalar @ARGV ==1 && ($ARGV[0] eq "-h" || $ARGV[0] eq "--help")) {
	print "usage: ",$0," <filename-sliced.*>\n";
	exit 1;
}

for my $inputName (@ARGV) {
	my $state_SEARCH = 0;
	my $state_HEADER = 1;
	my $state_SLICE = 2;
	my $state_FOOTER = 3;

	my $state = $state_SEARCH;
	my @header = ();
	my @footer = ();

	$inputName =~ m/(.*)-sliced(\.(s|c|C|cpp))$/ or die "ERROR: input file \"".$inputName."\" does not match \"*-sliced.(s|c|C|cpp)\"\n";

	my $baseName = $1;
	my $baseExt = $2;
	my $prelimExt = "-prelim";

	# first pass: find header and footer.
	# 
	open INPUT,"<".$inputName or die "ERROR: cannot open \"".$inputName."\"\n";
	while(<INPUT>) {
		if (m/^\/\/HEADER/ || m/^\@\@HEADER/) {
			$state = $state_HEADER;
		}
		elsif (m/^\/\/SLICE/ || m/^\@\@SLICE/) {
			$state = $state_SLICE;
		}
		elsif (m/^\/\/FOOTER/ || m/^\@\@FOOTER/) {
			$state = $state_FOOTER;
		}
		else {
			@header = (@header,$_), if $state==$state_HEADER;
			@footer = (@footer,$_), if $state==$state_FOOTER;
		}
	}
	close INPUT;

	# second pass: generate slices
	# 
	my $slice = 0;
	my $line;
	$state = $state_SEARCH;
	my $sliceName;
	my $sliceNamePrelim;
	
	open INPUT,"<".$inputName or die "ERROR: cannot open \"".$inputName."\"\n";
	while (<INPUT>) {
		if (m/^\/\/HEADER/ || m/^\@\@HEADER/) {
			$state = $state_HEADER;
		}
		elsif (m/^\/\/FOOTER/ || m/^\@\@FOOTER/) {
			$state = $state_FOOTER;
		}
		elsif (m/^\/\/SLICE/ || m/^\@\@SLICE/) {
			if ($slice > 0) {
				for $line (@footer) {
					print OUTPUT $line;
				}
				close OUTPUT;
				# now check, if these are different
				if (compare ($sliceName,$sliceNamePrelim)) {
					print STDERR "changed:$sliceName\n";
					rename ($sliceNamePrelim,$sliceName) or die "cannot rename $sliceNamePrelim to $sliceName\n";
				}
				else {
					unlink $sliceNamePrelim or die "cannot remove $sliceNamePrelim\n";
				}
				# in any case: update time stamp
				(-1!=system ("touch","-r$inputName","$sliceName")) or die "cannot touch $sliceName\n";	# time stamp of the slices' source
			}
			$slice++;
			$sliceName = sprintf "_slice-%s-%02d%s", $baseName, $slice, $baseExt;
			$sliceNamePrelim = "$sliceName$prelimExt";
			# print "$sliceName\n";
	
			open OUTPUT,">".$sliceNamePrelim or die "ERROR: cannot create \"".$sliceNamePrelim."\"\n";
			$state = $state_SLICE;

			# copy header
			for $line (@header) {
				print OUTPUT $line;
			}
		}
		else {
			print OUTPUT $_, if $state==$state_SLICE;
		}
	}
	close INPUT;

	# append to last slice
	if ($slice > 0) {
		for $line (@footer) {
			print OUTPUT $line;
		}
		close OUTPUT;
		# now check, if these are different
		if (compare ($sliceName,$sliceNamePrelim)) {
			print STDERR "changed:$sliceName\n";
			rename ($sliceNamePrelim,$sliceName) or die "cannot rename $sliceNamePrelim to $sliceName\n";
		}
		else {
			unlink $sliceNamePrelim or die "cannot remove $sliceNamePrelim\n";
		}
		# in any case: update time stamp
		(-1!=system ("touch","-r$inputName","$sliceName")) or die "cannot touch $sliceName\n";	# time stamp of the slices' source
	}

	# remove excess slices from previous runs
	do {
		$slice++;
		$sliceName = sprintf "_slice-%s-%02d%s", $baseName, $slice, $baseExt;
	} while (unlink $sliceName);
}
