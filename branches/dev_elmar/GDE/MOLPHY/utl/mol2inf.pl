#!/usr/local/bin/perl

# Copyright (C) 1993-1995 J. Adachi,  All rights reserved.  Dec 05 1995

# Usage: mol2info [sequence_file]

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

$numotu++;

print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";

foreach $name (@names) {
	$info = $infos{$name};
	print $info ? "$name $info\n" : "$name\n";
}

print "INFO Informaiton of each sites\n";


for ($offset = 0; $offset < $numsite; $offset += 60) {
	$imax = $numsite - $offset;
	$imax = 60 if $imax > 60;
	$infosite = 'o' x $imax;
	print "\n";
	foreach $name (@names) {
		$s = substr($seqs{$name}, $offset, 60);
		print $s, "\n";
		for ($i = 0; $i < $imax; $i++) {
			substr($infosite, $i, 1) = '-' if substr($s, $i, 1) eq '-';
		}
	}
	print $infosite, "\n";
}

# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
