#!/usr/local/bin/perl

# Copyright (C) 1996 J. Adachi,  All rights reserved.  Feb 05 1996

# Usage: clus2mol [sequence_file]

<>;  # skip first line
chop($_ = <>);  # second line
($datainfo) = ( $_ =~ /(\S.*)\s*$/ );

while(<>) { 
	next if /^\s*$/;  # white line skip
	chop;
	if (/^\S/) {
		($name) = ( $_ =~ /^(\S+)/ );
		($seq) = ( $_ =~ /^\S+\s+(\S.*)$/ );
		$seq =~ s/\s//g;
		#print "$name\t$seq\n";
		if (grep(/$name/,@names)) {
			$seqs{$name} .= $seq;
		} else {
			push(@names,$name);
			$numotu++;
			$seqs{$name} = $seq;
		}
	#} else {
	#	$_ =~ s/\s//g;
	#	$seq = $_;
	#	#print "$seq\n";
	#	$seqs{$name} .= $seq;
	}
}

# check

foreach $name (@names) {
	$seq = $seqs{$name};
	$nsite = length($seq);
	$numsite = $nsite if $nsite gt $numsite;
	#print "$name\t$nsite\n";
}

# output

print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$seq = $seqs{$name};
	warn "$name: $nsite sites!\n" if length($seq) ne $numsite;
	print "$name\n";
	#printf("%-3d %-10s %s\n", $num, $name, $info);
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
