#!/usr/bin/perl
use strict;
use warnings;

my $reg_summary = qr/^UnitTester:.*\stests=([0-9]+)\s/;

sub log_summary($) {
  my ($log) = @_;

  open(LOG,$log) || die "Failed to read '$log' (Reason: $!)";
  my $line;
  while (defined ($line=<LOG>)) {
    if ($line =~ $reg_summary) {
      my $testCount = $1;
      if ($testCount==0) {
        ; # looks like all tests in this module have been skipped
      }
      else {
        chomp($line);
        $line =~ s/^UnitTester:\s*//;
        my @items = split /\s+/, $line;
        my $module = undef;
        my $passed = undef;
        foreach (@items) {
          my ($key,$value) = split /=/, $_;
          if ($key eq 'target') {
            $value =~ s/\.a//;
            $module = $value;
          }
          elsif ($key eq 'passed') {
            $passed = $value;
            if ($passed eq 'ALL') { $passed = $testCount; }
          }
        }
        print "- $module ($passed/$testCount)\n";
      }

      close(LOG);
      return;
    }
  }

  close(LOG);

  # no summary found -> problem
  if ($log =~ /logs\/([^\.]+)\.test/) {
    my $module = $1;
    print "- $module (no summary; crashed?)\n";
  }
  else {
    print "- $log (no summary; invalid logname)\n";
  }
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=1) {
    die "Usage: log_result.pl unittest.log";
  }
  else {
    log_summary($ARGV[0]);
  }
}
main();
