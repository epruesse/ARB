#!/usr/bin/perl

use strict;
use warnings;

# ID is just a counter through all error occurrances

my %loc = (); # key=ID content=location
my %num = (); # key=ID content=errornumber
my %msg = (); # key=ID content=message

my %numUsed = (); # key=num content=times used

my $ID = 0;

sub calc_numUsed() {
  foreach (values %num) { $numUsed{$_}=0; }
  foreach (values %num) { $numUsed{$_}++; }
}

sub pad_locs() {
  my $maxlen = 0;
  foreach (values %loc) {
    my $len = length($_);
    if ($len>$maxlen) { $maxlen = $len; }
  }
  %loc = map {
    my $padded = $loc{$_};
    my $len =  length($padded);
    while ($len<$maxlen) {
      $padded .= ' ';
      $len++;
    }

    $_ => $padded;
  } keys %loc;
}

sub parse_errors() {
  my $cmd = "grep -n 'throw_error' *.cxx";
  open(GREP,$cmd.'|') || die "failed to run '$cmd' (Reason: $!)";
  foreach (<GREP>) {
    chomp;
    if (/throw_errorf?\(\s*([0-9]+)\s*,(.*)/) {
      my ($loc,$num,$msg) =  ($`,$1,$2);

      $msg =~ s/\);\s*$//g;
      $loc =~ s/^\s*([^\s]+)\s*.*/$1/og;

      # print "loc='$loc' num='$num' msg='$msg'\n";

      $loc{$ID} = $loc;
      $num{$ID} = $num;
      $msg{$ID} = $msg;

      $ID++;
    }
  }
  close(GREP);

  calc_numUsed();
  pad_locs();
}

sub head($) {
  my ($msg) = @_;
  print "\n---------------------------------------- $msg:\n\n";
}

sub show_by_num() {
  head('sorted by error-number');
  my @ID = sort { $num{$a} <=> $num{$b}; } keys %num;
  foreach $ID (@ID) {
    print sprintf("%s %3i %s\n", $loc{$ID}, $num{$ID}, $msg{$ID});
  }
}
sub show_by_msg() {
  head('sorted by message');
  my @ID = sort { $msg{$a} cmp $msg{$b}; } keys %num;
  foreach $ID (@ID) {
    print sprintf("%s %3i %s\n", $loc{$ID}, $num{$ID}, $msg{$ID});
  }
}

sub main() {
  parse_errors();

  show_by_num();
  show_by_msg();
}
main();
