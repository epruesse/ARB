#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Jun 05 1994

# Usage: disjoint [sequence_file]

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

$num = 0;
foreach $name (@names) {
	open(FILENAME, "> $name");
	print FILENAME $datainfo ? "1 $numsite $datainfo\n" : "1 $numsite\n";
	$num++;
	$info = $infos{$name};
	$seq = $seqs{$name};
	print FILENAME $info ? "$name $info\n" : "$name\n";
	#printf("%-3d %-10s %s\n", $num, $name, $info);
	for ($offset = 0; ($line = substr($seq, $offset, 60)); $offset += 60) {
		print FILENAME "$line\n";
	}
	#print "$seq\n";
}

# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
