#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Feb 26 1994

# Usage: molcodon [sequence_file]

# Process switches.

while ($ARGV[0] =~ /^-/) {
	$_ = shift;
	if (/^-1/) {       # -u : 1st codon position
		$postn = 1;
		$addinfo = "1st";
		warn "1st codon position.\n";
	} elsif (/^-2/) {  # -m : 2nd codon position
		$postn = 2;
		$addinfo = "2nd";
		warn "2nd codon position.\n";
	} elsif (/^-3/) {  # -m : 3rd codon position
		$postn = 3;
		$addinfo = "3rd";
		warn "3rd codon position.\n";
	} else {
		warn "-1 : 1st codon position.\n";
		warn "-2 : 2nd codon position.\n";
		warn "-3 : 3rd codon position.\n";
		die "Unrecognized switch: $_\n";
	}
}
if (!$postn) { # default
	$postn = 3;
	$addinfo = "3rd";
	warn "3rd codon position. ( -1 : 1st;  -2 : 2nd )\n";
}
$postn--;

chop($_ = <>);  # first line
($numotu,$numsite) = ( $_ =~ /(\d+)\s+(\d+)/ );
($datainfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)$/ );
#warn "\n$numotu OTUs $numsite sites.\n";
die "abnormal sequence size.\n" unless ($numsite % 3) eq 0;

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

$datainfo ? ($datainfo .= " $addinfo") : ($datainfo = $addinfo);

# output

$numsite /= 3;
print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$info = $infos{$name};
	$seq = $seqs{$name};
	$seq2 = "";
	for ($offset = $postn; ($xcodon = substr($seq, $offset, 1)); $offset += 3) {
		$seq2 .= $xcodon;
	}
	#$leng = length($seq2);
	#warn "$numsite - $leng\n";
	print $info ? "$name $info\n" : "$name\n";
	for ($offset = 0; ($line = substr($seq2, $offset, 60)); $offset += 60) {
		print "$line\n";
	}
	#print "$seq\n";
}

# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
