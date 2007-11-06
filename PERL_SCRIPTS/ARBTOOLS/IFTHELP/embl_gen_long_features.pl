#!/usr/bin/perl
#  =============================================================== #
#                                                                  #
#    File      : embl_gen_long_features.pl                         #
#    Purpose   : modifies embl feature table for easy scanning     #
#                with ARBs ift                                     #
#    Time-stamp: <Fri Mar/18/2005 15:27 MET Coder@ReallySoft.de>   #
#                                                                  #
#    Coded by Ralf Westram (coder@reallysoft.de) in March 2005     #
#    Institute of Microbiology (Technical University Munich)       #
#    http://www.arb-home.de/                                       #
#                                                                  #
#  =============================================================== #

use strict;
use warnings;

my $last_feature = undef;
my $line_number = 0;
my $line_to_print = undef;

eval {
  foreach (<>) {
    $line_number++;
    if (/^FT   /) {
      chomp;
      my $rest = $';
      if ($rest =~ /^([^ ]+)([ ]+)([^ ].*)$/) { # new feature
        my ($name,$spaces,$content) = ($1,$2,$3);
        $last_feature = $name.$spaces;
        if (defined $line_to_print) { print $line_to_print."\n"; $line_to_print=undef; }
        print "FT   $last_feature$content\n";
      }
      else { # continue last feature
        if (defined $last_feature) {
          if ($rest =~ /^                (.*)$/) {
            my $content = $1;
            if ($content =~ /^\//) { # start of new sub-entry
              if (defined $line_to_print) { print $line_to_print."\n"; $line_to_print=undef; }
              $line_to_print = "FTx  $last_feature$content";
            }
            else {
              $line_to_print .= "$content"; # append to previously started sub entry
            }
          }
          else {
            die "Expected some content behind 'FT'\n";
          }
        }
        else {
          die "Expected start of feature (e.g. 'FT   bla')\n";
        }
      }
    }
    else {
      if (defined $line_to_print) { print $line_to_print."\n"; $line_to_print=undef; }
      print $_;
    }
  }
  if (defined $line_to_print) { print $line_to_print."\n"; $line_to_print=undef; }
};
if ($@) {
  chomp $@;
  die "$@ in line $line_number of inputfile\n";
}
