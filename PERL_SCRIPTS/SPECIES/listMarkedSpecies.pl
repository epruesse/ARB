#!/usr/bin/perl

# =============================================================== #
#                                                                 #
#   File      : markedSpecies.pl                                  #
#   Purpose   : list names of species marked in running ARB       #
#   Time-stamp: <Fri Oct/13/2006 17:36 MET Coder@ReallySoft.de>   #
#                                                                 #
#   Coded by Ralf Westram (coder@reallysoft.de) in October 2006   #
#   Institute of Microbiology (Technical University Munich)       #
#   http://www.arb-home.de/                                       #
#                                                                 #
# =============================================================== #

use strict;
use warnings;

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  1;
}

use ARB;

sub noError($$) {
  my ($err,$where) = @_;
  if (defined $err) { die "Error at $where: $err"; }
}

sub listMarkedSpecies() {
  my $gb_main = ARB::open(":","rw");
  if (!$gb_main) {
    noError(ARB::get_error(), 'db connect (no running ARB?)');
  }

  noError(ARB::begin_transaction($gb_main), 'begin_transaction');

  for (my $gb_species = BIO::first_species($gb_main);
       $gb_species;
       $gb_species = BIO::next_species($gb_species))
       {
         my $marked = ARB::read_flag($gb_species);
         if ($marked==1) {
           my $species_name = BIO::read_string($gb_species, "name");
           print $species_name."\n";
         }
       }

  ARB::commit_transaction($gb_main);
  ARB::close($gb_main);
}

listMarkedSpecies();

