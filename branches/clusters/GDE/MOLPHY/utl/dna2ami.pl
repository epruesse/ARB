#!/usr/local/bin/perl

# Usage: dna2ami [options] [no_align_files]

# Process switches.

while ($ARGV[0] =~ /^-/) {
	$_ = shift;
	if (/^-u/) {
		$univ_optn = 1;
	} elsif (/^-m/) {
		$mt_optn = 1;
	} else {
		die "Unrecognized switch: $_\n";
	}
}


while(<>) { 
	next if /^\s*$/;
	next if /^#/;
	chop;
	($name,$site) = ( $_ =~ /(\w+)\s+(\d+)/ );
	($info) = ( $_ =~ /\w+\s+\d+\s+(\w.*)/ );
#	warn $name," ",$site," ",$info,"\n";
	warn $name,": abnormal sequence size.\n" if ($site % 3);
	$seq = "";
	while( length($seq) < $site ) {
		chop($_ = <>);
		next if /^\s*$/;
		next if /^#/;
		s/\s//g;
		tr/a-z/A-Z/;
		$seq = $seq . $_;
	}
	die "abnormal sequence size.\n" if (length($seq) != $site);
	push(@names,$name);
	$sites{$name} = $site;
	$infos{$name} = $info;
	$seqs{$name} = $seq;
}

%mtcode = (
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

	'***', '-',  '---', '-',
);

%univcode = (
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

	'***', '-',  '---', '-',
);

if ($univ_optn) {
	%code = %univcode;
	warn "univ code.\n";
} elsif ($mt_optn) {
	%code = %mtcode;
	warn "mt code.\n";
} else {
	%code = %univcode;;
	warn "univ code.\n";
}

$numotu = $#names+1;
print $numotu,"\n";
foreach $name (@names) {
	$seq = $seqs{$name};
	$ami = '';
	$offset = 0;
	while ($codon = substr($seq, $offset, 3)) {
		if ($a = $code{$codon}) {
			$ami .= $a;
		} else {
			$ami .= '?';
		}
		$offset += 3;
	}
	$size = $sites{$name} / 3;
	print $name," ",$size;
	print "  ",$infos{$name} if $infos{$name};
	print "\n";
	$offset = 0;
	while ($line = substr($ami, $offset, 60)) {
		print $line, "\n";
		$offset += 60;
	}
}
