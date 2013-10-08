#!/usr/bin/perl

use strict;
use warnings;

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=1) {
    die "Usage: postprocess_genhelp.pl filename\n".
      "(filters stdin to stdout, filename is only passed to trigger specific behavior)\n ";
  }

  my $titled = 0;

  foreach (<STDIN>) {
    # convert all lines starting at column zero into SECTIONs
    if (s/^([^ \#\n])/SECTION $1/) {
      # convert first SECTION into TITLE
      if ($titled==0) {
        s/^SECTION/TITLE/;
        $titled=1;
      }
    }
    print $_;
  }
}
main();
