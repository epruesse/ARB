#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Feb 05 1994

# Usage: rmleu [switches] [sequence_file]

%code = ( # Leu codons
	'TTT', 'F',  'TCT', 'S',  'TAT', 'Y',  'TGT', 'C',
	'TTC', 'F',  'TCC', 'S',  'TAC', 'Y',  'TGC', 'C',
	'TTA', '',   'TCA', 'S',  'TAA', '*',  'TGA', 'W',
	'TTG', '',   'TCG', 'S',  'TAG', '*',  'TGG', 'W',

	'CTT', '',   'CCT', 'P',  'CAT', 'H',  'CGT', 'R',
	'CTC', '',   'CCC', 'P',  'CAC', 'H',  'CGC', 'R',
	'CTA', '',   'CCA', 'P',  'CAA', 'Q',  'CGA', 'R',
	'CTG', '',   'CCG', 'P',  'CAG', 'Q',  'CGG', 'R',

	'ATT', 'I',  'ACT', 'T',  'AAT', 'N',  'AGT', 'S',
	'ATC', 'I',  'ACC', 'T',  'AAC', 'N',  'AGC', 'S',
	'ATA', 'I',  'ACA', 'T',  'AAA', 'N',  'AGA', 'S',
	'ATG', 'M',  'ACG', 'T',  'AAG', 'K',  'AGG', 'S',

	'GTT', 'V',  'GCT', 'A',  'GAT', 'D',  'GGT', 'G',
	'GTC', 'V',  'GCC', 'A',  'GAC', 'D',  'GGC', 'G',
	'GTA', 'V',  'GCA', 'A',  'GAA', 'E',  'GGA', 'G',
	'GTG', 'V',  'GCG', 'A',  'GAG', 'E',  'GGG', 'G',

	'---', '-',  '***', '*',
);

# Process switches.

$codon = 0;
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

# Leu codons

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
