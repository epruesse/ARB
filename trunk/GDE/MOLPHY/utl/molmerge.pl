#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Jul 06 1994

# Usage: molmerge [sequence_files]

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
	($dinfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)\s*$/ );
	$size = $numsite if !$size;
	die "$file: sequences size is not identical: $numsite != $size\n"
		if ($size != $numsite);
	push(@dinfos, $dinfo) unless grep(/^$dinfo$/, @dinfos);
	#warn "\n$numotu OTUs $numsite sites.\n";
	
	$notu = 0;
	while(<FILE>) { 
		next if /^\s*$/;  # white line skip
		next if /^\s*#/;  # comment line skip
		chop;
		($name) = ( $_ =~ /(\w\S*)/ );
		#print "$loop\t$notu\t$name\n";
		die "abnormal identifier(name): $_\n" unless $name;
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
		$notu++;
		if (grep(/^$name$/,@names)) {
			if ($seqs{$name} eq $seq) {
				warn "$file: Identifier \"$name\" is double defined, removed\n";
				last if ($notu eq $numotu);
				next;
			} else {
				$n = $number + 1;
				warn"$file: Identifier \"$name\" is defined -> $name $n\n";
				$name .= "_$n";
			}
		}
		push(@names,$name);
		$infos{$name} = $info;
		$seqs{$name} = $seq;
		$number++;
		last if ($notu eq $numotu);
	}
	die "only $notu OTUs; fewer OTUs than $numotu.\n" if ( $notu < $numotu );
	$loop++;
}

# output

$datainfo = join(' ', @dinfos) if grep(/\S/,@dinfos);

$numotu = $number;
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
