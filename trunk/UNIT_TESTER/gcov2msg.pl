#!/usr/bin/perl

use strict;
use warnings;

use Cwd;

my $verbose = 0;

# --------------------------------------------------------------------------------

sub parseCoveredLines($\@) {
  my ($gcov, $covered_lines_r) = @_;

  my ($lines,$covered) = (0,0);
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
        $lines++;
        $$covered_lines_r[$lineno] = '#';
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
    }
  }
  close(GCOV);
  return ($lines,$covered);
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

  my $cov = $gcov;
  $cov =~ s/\.gcov$/\.cov/g;
  if ($cov eq $gcov) { die "Invalid gcov name '$gcov'"; }

  my @covered_lines = ();
  my ($lines,$covered) = parseCoveredLines($gcov,@covered_lines);
  my $size = scalar(@covered_lines);

  my $percent = 100*$covered/$lines;
  $percent = int($percent*10)/10;

  # $verbose==0 ||
  print "collect_gcov_data($gcov): lines=$lines covered=$covered (coverage=$percent%)\n";

  if ($covered==$lines) {
    print "Full test-coverage for $source\n";
    unlink($gcov);
  }
  else {
    $covered>0 || die "Argh.. collected data for completely uncovered file '$source'";

    my $line = 0;
  SECTION: while (1) {
      my ($first,$last,$loc) = next_uncovered_section_after(@covered_lines, $size, $line);
      if (not defined $first) { last SECTION; }

      if ($first==$last) {
        print "$source:$first: Uncovered line\n";
      }
      else {
        # print "$source:$first: Uncovered section [start] ($loc lines of code)\n";
        # print "$source:$last:                    [end]\n";
        print "$source:$first: [start] $loc lines of uncovered code\n";
        print "$source:$last: [end]\n";
      }
      $line = $last;
    }

    rename($gcov,$cov) || die "Failed to rename '$gcov' -> '$cov' (Reason: $!)";
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
            my $fullsource = $dir.'/'.$source;
            if (-f $fullsource) { $source = $fullsource; }
            collect_gcov_data($source,$gcov);
          }
          else {
            # print "Completely untested: $source\n";
            unlink($gcov);
          }

          ($file,$percent,$lines,$source,$gcov) = (undef,undef,undef,undef,undef);
        }
        else {
          die "can't parse line '$_'";
        }
      }
      close(CMD);
    }
  };
  if ($@) {
    my $err = $@;
    chdir($olddir) || print "Failed to resume old working dir '$olddir' (Reason: $!)\n";
    die "Error: $err\n";
  }
}

main();
