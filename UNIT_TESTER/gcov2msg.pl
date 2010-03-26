#!/usr/bin/perl

use strict;
use warnings;

use Cwd;

my $verbose = 0;

# --------------------------------------------------------------------------------

my %code = (); # key=lineno, value=code (valid for recently parsed lines with type '#')

# --------------------------------------------------------------------------------

my ($loclen,$msglen) = (0,0);

sub reset_trim() {
  ($loclen,$msglen) = (0,0);
}

sub trim($\$) {
  my ($str, $len_r) = @_;
  my $len = length($str);
  if ($len > $$len_r) { $$len_r = $len; }
  else {
    $str = sprintf("%-*s", $$len_r, $str);
  }
  return $str;
}

sub print_trimmed($$$$) {
  my ($source,$lineno,$msg,$code) = @_;

  my $loc = $source.':'.$lineno.':';
  $loc .= ' ' if ($lineno<1000);
  $loc .= ' ' if ($lineno<100);

  $loc = trim($loc, $loclen);
  $msg = trim($msg, $msglen);

  $code =~ s/^\s*//go;

  print $loc.' '.$msg.' | '.$code."\n";
}

sub print_annotated_message($$$) {
  my ($source,$lineno,$msg) = @_;
  print_trimmed($source, $lineno, $msg, $code{$lineno});
}

# --------------------------------------------------------------------------------

sub parseCoveredLines($\@) {
  my ($gcov, $covered_lines_r) = @_;

  my ($lines,$covered,$tests_seen) = (0,0,0);
  open(GCOV,'<'.$gcov) || die "can't read '$gcov' (Reason: $!)";
  my $line;
  while (defined ($line = <GCOV>)) {
    if (not $line =~ /^\s*([^\s:][^:]*):\s*([^\s:][^:]*):(.*)$/o) { die "can't parse '$line'"; }
    my ($counter,$lineno,$code) = ($1,$2,$3);
    if ($lineno>0) {
      if ($counter eq '-') {
        $$covered_lines_r[$lineno] = '-';
      }
      elsif ($counter eq '#####') {
        if ($code =~ /NEED_NO_COV/) { # handle like there was no code here
          $$covered_lines_r[$lineno] = '-';
        }
        else {
          $lines++;
          $$covered_lines_r[$lineno] = '#';
          $code{$lineno} = $code;
        }
      }
      else {
        if ($counter =~ /^[0-9]+$/) {
          $lines++;
          $covered++;
          $$covered_lines_r[$lineno] = '+';
        }
        else {
          die "Invalid counter '$counter' (expected number)";
        }
      }

      if ($code =~ /^void\s+TEST_.*()/g) {
        $tests_seen++;
      }
    }
  }
  close(GCOV);

  return ($lines,$covered,$tests_seen);
}

sub next_uncovered_section_after(\@$$) {
  my ($covered_lines_r,$lines,$after_line) = @_;

  my $line = $after_line+1;
  while ($line<$lines) {
    my $type = $$covered_lines_r[$line];
    if ($type eq '#') {
      my ($first,$last,$loc) = ($line, $line, 0);

    LINE: while (1) {
        if ($type eq '+') { last LINE; } # covered -> stop
        if ($type eq '#') {
          $loc++;
          $last = $line;
        }
        ++$line;
        if ($line>=$lines) { last LINE; }
        $type = $$covered_lines_r[$line];
      }

      return ($first,$last,$loc);
    }
    ++$line;
  }
  return (undef,undef,undef);

}

