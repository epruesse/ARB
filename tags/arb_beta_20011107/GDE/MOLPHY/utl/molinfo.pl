#!/usr/local/bin/perl

# Copyright (C) 1993-1995 J. Adachi.  All rights reserved.  Dec 25 1995

# Usage: molinfo [switches] [sequence_file]

# Process switches.

$info_optn = 1;
while ($ARGV[0] =~ /^-/) {
	$_ = shift;
	if (/^-i/) {       # -i : Infomation sites
		$info_optn = 1;
	} elsif (/^-n/) {  # -n : Non infomation sites
		$info_optn = 0;
		warn "Non infomation sites.\n";
	} else {
		warn "-i : Infomation sites\n";
		warn "-n : Non infomation sites\n";
		die "Unrecognized switch: $_\n";
	}
}
warn "Infomation sites. ( -n : Non infomation sites )\n" if $info_optn;

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
		s/\*/\-/g;
		tr/a-z/A-Z/; # capital
		$seq .= $_;
	}
	$leng = length($seq);
	die "$name: abnormal sequence size, $leng sites\n" if ($leng != $numsite);
	push(@names,$name);
	$infos{$name} = $info;
	$seqs2{$name} = $seq;
	last if ( $#names+1 eq $numotu );
}
$notu = $#names+1;
die "only $notu OTUs; fewer OTUs than $numotu.\n" if ( $notu < $numotu );

# info - noinfo

$size = 0;
for ($i = 0; $i < $numsite; $i++) {
	$infonoinfo = 0;
	foreach $name (@names) {
		$ch = substr($seqs2{$name}, $i, 1);
		last if $ch ne '-';
	}
	if ($ch ne '-') {
		foreach $name (@names) {
			$x = substr($seqs2{$name}, $i, 1);
			$infonoinfo = 1 if ($x ne $ch && $x ne '-');
		}
		$infonoinfo = ($info_optn ? $infonoinfo : !$infonoinfo);
	} else {
		$infonoinfo = 0;
	}
	if ($infonoinfo) {
		foreach $name (@names) {
			$seqs{$name} .= substr($seqs2{$name}, $i, 1);
		}
		$size++;
	}
}

# output

$numsite = $size;
print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$info = $infos{$name};
	$seq = $seqs{$name};
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
