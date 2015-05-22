#!/usr/bin/perl

# determines cores used by jenkins builds
my $cores = `cat /proc/cpuinfo | grep processor | wc -l`;
if ($cores<1) { $cores = 1; }
my $jobs = int($cores * 0.8); # reserve some cores for docker etc.
if ($jobs<1) { $jobs = 1; }
print "$jobs";

