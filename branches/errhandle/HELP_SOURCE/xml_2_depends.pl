#!/usr/bin/perl

use strict;
use warnings;

sub main() {
  my $args = scalar(@ARGV);
  if ($args<2) {
    die ("Usage: xml_2_depends.pl XSLTPROC STYLESHEET XSLTPROCARGS\n".
         "   or  xml_2_depends.pl CHECK RESULTOFFIRSTCALL");
  }

  my $first = shift @ARGV;

  if ($first eq'CHECK') {
    # second pass:
    # * check for invalid dependencies
    # * if found -> delete depending .xml
    if ($args>2) { die "Too many arguments"; }
    my $depends = shift(@ARGV);
    my $deleted_xml = 0;
    open(DEPENDS,'<'.$depends) || die "can't read '$depends' (Reason: $!)";
    while (my $line=<DEPENDS>) {
      chomp($line);

      my @depends = split(/[ :]+/,$line);
      my $count   = scalar(@depends);

      my $xml = $depends[0];
      my $invalid = undef;
      for (my $d=1; ($d<$count) and (not defined $invalid); ++$d) {
        my $hlp = $depends[$d];
        if (not -f $hlp) {
          $invalid = $hlp;
        }
      }
      if (defined $invalid) {
        print STDERR "Invalid dependency '$invalid' found for '$xml' -> deleting .xml\n";
        unlink($xml) || die "Failed to unlink '$xml' (Reason: $!)";
        $deleted_xml++;
      }
    }
    close(DEPENDS);

    if ($deleted_xml>0) {
      print STDERR "Deleted $deleted_xml .xml-files that had invalid dependencies\n";
      print STDERR "Removing $depends\n";
      unlink($depends) || die "Failed to unlink '$depends' (Reason: $!)";
      die "Can't continue - please rerun (maybe caused by renaming a helpfile)";
    }
  }
  else {
    # first pass: generate dependencies from .xml
    if ($args<3) { die "Missing arguments"; }

    my $XSLTPROC     = $first;
    my $STYLESHEET   = shift @ARGV;
    my $XSLTPROCARGS = join(' ',@ARGV);

    while (my $line=<STDIN>) {
      chomp($line);
      my @xml = split(/ /,$line);
      foreach my $xml (@xml) {
        my $cmd = $XSLTPROC.' --stringparam xml '.$xml.' '.$XSLTPROCARGS.' '.$STYLESHEET.' '.$xml;
        system($cmd)==0 || die "Failed to execute '$cmd' (Reason: $?)";
      }
    }
  }
}
main();
