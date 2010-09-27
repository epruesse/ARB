#!/usr/bin/perl
# ================================================================= #
#                                                                   #
#   File      : postcompile.pl                                      #
#   Purpose   : filter gcc shadow spam                              #
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
my $reg_file = qr/^([^:]+):([0-9]+):(([0-9]+):)?\s/; # finds all messages
my $reg_included = qr/^In\sfile\sincluded\sfrom\s(.*)[,:]/;
my $reg_included2 = qr/^\s+from\s(.*)[,:]/;
my $reg_location = qr/^[^:]+:\sIn\sfunction\s/;
my $reg_location2 = qr/^[^:]+:\sAt\stop\slevel:/;

# regexps for messages:
my $reg_shadow_warning = qr/^warning:\sdeclaration\sof\s.*\sshadows\s/;
my $reg_shadow_location = qr/^warning:\sshadowed\s/;
my $reg_is_error = qr/^error:\s/i;

# regexps for files:
my $reg_user_include = qr/^\/usr\/include\//;

my $stop_after_first_error = 0;
my $hide_warnings          = 0;

sub warning($\@) {
  my ($msg,$out_r) = @_;
  push @$out_r, '[postcompile]: '.$msg;
}


my $shadow_warning = undef;

sub store_shadow($\@) {
  my ($warn,$out_r) = @_;
  if (defined $shadow_warning) {
    warning('unprocessed shadow_warning:', @$out_r);
    push @$out_r, $shadow_warning;
  }
  $shadow_warning = $warn;
}

sub push_loc_and_includes($$\@\@) {
  my ($location_info,$message,$included_r,$out_r) = @_;
  if (defined $location_info) {
    push @$out_r, $location_info;
    $location_info = undef;
  }
  push @$out_r, $message;
  foreach (@$included_r) {
    push @$out_r, $_.': included from here';
  }
}

sub parse_input(\@) {
  my ($out_r) = @_;
  my @included = ();
  my $location_info = undef;

  my @out = ();
  my @errout = ();

 LINE: foreach (<>) {
    chomp;
    my $is_error=0;
    if ($_ =~ $reg_file) {
      my ($file,$line,$msg) = ($1,$2,$');

      if ($msg =~ $reg_shadow_warning) {
        if (not $' =~ /this/) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                # don't store this warnings (no location follows)
          store_shadow($_,@out);
          $_ = undef;
        }
      }
      elsif ($msg =~ $reg_shadow_location) {
        if (not defined $shadow_warning) { warning('no shadow_warning seen',@out); }
        else {
          if ($file =~ $reg_user_include or $file eq '<built-in>') {
            # don't warn about /usr/include or <built-in> shadowing
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
      elsif ($msg =~ $reg_is_error) {
        $is_error = 1;
      }
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
      my $curr_out_r = $is_error==1 ? \@errout : \@out;
      push_loc_and_includes($location_info,$_,@included,@$curr_out_r);
      @included = ();

      if (($is_error==1) and ($stop_after_first_error==1)) {
        @out = (); # drop warnings
        last LINE;
      }
    }
  }

  @$out_r = @errout;
  if ($hide_warnings==0) { push @$out_r, @out; }
}

sub main() {
  my $args = scalar(@ARGV);
  while ($args>0) {
    my $arg = shift(@ARGV);
    if ($arg eq '--no-warnings') { $hide_warnings = 1; }
    elsif ($arg eq '--only-first-error') { $stop_after_first_error = 1; }
    else {
      die "Usage: postcompile.pl [--no-warnings] [--only-first-error]\n";
    }
    $args--;
  }

  my @out = ();
  parse_input(@out);
  store_shadow(undef,@out);
  foreach (@out) { print "$_\n"; }
}
main();
