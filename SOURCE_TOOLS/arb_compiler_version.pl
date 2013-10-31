#!/usr/bin/perl

use strict;
use warnings;

sub main() {
  my $detectedVersion  = 'failed_to_detect_compiler';
  my $detectedCompiler = 'unknown';

  my $args = scalar(@ARGV);
  if ($args != 1) {
    print STDERR 'Usage: arb_compiler_version.pl ${CXX}'."\n";
    $detectedVersion .= '__missing_arg_to__arb_compiler_version';
  }
  else {
    my $compiler = $ARGV[0];
    $detectedVersion = 'unknown_compiler_version';

    my $dumpedVersion   = `$compiler -dumpversion`;
    my $detailedVersion = `$compiler --version`;

    if ($detailedVersion =~ /\s/) {
      my $firstWord = $`;
      if ($firstWord eq 'gcc' or $firstWord eq 'clang') {
        $detectedCompiler = $firstWord;
      }
      elsif ($firstWord eq 'g++') { $detectedCompiler = 'gcc'; }
      elsif ($firstWord eq 'clang++') { $detectedCompiler = 'clang'; }
    }

    if ($detectedCompiler eq 'unknown') {
      if ($detailedVersion =~ /apple.*llvm.*clang/oi) { $detectedCompiler = 'clang'; }
    }

    if ($detectedCompiler eq 'unknown') {
      print STDERR "Problems detecting compiler type:\n";
      print STDERR "dumpedVersion='$dumpedVersion'\n";
      print STDERR "detailedVersion='$detailedVersion'\n";
    }

    if ($dumpedVersion =~ /\..*\./) {
      $detectedVersion = $dumpedVersion;
    }
    else { # if version info does not contain patchlevel -> use detailedVersion
      if ($detailedVersion =~ /\s([0-9]+\.[0-9]+\.[0-9]+)\s/) {
        $detectedVersion = $1;
      }
    }
  }

  chomp($detectedVersion);
  chomp($detectedCompiler);

  print $detectedCompiler." ".$detectedVersion."\n";
}
main();
