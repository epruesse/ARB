#!/usr/bin/perl

use strict;
use warnings;

my $reg_is_ASAN = qr/^\s+([#][0-9]+\s+.*)\s+([^\s]+):([0-9]+)$/o;

my $line;
while (defined($line=<STDIN>)) {
  my $printed = 0;
  chomp($line);
  if ($line =~ $reg_is_ASAN) {
    my ($msg,$file,$lineno) = ($1,$2,$3);
    if (-f $file) {
      print $file.':'.$lineno.': '.$msg."\n";
      $printed = 1;
    }
  }
  if ($printed==0) {
    print $line."\n";
  }
}



