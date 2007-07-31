#!/usr/local/bin/perl

# Copyright (C) 1995 J. Adachi,  All rights reserved.  Dec 14 1995

# Usage: eali2mol [EMBL_ALIGN_file]


while (<>) {
	if (/^ID/) {
		chop;
		($id) = /^ID\s+(\w.*)\s*$/;
		$datainfo .= $id;
	}
	if ($SL) {
		last unless /^SL /;
	} else {
		next unless /^SL /;
		$SL = 1;
	}
	chop;
	($name,$sciname) = ( /^SL\s+(\w+)\s+=\s+(\w+.*)\s*$/ );
	$numotu++;
	$fname = $name if !@names;
	push(@names,$name);
	$scinames{$name} = $sciname;
	#printf("%s\t%s\n", $name, $sciname);
}
warn "\nCan't read identifier(SL)!\n" unless $SL;

while (<>) {
	if (/^SQ/) {
		$SQ = 1;
		last;
	}
}
warn "\nCan't read sequences(SQ)!\n" unless $SQ;

while (<>) {
	last if /^[A-Z][A-Z]/;
	chop;
	next if /^\s*$/;  # white line skip
	if (/^\s*\d+\s*$/) { # number line skip
		($numsite) = /^\s*(\d+)\s*$/;
		next;
	}
	($name) = /\s+(\w+)/;
	($seq)  = /\s+\w+     (.*)$/;
	$seq =~ s/ /-/g;
	$seq =~ s/\*/-/g;
	#$seq =~ tr/ACGT/acgt/;
	die "ERROR: Identifier \"$name\" is not defined\n"
		unless grep(/^$name$/,@names);
	if ($name eq $fname) {
		$fseq = $seq;
		$sites = length($fseq);
	} else {
		for ($i = 0; $i < $sites; $i++) {
			if (($c = substr($seq, $i, 1)) eq '.') {
				$nseq .= substr($fseq, $i, 1);
			} else {
				$nseq .= $c;
			}
		}
		if (($n = $sites - length($nseq)) > 0) {
			for ($i = 0; $i < $n; $i++) {
				$nseq .= '-';
			}
		} elsif ($n < 0) {
			die "ERROR: seqence of $name is too long!\n";
		}
		$seq = $nseq;
		$nseq = '';
	}
	$seqs{$name} .= $seq;
	#printf("%s\t%s\n", $name, $seq);
}

# output

print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$sciname = $scinames{$name};
	print $sciname ? "$name $sciname\n" : "$name\n";
	$seq = $seqs{$name};
	for ($offset = 0; ($line = substr($seq, $offset, 60)); $offset += 60) {
		print "$line\n";
	}
	#print "$seq\n";
}

# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
