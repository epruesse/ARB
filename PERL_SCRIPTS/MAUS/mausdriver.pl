#!/usr/bin/perl
use strict;
use diagnostics;

use Maus;





## main section

my $logWanted = 0;


my %options = %{&getOptions};
my $testOptionsRef = intializeTestFields(\ %options);

# combined preparation of exclude accession numbers and selected accession number in initialization step
# introduction of excl and acpt instead of exfl, exff and acfl, acff

my $seqSourceRef = \*STDIN;      # default STDIN
my $seqDestRef = \*STDOUT;      # default STDOUT

local $/ = "//\n";		# sequence entry separator in InputFile

if (not defined $options{if}){
  if (not exists $options{force}){die "You specified no input file. To read form STDIN use -force.\n";}
}else{
  open (SEQ_SOURCE , "< $options{if}") or die "cannot open sequence file $options{if} to read\n";
  $seqSourceRef = \*SEQ_SOURCE;
}
if (defined $options{of}){
  open (SEQ_DEST , "> $options{of}") or die "cannot open sequence file $options{of} to write\n";
  $seqDestRef = \*SEQ_DEST;
}


my $sequenceEntry = "";
while ($sequenceEntry = <$seqSourceRef>){
#print $sequenceEntry."\n";
my $result = parseEntry(\$sequenceEntry, $testOptionsRef,$logWanted);

#
# analyze sequence entry
#

if ($result != 0) {print  "sequence accepted\n";}else{print "sequence rejected\n";}
print $seqDestRef $sequenceEntry;
}
