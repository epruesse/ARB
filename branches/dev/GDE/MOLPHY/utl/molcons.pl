#!/usr/local/bin/perl

# Copyright (C) 1995 J. Adachi,  All rights reserved.  Mar 31 1995

# Usage: molcons [sequence_file]

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

# manipulate

for ($i = 0; $i < $numsite; $i++) {
	foreach $name (@names) {
		$seq = $seqs{$name};
		$ch = substr($seq, $i, 1);
#		print $ch;
		$color{$ch}++;
	}
	#printf("\n");
	while (($ch, $count) = each %color) {
		#printf(" %s %2d", $ch, $count);
		if ($count > $maxcount && $ch ne "-" && $ch ne "*") {
			$maxcount = $count;
			$maxch = $ch;
		}
	}
	if ($maxcount == 0) { $maxch = '-'; };
#	printf(" max: %s %2d\n", $maxch, $maxcount);
	$conseq .= $maxch;
	$maxcount = 0;
	undef $maxch;
	undef %color;
}
#$conname = "CONSENSUS";
$conname = $names[0];
$conname .= "_$notu";

# output

print $datainfo ? "1 $numsite $datainfo\n" : "1 $numsite\n";
print "$conname\n";
for ($offset = 0; ($line = substr($conseq, $offset, 60)); $offset += 60) {
	print "$line\n";
}

# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
