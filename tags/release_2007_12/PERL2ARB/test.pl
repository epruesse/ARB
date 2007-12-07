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

print"Looking for a running arb server...\n";
my $gb_main = ARB::open(":",rw);
if (!$gb_main) {
  print "None found - using demo database demo.arb provided with ARB\n";
  $gb_main = ARB::open("../demo.arb",rw);
  if (!$gb_main) {
    die "Oops! demo.arb isn't in parent directory as expected";
  }
  print "Opened database.\n";
}
else {
  print "Connected to a running instance of ARB!\n";
}

print "\n";

# begin a transaction
ARB::begin_transaction($gb_main);

# read and print alignment name
my $gb_alignment_name = ARB::search($gb_main,"presets/use","none");
my $name = ARB::read_string($gb_alignment_name);
print "Current alignment name is '$name'\n";

# traverse all species
my $gb_species = BIO::first_species($gb_main);
if (!$gb_species) {
  print "Strange: Database does not contain species.\n";
}
else {
  my $species_count=0;
  while ($gb_species) {
    $species_count++;
    $gb_species=BIO::next_species($gb_species);
  }
  print "Number of species in database: $species_count\n";
}

# traverse all trees
my $tree_name = BIO::get_next_tree_name($gb_main,0);
my $tree_counter=0;
if ($tree_name) {
  while ($tree_name) {
    $tree_counter++;
    print "tree '$tree_name'\n";
    $tree_name = BIO::get_next_tree_name($gb_main,$tree_name);
  }
}
print "Number of trees in database: $tree_counter\n";

# commit the transaction
ARB::commit_transaction($gb_main);

print "\nClosing database\n";
ARB::close($gb_main);

print "All tests passed.\n";


