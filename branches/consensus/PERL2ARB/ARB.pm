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

sub show_arb_message($) {
  my ($msg) = @_;
  $msg =~ s/\n/\\n/g;
  $msg =~ s/'/"/g;
  system("arb_message '$msg'");
}

sub die {
  if ($already_dying==0) {
    $already_dying++; # do not recurse

    ARB::prepare_to_die(); # abort all transactions and close all databases (too avoid deadlock; see #603)

    my ($msg) = @_;
    $msg =~ s/\n+$//g; # remove trailing LFs
    if ($msg eq '') { $msg = 'Unknown macro execution error'; }
    else { $msg = "Macro execution error: '$msg'"; }
    show_arb_message($msg."\n(see console for details)");

    use Carp;
    Carp::confess("Macro execution error: '$msg'"); # recurses into this sub
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
