#!/usr/bin/perl

use strict;
use warnings;

sub main() {
  my $detectedVersion;

  my $gcc = $ENV{A_CXX};
  if (not defined $gcc) { $gcc = $ENV{A_CC}; }

  if (not defined $gcc) {
    $detectedVersion = 'failed_to_detect_compiler';
  }
  else {
    $detectedVersion = 'unknown_gcc_version';

    my $dumpedVersion = `$gcc -dumpversion`;
    if ($dumpedVersion =~ /\..*\./) {
      $detectedVersion = $dumpedVersion;
    }
    else {
      # version info does not contain patchlevel
      my $detailedVersion = `$gcc --version`;
      if ($detailedVersion =~ /\s([0-9]+\.[0-9]+\.[0-9]+)\s/) {
        $detectedVersion = $1;
      }
    }
  }
  chomp($detectedVersion);
  print $detectedVersion."\n";
}
main();
