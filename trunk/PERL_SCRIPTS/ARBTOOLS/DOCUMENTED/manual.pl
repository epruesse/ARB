#!/usr/bin/perl
#
# script that tests features documented in ../../../PERL2ARB/DOC.html
# (to make this test working you need to have LintSubs installed)
#
# @@@ TODO from: ../../../PERL2ARB/DOC.html@create_container

use strict;
use warnings;

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  push @INC, "$arbhome/PERL_SCRIPTS/lib";
  1;
}

use ARB;
use tools;

sub main() {
  my $gb_main = ARB::open(":","rw");
  dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');

  my $gb_name = ARB::create($gb_main,"name","STRING");
  dieOnError(ARB::write_string($gb_name,"niels"), 'write_string');
  ARB::delete($gb_name);

  ARB::commit_transaction($gb_main);
  ARB::close($gb_main);
}
main();
