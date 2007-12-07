#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Jun 05 1994

# Usage: seqwrap [file]

while(<>) { 
	next if /^\s*$/;  # white line skip
	next if /^\s*#/;  # comment line skip
	chop;
	$seq = $_;
	for ($offset = 0; ($line = substr($seq, $offset, 60)); $offset += 60) {
		print "$line\n";
	}
}
