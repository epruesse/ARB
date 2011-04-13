#!/usr/bin/perl

use strict;
use warnings;

my $oldfile = undef;
my $newfile = undef;

my $newpad = '';
my $oldpad = '';

sub set_padding() {
  if (defined $oldfile and defined $newfile) {
    my $oldlen = length($oldfile);
    my $newlen = length($newfile);

    $newpad = '';
    $oldpad = '';

    while ($oldlen<$newlen) { $oldpad .= ' '; $oldlen++; }
    while ($newlen<$oldlen) { $newpad .= ' '; $newlen++; }
  }
}

my ($o1,$o2,$n1,$n2);

while (defined ($_ = <>)) {
  chomp;
  if (/^--- ([^\t]+)\t/) {
    $oldfile = $1;
    set_padding();
  }
  elsif (/^\+\+\+ ([^\t]+)\t/) {
    $newfile = $1;
    set_padding();
  }
  elsif (/^-/) {
    print "$oldfile:".($o1).":$oldpad OLD: $'\n";
  }
  elsif (/^\+/) {
    print "$newfile:".($n1).":$newpad NEW: $'\n";
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
