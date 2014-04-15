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
  push @INC, "$arbhome/PERL_SCRIPTS/lib";
  1;
}

use ARB;
use tools;

# ------------------------------------------------------------

sub markSpecies(\%$$$$$) {
  my ($marklist_r, $wanted_mark, $clearRest,$field,$ambiguous,$partial) = @_;

  my $gb_main = ARB::open(":","rw");
  $gb_main || expectError('db connect (no running ARB?)');

  dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');

  my @count = (0,0);

  my %seen = ();
  my $had_field = 0;

  for (my $gb_species = BIO::first_species($gb_main);
       $gb_species;
       $gb_species = BIO::next_species($gb_species))
    {
      my $marked = ARB::read_flag($gb_species);
      my $field_content = BIO::read_string($gb_species, $field);

      if ($field_content) {
        $had_field++;

        my $matching_entry = undef;

        if (defined $$marklist_r{$field_content}) {
          $matching_entry = $field_content;
        }
        else {
        MATCH: foreach (keys %$marklist_r) {
            if ($field_content =~ /$_/) {
              $matching_entry = $_;
              last MATCH;
            }
          }
        }

        if (defined $matching_entry) {
          if ($marked!=$wanted_mark) {
            ARB::write_flag($gb_species,$wanted_mark);
            $count[$wanted_mark]++;
          }
          if ($ambiguous==1) {
            $seen{$matching_entry} = 1;
          }
          else {
            # expect field content to be unique
            # -> delete after use
            delete $$marklist_r{$matching_entry};
          }
        }
        else {
          if ($marked==$wanted_mark and $clearRest==1) {
            ARB::write_flag($gb_species,1-$wanted_mark);
            $count[1-$wanted_mark]++;
          }
        }
      }
    }

  if ($ambiguous==1) {
    # correct marklist
    foreach (keys %seen) { delete $$marklist_r{$_}; }
  }

  ARB::commit_transaction($gb_main);
  ARB::close($gb_main);

  if ($had_field==0) { die "No species has a field named '$field'\n"; }

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
  print "Usage: markSpecies.pl [-unmark] [-keep] specieslist [field]\n";
  print "       -unmark     Unmark species (default is to mark)\n";
  print "       -keep       Do not change rest (default is to unmark/mark rest)\n";
  print "       -ambiguous  Allow field to be ambiguous (otherwise it has to be unique)\n";
  print "       -partial    Allow partial match for field content (slow!)\n";
  print "\n";
  print "specieslist is a file containing one entry per line\n";
  print "   normally the entry will be the species ID (shortname) as used in your DB\n";
  print "   when you specify 'field' you may use other entries (e.g. 'acc')\n";
  print "\n";
  print "Use '-' as filename to read from STDIN\n";
  print "\n";
  die "Error: $err\n";
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args<1) { die_usage('Missing arguments'); }

  my $mark      = 1;
  my $clearRest = 1;
  my $ambiguous = 0;
  my $partial   = 0;

  while (substr($ARGV[0],0,1) eq '-') {
    my $arg = shift @ARGV;
    if ($arg eq '-unmark') { $mark = 0; }
    elsif ($arg eq '-keep') { $clearRest = 0; }
    elsif ($arg eq '-ambiguous') { $ambiguous = 1; }
    elsif ($arg eq '-partial') { $partial = 1; }
    else { die_usage("Unknown switch '$arg'"); }
    $args--;
  }

  my $file  = shift @ARGV;
  my $field = shift @ARGV;
  $field = 'name' if (not defined $field);

  my %marklist;
  buildMarklist($file,%marklist);
  my ($marked,$unmarked) = markSpecies(%marklist,$mark,$clearRest,$field,$ambiguous,$partial);

  if ($marked>0) { print "Marked $marked species\n"; }
  if ($unmarked>0) { print "Unmarked $unmarked species\n"; }

  my @notFound = keys %marklist;
  if (scalar(@notFound)) {
    if ($field eq 'name') {
      print "Some species were not found in database:\n";
    }
    else {
      print "Some entries did not match any species:\n";
    }
    foreach (@notFound) { print "- '$_'\n"; }
  }
}

main();

