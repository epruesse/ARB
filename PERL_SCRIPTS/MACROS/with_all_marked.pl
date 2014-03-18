#!/usr/bin/perl
use strict;
use warnings;

my $sleepAmount = 0; # try to increase (to a small amount of seconds) if you encounter problems

# This script calls a macro with all marked/found species.

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

sub exec_macro_with_species($$$$) {
  my ($gb_main,$gb_species,$speciesName,$macroName) = @_;

  BIO::mark_all($gb_main, 0); # unmark all
  markSpecies($gb_main,$gb_species,1);
  selectSpecies($gb_main,$speciesName);

  my $cmd = "perl '$macroName'";
  system($cmd)==0 || die "Error: failed to execute '$cmd'";

  if ($sleepAmount>0) {
    print "Sleep $sleepAmount sec..\n";
    sleep($sleepAmount);
  }

  selectSpecies($gb_main,'');
}

sub collectMarked($\@\%) {
  my ($gb_main,$name_r,$gbdata_r) = @_;

  dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');

  for (my $gb_species = BIO::first_marked_species($gb_main);
       $gb_species;
       $gb_species = BIO::next_marked_species($gb_species)) {
    my $species_name = BIO::read_string($gb_species, "name");
    $species_name || expectError('read_string');
    push @$name_r, $species_name;
    $$gbdata_r{$species_name} = $gb_species;
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
  my $full = acceptExisting($name); # accept macro specified with full path
  if (not defined $full) { $full = findMacroIn($name, ARB::getenvARBMACROHOME()); }
  if (not defined $full) { $full = findMacroIn($name, ARB::getenvARBMACRO()); }
  return $full;
}

sub execMacroWith() {
  my $gb_main = ARB::open(":","r");
  $gb_main || expectError('db connect (no running ARB?)');

  my $err = undef;
  {
    my $args = scalar(@ARGV);
    if ($args != 1) {
      die "Usage: with_all_marked.pl macro\n".
        "Executes 'macro' once for each marked species.\n".
          "For each call to 'macro', exactly one species will be marked AND selected.\n ";
    }

    my ($macro) = @ARGV;

    {
      my $omacro = $macro;
      $macro = findMacro($macro);
      if (not defined $macro) { die "Failed to detect macro '$omacro'\n "; }
    }

    my $restoreMarked = 1;

    my %gb_species = (); # key = name; value = GBDATA(species)
    my @names      = (); # contains names of %gb_species (in DB order)

    collectMarked($gb_main,@names,%gb_species);

    # perform loop with collected species:
    my $count = scalar(@names);
    if ($count<1) { die "No marked species - nothing to do\n"; }

    eval {
      if ($count>0) {
        print "Executing '$macro' with $count species:\n";
        for (my $c=0; $c<$count; ++$c) {
          my $species = $names[$c];
          my $gb_species = $gb_species{$species};
          print "- with '$species' ".($c+1)."/$count (".(int(($c+1)*10000/$count)/100)."%)\n";
          exec_macro_with_species($gb_main,$gb_species,$species,$macro);
        }

      }
    };
    $err = $@;

    # mark species again
    if ($restoreMarked and ($count>0)) {
      print "Restoring old marks..\n";
      dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');
      BIO::mark_all($gb_main, 0); # unmark all
      for (my $c=0; $c<$count; ++$c) {
        my $species = $names[$c];
        my $gb_species = $gb_species{$species};
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
