#!/usr/bin/perl

use strict;
use warnings;

sub get_depends($) {
  my ($lib) = @_;
  my @depends = ();

  my $cmd = './needed_libs.pl -D -U -n '.$lib;
  # print "cmd=$cmd\n";

  open(DEPENDS,$cmd.'|') || die "failed to execute '$cmd' (exitcode=$?)";
  foreach (<DEPENDS>) {
    chomp;
    my @d = split(/ /,$_);
    foreach (@d) { push @depends, $_; }
  }
  close(DEPENDS);

  return @depends;
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=1) {
    die "Usage: gen_dep.pl suffix\nFilter to create dep.*-files\n";
  }

  my $suffix = shift @ARGV;
  foreach my $lib (<>) {
    chomp($lib);

    my @depends_on = get_depends($lib);

    my $base = $lib;
    $base =~ s/\.(a|o|so)$//;
    if ($base =~ /^lib\/lib/) { $base = $'.'/'.$'; }

    print "$base.$suffix:";
    foreach (@depends_on) {
      print " $_.$suffix";
    }
    print "\n";
  }
}
main();

