# =============================================================== #
#                                                                 #
#   File      : tools.pm                                          #
#   Purpose   :                                                   #
#   Time-stamp: <Fri Oct/13/2006 17:59 MET Coder@ReallySoft.de>   #
#                                                                 #
#   Coded by Ralf Westram (coder@reallysoft.de) in October 2006   #
#   Institute of Microbiology (Technical University Munich)       #
#   http://www.arb-home.de/                                       #
#                                                                 #
# =============================================================== #

sub dieOnError($$) {
  my ($err,$where) = @_;
  if ($err) { die "Error at $where: $err"; }
}

sub expectError($) {
  my ($where) = @_;
  my $err = ARB::get_error();

  if (not $err) { $err = "Unknown error"; }
  dieOnError($err,$where);
}

1;
