#!/usr/bin/perl

# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN {print "1..1\n";}
END {print "not ok 1\n" unless $loaded;}
use ARB;
$loaded = 1;
print "ok 1\n";

######################### End of black magic.

# Insert your test code below (better if it prints "ok 13"
# (correspondingly "not ok 13") depending on the success of chunk 13
# of the test code):

$gb_main = &ARB::open(":");
&ARB::begin_transaction($gb_main);
$gb_alignment_name = &ARB::search($gb_main,"presets/use");
$name = &ARB::read_string($gb_alignment_name);
&ARB::commit_transaction($gb_main);
print "alignment name is $name\n";
&ARB::close($gb_main);

