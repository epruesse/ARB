#!/usr/bin/perl

use strict;
use warnings;
# use diagnostics;

# --------------------------------------------------------------------------------

my $ARBHOME = $ENV{ARBHOME};
if (not defined $ARBHOME) { die "Environmentvariable ARBHOME has be defined"; }
if (not -d $ARBHOME) { die "ARBHOME ('$ARBHOME') does not point to a valid directory"; }

# --------------------------------------------------------------------------------

my %allowDup = ( 'MACH' => 1 );

my %old = (); # key = var, value = line

# --------------------------------------------------------------------------------

sub analyze_line($) {
  my ($line) = @_;
  my ($commented, $var, $value,$comment) = ( 0, undef, undef );

  my $rest;

  if ($line =~ /^\s*([\#]*)\s*([A-Z][A-Z_0-9]+)\s*\:\=\s*([A-Z_0-9]+)\s*/o) {
    ($var,$value) = ($2,$3);
    if ($1 eq '#') { $commented = 1; }
    $rest = $';
  }
  else {
    $rest = $line;
  }

  if ($rest =~ /\s*\#\s*/o) {
    $comment = $';
  }

  if (0 and defined $var) {
    print "line     ='$line'\n";
    print "commented='$commented'\n";
    print "var      ='$var'\n";
    print "value    ='$value'\n";
    if (defined $comment) { print "comment  ='$comment'\n"; }
  }

  return ($commented, $var, $value,$comment);
}

sub loadOldConfig($) {
  my ($cfg) = @_;

  open(CFG,'<'.$cfg) || die "can't read '$cfg' (Reason: $!)";
  foreach my $line (<CFG>) {
    chomp($line);
    my ($commented, $var, $value,$comment) = analyze_line($line);
    if (defined $var) {
      if (defined $allowDup{$var}) {
        my $dupId = $var.'#'.$value;
        if (defined $old{$dupId}) { die "Duplicated entry for '$var=$value' in '$cfg'"; }
        $old{$dupId} = $line;
      }
      else {
        if (defined $old{$var}) { die "Duplicated entry for '$var' in '$cfg'"; }
        $old{$var} = $line;
      }
    }
  }
  close(CFG);
}

sub gen_line($$$$) {
  my ($commented, $var, $value, $comment) = @_;
  my $line;
  if (defined $var) {
    defined $value || die "var without value";
    $line = ($commented==1 ? "# ": "").$var.' := '.$value;
    if (defined $comment) {
      $line .= '# '.$comment;
    }
  }
  else {
    if (defined $comment) {
      $line = '# '.$comment;
    }
    else {
      $line = '';
    }
  }
  return $line;
}

sub updateConfig($$) {
  my ($src,$dest) = @_;

  # die "test";

  open(IN,'<'.$src) || die "can't read '$src' (Reason: $!)";
  open(OUT,'>'.$dest) || die "can't write '$dest' (Reason: $!)";

  foreach my $line (<IN>) {
    chomp($line);
    my ($commented, $var, $value, $comment) = analyze_line($line);

    if ($var) {
      my $oline = undef;
      if (defined $allowDup{$var}) {
        my $dupId = $var.'#'.$value;
        $oline = $old{$dupId};
      }
      else {
        $oline = $old{$var};
      }

      if (defined $oline) {
        my ($commented_old, $var_old, $value_old, $comment_old) = analyze_line($oline);

        print OUT gen_line($commented_old, $var_old, $value_old, $comment); # old settings, new comment
      }
      else {
        print OUT gen_line($commented, $var, $value, $comment); # line was not defined in old cfg
      }
    }
    else {
      print OUT gen_line($commented, $var, $value, $comment); # non-var line
    }
    print OUT "\n";
  }

  close(OUT);
  close(IN);
}

sub main() {
  my $dest = $ARBHOME.'/config.makefile';
  my $src  = $dest.'.template';

  if (not -f $dest) {
    my $cmd = "cp -p '$src' '$dest'";
    system($cmd)==0 || die "Failed to execute '$cmd' (result=$?)";
  }
  else {
    loadOldConfig($dest);

    my $backup = $dest.'.bak';
    my $cmd    = "cp -p '$dest' '$backup'";
    system($cmd)==0 || die "Failed to execute '$cmd' (result=$?)";

    my $new = $dest.'.new';
    updateConfig($src,$new);

    $cmd = "cp -p '$new' '$dest'";
    system($cmd)==0 || die "Failed to execute '$cmd' (result=$?)";

    unlink($new);
  }
}

main();
