#!/usr/bin/perl

use strict;
use warnings;

sub getContent($\%) {
  my ($fname,$content_r) = @_;

  my $dups_removed = 0;
  open(LIST,'<'.$fname) || die "can't read '$fname' (Reason: $!)";
  my $line;
  while (defined($line=<LIST>)) {
    # remove whitespace:
    chomp($line);
    $line =~ s/^\s+//og;
    $line =~ s/\s+$//og;

    if ($line ne '') {
      if (defined $$content_r{$line}) { $dups_removed++; }
      $$content_r{$line} = 1;
    }
  }
  close(LIST);
  if ($dups_removed>0) {
    print "Warning: $dups_removed duplicate lines were ignored (while reading $fname)\n";
  }
}

sub saveContent(\%$) {
  my ($content_r, $fname) = @_;

  open(LIST, '>'.$fname) || die "can't write to '$fname' (Reason: $!)";
  my @lines = sort keys %$content_r;
  foreach (@lines) { print LIST $_."\n"; }
  close(LIST);

  my $lines = scalar(@lines);
  print "$fname: $lines entries written\n";
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args != 2) {
    die
      "Usage: listDiff.pl list1 list2\n".
      "       Writes differences (to list1.only and list2.only)\n".
      "       and similarities (to list1.list2.common)\n";
  }

  my $list1 = $ARGV[0];
  my $list2 = $ARGV[1];

  if (not -f $list1) { die "No such file '$list1'\n"; }
  if (not -f $list2) { die "No such file '$list2'\n"; }

  my $list1_only  = $list1.'.only';
  my $list2_only  = $list2.'.only';
  my $list_common = $list1.'.'.$list2.'.common';

  if (-f $list1_only)  { die "File already exists: '$list1_only'\n"; }
  if (-f $list2_only)  { die "File already exists: '$list2_only'\n"; }
  if (-f $list_common) { die "File already exists: '$list_common'\n"; }

  my %content1 = (); getContent($list1,%content1);
  my %content2 = (); getContent($list2,%content2);

  my $content1 = scalar(keys %content1);
  my $content2 = scalar(keys %content2);

  print "$list1: $content1 unique entries\n";
  print "$list2: $content2 unique entries\n";

  my %unique1 = ();
  my %unique2 = ();
  my %common  = ();

  foreach (keys %content1) {
    if (defined $content2{$_}) { $common{$_} = 1; }
    else { $unique1{$_} = 1; }
  }
  foreach (keys %content2) {
    if (not defined $content1{$_}) { $unique2{$_} = 1; }
  }

  my $unique1 = scalar(keys %unique1);
  my $unique2 = scalar(keys %unique2);
  my $common  = scalar(keys %common);

  my $sum_old = $content1+$content2;
  my $sum_new = $unique1+2*$common+$unique2;

  if ($sum_old != $sum_new) { die "Number of entries changed -- logic error"; }

  saveContent(%unique1, $list1_only);
  saveContent(%unique2, $list2_only);
  saveContent(%common, $list_common);
}
main();