sub collect_gcov_data($$) {
  my ($source,$gcov) = @_;

  reset_trim();

  my $cov = $gcov;
  $cov =~ s/\.gcov$/\.cov/g;
  if ($cov eq $gcov) { die "Invalid gcov name '$gcov'"; }

  if (not -f $gcov) {
    print "No such file '$gcov' (assuming it belongs to a standard header)\n";
    return;
  }

  my @covered_lines = ();
  my ($lines,$covered,$tests_seen) = parseCoveredLines($gcov,@covered_lines);
  my $size = scalar(@covered_lines);

  my $percent = 100*$covered/$lines;
  $percent = int($percent*10)/10;

  my $source_name = $source;
  if ($source =~ /\/([^\/]+)$/) { $source_name = $1; }

  if ($covered==$lines) {
    print "Full test-coverage for $source_name\n";
    unlink($gcov);
  }
  else {
    # $verbose==0 ||
    print "collect_gcov_data($gcov): lines=$lines covered=$covered (coverage=$percent%)\n";
    $covered>0 || die "Argh.. collected data for completely uncovered file '$source'";

    if ($tests_seen==0) {
      print "$source_name defines no tests. lines=$lines covered=$covered (coverage=$percent%)\n";
    }
    else {
      my $line = 0;
    SECTION: while (1) {
        my ($first,$last,$loc) = next_uncovered_section_after(@covered_lines, $size, $line);
        if (not defined $first) { last SECTION; }

        if ($first==$last) {
          print_annotated_message($source, $first, 'Uncovered line');
        }
        else {
          print_annotated_message($source, $first, "[start] $loc uncovered lines");
          print_annotated_message($source, $last, '[end]');
        }
        $line = $last;
      }

      rename($gcov,$cov) || die "Failed to rename '$gcov' -> '$cov' (Reason: $!)";
    }
  }
}

# --------------------------------------------------------------------------------

my @known_source_ext = qw/cxx cpp c/;

sub find_gcda_files($) {
  my ($dir) = @_;
  my @gcda = (); 
  opendir(DIR,$dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ =~ /\.gcda$/o) { push @gcda, $_; }
  }
  closedir(DIR);
  return @gcda;
}
sub gcda2code($$) {
  my ($dir,$gcda) = @_;

  if (not $gcda =~ /\.gcda$/o) {
    die "wrong file in gcda2code: '$gcda'";
  }
  my $base = $`;
  foreach (@known_source_ext) {
    my $name = $base.'.'.$_;
    my $full = $dir.'/'.$name;
    if (-f $full) {
      return $name;
    }
  }
  die "Failed to find code file for '$gcda'";
}

sub die_usage($) {
  my ($err) = @_;
  print "Usage: gcov2msg.pl directory\n";
  die "Error: $err\n";
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=1) { die_usage("Missing argument\n"); }

  my $dir = $ARGV[0];
  if (not -d $dir) { die "No such directory '$dir'\n"; }

  my @gcda = find_gcda_files($dir);
  my %gcda2code = map { $_ => gcda2code($dir,$_); } @gcda;

  my $olddir = cwd();
  chdir($dir) || die "can't cd to '$dir' (Reason: $!)\n";;

  eval {
    foreach (sort @gcda) {
      my $code = $gcda2code{$_};
      # print "'$_' -> '$code'\n";

      my $fullcode = $dir.'/'.$code;
      my $cmd      = "gcov '$fullcode'";

      $verbose==0 || print "[Action: $cmd]\n";

      open(CMD,$cmd.'|') || die "can't execute '$cmd' (Reason: $!)";

      my ($file,$percent,$lines,$source,$gcov) = (undef,undef,undef,undef,undef);

      foreach (<CMD>) {
        chomp;
        if ($_ eq '') { ; } # ignore empty lines
        elsif (/^File '(.*)'$/o) { $file = $1; }
        elsif (/^Lines executed:([0-9.]+)% of ([0-9]+)$/o) {
          ($percent,$lines) = ($1,$2);
        }
        elsif (/^([^:]+):creating '(.*)'$/o) {
          ($source,$gcov) = ($1,$2);

          if ($percent>0 and $lines>0) {
            if ($source =~ /^\/usr\/include/o) {
              print "Skipping '$gcov'\n";
            }
            else {
              collect_gcov_data($source,$gcov);
            }
          }
          if (-f $gcov) { unlink($gcov); }

          ($file,$percent,$lines,$source,$gcov) = (undef,undef,undef,undef,undef);
        }
        else {
          die "can't parse line '$_'";
        }
      }
      close(CMD);

      -f $_ || die "No such file '$_'";
      unlink($_);
    }
  };
  if ($@) {
    my $err = $@;
    chdir($olddir) || print "Failed to resume old working dir '$olddir' (Reason: $!)\n";
    die "Error: $err\n";
  }
}

main();
