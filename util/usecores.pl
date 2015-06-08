#!/usr/bin/perl

# determines cores used by jenkins builds
my $cores = `cat /proc/cpuinfo | grep processor | wc -l`; # no /proc/cpuinfo (e.g. on OSX) -> falls back to minimum
if ($cores<1) { $cores = 1; }

my $usedcores = int($cores * 0.8); # reserve some cores for docker etc.
if ($usedcores<1) { $usedcores = 1; }

my $jobs = ($usedcores < 2) ? 2 : $usedcores; # use at least 2 jobs
print "$jobs";

