# =============================================================== #
#                                                                 #
#   File      : tools.pm                                          #
#   Purpose   :                                                   #
#                                                                 #
#   Coded by Ralf Westram (coder@reallysoft.de) in October 2006   #
#   Institute of Microbiology (Technical University Munich)       #
#   http://www.arb-home.de/                                       #
#                                                                 #
# =============================================================== #

package tools;

use strict;
use warnings;

BEGIN {
  use Exporter ();
  our ($VERSION,@ISA,@EXPORT,@EXPORT_OK,%EXPORT_TAGS);
  $VERSION     = 1.00;
  @ISA         = qw(Exporter);
  @EXPORT      = qw(
                    &dieOnError
                    &expectError
                   );
  %EXPORT_TAGS = qw();
  @EXPORT_OK   = qw();
}

use ARB;

sub dieOnError($$) {
  my ($err,$where) = @_;
  if ($err) { die "Error at $where: $err\n"; }
}

sub expectError($) {
  my ($where) = @_;
  my $err = ARB::await_error();

  dieOnError($err,$where);
}

1;
