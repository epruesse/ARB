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

  my $failed = 0;
  foreach (@modules) { if (test_script($_,1)==0) { $failed++; } }
  foreach (@scripts) { if (test_script($_,0)==0) { $failed++; } }

  if ($failed>0) {
    die "$failed perl scripts failed to compile\n";
  }
}
main();
