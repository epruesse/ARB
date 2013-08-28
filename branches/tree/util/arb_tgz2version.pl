#!/usr/bin/perl

use strict;
use warnings;

sub main() {
  my $args = scalar(@ARGV);
  if ($args != 1) { die "Usage: arb_tgz2version arbsrc.tgz\n"; }

  my $tgz = $ARGV[0];
  if (not -f $tgz) { die "No such file '$tgz'"; }

  my $command = "tar --wildcards -Ozxf $tgz '*/SOURCE_TOOLS/version_info'";
  open(PIPE,$command.'|') || die "can't execute '$command' (Reason: $!)";

  my $major = undef;
  my $minor = undef;
  foreach (<PIPE>) {
    if (/^MAJOR=([0-9]+)$/) { $major = $1; }
    if (/^MINOR=([0-9]+)$/) { $minor = $1; }
  }
  close(PIPE);

  if (defined $major and defined $minor) {
    print "ARB_VERSION=arb_$major.$minor\n";
  }
  else {
    die "Failed to retrieve version from '$tgz'";
  }
}

main();
