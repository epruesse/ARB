#!/usr/bin/perl

use strict;
use warnings;

use lib $ENV{ARBHOME}.'/SOURCE_TOOLS';
use arb_build_common;

my $reg_is_ASAN = qr/^\s+([#][0-9]+\s+.*)\s+([^\s]+):([0-9]+)$/o;

my $topdir = $ENV{ARBHOME};
my $line;
while (defined($line=<STDIN>)) {
  my $formatted_line = format_asan_line($line,$topdir);
  if (defined $formatted_line) {
    print $formatted_line;
  }
  else {
    print $line;
  }
}



