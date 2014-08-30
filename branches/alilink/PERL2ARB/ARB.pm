package ARB;

use strict;
use vars qw($VERSION @ISA @EXPORT);

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(

);
$VERSION = '0.01';

bootstrap ARB $VERSION;

# globally catch die, redirect to arb_message, then confess
package CORE::GLOBAL;
use subs 'die';
my $already_dying = 0;

sub die {
  if ($already_dying==0) {
    $already_dying++; # do not recurse
    my ($msg) = @_;
    if ($msg eq '') { $msg = 'unknown error'; }
    $msg =~ s/\'/"/g;
    system("arb_message 'Macro execution error:\n$msg'");
    use Carp;
    Carp::confess("Macro execution error: '@_'");
  }
  else {
    CORE::die @_;
  }
}

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
# Below is the stub of documentation for your module. You better edit it!

=head1 NAME

ARB - Perl extension for ARB

=head1 SYNOPSIS

  use ARB;

=head1 DESCRIPTION

The ARB perl module provides access to a ARB databases.  You may
connect to a remote database (e.g. a running instance of ARB_NTREE) or
open your own database.

=head1 AUTHOR

ARB development, devel@arb-home.de

=head1 SEE ALSO

perl(1).

=cut
