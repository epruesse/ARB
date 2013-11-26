#!/usr/bin/perl

use strict;
use warnings;

my $reg_old = undef;
my $reg_new = undef;
my $require_both_hits = 0;

my $args = scalar(@ARGV);
if ($args>0) {
  while (scalar(@ARGV)) {
    my $arg = shift @ARGV;
    if ($arg eq '--help' or $arg eq '-h') {
      print "Usage: diff2grep.pl [switches]\n";
      print "Converts a patch into grep-alike output\n";
      print "switches:\n";
      print "    --old  regexp      only list old lines matching regexp\n";
      print "    --new  regexp      only list new lines matching regexp\n";
      print "    --both regexp      only list if old and new lines match regexp\n";
      print "                       (specifying --old and --new has same effect as --both)\n";
      exit(0);
    }
    elsif ($arg eq '--old') {
      my $reg = shift @ARGV;
      die "--old expects argument 'regexp'" if not defined $reg;
      $reg_old = qr/$reg/;
    }
    elsif ($arg eq '--new') {
      my $reg = shift @ARGV;
      die "--new expects argument 'regexp'" if not defined $reg;
      $reg_new = qr/$reg/;
    }
    elsif ($arg eq '--both') {
      my $reg = shift @ARGV;
      die "--both expects argument 'regexp'" if not defined $reg;
      $reg_old = qr/$reg/;
      $reg_new = $reg_old;
      $require_both_hits = 1;
    }
    else {
      die "Unknown argument '$arg'";
    }
  }
}

my $reg_all  = qr/./;
my $reg_none = qr/jkasfgadgsfaf/;
if (defined $reg_old) {
  if (defined $reg_new) { $require_both_hits = 1; }
  else { $reg_new = $reg_none; }
}
elsif (defined $reg_new) { $reg_old = $reg_none; }
else {
  $reg_old = $reg_all;
  $reg_new = $reg_all;
}

my $oldfile = undef;
my $newfile = undef;

my $newpad = '';
my $oldpad = '';

my $correct   = 0; # perform correction?
my $corr      = 0; # correct by number of lines (calc for next hunk)
my $this_corr = 0; # correct by number of lines (value for current hunk)

sub set_padding_and_correction() {
  $correct = 1;
  $corr = 0;

  if (defined $oldfile and defined $newfile) {
    my $oldlen = length($oldfile);
    my $newlen = length($newfile);

    $newpad = '';
    $oldpad = '';

    while ($oldlen<$newlen) { $oldpad .= ' '; $oldlen++; }
    while ($newlen<$oldlen) { $newpad .= ' '; $newlen++; }

    if ($oldfile ne $newfile) { $correct = 0; }

    print "diff2grep.pl: oldfile='$oldfile'\n";
    print "diff2grep.pl: newfile='$newfile'\n";
  }
}

sub print_tagged($$) {
  my ($tag,$line) = @_;
  if ($line =~ /^\s+/o) {
    my ($pad,$rest) = ($&,$');
    my $tag_len = length($tag);
    my $pad_len = length($pad);

    if ($pad_len>($tag_len+1)) {
      my $extra_pad = substr($pad,$pad_len-$tag_len-1);
      $line = $extra_pad.$rest;
    }
    else {
      $line = $rest;
    }
  }
  print $tag.' '.$line."\n";
}

my $pwd = `pwd`; chomp($pwd);

print "diff2grep.pl: Entering directory `$pwd'\n";

my ($o1,$o2,$n1,$n2);
my $old_hitten = undef;

while (defined ($_ = <STDIN>)) {
  chomp;
  if (/^--- ([^\t]+)\t/) {
    $oldfile = $1;
    set_padding_and_correction();
  }
  elsif (/^\+\+\+ ([^\t]+)\t/) {
    $newfile = $1;
    set_padding_and_correction();
    $this_corr = $corr;
  }
  elsif (/^-/) {
    my $rest = $';
    if ($rest =~ $reg_old) {
      my $hitmsg = "$oldfile:".($o1+$this_corr*$correct).":$oldpad OLD: $rest\n";
      if ($require_both_hits) { $old_hitten = $hitmsg; }
      else { print $hitmsg; }
    }
    else {
      print_tagged('[OLD]', $rest);
    }
    $corr--;
    $o1++;
  }
  elsif (/^\+/) {
    my $rest = $';
    if ($rest =~ $reg_new) {
      my $showNew = 1;
      if ($require_both_hits==1) {
        if (defined $old_hitten) {
          print $old_hitten;
        }
        else {
          $showNew = 0;
        }
      }
      if ($showNew==1) {
        print "$newfile:".($n1).":$newpad NEW: $rest\n";
      }
    }
    else {
      print_tagged('[NEW]', $rest);
    }
    $corr++;
    $n1++;
  }
  elsif (/^\@\@ -(.*),(.*) \+(.*),(.*) \@\@$/) {
    ($o1,$o2,$n1,$n2) = ($1,$2,$3,$4);
    print $_."\n";
    $this_corr = $corr;
    $old_hitten = undef;
  }
  else {
    print $_."\n";
    $o1++;
    $n1++;
  }
}

print "diff2grep.pl: Leaving directory `$pwd'\n";

