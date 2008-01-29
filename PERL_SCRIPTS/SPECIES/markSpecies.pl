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
  my $args = scalar(@ARGV);
  if ($args<1 or $ARGV[0] eq '-h') {
    die(
        "Missing arguments\n".
        "\n".
        "Usage: cat yourFile \| perl markSpecies.pl [options] field\n".
        "Purpose: Marks all species for which the content of the given <field>\n".
        "         matches one line of <yourFile>.\n".
        "         Commonly used <field>s are 'name' or 'acc', given you have a list of\n".
        "         species shortnames or accession numbers.\n".
        "options: -w             remove leading/trailing whitespace\n".
        "");
  }

  my $removeWhitespace = 0;

  while ($args>1) {
    my $flag = $ARGV[0];
    shift @ARGV;
    $args--;

    if ($flag eq '-w') {
      $removeWhitespace = 1;
    }
    else {
      die "Unkown argument '$flag'";
    }
  }

  my $field = $ARGV[0];
  shift @ARGV; # needed (otherwise PERL spits some strange error: implicit open..)

  my $gb_main = ARB::open(":","rw"); # connect to running ARB
  if (!$gb_main) { die "No running ARB found\n"; }

  print "Connected to running ARB.\n";

  eval {
    my $error = ARB::begin_transaction($gb_main); # start transaction
    if ($error) { die "transaction could not be initialized\n"; }

    # read species names from STDIN into hash
    my %marklist = ();
    while (<>) {
	s/\n\r/\n/go; # mac -> unix
	s/\r\n/\n/go; # dos -> unix
      chomp;
      if ($removeWhitespace==1) {
        $_ =~ s/^[ \t]+//go;
        $_ =~ s/[ \t]+$//go;
      }
      $marklist{$_} = 1;
    }

    my $inCount = scalar(keys %marklist);
    if ($inCount<1) { print "Warning: Empty input\n"; }
    else { print "Read $inCount $field\'s from input\n"; }

    my $been_marked    = 0;
    my $already_marked = 0;

    my $gb_species = BIO::first_species($gb_main);
    if (!$gb_species) { die "Database doesn't contain any matching species\n"; }

    my %seen = ();
    while ($gb_species) {
      #my $species_name = BIO::read_string($gb_species, "name");
      my $species_content = BIO::read_string($gb_species, $field);
      my $was_marked = ARB::read_flag($gb_species);
      if ($marklist{$species_content}) {
        if ($was_marked==1) {
          $already_marked++;
        }
        else {
          $error = ARB::write_flag($gb_species, "1");
          $been_marked++;
        }
        $seen{$species_content} = 1;
        # delete $marklist{$species_content};
      }
      $gb_species = BIO::next_species($gb_species);
    }

    foreach (keys %seen) {
      delete $marklist{$_};
    }

    my @notFound = keys %marklist;
    my $notFound = scalar(@notFound);
    if ($notFound>0) {
      print("$notFound $field\'s not found in ARBDB:\n");
      my $i;
      for ($i=0; $i<$notFound and $i<15; $i++) {
        print $notFound[$i]."\n";
      }
      # print @notFound."\n";
      if ($i<$notFound) {
        print "[Rest skipped]\n";
      }
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
