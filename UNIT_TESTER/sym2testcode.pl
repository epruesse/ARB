#!/usr/bin/perl

use strict;
use warnings;

# --------------------------------------------------------------------------------

my %location = (); # key=symbol, value=location

my %exported = (); # key=exported symbols

my %simple_test = (); # key=names of existing simple test functions


# --------------------------------------------------------------------------------

sub symbol_error($$) {
  my ($symbol,$message) = @_;
  my $loc = $location{$symbol};

  if (defined $loc) {
    print STDERR "$loc: Error: $message ($symbol)\n";
  }
  else {
    print STDERR "sym2testcode.pl: Error: Location of '$symbol' is unknown\n";
    print STDERR "sym2testcode.pl: Error: $message ($symbol)\n";
  }
}

sub parse($) {
  my ($nm_output) = @_;
  open(IN,'<'.$nm_output) || die "can't read '$nm_output' (Reason: $!)";

  my $line;
  my $lineNr=0;
  eval {
    while (defined ($line = <IN>)) {
      $lineNr++;
      if ($line =~ /^........ (.) (.*)\n/o) {
        my ($type,$rest) = ($1,$2);
        my $symbol;
        if ($rest =~ /\t/o) {
          my $location;
          ($symbol,$location) = ($`,$');
          if ($symbol =~ /\(/o) { $symbol = $`; } # skip prototype
          $location{$symbol} = $location;
        }
        else { # symbol w/o location
          $symbol = $rest;
        }

        my $is_unit_test = undef;
        if ($symbol =~ /^TEST_/o) { $is_unit_test = 1; }

        if ($type eq 'T') {
          $exported{$symbol} = 1;
        }
        else {
          if ($is_unit_test) {
            symbol_error($symbol, "unit-test needs global scope");
          }
        }
        if ($is_unit_test) {
          $simple_test{$symbol} = 1;
        }
      }
      else {
        die "can't parse line '$line'\n";
      }
    }
  };
  if ($@) {
    print "$nm_output:$lineNr: $@\n";
    die;
  }

  close(IN);
}

# --------------------------------------------------------------------------------

sub UT_type($) { my ($name) = @_; return 'UnitTest_'.$name; }
sub UT_name($) { my ($name) = @_; return 'unitTest_'.$name; }

sub prototype_simple($) {
  my ($fun) = @_;
  return 'void '.$fun.'();'."\n";
}

sub generate_table($$\%\&) {
  my ($type,$name,$id_r,$prototyper_r) = @_;

  my @tests = sort keys %$id_r;

  my $code = '';

  # "prototypes"
  foreach (@tests) {
    $code .= &$prototyper_r($_);
  }
  $code .= "\n";

  # table
  $code .= 'static '.$type.' '.$name.'[] = {'."\n";
  foreach (@tests) {
    $code .= '    { '.$_.', "'.$_.'", "'.$location{$_}.'" },'."\n";
  }
  $code .= '    { NULL, NULL, NULL },'."\n";
  $code .= '};'."\n";
  $code .= "\n";

  return $code;
}

sub create($$) {
  my ($libname,$gen_cxx) = @_;
  open(OUT, '>'.$gen_cxx) || die "can't write '$gen_cxx' (Reason: $!)";

  my $HEAD = <<HEAD;
#include <UnitTester.hxx>
#include <cstdlib>
HEAD

  my $TABLES = generate_table(UT_type('simple'), UT_name('simple'), %simple_test, &prototype_simple);

  my $UNIT_TESTER = 'UnitTester unitTester("'.$libname.'", ';
  $UNIT_TESTER .= UT_name('simple');
  $UNIT_TESTER .= ');';

  my $MAIN = '';
  my $have_main = defined $exported{'main'};
  if ($have_main==0) {
    $MAIN .= 'int main(void) {'."\n";
    $MAIN .= '    '.$UNIT_TESTER."\n";
    $MAIN .= '    return EXIT_SUCCESS;'."\n";
    $MAIN .= '}'."\n";
  }
  else {
    $MAIN .= 'static '.$UNIT_TESTER."\n";
  }

  print OUT $HEAD."\n";
  print OUT $TABLES."\n";
  print OUT $MAIN."\n";
  close(OUT);
}

# --------------------------------------------------------------------------------

sub main() {
  my $args = scalar(@ARGV);
  if ($args != 3) {
    die "Usage: sym2testcode.pl libname nm-output gen-cxx\n".
      "Error: Expected 2 arguments\n";
  }

  my $libname   = $ARGV[0];
  my $nm_output = $ARGV[1];
  my $gen_cxx   = $ARGV[2];

  parse($nm_output);
  eval {
    create($libname,$gen_cxx);
  };
  if ($@) {
    my $err = "Error: Failed to generate '$gen_cxx' (Reason: $@)";
    if (-f $gen_cxx) {
      if (not unlink($gen_cxx)) {
        $err .= "\nError: Failed to unlink '$gen_cxx' (Reason: $!)";
      }
    }
    die $err;
  }
}
main();
