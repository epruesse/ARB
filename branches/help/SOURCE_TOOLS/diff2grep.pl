#!/usr/bin/perl

use strict;
use warnings;

my $oldfile = undef;
my $newfile = undef;

my $newpad = '';
my $oldpad = '';

my $correct = 0; # perform correction?
my $corr    = 0; # correct by number of lines

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

my $pwd = `pwd`; chomp($pwd);

print "diff2grep.pl: Entering directory `$pwd'\n";

my ($o1,$o2,$n1,$n2);

while (defined ($_ = <>)) {
  chomp;
  if (/^--- ([^\t]+)\t/) {
    $oldfile = $1;
    set_padding_and_correction();
  }
  elsif (/^\+\+\+ ([^\t]+)\t/) {
    $newfile = $1;
    set_padding_and_correction();
  }
  elsif (/^-/) {
    print "$oldfile:".($o1+$corr*$correct).":$oldpad OLD: $'\n";
    $o1++;
    $corr--;
  }
  elsif (/^\+/) {
    print "$newfile:".($n1).":$newpad NEW: $'\n";
    $n1++;
    $corr++;
  }
  elsif (/^\@\@ -(.*),(.*) \+(.*),(.*) \@\@$/) {
    ($o1,$o2,$n1,$n2) = ($1,$2,$3,$4);
    print $_."\n";
  }
  else {
    print $_."\n";
    $o1++;
    $n1++;
  }
}

print "diff2grep.pl: Leaving directory `$pwd'\n";

