#!/usr/bin/perl -w
my $mtime = (stat($ARGV[0]))[9];
print "$mtime\n";
