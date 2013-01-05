#!/usr/bin/perl

use strict;
use warnings;

sub main() {
  my $gcc = $ENV{GCC};
  if (not defined $gcc) { $gcc = 'gcc'; }
  my $dumpedVersion = `$gcc -dumpversion`;
  my $detectedVersion = 'unknown_gcc_version';

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

  chomp($detectedVersion);
  print $detectedVersion."\n";
}
main();
