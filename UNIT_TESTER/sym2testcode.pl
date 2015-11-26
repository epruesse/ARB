#!/usr/bin/perl

use strict;
use warnings;

# --------------------------------------------------------------------------------

my %location      = (); # key=symbol, value=location
my %exported      = (); # key=exported symbols
my %simple_test   = (); # key=names of existing simple test functions; value=1
my %postcond      = (); # key=names of existing post-condition tests; value=1
my %skipped_test  = (); # like simple_test, but not performed (atm)
my %test_priority = (); # key=test value=priority

# --------------------------------------------------------------------------------

my $warn_level = undef;

# when file exists, skip slow tests (see reporter.pl@SkipSlow)
my $skip_slow  = (-e 'skipslow.stamp');

# --------------------------------------------------------------------------------

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


sub fail_if_no_tests_defined($) {
  my ($libname) = @_;
  my $skipped = scalar(keys %skipped_test);
  my $active  = scalar(keys %simple_test);

  if (($skipped+$active)==0) {
    my $makefileDefiningTests = $ENV{ARBHOME}.'/Makefile';
    # my $makefileDefiningTests = '../Makefile';
    my $thisTest = $libname;

    $thisTest =~ s/\.(a|o|so)/.test/o;
    $thisTest =~ s/^lib//o;

    my $cmd = "grep -Hn '$thisTest' $makefileDefiningTests";

    open(GREP,$cmd.'|') || die "can't execute '$cmd' (Reason: $!)";
    my $lineCount = 0;
    foreach (<GREP>) {
      if (/^([^:]+:[0-9]+:)/o) {
        my ($loc,$line) = ($1,$');
        chomp($line);
        print $1.' Error: No tests defined by '.$libname." (do not call this test!)\n";
        $lineCount++;
      }
      else { print "unhandled grep out='$_'\n"; }
    }
    close(GREP);
    if ($lineCount!=1) {
      die "expected exactly one line from grep (got $lineCount)\n".
        "grep-cmd was '$cmd'";
    }
    die "sym2testcode.pl: won't generate useless test code\n";
  }

  return $active; # return number of active tests
}

sub skip_slow_tests() {
  foreach (keys %simple_test) {
    if (/^TEST_SLOW_/) { $skipped_test{$_} = 1; }
  }
  foreach (keys %skipped_test) { delete $simple_test{$_}; }
}

sub calculate_priorities() {
  foreach (keys %simple_test) {
    if    (/^TEST_BASIC_/)      { $test_priority{$_} = 20; }
    elsif (/^TEST_EARLY_/)      { $test_priority{$_} = 50; }
    elsif (/^TEST_LATE_/)       { $test_priority{$_} = 200; }
    elsif (/^TEST_SLOW_/)       { $test_priority{$_} = 900; }
    elsif (/^TEST_AFTER_SLOW_/) { $test_priority{$_} = 910; }
    elsif (/^TEST_([0-9]+)_/)   { $test_priority{$_} = $1; }
    else                        { $test_priority{$_} = 100; }
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
      chomp($line);
      if ($line =~ /^(([0-9a-f]|\s)+) (.+?) (.*)$/o) {
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
        my $is_postcond      = undef;
        my $is_disabled_test = undef;

        if ($symbol =~ /^TEST_/o) {
          $is_unit_test = 1;
          if ($' =~ /^POSTCOND_/o) { $is_postcond = 1; }
        }
        elsif ($symbol =~ /TEST_/o) {
          if (not $` =~  /publish/) { # skip publishers
            $is_disabled_test = 1;
          }
        }

        my $is_global_symbol = ($type eq 'T');

        if ($is_global_symbol) { $exported{$symbol} = 1; }

        if ($is_unit_test) {
          if ($is_global_symbol) {
            if ($is_postcond) {
              $postcond{$symbol} = 1;
            }
            else {
              $simple_test{$symbol} = 1;
            }
          }
          else {
            symbol_warning($symbol, "unit-tests need global scope (type='$type' symbol='$symbol')");
          }
        }
        elsif ($is_disabled_test) {
          if ($is_global_symbol) {
            symbol_warning($symbol, "Test looks disabled");
            $skipped_test{$symbol} = 1; # just note down for summary
          }
        }
      }
      elsif (($line ne "\n") and ($line ne '') and
             not ($line =~ /^[A-Za-z0-9_]+\.o:$/) and
             not ($line =~ /\([A-Za-z0-9_]+\.o\):$/)) {
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

sub filter($$) {
  my ($expr_mod,$expr_fun) = @_;

  my $reg_mod = qr/$expr_mod/;
  my $reg_fun = qr/$expr_fun/;

  my %del = ();
  foreach my $symbol (keys %simple_test) {
    my $loc = $location{$symbol};
    if (defined $loc and not $loc =~ $reg_mod) { $del{$symbol} = 1; }
    elsif (not $symbol =~ $reg_fun) { $del{$symbol} = 2; }
    else { $del{$symbol} = 0; }
  }

  {
    my $warned = 0;
    foreach (sort keys %simple_test) {
      my $del = $del{$_};
      if (defined $del and $del==1) {
        if ($warned==0) {
          print "Skipped tests (restricting to modules matching '$expr_mod'):\n";
          $warned = 1;
        }
        print '* '.$_."\n";
      }
    }
    $warned = 0;
    foreach (sort keys %simple_test) {
      my $del = $del{$_};
      if (defined $del and $del==2) {
        if ($warned==0) {
          print "Skipped tests (restricting to functions matching '$expr_fun'):\n";
          $warned = 1;
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

  if ($skip_slow) { skip_slow_tests(); }
  calculate_priorities();

  my @tests = sort {
    my $prioa = $test_priority{$a};
    my $priob = $test_priority{$b};

    my $cmp = $prioa - $priob;

    if ($cmp == 0) {
      my $loca = $location{$a};
      my $locb = $location{$b};
      if (defined $loca) {
        if (defined $locb) {
          my ($fa,$la,$fb,$lb);
          if ($loca =~ /^(.*):([0-9]+)$/) { ($fa,$la) = ($1,$2); } else { die "Invalid location '$loca'"; }
          if ($locb =~ /^(.*):([0-9]+)$/) { ($fb,$lb) = ($1,$2); } else { die "Invalid location '$locb'"; }
          $cmp = $fa cmp $fb;
          if ($cmp==0) { $cmp = $la <=> $lb; }
        }
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
  $TABLES   .= generate_table(UT_type('simple'), UT_name('postcond'), %postcond, &prototype_simple);

  my $skipped_count = scalar(keys %skipped_test);

  # UnitTester is declared in UnitTester.cxx@InvokeUnitTester
  my $UNIT_TESTER = 'UnitTester unitTester("'.$libname.'", ';
  $UNIT_TESTER .= UT_name('simple');
  $UNIT_TESTER .= ', '.$warn_level;
  $UNIT_TESTER .= ', '.$skipped_count;
  $UNIT_TESTER .= ', '.UT_name('postcond');
  $UNIT_TESTER .= ');';

  my $MAIN = '';
  my $have_main = defined $exported{'main'};
  if ($have_main==1) {
    $MAIN .= "#error tested code uses main() - not possible. use ARB_main instead and link normal executable with arb_main.o\n";
  }
  $MAIN .= 'int main(void) {'."\n";
  $MAIN .= '    '.$UNIT_TESTER."\n";
  $MAIN .= '    return EXIT_SUCCESS;'."\n";
  $MAIN .= '}'."\n";

  print OUT $HEAD."\n";
  print OUT $TABLES."\n";
  print OUT $MAIN."\n";
  close(OUT);
}

# --------------------------------------------------------------------------------

sub main() {
  my $args = scalar(@ARGV);
  if ($args != 6) {
    die("Usage: sym2testcode.pl libname restrict-mod restrict-fun nm-output gen-cxx warn-level\n".
        "    libname        name of library to run tests for\n".
        "    restrict-mod   regexpr to restrict to specific module in library\n".
        "    restrict-fun   regexpr to restrict to matching test functions\n".
        "    nm-output      output of nm\n".
        "    gen_cxx        name of C++ file to generate\n".
        "    warn-level     (0=quiet|1=noisy)\n".
        "Error: Expected 5 arguments\n");
  }

  my $libname     = shift @ARGV;
  my $restrictMod = shift @ARGV;
  my $restrictFun = shift @ARGV;
  my $nm_output   = shift @ARGV;
  my $gen_cxx     = shift @ARGV;
  $warn_level     = shift @ARGV;

  parse($nm_output);
  fail_if_no_tests_defined($libname); # comment out to disableErrorOnUnitsWithoutTests

  filter($restrictMod,$restrictFun);
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
