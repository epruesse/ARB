#! /usr/bin/perl

#############################################################################
# Harald Meier (meierh@in.tum.de); 2006 LRR, Technische Universität München #
#############################################################################

use strict;
use warnings;
# use diagnostics;

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  push @INC, "$arbhome/lib/PERL_SCRIPTS/SPECIES";
  1;
}

use ARB;
use tools;

# ------------------------------------------------------------

sub markSpecies(\%$$) {
  my ($marklist_r, $wanted_mark, $clearRest) = @_;

  my $gb_main = ARB::open(":","rw");
  $gb_main || expectError('db connect (no running ARB?)');

  dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');

  my @count = (0,0);

  for (my $gb_species = BIO::first_species($gb_main);
       $gb_species;
       $gb_species = BIO::next_species($gb_species))
    {
      my $marked = ARB::read_flag($gb_species);
      my $species_name = BIO::read_string($gb_species, "name");
      $species_name || expectError('read_string');

      if (exists $$marklist_r{$species_name}) {
        if ($marked!=$wanted_mark) {
          ARB::write_flag($gb_species,$wanted_mark);
          $count[$wanted_mark]++;
        }
        delete $$marklist_r{$species_name};
      }
      else {
        if ($marked==$wanted_mark and $clearRest==1) {
          ARB::write_flag($gb_species,1-$wanted_mark);
          $count[1-$wanted_mark]++;
        }
      }
    }

  ARB::commit_transaction($gb_main);
  ARB::close($gb_main);

  return ($count[1],$count[0]);
}

sub buildMarklist($\%) {
  my ($file,$marklist_r) = @_;

  my @lines;
  if ($file eq '-') { # use STDIN
    @lines = <STDIN>;
  }
  else {
    open(FILE,'<'.$file) || die "can't open '$file' (Reason: $!)";
    @lines = <FILE>;
    close(FILE);
  }

  %$marklist_r = map {
    chomp;
    $_ => 1;
  } @lines;
}

# ------------------------------------------------------------

sub die_usage($) {
  my ($err) = @_;
  print "Purpose: Mark/unmark species in running ARB\n";
  print "Usage: markSpecies.pl [-unmark] [-keep] specieslist\n";
  print "       -unmark   Unmark species (default is to mark)\n";
  print "       -keep     Do not change rest (default is to unmark/mark rest)\n";
  print "\n";
  print "specieslist is a file containing one species per line\n";
  print "Use '-' as filename to read from STDIN\n";
  print "\n";
  die "Error: $err\n";
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args<1) { die_usage('Missing arguments'); }

  my $mark      = 1;
  my $clearRest = 1;

  while ($args>1) {
    my $arg = shift @ARGV;
    if ($arg eq '-unmark') { $mark = 0; }
    elsif ($arg eq '-keep') { $clearRest = 0; }
    else { die_usage("Unknown switch '$arg'"); }
    $args--;
  }

  my $file = shift @ARGV;
  my %marklist;
  buildMarklist($file,%marklist);
  my ($marked,$unmarked) = markSpecies(%marklist,$mark,$clearRest);

  if ($marked>0) { print "Marked $marked species\n"; }
  if ($unmarked>0) { print "Unmarked $unmarked species\n"; }

  my @notFound = keys %marklist;
  if (scalar(@notFound)) {
    print "Some species were not found in database:\n";
    foreach (@notFound) { print "- '$_'\n"; }
  }
}

main();

