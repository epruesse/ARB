#!/usr/local/bin/perl

# Copyright (C) 1993-1995 J. Adachi,  All rights reserved.  Dec 05 1995

# Usage: info2mol [sequence_file]

chop($_ = <>);  # first line
($numotu,$numsite) = ( $_ =~ /(\d+)\s+(\d+)/ );
($datainfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)$/ );
#warn "\n$numotu OTUs $numsite sites.\n";

for ($i = 0; $i < $numotu; $i++) {
	while(<>) { 
		next if /^\s*$/;  # white line skip
		#next if /^\s*#/;  # comment line skip
		chop;
		($name) = ( $_ =~ /(\w\S*)/ );
		die "abnormal identifier(name): $_\n" unless $name;
		die "Identifier \"$name\" is double defined\n" if grep(/^$name$/,@names);
		($info) = ( $_ =~ /\w\S*\s+(\S.*)$/ );
		push(@names,$name);
		$infos{$name} = $info;
		last;
	}
}

while($leng < $numsite) { 
	for ($i = 0; $i < $numotu; $i++) {
		while(<>) { 
			next if /^\s*$/;  # white line skip
			#next if /^\s*#/;  # comment line skip
			chop;
			s/\s//g;
			$seq = $_;
			$seqs{$names[$i]} .= $seq;
			$leng = length($seqs{$names[$i]});
			#print "$names[$i] $leng $seq\n";
			die "$names[$i]: abnormal sequence size, $leng sites\n"
				if ($leng > $numsite);
			last;
		}
	}
	#print "$leng\n";
}

# sequences manipulation

$numotu2 = $numotu - 1;
$infosite = $seqs{"INFO"};
$infosite =~ tr/T/F/;
$infosite =~ tr/o+/TT/;
$infoonly = $infosite;
$infoonly =~ tr/T//cd;
$numsite2 = length($infoonly);
#print "$infosite\n";
#print "$infoonly\n";

foreach $name (@names) {
	next if $name eq "INFO";
	$seq = $seqs{$name};
	$seq2 = '';
	for ($i = 0; $i < $numsite; $i++) {
		$seq2 .= substr($seq, $i, 1) if substr($infosite, $i, 1) eq 'T';
	}
	#print "$seq2\n";
	$seqs2{$name} = $seq2;
}

# output


print $datainfo ? "$numotu2 $numsite2 $datainfo\n" : "$numotu2 $numsite2\n";
$num = 0;
foreach $name (@names) {
	next if $name eq "INFO";
	$num++;
	$info = $infos{$name};
	print $info ? "$name $info\n" : "$name\n";
	#printf("%-3d %-10s %s\n", $num, $name, $info);
	$seq = $seqs2{$name};
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
