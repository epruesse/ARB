#!/usr/bin/perl

use strict;
use warnings;

my $line;
my $lastWasEmpty = 1;
while (defined ($line=<>)) {
  if ($line =~ /^[#]\s*[0-9]/o) { ; }
  elsif ($line eq "\n") {
    if ($lastWasEmpty) { ; }
    else {
      print $line;
      $lastWasEmpty = 1;
    }
  }
  else {
    print $line;
    $lastWasEmpty = 0;
  }
}
