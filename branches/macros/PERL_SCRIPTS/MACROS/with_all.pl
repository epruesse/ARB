#!/usr/bin/perl
use strict;
use warnings;

# This script adds all marked species to the currently selected tree using ARB_PARS.
# Each species uses itself as filter.

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  push @INC, "$arbhome/PERL_SCRIPTS/lib";
  1;
}

use ARB;
use tools;


sub selectSpecies($$) {
  my ($gb_main,$speciesName) = @_;
  BIO::remote_awar($gb_main,"ARB_NT","tmp/focus/species_name", $speciesName);
}

sub markSpecies($$$) {
  my ($gb_main,$gb_species,$mark) = @_;

  dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');
  ARB::write_flag($gb_species, $mark);
  ARB::commit_transaction($gb_main);
}

my $sleepAmount = 1;

sub exec_macro_with_species($$$$) {
  my ($gb_main,$gb_species,$speciesName,$macroName) = @_;

  BIO::mark_all($gb_main, 0); # unmark all
  markSpecies($gb_main,$gb_species,1);
  selectSpecies($gb_main,$speciesName);

  my $cmd = "perl $macroName";
  system($cmd)==0 || die "Error: failed to execute '$cmd'";

  if ($sleepAmount>0) {
    print "Sleep $sleepAmount sec..\n";
    sleep($sleepAmount);
  }

  selectSpecies($gb_main,'');
}

sub collectMarked($\%) {
  my ($gb_main,$collection_r) = @_;

  dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');

  for (my $gb_species = BIO::first_marked_species($gb_main);
       $gb_species;
       $gb_species = BIO::next_marked_species($gb_species)) {
    my $species_name = BIO::read_string($gb_species, "name");
    $species_name || expectError('read_string');
    $$collection_r{$species_name} = $gb_species;
  }

  ARB::commit_transaction($gb_main);
}

sub acceptExisting($) {
  my ($file) = @_;
  return (-f $file) ? $file : undef;
}
sub findMacroIn($$) {
  my ($name,$dir) = @_;
  my $full = acceptExisting($dir.'/'.$name);
  if (not defined $full) { $full = acceptExisting($dir.'/'.$name.'.amc'); }
  return $full;
}
sub findMacro($) {
  my ($name) = @_;
  my $full = findMacroIn($name, ARB::getenvARBMACROHOME());
  if (not defined $full) { $full = findMacroIn($name, ARB::getenvARBMACRO()); }
  return $full;
}

sub execMacroWith() {
  my $gb_main = ARB::open(":","r");
  $gb_main || expectError('db connect (no running ARB?)');

  my $err = undef;
  {
    my $args = scalar(@ARGV);
    if ($args != 2) {
      die "Usage: with_all.pl which macro\n".
        "Executes 'macro' with 'which'\n".
          "(where 'which' is 'MARKED')\n ";
    }

    my ($which,$macro) = @ARGV;

    {
      my $omacro = $macro;
      $macro = findMacro($macro);
      if (not defined $macro) { die "Failed to detect macro '$omacro'\n "; }
    }

    my $markAgain = 0;
    my %collected = (); # key = name; value = GBDATA(species)

    if ($which eq 'MARKED') {
      collectMarked($gb_main,%collected);
      $markAgain = 1;
    }
    else {
      die "Error: Unknown value for 'which' ('$which')\n ";
    }

    # perform loop with collected species:
    my @collected = keys %collected;
    my $count = scalar(@collected);

    if ($count<1) { die "No $which species - nothing to do\n"; }

    eval {
      if ($count>0) {
        print "Executing '$macro' with $count species:\n";
        for (my $c=0; $c<$count; ++$c) {
          my $species = $collected[$c];
          my $gb_species = $collected{$species};
          print "- with '$species' ".($c+1)."/$count (".(int(($c+1)*10000/$count)/100)."%)\n";
          exec_macro_with_species($gb_main,$gb_species,$species,$macro);
        }

      }
    };
    $err = $@;

    # mark species again
    if ($markAgain and ($count>0)) {
      print "Restoring old marks..\n";
      dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');
      BIO::mark_all($gb_main, 0); # unmark all
      for (my $c=0; $c<$count; ++$c) {
        my $species = $collected[$c];
        my $gb_species = $collected{$species};
        ARB::write_flag($gb_species, 1);
      }
      ARB::commit_transaction($gb_main);
    }
  }
  ARB::close($gb_main);

  if ($err) {
    {
      my $errEsc = $err;
      $errEsc =~ s/\"/\\\"/go;
      my $cmd = "arb_message \"$errEsc\"";
      system($cmd)==0 || print "failed to execute '$cmd'";
    }
    die $err;
  }
}

execMacroWith();
