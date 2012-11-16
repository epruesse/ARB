#!/usr/bin/perl

use strict;
use warnings;

sub version_compare($$);
sub version_compare($$) {
  my ($v1,$v2) = @_;

  if (not defined $v1) {
    $v1 = 0;
    if (not defined $v2) {
      return 0;
    }
  }
  elsif (not defined $v2) { $v2 = 0; }

  my ($mj1,$mn1) = ($v1,undef);
  my ($mj2,$mn2) = ($v2,undef);

  if ($v1 =~ /\./) { ($mj1,$mn1) = ($`,$'); }
  if ($v2 =~ /\./) { ($mj2,$mn2) = ($`,$'); }

  my $cmp = $mj1 <=> $mj2;
  if ($cmp==0) {
    $cmp = version_compare($mn1,$mn2);
  }
  return $cmp;
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=2) {
    die "Usage: requireVersion.pl x.x y.y\n".
      "       Prints 1 if y.y is greater equal the required version x.x\n";
  }

  my $req  = $ARGV[0];
  my $test = $ARGV[1];

  my $cmp = version_compare($req,$test);
  if ($cmp<=0) { print "1\n"; }
  else { print "0\n"; }
}
main();
