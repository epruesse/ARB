#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  May 06 1994

# Usage: degene4l [switches] [sequence_file]

%code = ( # 4fold-degenerate codons
	'TTT', '',   'TCT', '',  'TAT', '',  'TGT', '',
	'TTC', '',   'TCC', '',  'TAC', '',  'TGC', '',
	'TTA', 'L',  'TCA', '',  'TAA', '',  'TGA', '',
	'TTG', 'L',  'TCG', '',  'TAG', '',  'TGG', '',

	'CTT', '',   'CCT', '',  'CAT', '',  'CGT', '',
	'CTC', '',   'CCC', '',  'CAC', '',  'CGC', '',
	'CTA', 'L',  'CCA', '',  'CAA', '',  'CGA', '',
	'CTG', 'L',  'CCG', '',  'CAG', '',  'CGG', '',

	'ATT', '',   'ACT', '',  'AAT', '',  'AGT', '',
	'ATC', '',   'ACC', '',  'AAC', '',  'AGC', '',
	'ATA', '',   'ACA', '',  'AAA', '',  'AGA', '',
	'ATG', '',   'ACG', '',  'AAG', '',  'AGG', '',

	'GTT', '',   'GCT', '',  'GAT', '',  'GGT', '',
	'GTC', '',   'GCC', '',  'GAC', '',  'GGC', '',
	'GTA', '',   'GCA', '',  'GAA', '',  'GGA', '',
	'GTG', '',   'GCG', '',  'GAG', '',  'GGG', '',

	'---', '',   '***', '',
);

# Process switches.

$codon = 0; # /* Leu */
$res = 1;
while ($ARGV[0] =~ /^-/) {
	$_ = shift;
	if (/^-t/) {       # -t : triplet (codon)
		$codon = 0;
		$res = 3;
		warn "triplet (codon).\n";
	} else {
		warn "-t : triplet (codon)\n";
		die "Unrecognized switch: $_\n";
	}
}

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
		tr/a-z/A-Z/;
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

# 4fold-degenerate codons

for ($i = 0, $n = 0; $i < $numsite; $i += 3) {
	if ($ch = $code{substr($seqs{$names[0]}, $i, 3)}) {
		$size++;
		$reduc[$n] = 1;
		foreach $name (@names) {
			if (!$code{substr($seqs{$name}, $i, 3)}) {
				$size--;
				$reduc[$n] = 0;
				last;
			}
		}
	}
	$n++;
}

# output

($res == 1) ? ($numsite = $size) : ($numsite = $size * 3);
print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
foreach $name (@names) {
	$info = $infos{$name};
	$seq = $seqs{$name};
	$seq2 = '';
	for ($ofst = $codon, $n = 0; $third = substr($seq, $ofst, $res); $ofst += 3, $n++) {
		$seq2 .= $third if $reduc[$n];
	}
	print $info ? "$name $info\n" : "$name\n";
	for ($ofst = 0; ($line = substr($seq2, $ofst, 60)); $ofst += 60) {
		print "$line\n";
	}
	#print "$seq\n";
}

# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
