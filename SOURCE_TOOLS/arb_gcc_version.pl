#!/usr/bin/perl

use strict;
use warnings;

sub main() {
  my $detectedVersion = 'failed_to_detect_compiler';

  my $args = scalar(@ARGV);
  if ($args != 1) {
    print STDERR 'Usage: arb_gcc_version.pl ${CXX}'."\n";
    $detectedVersion .= '__missing_arg_to__arb_gcc_version';
  }
  else {
    my $gcc = $ARGV[0];
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
