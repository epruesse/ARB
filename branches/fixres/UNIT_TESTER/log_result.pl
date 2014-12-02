#!/usr/bin/perl
use strict;
use warnings;

my $reg_summary     = qr/^UnitTester:.*\stests=([0-9]+)\s/;
my $reg_interrupted = qr/interrupting.*deadlocked.*test/oi;
my $reg_sanitizer   = qr/error.*addresssanitizer/oi;

sub log_summary($) {
  my ($log) = @_;

  my $interrupted = 0;
  my $sanitizer   = 0;

  open(LOG,'<'.$log) || die "Failed to read '$log' (Reason: $!)";
  my $line;
 LINE:  while (defined ($line=<LOG>)) {
    if ($line =~ $reg_summary) {
      my $summary = undef;
      my $failure = 0;
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
        $summary = "- $module ($passed/$testCount)";
        if ($passed != $testCount) { $failure = 1; }
      }

      close(LOG);
      return ($summary,$failure);
    }
    elsif ($line =~ $reg_interrupted) {
      $interrupted = 1;
      last LINE;
    }
    elsif ($line =~ $reg_sanitizer) {
      $sanitizer = 1;
      last LINE;
    }
  }

  close(LOG);

  # no summary found -> problem
  my $module   = $log; # fallback
  my $extraMsg = '; invalid logname';

  if ($log =~ /logs\//o) {
    my $name = $';
    if ($name =~ /^([^\.]+)\.test/o) {
      $module = $1;
      $extraMsg = '';
    }
    else {
      $module = $name;
    }
  }

  my $msg = 'no summary; crashed?';
  if ($interrupted==1) { $msg = 'interrupted; deadlock?'; }
  if ($sanitizer==1) { $msg = 'aborted by AddressSanitizer'; }

  return ("- $module ($msg$extraMsg)",1);
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=1) {
    die "Usage: log_result.pl unittest.log";
  }
  else {
    my ($summary,$failure) = log_summary($ARGV[0]);
    if (defined $summary) {
      if ($failure==1) {
        print sprintf("%-35s <- FAILED\n", $summary);
      }
      else {
        print $summary."\n";
      }
    }
  }
}
main();
