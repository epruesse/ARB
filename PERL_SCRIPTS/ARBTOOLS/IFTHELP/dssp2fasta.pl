#!/usr/bin/perl

use warnings;
use strict;

my $line = undef;
my $source = undef;
my $seq = '';
my $second = '';
my $mode = 0;
foreach $line (<>) {
    chomp($line);
    if ($mode==0) {
	if ($line =~ /^SOURCE\s+/) {
	    $source = $';
	    $mode++;
	}
    }
    elsif ($mode==1) {
	if ($line =~ /RESIDUE AA/) {
	    $mode++;
	}
    }
    elsif ($mode==2) {
	if ($line =~ /^\s+[0-9]+\s+[0-9]+\s+([A-Z])\s\s([A-Z\s])/io) {
	    $seq .= $1;
            $second .= $2 eq ' ' ? '-' : $2;
	}
	else {
	    die "Can't parse '$line'";
	}
    }
}

if (not defined $source) { die "Could not find SOURCE entry"; }
if ($mode!=2) { die "Unknown parse error"; }

print ">$source\n";
print "$seq\n";
print ">fold_$source\n";
print "$second\n";
