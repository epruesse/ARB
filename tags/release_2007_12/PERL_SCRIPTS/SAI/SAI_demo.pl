#!/usr/bin/perl

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
use SAI;

# -----------------------

sub expectEntry($$) {
  my ($gb_father,$key) = @_;
  my $gb = ARB::search($gb_father, $key, 'NONE');
  defined $gb || die "Expected entry '$key' in '".ARB::get_db_path($gb_father)."'";
  return $gb;
}

# -----------------------

SAI::connectDB();

my @SAI = ();
SAI::scanSAIs(@SAI);

print "SAIs in current database:\n";
foreach (@SAI) {
  print "- $_\n";
}

my $sai_name = 'POS_VAR_BY_PARSIMONY';
my $gb_posvar = SAI::findSAI($sai_name);
if ($gb_posvar) {
  print "Details for SAI '$sai_name':\n";

  my @alis = ();
  SAI::getSAIalignments($gb_posvar,@alis);

  print "- Contains information for the following alignments:\n";
  foreach my $ali (@alis) {
    print "  - $ali\n";
    my $gb_ali = expectEntry($gb_posvar, $ali);

    my $gb_cat = expectEntry($gb_ali, '_CATEGORIES');
    my $cat    = ARB::read_string($gb_cat);
    print "    cat='$cat'\n";

    my $gb_data = expectEntry($gb_ali, 'data');
    my $data = ARB::read_string($gb_data);
    # print "    data='$data'\n";
    print "    data contains ".length($data)." characters.\n";

    my $gb_freqs = expectEntry($gb_ali, 'FREQUENCIES');
    my @freq_entries = ( 'NA', 'NC', 'NG', 'NU');
    foreach my $fentry (@freq_entries) {
      my $gb_f = expectEntry($gb_freqs, $fentry);
      my @values = ();
      SAI::readIntegerArray($gb_f,@values);
      print "    $fentry contains ".scalar(@values)." integer values, ";
      my $sum = 0;
      foreach (@values) { $sum += $_; }
      print "    $fentry sum is $sum\n";
    }
  }
}
else {
  print "No such SAI '$sai_name'\n";
}

SAI::disconnectDB();

