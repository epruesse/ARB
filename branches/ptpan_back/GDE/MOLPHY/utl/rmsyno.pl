#!/usr/local/bin/perl

# Copyright (C) 1993, 1994 J. Adachi,  All rights reserved.  Feb 05 1994

# Usage: rmsyno [switches] [sequence_file]

%univcode = ( # Universal code
	'TTT', 'TTY',  'TCT', 'TCN',  'TAT', 'TAY',  'TGT', 'TGY',
	'TTC', 'TTY',  'TCC', 'TCN',  'TAC', 'TAY',  'TGC', 'TGY',
	'TTA', 'YTR',  'TCA', 'TCN',  'TAA', '***',  'TGA', '***',
	'TTG', 'YTR',  'TCG', 'TCN',  'TAG', '***',  'TGG', 'TGG',

	'CTT', 'CTN',  'CCT', 'CCN',  'CAT', 'CAY',  'CGT', 'CGN',
	'CTC', 'CTN',  'CCC', 'CCN',  'CAC', 'CAY',  'CGC', 'CGN',
	'CTA', 'YTN',  'CCA', 'CCN',  'CAA', 'CAR',  'CGA', 'MGN',
	'CTG', 'YTN',  'CCG', 'CCN',  'CAG', 'CAR',  'CGG', 'MGN',

	'ATT', 'ATH',  'ACT', 'ACN',  'AAT', 'AAY',  'AGT', 'AGY',
	'ATC', 'ATH',  'ACC', 'ACN',  'AAC', 'AAY',  'AGC', 'AGY',
	'ATA', 'ATH',  'ACA', 'ACN',  'AAA', 'AAR',  'AGA', 'MGR',
	'ATG', 'ATG',  'ACG', 'ACN',  'AAG', 'AAR',  'AGG', 'MGR',

	'GTT', 'GTN',  'GCT', 'GCN',  'GAT', 'GAY',  'GGT', 'GGN',
	'GTC', 'GTN',  'GCC', 'GCN',  'GAC', 'GAY',  'GGC', 'GGN',
	'GTA', 'GTN',  'GCA', 'GCN',  'GAA', 'GAR',  'GGA', 'GGN',
	'GTG', 'GTN',  'GCG', 'GCN',  'GAG', 'GAR',  'GGG', 'GGN',

	'***', '---',  '---', '---',
);

%mitcode = ( # Mitochondrial code
	'TTT', 'TTY',  'TCT', 'TCN',  'TAT', 'TAY',  'TGT', 'TGY',
	'TTC', 'TTY',  'TCC', 'TCN',  'TAC', 'TAY',  'TGC', 'TGY',
	'TTA', 'YTR',  'TCA', 'TCN',  'TAA', '***',  'TGA', 'TGR',
	'TTG', 'YTR',  'TCG', 'TCN',  'TAG', '***',  'TGG', 'TGR',

	'CTT', 'CTN',  'CCT', 'CCN',  'CAT', 'CAY',  'CGT', 'CGN',
	'CTC', 'CTN',  'CCC', 'CCN',  'CAC', 'CAY',  'CGC', 'CGN',
	'CTA', 'YTN',  'CCA', 'CCN',  'CAA', 'CAR',  'CGA', 'CGN',
	'CTG', 'YTN',  'CCG', 'CCN',  'CAG', 'CAR',  'CGG', 'CGN',

	'ATT', 'ATY',  'ACT', 'ACN',  'AAT', 'AAY',  'AGT', 'AGY',
	'ATC', 'ATY',  'ACC', 'ACN',  'AAC', 'AAY',  'AGC', 'AGY',
	'ATA', 'ATR',  'ACA', 'ACN',  'AAA', 'AAR',  'AGA', '***',
	'ATG', 'ATR',  'ACG', 'ACN',  'AAG', 'AAR',  'AGG', '***',

	'GTT', 'GTN',  'GCT', 'GCN',  'GAT', 'GAY',  'GGT', 'GGN',
	'GTC', 'GTN',  'GCC', 'GCN',  'GAC', 'GAY',  'GGC', 'GGN',
	'GTA', 'GTN',  'GCA', 'GCN',  'GAA', 'GAR',  'GGA', 'GGN',
	'GTG', 'GTN',  'GCG', 'GCN',  'GAG', 'GAR',  'GGG', 'GGN',

	'***', '---',  '---', '---',
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

# output

print $datainfo ? "$numotu $numsite $datainfo\n" : "$numotu $numsite\n";
$num = 0;
foreach $name (@names) {
	$num++;
	$info = $infos{$name};
	$seq = $seqs{$name};
	$ami = '';
	for ($offset = 0; $codon = substr($seq, $offset, 3); $offset += 3) {
		($a = $code{$codon}) ? ($ami .= $a) : ($ami .= '???');
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
