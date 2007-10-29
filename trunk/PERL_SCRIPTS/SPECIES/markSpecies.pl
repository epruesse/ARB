#!/usr/bin/perl

#############################################################################
# Harald Meier (meierh@in.tum.de); 2006 LRR, Technische Universität München #
#############################################################################

use strict;
use warnings;
use diagnostics;

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  1;
}

use ARB;

eval {
  my $gb_main = ARB::open(":","rw"); # connect to running ARB
  if (!$gb_main) { die "No running ARB found\n"; }

  print "Connected to running ARB.\n";

  eval {
    my $error = ARB::begin_transaction($gb_main); # start transaction
    if ($error) { die "transaction could not be initialized\n"; }

    # read species names from STDIN into hash
    my %marklist = ();
    while (<>) {
      chomp;
      $marklist{$_} = 1;
    }

    my $inCount = scalar(keys %marklist);
    if ($inCount<1) { print "Warning: Empty input\n"; }
    else { print "Read $inCount species names\n"; }

    my $been_marked    = 0;
    my $already_marked = 0;

    my $gb_species = BIO::first_species($gb_main);
    if (!$gb_species) { die "Database doesn't contain any species\n"; }

    while ($gb_species) {
      my $species_name = BIO::read_string($gb_species, "name");
      my $was_marked = ARB::read_flag($gb_species);
      if ($marklist{$species_name}) {
        if ($was_marked==1) {
          $already_marked++;
        }
        else {
          $error = ARB::write_flag($gb_species, "1");
          $been_marked++;
        }
        delete $marklist{$species_name};
      }
      $gb_species = BIO::next_species($gb_species);
    }

    my @notFound = keys %marklist;
    my $notFound = scalar(@notFound);
    if ($notFound>0) {
      print("$notFound species not found in ARBDB:\n");
      print @notFound."\n";
    }
    print "Marked $been_marked species";
    if ($already_marked>0) { print " ($already_marked species already marked)"; }
    print "\n";

    ARB::commit_transaction($gb_main);
  };
  if ($@) {
    ARB::abort_transaction($gb_main);
    print "Error in markSpecies.pl (aborting transaction):\n$@\n";
  }

  ARB::close($gb_main); # close connection to ARBDB
};
if ($@) {
  print "Error in markSpecies.pl: $@\n";
}
