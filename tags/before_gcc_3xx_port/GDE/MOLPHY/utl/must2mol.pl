#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Jun 13 1994

# Usage: must2mol [sequence_file]

$datainfo = "";

while(<>) { 
	next if /^\s*$/;  # white line skip
	next if /^\s*#/;  # comment line skip
	chop;
	($info) = ( $_ =~ />(\S.*)\s*$/ );
	($name1) = ( $_ =~ />(\w\S*)/ );
	($name2) = ( $_ =~ />\w\S*\s+(\w\S*)/ );
	($name3) = ( $_ =~ />\w\S*\s+\w\S*_(\w\S*)/ );
	$name = substr($name1, 0, 3);
	$name .= substr($name2, 0, 2);
	$name .= substr($name3, 0, 1);
	die "abnormal identifier(name): $_\n" unless $name;
	if (grep(/^$name$/,@names)) {
		$nameold = $name;
		$name .= '2';
		warn "Identifier \"$nameold\" is double defined. --> \"$name\"\n";
	}
	#die "Identifier \"$name\" is double defined\n" if grep(/^$name$/,@names);

	chop($_ = <>);
	s/\s//g;
	$seq = $_;

	$leng = length($seq);
	$numsite = $leng if ($leng > $numsite);
	push(@names,$name);
	$infos{$name} = $info;
	$seqs{$name} = $seq;
}
$numotu = $#names+1;

# output

print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$info = $infos{$name};
	$seq = $seqs{$name};
	$n = $numsite - length($seq);
	for ($i = 0; $i < $n; $i++) { $seq .= '-'; }
	print $info ? "$name $info\n" : "$name\n";
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
