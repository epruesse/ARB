#!/usr/bin/perl -w
use strict;
use diagnostics;

use Maus;

## main section

my $logWanted = 0;


my %options = %{&getOptions};
my $testOptionsRef = intializeTestFields(\ %options);
my %listArgs = ();
$listArgs{"exclude"} = prepareExcludes(\ %options);

foreach my $element(@{$listArgs{"exclude"}}){print $element."\n";}

my $seqSourceRef = \*STDIN;      # default STDIN
my $seqDestRef = \*STDOUT;      # default STDOUT

local $/ = "//\n";		# sequence entry separator in InputFile

if (defined $options{if}){
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
my $result = parseEntry(\$sequenceEntry, $testOptionsRef, \%listArgs, $logWanted);

#
# analyze sequence entry
#

#if ($result != 0) {print $seqDestRef $sequenceEntry;}
print $seqDestRef $sequenceEntry;
}
