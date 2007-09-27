#!/usr/bin/perl
# ================================================================= #
#                                                                   #
#   File      : postcompile.pl                                      #
#   Purpose   : filter gcc shadow spam                              #
#   Time-stamp: <Thu Sep/27/2007 13:40 MET Coder@ReallySoft.de>     #
#                                                                   #
#   Coded by Ralf Westram (coder@reallysoft.de) in September 2007   #
#   Institute of Microbiology (Technical University Munich)         #
#   http://www.arb-home.de/                                         #
#                                                                   #
# ================================================================= #

use strict;
use warnings;

# Note: g++ must be called with -fmessage-length=0

# regexps for whole line:
my $reg_file = qr/^([^:]+):([0-9]+):\s/;
my $reg_included = qr/^In\sfile\sincluded\sfrom\s(.*)[,:]/;
my $reg_included2 = qr/^\s+from\s(.*)[,:]/;
my $reg_location = qr/^[^:]+:\sIn\sfunction\s/;
my $reg_location2 = qr/^[^:]+:\sAt\stop\slevel:/;

# regexps for messages:
my $reg_shadow_warning = qr/^warning:\sdeclaration\sof\s.*\sshadows\s/;
my $reg_shadow_location = qr/^warning:\sshadowed\s/;

# regexps for files:
my $reg_user_include = qr/^\/usr\/include\//;

# output buffer
my @out = ();

sub warning($) {
  my ($msg) = @_;
  push @out, '[postcompile]: '.$msg;
}


my $shadow_warning = undef;

sub store_shadow($) {
  my ($warn) = @_;
  if (defined $shadow_warning) {
    warning('unprocessed shadow_warning:');
    push @out, $shadow_warning;
  }
  $shadow_warning = $warn;
}

my @included = ();
my $location_info = undef;

foreach (<>) {
  chomp;
  if ($_ =~ $reg_file) {
    my ($file,$line,$msg) = ($1,$2,$');

    if ($msg =~ $reg_shadow_warning) {
      if (not $' =~ /this/) { # dont store this warnings (no location follows)
        store_shadow($_);
        $_ = undef;
      }
    }
    elsif ($msg =~ $reg_shadow_location) {
      if (not defined $shadow_warning) { warning('no shadow_warning seen'); }
      else {
        if ($file =~ $reg_user_include or $file eq '<built-in>') {
          # dont warn about /usr/include or <built-in> shadowing
          $_ = undef;
          @included = ();
          $location_info = undef;
        }
        else {
          if (defined $location_info) {
            push @out, $location_info;
            $location_info = undef;
          }
          push @out, $shadow_warning;
        }
        $shadow_warning = undef;
      }
    }

    # if (defined $_) { $_ .= ' [reg_file]'; } # # test
  }
  elsif ($_ =~ $reg_location or $_ =~ $reg_location2) {
    $location_info = $_;
    $_ = undef;
  }
  elsif ($_ =~ $reg_included) {
    push @included, $1;
    $_ = undef;
  }
  elsif (@included) {
    if ($_ =~ $reg_included2) {
      push @included, $1;
      $_ = undef;
    }
  }

  if (defined $_) {
    if (defined $location_info) {
      push @out, $location_info;
      $location_info = undef;
    }
    push @out, $_;
    if (@included) {
      foreach (@included) {
        push @out, $_.': included from here';
      }
    }
  }
}

store_shadow(undef);

foreach (@out) {
  print "$_\n";
}
