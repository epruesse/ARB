#!/usr/bin/perl

use strict;
use warnings;

use lib $ENV{ARBHOME}.'/SOURCE_TOOLS';
use arb_build_common;

# --------------------------------------------------------------------------------

my $logdirectory = undef;
my $slow_delay   = undef;

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

sub dump_log($) {
  my ($log) = @_;

  my $topdir = $ENV{ARBHOME};

  open(LOG,'<'.$log) || die "can't open '$log' (Reason: $!)";
  my $seen_AS = 0;
  my $line;
  while (defined($line=<LOG>)) {
    my $printed = 0;
    if ($seen_AS==1) {
      my $formatted_line = format_asan_line($line,$topdir);
      if (defined $formatted_line) {
        print $formatted_line;
        $printed = 1;
      }
    }
    else {
      if ($line =~ /(AddressSanitizer|LeakSanitizer)/o) {
        $seen_AS = 1;
        if (defined $topdir) { print('fake[2]: Entering directory `'.$topdir."\'\n"); }
      }
    }
    if ($printed==0) {
      print_expand_pathless_messages($line);
    }
  }
  if (defined $topdir and $seen_AS==1) { print('fake[2]: Leaving directory `'.$topdir."\'\n"); }
  close(LOG);
}

# --------------------------------------------------------------------------------

sub dump_junitlog(\@) {
  my ($content_r) = @_;
  my $logfile = "logs/junit_log.xml"; # see also Makefile.suite@JUNITLOGNAME
  open(JLOG,'>'.$logfile) || die "can't write '$logfile' (Reason: $!)";
  print JLOG "<testsuites>\n";
  foreach (@$content_r) {
    print JLOG $_."\n";
  }
  print JLOG "</testsuites>\n";
  close(JLOG);
}

sub removeDonefileFor($) {
  my ($unitName) = @_;

  my @donefiles = ();
  my $donefile_dir = ($slow_delay==0) ? 'tests.slow' : 'tests';
  opendir(DIR,$donefile_dir) || die "can't read directory '$donefile_dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if (/\.done$/o) {
      if (/$unitName/) {
        push @donefiles, $_;
      }
    }
  }
  closedir(DIR);

  my $donefiles = scalar(@donefiles);
  if ($donefiles==1) {
    my $donefile = $donefile_dir.'/'.$donefiles[0];
    print "Unlinking $donefile (for unit '$unitName')\n";
    unlink($donefile);
  }
  else {
    print "donefiles found: $donefiles\n";
    if ($donefiles>0) {
      foreach (@donefiles) { print "- $_\n"; }
      die "could not determine .done-file for '$unitName'";
    }
  }
}

# --------------------------------------------------------------------------------

my $tests     = 0;
my $skipped   = 0;
my $passed    = 0;
my $failed    = 0;
my $warnings  = 0;
my $elapsed   = 0;
my $crashed   = 0;
my $valgrind  = 0;
my $sanitized = 0;

my %duration = (); # key=unit, value=ms

