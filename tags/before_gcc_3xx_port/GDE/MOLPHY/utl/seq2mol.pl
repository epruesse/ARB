#!/usr/local/bin/perl

# Copyright (C) 1996 J. Adachi,  All rights reserved.  Mar 05 1996

# Usage: seq2mol [sequence_file]

# input

chop($_ = <>);  # first line
($numotu,$numsite) = ( $_ =~ /(\d+)\s+(\d+)/ );
($datainfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)$/ );
#warn "\n$numotu OTUs $numsite sites.\n";

while(<>) { 
	next if /^\s*$/;  # white line skip
	chop;
	if (/^\s*#/) {
		($name) = ( $_ =~ /#\s*(\w\S*)/ );
		die "Identifier \"$name\" is not defined\n"
			unless grep(/^$name$/,@names);
		($info) = ( $_ =~ /#\s*\w\S*\s+(\S.*)$/ );
		$infos{$name} = $info;
		# print "$name\t$info\n";
	} else {
		($name) = ( $_ =~ /(\w\S*)/ );
		die "abnormal identifier(name): $_\n" unless $name;
		die "Identifier \"$name\" is double defined\n"
			if grep(/^$name$/,@names);
		($seq) = ( $_ =~ /\w\S*\s+(\S.*)$/ );
		$seq =~ s/\s//g;
		push(@names,$name);
		$seqs{$name} = $seq;
		$leng = length($seq);
		$lengs{$name} = $leng;
		$maxleng = $leng if $leng > $maxleng;
	}
}
$notu = $#names+1;
die "only $notu OTUs; fewer OTUs than $numotu.\n" if ( $notu < $numotu );

# processing

$numsite = $maxleng;

foreach $name (@names) {
	if (length($seqs{$name}) < $maxleng) {
		$diff = $maxleng - length($seqs{$name});
		foreach (1..$diff) {
			$seqs{$name} .= '-';
		}
	}
}

# output

print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$info = $infos{$name};
	print $info ? "$name $info\n" : "$name\n";
	#printf("%-3d %-10s %s\n", $num, $name, $info);
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
