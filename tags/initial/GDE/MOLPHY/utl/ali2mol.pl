#!/usr/local/bin/perl

# Copyright (C) 1993-1995 J. Adachi,  All rights reserved.  Feb 05 1994

# Usage: ali2mol [sequence_file]

chop($_ = <>);  # first line
($numotu,$numsite) = ( $_ =~ /(\d+)\s+(\d+)/ );
($datainfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)$/ );
#warn "\n$numotu OTUs $numsite sites.\n";

for ($i = 0; $i < $numotu; $i++) {
	while(<>) { 
		next if /^\s*$/;  # white line skip
		#next if /^\s*#/;  # comment line skip
		chop;
		s/^\s*//;
		$name = substr($_, 0, 10);
		$name =~ s/\s*//g;
		die "abnormal identifier(name): $_\n" unless $name;
		die "Identifier \"$name\" is double defined\n"
			if grep(/^$name$/,@names);
		$seq = substr($_, 10);
		$seq =~ s/\s//g;
		$leng = length($seq);
		die "$name: abnormal sequence size, $leng sites\n"
			if ($leng > $numsite);
		push(@names,$name);
		$seqs{$name} = $seq;
		#print "$name $seq\n";
		last;
	}
}

while($leng < $numsite) { 
	for ($i = 0; $i < $numotu; $i++) {
		while(<>) { 
			next if /^\s*$/;  # white line skip
			#next if /^\s*#/;  # comment line skip
			chop;
			s/^\s*//;
			$name = substr($_, 0, 10);
			$name =~ s/\s*//g;
			die "Identifier \"$name\" is not defined\n"
				unless grep(/^$name$/,@names);

			$seq = substr($_, 11);
			$seq =~ s/\s//g;
			$seqs{$name} .= $seq;
			$leng = length($seqs{$name});
			#print "$name $leng $seq\n";
			die "$name: abnormal sequence size, $leng sites\n"
				if ($leng > $numsite);
			last;
		}
	}
	#print "$leng\n";
}

# output

print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$seq = $seqs{$name};
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
