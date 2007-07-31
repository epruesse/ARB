#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Jun 27 1994

# Usage: rmid3 [switches] [sequence_file]

# switches:  -v : verbose to stderr, ins/del sites

# Process switches.

while ($ARGV[0] =~ /^-/) {
	$_ = shift;
	if (/^-v/) {       # -v : verbose
		$verbose_optn = 1;
	} else {
		warn "-v : verbose to stderr, ins/del sites\n";
		die "Unrecognized switch: $_\n";
	}
}


chop($_ = <>);  # first line
($numotu,$numsite) = ( $_ =~ /(\d+)\s+(\d+)/ );
($datainfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)$/ );
warn "\n$numotu OTUs $numsite sites.\n";
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
		s/\*/\-/g;
		$seq .= $_;
	}
	$leng = length($seq);
	die "$name: abnormal sequence size, $leng sites\n" if ($leng != $numsite);
	push(@names,$name);
	$infos{$name} = $info;
	$seqs2{$name} = $seq;
	for ($i = 0; $i < $numsite; $i++) {
			$insdel[$i] = 1 if (substr($seq, $i, 1) eq '-');
	}
	last if ( $#names+1 eq $numotu );
}
$notu = $#names+1;
die "only $notu OTUs; fewer OTUs than $numotu.\n" if ( $notu < $numotu );

# remove ins/del

for ($i = 0; $i < $numsite; $i += 3) {
	if ($insdel[$i] != 1 && $insdel[$i+1] != 1 && $insdel[$i+2] != 1) {
		foreach $name (@names) {
			$seqs{$name} .= substr($seqs2{$name}, $i, 3);
		}
		$size += 3;
	} elsif ($verbose_optn) {
		$i1 = $i + 1;
		$c = "";
		foreach $name (@names) {
			$c .= substr($seqs2{$name}, $i, 3);
		}
		warn "$i1\t$c\n";
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
