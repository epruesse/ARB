#!/usr/bin/perl

use strict;
use warnings;

sub clustalTopic2Section($) {
  my ($line) = @_;
  if ($line =~ /^\s*\>\>\s*([^<>]+)\s*\<\<\s*(.*)$/o) {
    # '>>topic<< moretext' -> 'SECTION topic moretext'
    $line = "SECTION $1 $2\n";
  }
  return $line;
}

sub plus2star($) {
  my ($line) = @_;
  $line =~ s/^(\s+)\+\s/$1\* /;
  return $line;
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=1) {
    die "Usage: postprocess_genhelp.pl filename\n".
      "(filters stdin to stdout, filename is only passed to trigger specific behavior)\n ";
  }
  my $filename = $ARGV[0];

  my @reg = ();
  my @fun = ();

  if ($filename =~ /clustalw/) {
    push @reg, qr/([^\s]\s)\s+([\(])/;
    push @reg, qr/([\.:])(\s)\s+/;
    push @fun, \&clustalTopic2Section;
  }
  elsif ($filename =~ /readseq/) {
    push @reg, qr/([\.])(\s)\s+/;
    push @reg, qr/(LINE\s)\s+([0-9]+)/;
    push @fun, \&plus2star;
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

    foreach my $re (@reg) {
      while ($_ =~ $re) {
        $_ = $`.$1.$2.$';
      }
    }
    foreach my $fn (@fun) {
      $_ = &$fn($_);
    }
    print $_;
  }
}
main();
