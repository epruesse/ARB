#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Feb 05 1994

# Usage: molcut [sequence_file]

chop($_ = <>);  # first line
($numotu,$numsite) = ( $_ =~ /(\d+)\s+(\d+)/ );
($datainfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)$/ );
#warn "\n$numotu OTUs $numsite sites.\n";

while(<>) { 
	next if /^\s*$/;  # white line skip
	next if /^\s*#/;  # comment line skip
	chop;
	($name) = ( $_ =~ /(\w\S*)/ );
	die "abnormal identifier(name): $_\n" unless $name;
	die "Identifier \"$name\" is double defined\n" if grep(/^$name$/,@names);
	($info) = ( $_ =~ /\w\S*\s+(\S.*)$/ );
	$seq = "";
	while( length($seq) < $numsite ) {
		chop($_ = <>);
		next if /^\s*$/;  # white line skip
		next if /^\s*#/;  # comment line skip
		s/\s//g;
		$seq .= $_;
	}
	$leng = length($seq);
	die "$name: abnormal sequence size, $leng sites\n" if ($leng != $numsite);
	push(@names,$name);
	$infos{$name} = $info;
	$seqs{$name} = $seq;
	last if ( $#names+1 eq $numotu );
}
$notu = $#names+1;
die "only $notu OTUs; fewer OTUs than $numotu.\n" if ( $notu < $numotu );

# output

print "$numotu OTUs $numsite sites.  ( terminate: \"quit\" )\n";
while () {
	print " OUTPUT FILE NAME: "; chop($nfile = <STDIN>);
	last if $nfile eq "quit";
	open (NFILE, ">$nfile") || die "Can't open $nfile: $!\n";
	print "start site number:  "; chop($snum = <STDIN>);
	redo if $snum eq "quit";
	redo if $snum > $numsite;
	print "  end site number:  "; chop($enum = <STDIN>);
	redo if $enum eq "quit";
	redo if $enum < $snum;
	$start = $snum - 1;
	$enum = $numsite if $enum > $numsite;
	$size = $enum - $start;

	print NFILE $datainfo ? "$numotu $size $datainfo" : "$numotu $size";
	print NFILE " $snum-$enum\n";
	$num = 0;
	foreach $name (@names) {
		$num++;
		$info = $infos{$name};
		$seq = substr($seqs{$name}, $start, $size);
		print NFILE $info ? "$name $info\n" : "$name\n";
		for ($offset = 0; ($line = substr($seq, $offset, 60)); $offset += 60) {
			print NFILE "$line\n";
		}
		#print "$seq\n";
	}

	close NFILE;
}

# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