sub parse_log($\@) {
  # parse reports generated by UnitTester.cxx@generateReport
  my ($log,$junit_r) = @_;
  open(LOG,'<'.$log) || die "can't open '$log' (Reason: $!)";

  my $tests_this    = 0;
  my $skipped_this  = 0;
  my $passedALL     = 0;
  my $seenSummary   = 0;
  my $seenSanitized = 0;

  my $curr_target        = undef;
  my $last_error_message = undef;

  my $unitName = 'unknownUnit';
  if ($log =~ /\/([^\.\/]+)\.[^\/]+$/o) { $unitName = $1; }

  my $dump_log = 0;
  my $remove_donefile = 0;

  my @testcases   = ();
  my $case_ok     = 0;
  my $case_failed = 0;

  while ($_ = <LOG>) {
    chomp;
    if (/^UnitTester:/) {
      my $rest = $';
      if ($rest =~ /^\s+\*\s+([A-Za-z0-9_]+)\s+=\s+([A-Z]*)/o) {
        my ($testname,$result) = ($1,$2);
        my $err = undef;
        if ($result ne 'OK') {
          if (defined $last_error_message) {
            $err = $last_error_message;
          }
          else {
            $err = 'unknown failure reason';
          }
        }
        # append to junit log
        my $testcase = "  <testcase name=\"$testname\" classname=\"$unitName.noclass\"";
        if (defined $err) {
          $testcase .= ">\n";
          $testcase .= "   <error message=\"$err\"/>\n";
          $testcase .= "  </testcase>";
          $case_failed++;
        }
        else {
          $testcase .= '/>';
          $case_ok++;
        }
        push @testcases, $testcase;
        $last_error_message = undef;
      }

      if (/tests=([0-9]+)/)   { $tests_this += $1; $seenSummary=1; }
      if (/skipped=([0-9]+)/) {
        $skipped_this += $1;
        if (shall_run_slow()) {
          $dump_log = 1; # @@@ TODO: should dump log only if warnings are enabled
        }
      }

      if (/passed=([0-9]+)/)  { $passed += $1; }
      if (/passed=ALL/)       { $passedALL = 1; }

      if (/failed=([0-9]+)/)  { $failed += $1; $dump_log = 1; }
      if (/warnings=([0-9]+)/)  { $warnings += $1; if ($failed==0) { $dump_log = 1; } }
      if (/target=([^\s]+)/)  { $curr_target = $1; }
      if (/time=([0-9.]+)/)   {
        $elapsed += $1;
        if (not defined $curr_target) { die "Don't know current target"; }
        $duration{$curr_target} = $1;
      }
      if (/valgrind.*error/)  { $valgrind++; $dump_log = 1; $remove_donefile = 1; }
      if (/coverage/)  { $dump_log = 1; }
    }
    elsif (/^[^\s:]+:[0-9]+:\s+Error:\s+/o) {
      if (not /\(details\sabove\)/) {
        $last_error_message = $';
      }
    }
    elsif (/^-+\s+(ARB-backtrace.*):$/) {
      $last_error_message = $1;
    }
    elsif (/ERROR:\s*(AddressSanitizer|LeakSanitizer):/o) {
      $last_error_message = $';
      $seenSanitized++;
      $remove_donefile = 1;
    }
    elsif (/\s+runtime\s+error:\s+/o) {
      $dump_log = 1;
    }
  }
  close(LOG);

  if ($remove_donefile == 1) {
    removeDonefileFor($unitName);
  }

  # write whole suite to junit log
  {
    my $case_all = $case_ok + $case_failed;
    # my $stamp    = localtime;
    my $stamp    = `date "+%Y-%m-%dT%T.%N%:z"`;
    chomp($stamp);
    push @$junit_r, " <testsuite name=\"$unitName\" tests=\"$case_all\" failures=\"$case_failed\" timestamp=\"$stamp\">";
    foreach (@testcases) { push @$junit_r, $_; }
    push @$junit_r, " </testsuite>";
  }

  if (not $seenSummary) { $dump_log = 1; }
  if ($seenSanitized>0) { $dump_log = 1; }

  if ($dump_log==1) {
    dump_log($log);
  }

  if (not $seenSummary) {
    my $ARBHOME = $ENV{ARBHOME};
    print "$ARBHOME/UNIT_TESTER/$log:1:0: Warning: No summary found in '$log' ";
    if ($seenSanitized>0) {
      $sanitized++;
      print "(was aborted by Sanitizer)\n";
    }
    else {
      $crashed++;
      print "(maybe the test did not compile or crashed)\n";
    }
  }
  else {
    if ($seenSanitized>0) {
      $sanitized++;
      print "Detected Sanitizer warnings\n";
    }
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


sub readableDuration($) {
  # result should not be longer than 9 characters! (5 chars value, space, 3 chars unit)
  my ($ms) = @_;
  if ($ms>5000) {
    my $sec = $ms / 1000.0;
    if ($sec>99) {
      my $min = $sec / 60.0;
      return sprintf("%5.2f min", $min);
    }
    return sprintf("%5.2f sec", $sec);
  }
  return sprintf("%5i ms ", $ms);
}

sub print_summary($) {
  my ($tests_failed) = @_;
  print "\n-------------------- [ Unit-test summary ] --------------------\n";

  my @summary = ();

  push @summary, sprintf(" Tests   : %5i", $tests);
  push @summary, sprintf(" Skipped : %5i =%s%s", $skipped, percent($skipped,$tests), slow_note());
  push @summary, sprintf(" Passed  : %5i =%s", $passed, percent($passed,$tests));
  push @summary, sprintf(" Failed  : %5i =%s", $failed, percent($failed,$tests));
  push @summary, sprintf(" Sum.dur.: %9s", readableDuration($elapsed));
  {
    my @slowest = sort {
      $duration{$b} <=> $duration{$a};
    } map {
      if ((defined $_) and ($_ ne '') and $duration{$_}>0) { $_; }
      else { ; }
    } keys %duration;

    my $show = scalar(@slowest);
    if ($show>3) { $show = 3; }
    if ($show>0) {
      for (my $s=0; $s<$show; ++$s) {
        my $slowunit = $slowest[$s];
        push @summary, sprintf("%s%9s (%s)", ($s==0 ? " Max.dur.: " : "           "), readableDuration($duration{$slowunit}), $slowunit);
      }
    }
  }
  if ($sanitized>0) {
    push @summary, sprintf(" Sanitizd: %5i units", $sanitized);
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
  my @junit = ();
  my @logs = get_existing_logs();
  foreach (@logs) {
    parse_log($_,@junit);
  }

  dump_junitlog(@junit);

  my $tests_failed = (($failed>0) or ($crashed>0) or ($valgrind>0) or ($sanitized>0));
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
