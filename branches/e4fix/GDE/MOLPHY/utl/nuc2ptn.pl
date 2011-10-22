#!/usr/local/bin/perl

# Copyright (C) 1993-1995 Jun Adachi.  All rights reserved.  Dec 16 1995

# Usage: nuc2ptn [switches] [sequence_file]

%univcode = ( # Universal code
	'TTT', 'F',  'TCT', 'S',  'TAT', 'Y',  'TGT', 'C',
	'TTC', 'F',  'TCC', 'S',  'TAC', 'Y',  'TGC', 'C',
	'TTA', 'L',  'TCA', 'S',  'TAA', '*',  'TGA', '*',
	'TTG', 'L',  'TCG', 'S',  'TAG', '*',  'TGG', 'W',

	'CTT', 'L',  'CCT', 'P',  'CAT', 'H',  'CGT', 'R',
	'CTC', 'L',  'CCC', 'P',  'CAC', 'H',  'CGC', 'R',
	'CTA', 'L',  'CCA', 'P',  'CAA', 'Q',  'CGA', 'R',
	'CTG', 'L',  'CCG', 'P',  'CAG', 'Q',  'CGG', 'R',

	'ATT', 'I',  'ACT', 'T',  'AAT', 'N',  'AGT', 'S',
	'ATC', 'I',  'ACC', 'T',  'AAC', 'N',  'AGC', 'S',
	'ATA', 'I',  'ACA', 'T',  'AAA', 'K',  'AGA', 'R',
	'ATG', 'M',  'ACG', 'T',  'AAG', 'K',  'AGG', 'R',

	'GTT', 'V',  'GCT', 'A',  'GAT', 'D',  'GGT', 'G',
	'GTC', 'V',  'GCC', 'A',  'GAC', 'D',  'GGC', 'G',
	'GTA', 'V',  'GCA', 'A',  'GAA', 'E',  'GGA', 'G',
	'GTG', 'V',  'GCG', 'A',  'GAG', 'E',  'GGG', 'G',

	'---', '-',  '***', '-',
);

%mitcode = ( # Mitochondrial code
	'TTT', 'F',  'TCT', 'S',  'TAT', 'Y',  'TGT', 'C',
	'TTC', 'F',  'TCC', 'S',  'TAC', 'Y',  'TGC', 'C',
	'TTA', 'L',  'TCA', 'S',  'TAA', '*',  'TGA', 'W',
	'TTG', 'L',  'TCG', 'S',  'TAG', '*',  'TGG', 'W',

	'CTT', 'L',  'CCT', 'P',  'CAT', 'H',  'CGT', 'R',
	'CTC', 'L',  'CCC', 'P',  'CAC', 'H',  'CGC', 'R',
	'CTA', 'L',  'CCA', 'P',  'CAA', 'Q',  'CGA', 'R',
	'CTG', 'L',  'CCG', 'P',  'CAG', 'Q',  'CGG', 'R',

	'ATT', 'I',  'ACT', 'T',  'AAT', 'N',  'AGT', 'S',
	'ATC', 'I',  'ACC', 'T',  'AAC', 'N',  'AGC', 'S',
	'ATA', 'M',  'ACA', 'T',  'AAA', 'K',  'AGA', '*',
	'ATG', 'M',  'ACG', 'T',  'AAG', 'K',  'AGG', '*',

	'GTT', 'V',  'GCT', 'A',  'GAT', 'D',  'GGT', 'G',
	'GTC', 'V',  'GCC', 'A',  'GAC', 'D',  'GGC', 'G',
	'GTA', 'V',  'GCA', 'A',  'GAA', 'E',  'GGA', 'G',
	'GTG', 'V',  'GCG', 'A',  'GAG', 'E',  'GGG', 'G',

	'---', '-',  '***', '-',
);

# Process switches.

while ($ARGV[0] =~ /^-/) {
	$_ = shift;
	if (/^-u/) {       # -u : Universal code
		$univ_optn = 1;
		%code = %univcode;
		warn "Universal code.\n";
	} elsif (/^-m/) {  # -m : Mitochondrial code
		$mit_optn = 1;
		%code = %mitcode;
		warn "Mitochondrial code.\n";
	} elsif (/^-1/) {  # -1 : first site is 1st codon.
		$fsite = 0;
		#warn "first site is 1st codon. ( -2 : 2nd, -3 : 3rd )\n";
	} elsif (/^-2/) {  # -2 : first site is 2nd codon.
		$fsite = 1;
		warn "first site is 2nd codon. ( -1 : 1st, -3 : 3rd )\n";
	} elsif (/^-3/) {  # -3 : first site is 3rd codon.
		$fsite = 2;
		warn "first site is 3rd codon. ( -1 : 1st, -2 : 2nd )\n";
	} else {
		warn "-u : Universal code\n";
		warn "-m : Mitochondrial code\n";
		die "Unrecognized switch: $_\n";
	}
}
if (!$univ_optn && !$mit_optn) {
	%code = %univcode;
	warn "Universal code. ( -m : Mitochondrial code )\n";
}
if (!$fsite) {
	$fsite = 0;
	warn "first site is 1st codon. ( -2 : 2nd, -3 : 3rd )\n";
}

chop($_ = <>);  # first line
($numotu,$numsite) = ( $_ =~ /(\d+)\s+(\d+)/ );
($datainfo) = ( $_ =~ /\d+\s+\d+\s+(\S.*)$/ );
#warn "\n$numotu OTUs $numsite sites.\n";
$cds = $numsite - $fsite;
$numami = int($cds / 3);
$nnuc = $fsite + ($cds % 3);
warn "$numami amino acids and $nnuc nucleic acids.\n";


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

# output

print $datainfo ? "$numotu $numami $datainfo\n" : "$numotu $numami\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$info = $infos{$name};
	$seq = substr($seqs{$name}, $fsite);
	$ami = '';
	for ($offset = 0; $codon = substr($seq, $offset, 3); $offset += 3) {
		last if length($codon) < 3;
		($a = $code{$codon}) ? ($ami .= $a) : ($ami .= 'x');
	}
	print $info ? "$name $info\n" : "$name\n";
	#printf("%-3d %-10s %s\n", $num, $name, $info);
	for ($offset = 0; ($line = substr($ami, $offset, 60)); $offset += 60) {
		print "$line\n";
	}
	#print "$seq\n";
}

# other lines

#while(<>) { 
#	s/ +$//;
#	print;
#}
