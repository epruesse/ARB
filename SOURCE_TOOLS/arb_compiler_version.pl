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

    my $dumpedVersion   = undef;
    my $detailedVersion = undef;
    my $undetectable    = 'undetectable (via arg="'.$compiler.'")';

    $dumpedVersion   = `$compiler -dumpversion`;
    $detailedVersion = `$compiler --version`;

    if (not defined $dumpedVersion)   { $dumpedVersion   = $undetectable; }
    if (not defined $detailedVersion) { $detailedVersion = $undetectable; }

    chomp($dumpedVersion);
    chomp($detailedVersion);

    my $cmd = "$compiler -dM -E -x c /dev/null";
    if (open(CMD,$cmd.'|')) {
    LINE: foreach (<CMD>) {
        if (/__GNUC__/) {
          $detectedCompiler = 'gcc';
          # clang also defines __GNUC__ so don't "last" here
        }
        elsif (/__clang__/) {
          $detectedCompiler = 'clang';
          last LINE;
        }
      }
      close(CMD);
    }
    else {
      print STDERR "failed to execute '$cmd'";
    }

    if ($detectedCompiler eq 'unknown') {
      print STDERR "Problems detecting compiler type:\n";
      print STDERR "dumpedVersion='$dumpedVersion'\n";
      print STDERR "detailedVersion='$detailedVersion'\n";
    }

    {
      my $dv_from_dumped = undef;
      my $dv_from_detail = undef;

      if ($dumpedVersion   =~ /^([0-9]+(?:\.[0-9]+)+)$/)   { $dv_from_dumped = $dumpedVersion; }
      if ($detailedVersion =~ /\s([0-9]+(?:\.[0-9]+)+)\s/) { $dv_from_detail = $1; }

      if (defined $dv_from_dumped) {
        $detectedVersion =
          ((defined $dv_from_detail) and (length($dv_from_detail)>length($dv_from_dumped)))
          ? $dv_from_detail
          : $dv_from_dumped;
      }
      else {
        if (defined $dv_from_detail)  { $detectedVersion = $dv_from_detail; }
        else {
          print STDERR "Problems detecting compiler version:\n";
          print STDERR "dumpedVersion='$dumpedVersion'\n";
          print STDERR "detailedVersion='$detailedVersion'\n";
          print STDERR "dv_from_dumped='$dv_from_dumped'\n";
          print STDERR "dv_from_detail='$dv_from_detail'\n";
        }
      }

    }

    chomp($detectedVersion);
    chomp($detectedCompiler);
  }

  my $result = $detectedCompiler." ".$detectedVersion;
  print $result."\n";
}
main();
