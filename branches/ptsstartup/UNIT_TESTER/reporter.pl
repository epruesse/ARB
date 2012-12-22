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

my $expand_list_read = 0;
my @expand_list = ();

sub read_expand_list() {
  my $expand_list = "../SOURCE_TOOLS/valgrind2grep.lst";

  if (not -f $expand_list) {
    my $cmd = '(cd ../SOURCE_TOOLS; make valgrind_update)';
    system($cmd)==0 || die "failed to execute '$cmd' (Reason: $?)";
  }

  my $dir = `pwd`;
  open(LIST,'<'.$expand_list) || die "can't read '$expand_list' (Reason: $!) in dir '$dir'";
  my $line;
  while (defined($line=<LIST>)) {
    chomp($line);
    push @expand_list, $line;
  }
  close(LIST);

  $expand_list_read = 1;
}

my %expanded = (); # key=$filename, value=ref to array of possible expanded filenames.

sub get_expanded_filenames($) {
  my ($file) = @_;

  my $found_r = $expanded{$file};
  if (not defined $found_r) {
    if ($expand_list_read==0) { read_expand_list(); }
    my @expanded = ();
    foreach (@expand_list) { if (/\/$file$/) { push @expanded, $_; } }
    $expanded{$file} = \@expanded;
    $found_r = $expanded{$file};
  }
  return @$found_r;
}

sub print_expand_pathless_messages($) {
  my ($line) = @_;
  chomp($line);
  if ($line =~ /^([a-z0-9_\.]+):([0-9:]+):/oi) {
    my ($file,$lineCol,$rest) = ($1,$2,$');
    my @expanded = get_expanded_filenames($file);

    if (scalar(@expanded)==0) {
      print "$file:$lineCol: [unknown -> call 'make valgrind_update'] $rest\n";
    }
    else {
      foreach (@expanded) {
        print "$_:$lineCol: $rest\n";
      }
    }
  }
  else {
    print $line."\n";
  }
}

# --------------------------------------------------------------------------------

my $tests    = 0;
my $skipped  = 0;
my $passed   = 0;
my $failed   = 0;
my $warnings = 0;
my $elapsed  = 0;
my $crashed  = 0;
my $valgrind = 0;

my $max_dur      = 0;
my $max_dur_unit = undef;

sub parse_log($) {
  # parse reports generated by UnitTester.cxx@generateReport
  my ($log) = @_;
  open(LOG,$log) || die "can't open '$log' (Reason: $!)";

  my $tests_this   = 0;
  my $skipped_this = 0;
  my $passedALL    = 0;
  my $seenSummary  = 0;
  my $curr_target  = undef;

  my $dump_log = 0;

  while ($_ = <LOG>) {
    chomp;
    if (/^UnitTester:/) {
      if (/tests=([0-9]+)/)   { $tests_this += $1; $seenSummary=1; }
      if (/skipped=([0-9]+)/) { $skipped_this += $1; $dump_log = 1; }

      if (/passed=([0-9]+)/)  { $passed += $1; }
      if (/passed=ALL/)       { $passedALL = 1; }

      if (/failed=([0-9]+)/)  { $failed += $1; $dump_log = 1; }
      if (/warnings=([0-9]+)/)  { $warnings += $1; if ($failed==0) { $dump_log = 1; } }
      if (/target=([^\s]+)/)  { $curr_target = $1; }
      if (/time=([0-9.]+)/)   {
        $elapsed += $1;
        if ($1>$max_dur) {
          $max_dur = $1;
          if (not defined $curr_target) { die "Don't know current target"; }
          $max_dur_unit = $curr_target;
        }
      }
      if (/valgrind.*error/)  { $valgrind++; $dump_log = 1; }
      if (/coverage/)  { $dump_log = 1; }
    }
  }
  close(LOG);

  if (not $seenSummary) { $dump_log = 1; }

  if ($dump_log==1) {
    open(LOG,$log) || die "can't open '$log' (Reason: $!)";
    my $line;
    while (defined($line=<LOG>)) { print_expand_pathless_messages($line); }
    close(LOG);
  }
  else {
    my $log_ptr = $log;
    $log_ptr =~ s/^\./UNIT_TESTER/;
    # print "Suppressing dispensable $log_ptr\n";
  }

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

  push @summary, sprintf(" Tests   : %5i", $tests);
  push @summary, sprintf(" Skipped : %5i =%s%s", $skipped, percent($skipped,$tests), slow_note());
  push @summary, sprintf(" Passed  : %5i =%s", $passed, percent($passed,$tests));
  push @summary, sprintf(" Failed  : %5i =%s", $failed, percent($failed,$tests));
  push @summary, sprintf(" Sum.dur.: %5i ms", $elapsed);
  if (defined $max_dur_unit) {
    push @summary, sprintf(" Max.dur.: %5i ms (%s)", $max_dur, $max_dur_unit);
  }
  push @summary, sprintf(" Crashed : %5i units", $crashed);
  push @summary, sprintf(" Warnings: %5i", $warnings);
  if ($valgrind>0) {
    push @summary, sprintf(" Valgrind: %5i failures", $valgrind);
  }

  my @big;
  my $Big = $tests_failed ? $BigFailed : $BigOk;
  @big= split '\n', $Big;

  my $vOffset = scalar(@summary) - scalar(@big);
  if ($vOffset<0) { $vOffset = 0; }

  my $col = 0;
  for (my $i=0; $i<scalar(@big); $i++) {
    my $j = $i+$vOffset;
    my $len = length($summary[$j]);
    if ($len>$col) { $col = $len; }
  }

  $col += 6; # add horizontal offset

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

  my $tests_failed = (($failed>0) or ($crashed>0) or ($valgrind>0));
  print_summary($tests_failed);
  slow_cleanup($tests_failed);
  if ($tests_failed) {
    die "tests failed\n";
  }
  return undef;
}

sub check_obsolete_restricts() {
  my $restrict = $ENV{CHECK_RESTRICT};
  if (not defined $restrict) {
    print "Can't check restriction (empty)\n";
  }
  else {
    $restrict = ':'.$restrict.':';
    if ($restrict =~ /:(WINDOW|ARBDB|AWT|CORE):/) {
      my $lib = $1;
      my $msl = 'Makefile.setup.local';

      print "UNIT_TESTER/$msl:1: Error: Obsolete restriction '$lib' (should be 'lib$lib')\n";
      my $grepcmd = "grep -n \'RESTRICT_LIB.*=.*$lib\' $msl";
      open(GREP,$grepcmd.'|') || die "can't execute '$grepcmd' (Reason: $?)";
      foreach (<GREP>) {
        print "UNIT_TESTER/$msl:$_";
      }
      die;
    }
  }
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
    check_obsolete_restricts();
    eval { $error = &$cb(); };
    if ($@) { $error = $@; }
  }
  if ($error) { die "Error: ".$error; }
}
main();
