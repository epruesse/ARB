#!/usr/bin/perl

use strict;
use warnings;
# use diagnostics;

# --------------------------------------------------------------------------------

my $ARBHOME = $ENV{ARBHOME};
if (not defined $ARBHOME) { die "Environmentvariable ARBHOME has be defined"; }
if (not -d $ARBHOME) { die "ARBHOME ('$ARBHOME') does not point to a valid directory"; }


sub main() {
  print "Touching modified files:\n";

  my $svn_status = "svn status";
  open(STATUS, $svn_status.'|') || die "can't execute '$svn_status' (Reason: $!)";
  foreach my $line (<STATUS>) {
    chomp($line);
    my $handled = 0;
    if ($line =~ /^([^\s]+)[\s+]+(.*)$/) {
      my ($st,$file) = ($1,$2);
      my $touch         = 0;

      if (($st eq 'M') or ($st eq 'R') or ($st eq 'A')) { # touch modified, replaced and added files
        $touch = 1;
      }
      elsif (($st eq '?') or ($st eq 'D')) { # ignore unknown and deleted files
        $handled = 1;
      }

      if ($touch==1) {
        if (-d $file) {
          $handled = 1;
        }
        else {
          if (not -f $file) { die "File '$file' not found (statusline='$line')" }

          my $touch_cmd = "touch '$file'";
          print '['.$touch_cmd."]\n";
          system($touch_cmd);
          $handled = 1;
        }
      }
    }
    elsif ($line =~ /^\s+M\s+\.$/) { # accept changed merge-info for rootdir
      $handled = 1;
    }
    if ($handled==0) { die "Can't handle status line '$line'"; }
  }
  close(STATUS);
}

main();
