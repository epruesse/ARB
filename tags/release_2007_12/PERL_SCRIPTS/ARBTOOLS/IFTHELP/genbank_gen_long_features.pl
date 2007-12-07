
#!/usr/bin/perl
# ================================================================ #
#                                                                  #
#   File      : genbank_gen_long_features.pl                       #
#   Purpose   : modifies genbank feature table for easy scanning   #
#               with ARBs ift                                      #
#   Time-stamp: <Fri Dec/07/2007 13:28 MET Coder@ReallySoft.de>    #
#                                                                  #
#   Coded by Ralf Westram (coder@reallysoft.de) in November 2007   #
#   Institute of Microbiology (Technical University Munich)        #
#   http://www.arb-home.de/                                        #
#                                                                  #
# ================================================================ #

use strict;
use warnings;

my $last_feature = undef;
my $line_number = 0;
my $line_to_print = undef;

eval {
  my $mode = 0;
  foreach (<>) {
    $line_number++;
    if ($mode==0) { # copy all till feature table
      if (/^FEATURES/o) { $mode = 1; }
      print $_;
    }
    elsif ($mode==1) { # reformat feature table (qualifiers)
      if (/^                     /o) { # qualifier-line
        my ($white,$rest) = ($&,$');

        chomp($rest);
        if ($rest =~ /^\//o) { # start of qualifier
          if (defined $line_to_print) { print $line_to_print."\n"; $line_to_print=undef; }
          # $line_to_print = $white.$rest;
          $line_to_print = 'FTx  '.$last_feature.substr("                ",length($last_feature)).$rest;
        }
        else {
          if (not defined $line_to_print) {
            die "Found continued qualifier line (expected start of qualifier or new feature)"
          }
          $line_to_print .= $rest;
        }
      }
      elsif (/^ORIGIN/) {
        if (defined $line_to_print) { print $line_to_print."\n"; $line_to_print=undef; }
        print $_;

        $mode=2; # switch mode
      }
      else { # new feature
        if (defined $line_to_print) { print $line_to_print."\n"; $line_to_print=undef; }
        if (/^(     )([a-z]+)( .*)$/io) { # checked - really new feature
          my ($white1,$feature,$rest) = ($1,$2,$3);
          $last_feature = $feature;
          chomp($rest);
          $line_to_print = "FT   ".$feature.$rest;
        }
        else {
          die "Unexpected case (expected new feature)";
        }
      }
    }
    else { # mode==2 -> copy sequence
      print $_;
      if ($_ eq "//\n") {
        $mode = 0; # reset mode
      }
    }
  }
  if (defined $line_to_print) { die "Unexpected content in internal feature-buffer"; }
};
if ($@) {
  chomp $@;
  die "$@ in line $line_number of inputfile\n";
}
