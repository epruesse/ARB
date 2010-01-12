#!/usr/bin/perl

use warnings;
use strict;

my $line = undef;
my $titleline = undef;
my $pdb_id = undef;
my $header = undef;
my $compnd = undef;
my $source = undef;
my $date = undef;
my $author = undef;
my $reference = undef;
my @chains = (' ');
my @sequences = ('');
my @secstructs = ('');
my $seqnum = 0;
my $mode = 0;

foreach $line (<STDIN>) {
  chomp($line);
  if ($mode==0) {
    if ($line =~ /^(==== Secondary Structure Definition.+)\s+\.$/) {
      $titleline = $1;
    }
    elsif ($line =~ /^REFERENCE\s+(.+);*\s+\.$/) {
      $reference = $1;
      $reference =~ s/(;*\s*$)//; # remove semicolon and whitespace at the end (if present)
    }
    elsif ($line =~ /^HEADER\s+(.+)\s+(\d\d-\w\w\w-\d\d)\s+(\w{4})\s+\.$/) {
      $header = $1;
      $date = $2;
      $pdb_id = $3;
      $header =~ s/(;*\s*$)//;  # remove semicolon and whitespace at the end (if present)
    }
    elsif ($line =~ /^COMPND\s+\d?\s+(.+);*\s+\.$/) {
      $compnd = $1;
      $compnd =~ s/(;*\s*$)//;  # remove semicolon and whitespace at the end (if present)
    }
    elsif ($line =~ /^SOURCE\s+\d?\s+(.+);*\s+\.$/) {
      $source = $1;
      $source =~ s/(;*\s*$)//;  # remove semicolon and whitespace at the end (if present)
    }
    elsif ($line =~ /^AUTHOR\s+(.+);*\s+\.$/) {
      $author = $1;
      $author =~ s/(;*\s*$)//;  # remove semicolon and whitespace at the end (if present)
      $mode++;
    }
  }
  elsif ($mode==1) {
    if ($line =~ /RESIDUE AA/) {
      $mode++;
    }
  }
  elsif ($mode==2) {
    if ($line =~ /^.{11}(.)\s([A-Z!])..([A-Z\s])/io) {
      if ($2 eq '!') {          # chain break encountered (-> start new protein sequence)
        $seqnum++;
        $sequences[$seqnum] = '';
        $secstructs[$seqnum] = '';
        $chains[$seqnum] = ' ';
      }
      else {                    # append protein sequence and secondary structure
        $sequences[$seqnum] .= $2;
        $secstructs[$seqnum] .= $3 eq ' ' ? '-' : $3;
        if ($1 ne $chains[$seqnum]) {
          $chains[$seqnum] = $1;
        }
      }
    }
    else {
      die "Can't parse '$line'";
    }
  }
}

if (not defined $titleline) { die "Could not find title line"; }
if (not defined $pdb_id) { die "Could not extract PDB_ID entry from HEADER"; }
if (not defined $header) { die "Could not find HEADER entry"; }
if (not defined $compnd) { die "Could not find COMPND entry"; }
if (not defined $source) { die "Could not find SOURCE entry"; }
if (not defined $date) { die "Could not extract DATE entry from HEADER"; }
if (not defined $author) { die "Could not find AUTHOR entry"; }
if (not defined $reference) { die "Could not find REFERENCE entry"; }

if ($mode!=2) { die "Unknown parse error"; }

for (my $i = 0; $i <= $seqnum; $i++) {
  print "$titleline\n";
  print "REFERENCE $reference\n";
  if ($chains[$i] ne ' ') {
    print "PDB_ID    $pdb_id\_$chains[$i]\n";
  }
  else {
    print "PDB_ID    $pdb_id\n";
  }
  print "DATE      $date\n";
  print "HEADER    $header\n";
  print "COMPND    $compnd\n";
  print "SOURCE    $source\n";
  print "AUTHOR    $author\n";
  print "SECSTRUCT $secstructs[$i]\n";
  print "SEQUENCE\n$sequences[$i]\n";
}
