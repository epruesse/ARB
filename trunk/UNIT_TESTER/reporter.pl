#!/usr/bin/perl

use strict;
use warnings;

# --------------------------------------------------------------------------------

my $logdirectory = undef;
my $slow_delay = undef;

# --------------------------------------------------------------------------------

sub getModtime($) {
  my ($file_or_dir) = @_;
  if (-f $file_or_dir or -d $file_or_dir) {
    my @st = stat($file_or_dir);
    if (not @st) { die "can't stat '$file_or_dir' ($!)"; }
    return $st[9];
  }
  return 0; # does not exist -> use epoch
}
sub getAge($) { my ($file_or_dir) = @_; return time-getModtime($file_or_dir); }

# --------------------------------------------------------------------------------

# when file $slow_stamp exists, slow tests get skipped (see sym2testcode.pl@SkipSlow)
my $slow_stamp = 'skipslow.stamp';
my $slow_age   = getAge($slow_stamp); # seconds since of last successful slow test

sub shall_run_slow() { return (($slow_delay==0) or ($slow_age>($slow_delay*60))); }

sub slow_init() {
  if (shall_run_slow()) {
    print "Running SLOW tests\n";
    unlink($slow_stamp);
  }
  else {
    print "Skipping SLOW tests\n";
  }
}

sub slow_cleanup($) {
  my ($tests_failed) = @_;

  if (shall_run_slow() and not $tests_failed) {
    system("touch $slow_stamp");
  }
}

# --------------------------------------------------------------------------------

sub get_existing_logs() {
  my @logs;
  opendir(LOGDIR,$logdirectory) || die "can't read directory '$logdirectory' (Reason: $!)";
  foreach (readdir(LOGDIR)) {
    if (/\.log$/o) { push @logs, $logdirectory.'/'.$_; }
  }
  closedir(LOGDIR);
  return @logs;
}

sub do_init() {
  if (-d $logdirectory) {
    my @logs = get_existing_logs();
    foreach (@logs) { unlink($_) || die "can't unlink '$_' (Reason: $!)"; }
  }
  slow_init();
  return undef;
}
# --------------------------------------------------------------------------------

my $tests    = 0;
my $skipped  = 0;
my $passed   = 0;
my $failed   = 0;
my $warnings = 0;
my $elapsed  = 0;
my $crashed  = 0;

sub parse_log($) {
  # parse reports generated by UnitTester.cxx@generateReport
  my ($log) = @_;
  open(LOG,$log) || die "can't open '$log' (Reason: $!)";

  my $tests_this   = 0;
  my $skipped_this = 0;
  my $passedALL    = 0;
  my $seenSummary  = 0;

  while ($_ = <LOG>) {
    chomp;
    if (/^UnitTester:/) {
      if (/tests=([0-9]+)/)   { $tests_this += $1; $seenSummary=1; }
      if (/skipped=([0-9]+)/) { $skipped_this += $1; }

      if (/passed=([0-9]+)/)  { $passed += $1; }
      if (/passed=ALL/)       { $passedALL = 1; }

      if (/failed=([0-9]+)/)  { $failed += $1; }
      if (/warnings=([0-9]+)/)  { $warnings += $1; }
      if (/time=([0-9.]+)/)   { $elapsed += $1; }
    }
  }
  close(LOG);

  if (not $seenSummary) {
    print "Warning: No summary found in '$log' (maybe the test did not compile or crashed)\n";
    $crashed++;
  }

  $tests   += $tests_this;
  $skipped += $skipped_this;

  if ($passedALL==1) { $passed += ($tests_this-$skipped_this); }
}

sub percent($$) {
  my ($part,$all) = @_;
  if ($all) {
    my $percent = 100*$part/$all;
    return sprintf("%5.1f%%", $percent);
  }
  else {
    $part==0 || die;
    return "  0.0%";
  }
}

sub slow_note() {
  return (shall_run_slow() ? "" : " (slow tests skipped)");
}

my $BigOk = <<EndOk;
  __  __ _    _  _
 /  \\(  / )  (_)( \\
(  O ))  (    _  ) )
 \\__/(__\\_)  (_)(_/
EndOk

my $BigFailed = <<EndFailed;
 ____  __   __  __    ____  ____   _
(  __)/ _\\ (  )(  )  (  __)(    \\ / \\
 ) _)/    \\ )( / (_/\\ ) _)  ) D ( \\_/
(__) \\_/\\_/(__)\\____/(____)(____/ (_)
EndFailed


sub print_summary($) {
  my ($tests_failed) = @_;
  print "\n-------------------- [ Unit-test summary ] --------------------\n";

  my @summary = ();

  push @summary, sprintf(" Tests   : %4i", $tests);
  push @summary, sprintf(" Skipped : %4i =%s%s", $skipped, percent($skipped,$tests), slow_note());
  push @summary, sprintf(" Passed  : %4i =%s", $passed, percent($passed,$tests));
  push @summary, sprintf(" Failed  : %4i =%s", $failed, percent($failed,$tests));
  push @summary, sprintf(" Elapsed : %4i ms", $elapsed);
  push @summary, sprintf(" Crashed : %4i units", $crashed);
  push @summary, sprintf(" Warnings: %4i", $warnings);

  my @big;
  my $Big = $tests_failed ? $BigFailed : $BigOk;
  @big= split '\n', $Big;

  my $col = 0;
  for (my $i=0; $i<scalar(@big); $i++) {
    my $len = length($summary[$i]);
    if ($len>$col) { $col = $len; }
  }

  $col += 6; # add horizontal offset

  my $vOffset = scalar(@summary) - scalar(@big);
  if ($vOffset<0) { $vOffset = 0; }

  for (my $i=0; $i<scalar(@big); $i++) {
    my $j = $i+$vOffset;
    my $padded = $summary[$j];
    my $len = length($padded);
    while ($len<$col) { $padded .= ' '; $len++; }
    $summary[$j] = $padded.$big[$i];
  }

  foreach (@summary) { print $_."\n"; }
}

sub do_report() {
  my @logs = get_existing_logs();
  foreach (@logs) {
    parse_log($_);
  }

  my $tests_failed = (($failed>0) or ($crashed>0));
  print_summary($tests_failed);
  slow_cleanup($tests_failed);
  if ($tests_failed) {
    die "tests failed\n";
  }
  return undef;
}

# --------------------------------------------------------------------------------

sub main() {
  my $error = undef;
  my $cb    = undef;
  {
    my $args = scalar(@ARGV);
    if ($args==3) {
      my $command   = shift @ARGV;

      if ($command eq 'init') { $cb = \&do_init; }
      elsif ($command eq 'report') { $cb = \&do_report; }
      else { $error = "Unknown command '$command'"; }

      if (not $error) {
        $logdirectory = shift @ARGV;
        $slow_delay = shift @ARGV;
      }
    }
    else {
      $error = 'Wrong number of arguments';
    }
  }
  if ($error) {
    print "Usage: reporter.pl [init|report] logdirectory slow-delay\n";
    print "       slow-delay    >0 => run slow tests only every slow-delay minutes\n";
  }
  else {
    eval { $error = &$cb(); };
    if ($@) { $error = $@; }
  }
  if ($error) { die "Error: ".$error; }
}
main();
