#!/usr/bin/perl

use strict;
use warnings;

# ID is just a counter through all error occurrences

my %loc = (); # key=ID content=location
my %num = (); # key=ID content=errornumber
my %msg = (); # key=ID content=message
my %type = (); # key=ID content=W|E|? (warning|error|unknown)

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
  my $cmd = "grep -En 'throw_error|warning' *.cxx *.h";
  open(GREP,$cmd.'|') || die "failed to run '$cmd' (Reason: $!)";
 LINE: foreach (<GREP>) {
    chomp;
    if (/(throw_errorf?|warningf?)\(\s*([0-9]+)\s*,(.*)/) {
      my ($loc,$what,$num,$msg) =  ($`,$1,$2,$3);

      my $msgType = '?';
      if ($what =~ /warning/o) { $msgType = 'W'; }
      if ($what =~ /error/o) { $msgType = 'E'; }
      if ($msgType eq '?') {
        die "what='$what'";
      }

      if ($loc =~ /\/\//o) { next LINE; } # ignore comments

      $msg =~ s/\);\s*$//g;
      $loc =~ s/^\s*([^\s]+)\s*.*/$1/og;

      # print "loc='$loc' num='$num' msg='$msg'\n";

      $loc{$ID} = $loc;
      $num{$ID} = $num;
      $msg{$ID} = $msg;
      $type{$ID} = $msgType;

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

sub show(\@) {
  my ($ID_r) = @_;
  foreach $ID (@$ID_r) {
    print sprintf("%s %3i [%s] %s\n", $loc{$ID}, $num{$ID}, $type{$ID}, $msg{$ID});
  }
}

sub show_by_num() {
  head('sorted by error-number');
  my @ID = sort { $num{$a} <=> $num{$b}; } keys %num;
  show(@ID);
}
sub show_by_msg() {
  head('sorted by message');
  my @ID = sort { $msg{$a} cmp $msg{$b}; } keys %num;
  show(@ID);
}

sub show_duplicates() {
  my @ID = sort { $num{$a} <=> $num{$b}; } keys %num;
  my %seen = (); # key=num, value=ID of 1st occurrence
  my $err_count = 0;

  foreach $ID (@ID) {
    my $num = $num{$ID};
    my $type = $type{$ID};
    my $key = $type.'.'.$num;
    my $seen = $seen{$key};
    if (defined $seen) {
      my $what = $type eq 'E' ? 'error' : 'warning';
      print STDERR sprintf("%s Error: Duplicated use of $what-code '%i'\n", $loc{$ID}, $num);
      my $id_1st = $seen{$type.'.'.$num};
      print STDERR sprintf("%s (%i first used here)\n", $loc{$id_1st}, $num{$id_1st});
      $err_count++;
    }
    else {
      $seen{$key} = $ID;
    }
  }
  return ($err_count>0);
}

sub main() {
  my $mode = $ARGV[0];
  if (not defined $mode or $mode =~ /help/i) {
    print "Usage: show_error_list.pl [-check|-listnum|-listmsg]\n";
    die "No mode specified";
  }
  else {
    parse_errors();

    if ($mode eq '-check') { show_duplicates(); }
    elsif ($mode eq '-listnum') { show_by_num(); }
    elsif ($mode eq '-listmsg') { show_by_msg(); }
    else { die "Unknown mode"; }
  }
}
main();
