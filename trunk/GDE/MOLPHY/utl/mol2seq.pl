#!/usr/local/bin/perl

# Copyright (C) 1996 J. Adachi,  All rights reserved.  Mar 05 1996

# Usage: mol2seq [sequence_file]

# Process switches.

while ($ARGV[0] =~ /^-/) {
	$_ = shift;
	if (/^-c/) {       # -c : coding region
		$code = 1;
	} else {
		die "Unrecognized switch: $_\n";
	}
}

# input

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

print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";

foreach $name (@names) {
	printf("%-10s ", $name);
	$seq = $seqs{$name};
	if ($code) {
		for ($offset = 0; ($codon = substr($seq, $offset, 3)); $offset += 3) {
			print "$codon";
			if (($offset + 3) == $numsite) {
				print "\n";
			} else {
				print " ";
			}
		}
	} else {
		print "$seq\n";
	}
}

foreach $name (@names) {
	$info = $infos{$name};
	print $info ? "# $name $info\n" : "#$name\n";
}


# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
