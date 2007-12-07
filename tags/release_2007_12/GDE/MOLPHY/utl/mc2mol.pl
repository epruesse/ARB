#!/usr/local/bin/perl

# Copyright (C) 1996 J. Adachi,  All rights reserved.  June 07 1996

# Usage: mc2mol [sequence_file]


while(<>) { 
	# print;
	last if /^MATRIX/;
	if (/^DIMENSIONS/) {
		($headotu, $headsite) = 
			( $_ =~ /^DIMENSIONS\s+NTAX\s*=\s*(\d+)\s+NCHAR\s*=\s*(\d+)/ );
	}
}
# print "$headotu $headsite\n";

while(<>) { 
	last if /^;/;
	last if /^END;/;
	next if /^\s*$/;  # white line skip
	next if /^\[/;  # "[" line skip
	if (/^\S/) {
		($name, $seq) = ( $_ =~ /^(\S+)\s+(\S+)/ );
		$seq =~ s/\s//g;
		# print "$name\t$seq\n";
		if (grep(/$name/,@names)) {
			$seqs{$name} .= $seq;
		} else {
			push(@names,$name);
			$numotu++;
			$seqs{$name} = $seq;
		}
	}
}

# check

foreach $name (@names) {
	$seq = $seqs{$name};
	$nsite = length($seq);
	$numsite = $nsite if $nsite gt $numsite;
	#print "$name\t$nsite\n";
}

warn "irregular number of OTUs! $headotu/$numotu\n"  if $headotu ne $numotu;
warn "irregular number of sites! $headsite/$numsite\n" if $headsite ne $numsite;

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
