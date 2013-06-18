#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Apr 29 1994

# Usage: molcat [sequence_files]

if (!@ARGV) {
	@ARGV = <STDIN>;
	chop(@ARGV);
}

foreach $file (@ARGV) {
	open(FILE, $file) || do { warn "Can't open $file: $!\n"; next; };
	next if -d FILE;
	next if -B FILE;

	chop($_ = <FILE>);  # first line
	($numotu,$numsite) = ( $_ =~ /(\d+)\s+(\d+)/ );
	die "Bad format: $file\n" unless ($numotu && $numsite);
	($dinfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)$/ );
	$size += $numsite;
	$datainfo .= "$dinfo ";
	#warn "\n$numotu OTUs $numsite sites.\n";
	
	$notu = 0;
	while(<FILE>) { 
		next if /^\s*$/;  # white line skip
		next if /^\s*#/;  # comment line skip
		chop;
		($name) = ( $_ =~ /(\w\S*)/ );
		#print "$loop\t$notu\t$name\n";
		die "abnormal identifier(name): $_\n" unless $name;
		if ($loop == 0) {
			die "Identifier \"$name\" is double defined\n"
				if grep(/^$name$/,@names);
		} else {
			die "Identifier \"$name\" is not defined\n"
				unless grep(/^$name$/,@names);
		}
		($info) = ( $_ =~ /\w\S*\s+(\S.*)$/ );
		$seq = "";
		while( length($seq) < $numsite ) {
			chop($_ = <FILE>);
			next if /^\s*$/;  # white line skip
			next if /^\s*#/;  # comment line skip
			s/\s//g;
			$seq .= $_;
		}
		$leng = length($seq);
		die "$name: abnormal sequence size, $leng sites\n" if ($leng != $numsite);
		push(@names,$name) if $loop == 0;
		$infos{$name} = $info if $loop == 0;
		$seqs{$name} .= $seq;
		$notu++;
		last if ($notu eq $numotu);
	}
	die "$file: only $notu OTUs, fewer OTUs than $numotu.\n"
		if ( $notu < $numotu );
	$loop++;
}

# output

$numsite = $size;
print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
foreach $name (@names) {
	$info = $infos{$name};
	$seq = $seqs{$name};
	print $info ? "$name $info\n" : "$name\n";
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
