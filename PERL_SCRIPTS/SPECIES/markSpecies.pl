#! /usr/bin/perl

#############################################################################
# Harald Meier (meierh@in.tum.de); 2006 LRR, Technische Universität München #
#############################################################################

########################
#BEGIN {print "1..1\n";}
#END {print "not ok 1\n" unless $loaded;}
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

#$loaded = 1;
#print "ok 1\n";
#######################

#An einen laufenden ARB-Server hängen
my $gb_main = ARB::open(":","rw");
if (!$gb_main)   {
  printf "You must run an arbdb server first \n";
  exit;
}
else {
  print "Connected to a running instance of ARB!\n";
}


#Mit der Transaktion beginnen
my $error = ARB::begin_transaction($gb_main);
if ($error) {
  print ("Error, transaction could not be initiated!");
  exit;
}

# Einlesen über Stdin und in Hash laden
# @ARGV=("/home/meierh/ARB/PERL2ARB/otu_test_ohne_short");
my %marklist;
while (<>){
  chomp;
  $marklist{$_}=1;
}

my $gb_species = BIO::first_species($gb_main);

if (!$gb_species){
  print ("Cannot find a species in your ARB database");
} else {
  #  print ("$gb_species \n");


  while ($gb_species) {
    my $species_name = BIO::read_string($gb_species, "name");
    if ($marklist{$species_name}) {
      $error=ARB::write_flag($gb_species, "1");
      delete ($marklist{$species_name});
    }
    
    else {
      # print ("$species_name nonononononononono \n");
    }
    $gb_species = BIO::next_species($gb_species);
  }
  print("Following species not in ARB-DB: \n");
  print keys(%marklist);
}

#FILE schließen
#close (FILEHANDLER);

#Datenbankänderungen bestätigen
ARB::commit_transaction($gb_main);
print("\nTransaction committed \n");

#Verbindung zur ARB-DB beenden
ARB::close($gb_main);
print("Database connection closed \n");

#print "All tests passed.\n";
# while(<>) { chomp;$mark{$_} = 1;

