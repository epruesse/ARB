# =============================================================== #
#                                                                 #
#   File      : SAI.pm                                            #
#   Purpose   : Basic SAI access                                  #
#   Time-stamp: <Fri Jun/23/2006 11:55 MET Coder@ReallySoft.de>   #
#                                                                 #
#   Coded by Ralf Westram (coder@reallysoft.de) in June 2006      #
#   Institute of Microbiology (Technical University Munich)       #
#   http://www.arb-home.de/                                       #
#                                                                 #
# =============================================================== #

package SAI;
use ARB;

my $gb_main;

sub connectDB() {
  print "Connecting to running ARB database\n";
  $gb_main = ARB::open(":","r");
  if (! $gb_main ) {
    $error = ARB::get_error();
    print("$error\n");
    exit 0;
  }
  ARB::begin_transaction($gb_main);
}

sub disconnectDB() {
  ARB::commit_transaction($gb_main);
  ARB::close($gb_main);
}

sub scanSAIs(\@){
  my ($array_r) = @_;

  for (my $gb_sai = BIO::first_SAI($gb_main); $gb_sai; $gb_sai = BIO::next_SAI($gb_sai)) {
    my $gb_name = ARB::search($gb_sai, 'name', 'NONE');
    if ($gb_name) {
      my $name = ARB::read_string($gb_name);
      push @$array_r, $name;
    }
    else {
      die "No name found for SAI";
    }
  }
}

sub findSAI($) {
  my ($name) = @_;
  return BIO::find_SAI($gb_main, $name);
}

sub getSAIalignments($\@) {
  my ($gb_sai,$array_r) = @_;

  for (my $gb=ARB::first($gb_sai); $gb; $gb = ARB::next($gb)) {
    my $key = ARB::read_key($gb);
    if ($key =~ /^ali_/o) {
      push @$array_r, $key;
    }
  }
}

sub readIntegerArray($\@) {
  my ($gb_ints, $array_r) = @_;
  my $count = ARB::read_ints_count($gb_ints);
  if ($count>0) {
    for (my $i=0; $i<$count; $i++) {
      push @$array_r, ARB::read_from_ints($gb_ints, $i);
    }
  }
}

1; # module initialization ok


