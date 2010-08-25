#!/usr/bin/perl

use strict;
use warnings;

# --------------------------------------------------------------------------------

my %location = (); # key=symbol, value=location
my %exported = (); # key=exported symbols
my %simple_test = (); # key=names of existing simple test functions
my %test_priority = (); # key=test value=priority

# --------------------------------------------------------------------------------

my $warn_level = 1;

sub symbol_message($$$) {
  my ($symbol,$message,$type) = @_;
  my $loc = $location{$symbol};

  if (defined $loc) {
    print STDERR "$loc: $type: $message ($symbol)\n";
  }
  else {
    print STDERR "sym2testcode.pl: Error: Location of '$symbol' is unknown\n";
    print STDERR "sym2testcode.pl: $type: $message ($symbol)\n";
  }
}
sub symbol_warning($$) { my($symbol, $message)= @_; $warn_level==0 || symbol_message($symbol, $message, "Warning"); }
sub symbol_error($$)   { my($symbol, $message)= @_; symbol_message($symbol, $message, "Error");   }

sub calculate_priorities() {
  foreach (keys %simple_test) {
    if (/^TEST_SLOW_/)        { $test_priority{$_} = 900; }
    elsif (/^TEST_([0-9]+)_/) { $test_priority{$_} = $1; }
    else                      { $test_priority{$_} = 100; }
  }
}

sub parse($) {
  my ($nm_output) = @_;
  open(IN,'<'.$nm_output) || die "can't read '$nm_output' (Reason: $!)";

  my $line;
  my $lineNr=0;
  eval {
  LINE: while (defined ($line = <IN>)) {
      $lineNr++;
      if ($line =~ /^(([0-9a-f]|\s)+) (.) (.*)\n/o) {
        my ($type,$rest) = ($3,$4);
        my $symbol;
        my $location = undef;
        if ($rest =~ /\t/o) {
          ($symbol,$location) = ($`,$');
        }
        else { # symbol w/o location
          $symbol = $rest;
        }

        next LINE if ($symbol =~ /::/); # skip static variables and other scoped symbols
        if ($symbol =~ /\(/o) { $symbol = $`; } # skip prototype
        if (defined $location) { $location{$symbol} = $location; }

        my $is_unit_test     = undef;
        my $is_disabled_test = undef;

        if ($symbol =~ /^TEST_/o) { $is_unit_test = 1; }
        elsif ($symbol =~ /TEST_/o) { $is_disabled_test = 1; }

        my $is_global_symbol = ($type eq 'T');

        if ($is_global_symbol) { $exported{$symbol} = 1; }

        if ($is_unit_test) {
          if ($is_global_symbol) {
            $simple_test{$symbol} = 1;
          }
          else {
            symbol_warning($symbol, "unit-tests need global scope (type='$type' symbol='$symbol')");
          }
        }
        elsif ($is_disabled_test) {
          if ($is_global_symbol) {
            symbol_warning($symbol, "Test looks disabled");
          }
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

sub filter($) {
  my ($expr) = @_;
  my $regexpr = qr/$expr/;

  my %del = ();
  foreach my $symbol (keys %location) {
    my $loc = $location{$symbol};
    if (not $loc =~ $regexpr) { $del{$symbol} = 1; }
    else { $del{$symbol} = 0; }
  }

  {
    my $warn = 0;
    foreach (sort keys %simple_test) {
      my $del = $del{$_};
      if (defined $del and $del==1) {
        if ($warn==0) {
          print "Skipped tests (restriction set to '$expr'):\n";
          $warn = 1;
        }
        print '* '.$_."\n";
      }
    }
  }

  %simple_test = map {
    my $del = $del{$_};
    if (not defined $del or $del==0) { $_ => $simple_test{$_}; }
    else { ; }
  } keys %simple_test;
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

  calculate_priorities();

  my @tests = sort {
    my $prioa = $test_priority{$a};
    my $priob = $test_priority{$b};

    my $cmp = $prioa - $priob;

    if ($cmp ==0) {
      my $loca = $location{$a};
      my $locb = $location{$b};
      if (defined $loca) {
        if (defined $locb) { $cmp = $loca cmp $locb; }
        else { $cmp = 1; }
      }
      else {
        if (defined $locb) { $cmp = -1; }
        else { $cmp = $a cmp $b; }
      }
    }
    $cmp;
  } keys %$id_r;

  my $code = '';

  # "prototypes"
  foreach (@tests) {
    $code .= &$prototyper_r($_);
  }
  $code .= "\n";

  # table
  $code .= 'static '.$type.' '.$name.'[] = {'."\n";
  foreach (@tests) {
    my $loc = $location{$_};
    if (defined $loc)  { $loc = '"'.$loc.'"'; }
    else { $loc = 'NULL'; }

    $code .= '    { '.$_.', "'.$_.'", '.$loc.' },'."\n";
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
  $UNIT_TESTER .= ', '.$warn_level;
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
  if ($args != 5) {
    die("Usage: sym2testcode.pl libname restrict-expr nm-output gen-cxx warn-level\n".
        "    libname        name of library to run tests for\n".
        "    restrict-expr  regexpr to restrict to specific module in library\n".
        "    nm-output      output of nm\n".
        "    gen_cxx        name of C++ file to generate\n".
        "    warn-level     (0=quiet|1=noisy)\n".
        "Error: Expected 5 arguments\n");
  }

  my $libname   = $ARGV[0];
  my $restrict  = $ARGV[1];
  my $nm_output = $ARGV[2];
  my $gen_cxx   = $ARGV[3];
  $warn_level   = $ARGV[4];

  parse($nm_output);
  filter($restrict);
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
