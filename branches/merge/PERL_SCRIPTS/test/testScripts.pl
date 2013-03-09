#!perl

use strict;
use warnings;

my %dirOf = ();
my $arbhome = undef;

sub lookForScripts($\@\@);
sub lookForScripts($\@\@) {
  my ($dir,$scripts_r,$modules_r) = @_;
  my @subdirs = ();

  opendir(DIR,$dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $dir.'/'.$_;
      if (-d $full) {
        push @subdirs, $full;
      }
      elsif (/\.(pl|amc)$/io) { push @$scripts_r, $full; $dirOf{$_} = $dir; }
      elsif (/\.pm$/io) { push @$modules_r, $full; $dirOf{$_} = $dir; }
    }
  }
  closedir(DIR);
  foreach (@subdirs) {
    lookForScripts($_,@$scripts_r,@$modules_r);
  }
}

sub convertErrors($) {
  my ($lines) = @_;
  my @lines = split("\n",$lines);
  my $seen_error = 0;
  my @out = ();
  foreach (@lines) {
    if (/ at (.*) line ([0-9]+)/o) {
      my ($err,$name,$line,$rest) = ($`,$1,$2,$');
      my $full = $dirOf{$name};
      if (defined $full) {
        $full .= '/'.$name;
      }
      else {
        $full = $name;
      }

      my $msg = $full.':'.$line.': '.$err.$rest;
      push @out, $msg;
      $seen_error = 1;
    }
    else {
      push @out, $_;
    }
  }

  if ($seen_error==1) {
    print " FAILED\n";
    foreach (@out) { print $_."\n"; }
  }
  else {
    print " OK"
  }
  return $seen_error;
}

sub splitDirName($) {
  my ($file) = @_;
  if ($file =~ /\/([^\/]+)$/o) {
    my ($dir,$name) = ($`,$1);
    return ($dir,$name);
  }
  die "can't split '$file'";
}

sub test_script($$) {
  my ($script,$isModule) = @_;

  my ($dir,$name) = splitDirName($script);
  if (not chdir($dir)) { die "can't cd to '$dir' (Reason: $!)"; }

  my @tests = (
               '-c',
               '-MO=LintSubs',
               '-MO=Lint,no-regexp-variables',
              );

  print "Testing $name";
  foreach (@tests) {
    if ($isModule) { $_ = '-I'.$arbhome.'/lib '.$_; }
    my $test = 'perl '.$_.' '.$name.' 2>&1';
    # print "\ntest='$test'\n";
    my $result = `$test`;
    if (convertErrors($result)) {
      return 0;
    }
  }
  print "\n";
  return 1;
}

my %failed_test = (); # scripts in this directory (key=failed script,value=1)
my $failed_bioperl = 0; # script in BIOPERL directory
my $failed_normal  = 0; # other scripts

sub announce_failure($) {
  my ($script) = @_;
  if ($script =~ /\/PERL_SCRIPTS\/test\//) {
    $failed_test{$'} = 1;
  }
  elsif ($script =~ /\/PERL_SCRIPTS\/BIOPERL\//) { $failed_bioperl++; }
  else { $failed_normal++; }
}

sub main() {
  $arbhome = $ENV{ARBHOME};
  if (not defined $arbhome) { die "ARBHOME undefined"; }

  my $script_root = $ENV{ARBHOME}.'/PERL_SCRIPTS';
  my $macros_root = $ENV{ARBHOME}.'/lib/macros';
  if (not -d $script_root) { die "No such directory '$script_root'"; }
  if (not -d $macros_root) { die "No such directory '$macros_root'"; }

  my @scripts = ();
  my @modules = ();
  lookForScripts($script_root,@scripts,@modules);
  lookForScripts($macros_root,@scripts,@modules);

  # print "Existing perl scripts:\n";
  # foreach (@scripts) { print "- '$_'\n"; }

  foreach (@modules) { if (test_script($_,1)==0) { announce_failure($_); } }
  foreach (@scripts) { if (test_script($_,0)==0) { announce_failure($_); } }

  $| = 1;

  if (defined $failed_test{'check_lint.pl'}) {
    print "Assuming Lint/LintSubs is not installed (cannot test whether perl modules compile)\n";
  }
  else {
    my $failed = $failed_normal+$failed_bioperl;

    if (defined $failed_test{'check_arb.pl'}) {
      die "Fatal error: Failed to load ARB perl module\n";
    }
    if (defined $failed_test{'check_bioperl.pl'}) {
      print "Assuming BIOPERL is not installed\n";
      if ($failed_bioperl==0) {
        die "but all BIOPERL scripts compiled - sth is completely wrong here\n";
      }
      print "accepting $failed_bioperl failing scripts that use BIOPERL\n";
      $failed -= $failed_bioperl;
    }
    else {
      print "Assuming BIOPERL is installed\n";
    }

    if ($failed>0) {
      die "$failed scripts failed to compile\n";
    }
  }
}
main();
