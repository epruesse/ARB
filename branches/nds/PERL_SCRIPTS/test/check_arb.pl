#!perl
# this script is used to test whether ARB perl module is installed and useable
use strict;
use warnings;

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  push @INC, "$arbhome/PERL_SCRIPTS/GENOME";
  1;
}

use ARB;

