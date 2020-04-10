#!/usr/bin/perl
#
# (C) Marc Prager, 2005
#
# generator for C-header file for special function registers of ARM processor
# syntax:
#  sfr name value #comment	: 32bit unsigned
#  sfrh name value #comment	: 16bit unsigned
#  sfrb name value #comment	: 8bit unsigned
#  asm name value #comment	: assembler only, number
#  const name value #comment	: value only, not a memory location

die "usage $0 <file.def>\n", if scalar @ARGV ne 1;

my $defname = $ARGV[0];
my $hname, $sname;

if ($defname =~ m/^(.*)\.def$/) {
	$hname = $1.".h";
	$sname = $1.".s";
}
else {
	die "ERROR: input file must end with .def\n";
}

open DEF,"<$defname" or die "ERROR: cannot open input file: \"$defname\"\n";
open HFILE,">$hname" or die "ERROR: cannot create output file: \"$hname\"\n";
open SFILE,">$sname" or die "ERROR: cannot create output file: \"$sname\"\n";

print HFILE "// generated from: $defname\n";
print SFILE "@ generated from: $defname\n";

my $line = 0;
my $errors = 0;

while (<DEF>) {
	chomp;
	$line++;

	if (m/^\s*(asm|sfr|sfrb|sfrh|const)(\s.*)$/) {
		my $keyword = $1;
		$_ = $2;
		if (m/\s+(\S+)\s+(\S+)(.*)$/) {
			my $symbol = $1;
			my $value = $2;
			my $hcomment = "";
			my $scomment = "";

			$_ = $3;
			if (m/\s*#(.*)$/) {
				$hcomment = " //$1";
				$scomment = " @$1";
			}

			if ($keyword =~ m/sfr(.?)/x) {
				print SFILE "\t.equ $symbol, $value",$scomment,"\n";
				print HFILE "#define $symbol ";
				print HFILE "*(volatile unsigned char*)", if $1 eq "b";
				print HFILE "*(volatile unsigned short*)", if $1 eq "h";
				print HFILE "*(volatile unsigned long*)", if $1 eq "";
				print HFILE "($value)","$comment\n";
			}
			elsif ($keyword =~ "asm") {
				print SFILE "\t.equ $symbol, $value",$scomment,"\n";
			}
			elsif ($keyword =~ "const") {
				print SFILE "\t.equ $symbol, ($value)",$scomment,"\n";
				print HFILE "#define $symbol ($value)",$hcomment,"\n";
			}
			else {
				die "INTERNAL ERROR!";
			}
		}
		else {
			print STDERR "$defname:$line:missing declaration\n";
			$errors++;
		}
	}
	elsif (m/^\s*#(.*)$/) {	# comment only
		print SFILE "@ $1\n";
		print HFILE "// $1\n";
	}
	elsif (m/^$/){
		print SFILE "\n";
		print HFILE "\n";
	}
	else {
		print STDERR "$defname:$line:invalid definition: \"$_\"\n";
		$errors++;
	}

}

#close HFILE;
#close SFILE;

if ($errors ne 0) {
	print STDERR "$errors syntax errors total.\n";
	unlink $hname,$sname;

	exit $1;
}

