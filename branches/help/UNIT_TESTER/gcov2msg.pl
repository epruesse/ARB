#!/usr/bin/perl

use strict;
use warnings;

use Cwd;

my $verbose = 0;

my $showCoverageForAll; # 1=all; 0=files containing tests only
my $showCoverageForFilesMatching; # regexpr filter for covered files
my $sortBySectionSize; # 0=by location; 1=by section size

sub setup() {
  sub env_defined_non_empty($) {
    my ($varname) = @_;
    my $value = $ENV{$varname};
    if (not defined $value) { die "environment variable $varname is undefined"; }
    if ($value eq '') { die "environment variable $varname is empty"; }
    chomp($value);
    $value =~ s/^\s+//go;
    $value =~ s/\s+$//go;

    if ($value =~ /^'(.*)'$/o) { $value = $1; }
    return $value;
  }

  my $SORT_COVERAGE     = env_defined_non_empty('SORT_COVERAGE');
  my $RESTRICT_COVERAGE = env_defined_non_empty('RESTRICT_COVERAGE');

  print "SORT_COVERAGE='$SORT_COVERAGE'\n";
  print "RESTRICT_COVERAGE='$RESTRICT_COVERAGE'\n";

  $showCoverageForAll = 1;
  $showCoverageForFilesMatching = qr/.*/;

  if ($RESTRICT_COVERAGE eq 'NO') {
    ;
  }
  elsif ($RESTRICT_COVERAGE eq 'MODULE') {
    my $RESTRICT_MODULE   = env_defined_non_empty('RESTRICT_MODULE');
    print "RESTRICT_MODULE='$RESTRICT_MODULE'";
    if ($RESTRICT_MODULE eq '.') {
      print " (restricting to tested modules)\n";
      $showCoverageForAll = 0;
    }
    else {
      print "\n";
      $showCoverageForFilesMatching = qr/$RESTRICT_MODULE/;
    }
  }
  else {
    $showCoverageForFilesMatching = qr/$RESTRICT_COVERAGE/;
  }

  if ($SORT_COVERAGE eq 'LOCATION') { $sortBySectionSize = 0; }
  elsif ($SORT_COVERAGE eq 'SIZE') { $sortBySectionSize = 1; }
  else { die "SORT_COVERAGE '$SORT_COVERAGE' is unknown"; }
}

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
  open(GCOV,'<'.$gcov) || die "gcov2msg.pl: can't read '$gcov' (Reason: $!)";
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
    my $summary = "lines=$lines covered=$covered (coverage=$percent%)";

    $verbose==0 || print "collect_gcov_data($gcov): $summary\n";
    $covered>0 || die "Argh.. collected data for completely uncovered file '$source'";

    if    ($tests_seen==0 and $showCoverageForAll==0)         { print "$source_name defines no tests. $summary\n"; }
    elsif (not $source_name =~ $showCoverageForFilesMatching) { print "Skipping $source_name by mask. $summary\n"; }
    else {
      my $line = 0;
      my @sections = ();

    SECTION: while (1) {
        my ($first,$last,$loc) = next_uncovered_section_after(@covered_lines, $size, $line);
        if (not defined $first) { last SECTION; }
        push @sections, [$first,$last,$loc];
        $line = $last;
      }

      if ($sortBySectionSize==1) { @sections = sort { $$a[2] <=> $$b[2]; } @sections; }

      foreach my $sec_r (@sections) {
        my ($first,$last,$loc) = ($$sec_r[0],$$sec_r[1], $$sec_r[2]);
        if ($first==$last) {
          print_annotated_message($source, $first, 'Uncovered line');
        }
        else {
          print_annotated_message($source, $first, "[start] $loc uncovered lines");
          print_annotated_message($source, $last, '[end]');
        }
      }

      if ($percent<90) { print "$source_name:0: Warning: Summary $summary\n"; }
      else { print "Summary $source_name: $summary\n"; }
      
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
sub gcda2code($\@) {
  my ($gcda, $srcdirs_r) = @_;

  if (not $gcda =~ /\.gcda$/o) {
    die "wrong file in gcda2code: '$gcda'";
  }
  my $base = $`;
  foreach my $dir (@$srcdirs_r) {
    foreach (@known_source_ext) {
      my $name = $base.'.'.$_;
      my $full = $dir.'/'.$name;
      if (-f $full) {
        return [$name,$dir];
      }
    }
  }
  die "Failed to find code file for '$gcda'";
}

sub die_usage($) {
  my ($err) = @_;
  print("Usage: gcov2msg.pl [options] directory\n".
        "Options: --srcdirs=dir,dir,dir  set sourcedirectories (default is 'directory')\n".
        "         --builddir=dir         set dir from which build was done (default is 'directory')\n");
  die "Error: $err\n";
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args<1) { die_usage("Missing argument\n"); }

  setup();

  my $dir;
  my @srcdirs;
  my $builddir = undef;
  {
    my $srcdirs  = undef;

    while ($ARGV[0] =~ /^--/) {
      if ($ARGV[0] =~ /^--srcdirs=/) {
        $srcdirs = $';
        shift @ARGV;
      }
      elsif ($ARGV[0] =~ /^--builddir=/) {
        $builddir = $';
        shift @ARGV;
      }
    }
    $dir = $ARGV[0];
    if (not -d $dir) { die "No such directory '$dir'\n"; }

    if (not defined $builddir) { $builddir = $dir; }
    if (not defined $srcdirs) { $srcdirs = $dir; }
    @srcdirs = split(',', $srcdirs);
  }

  my @gcda = find_gcda_files($dir);
  my %gcda2code = map { $_ => gcda2code($_,@srcdirs); } @gcda; # value=[name,srcdir]

  my $olddir = cwd();
  chdir($dir) || die "can't cd to '$dir' (Reason: $!)\n";;

  eval {
    foreach (sort @gcda) {
      my $cs_ref = $gcda2code{$_};
      my ($code,$srcdir) = @$cs_ref;

      my $fullcode  = $srcdir.'/'.$code;
      my $objSwitch = '';

      if ($srcdir ne $dir) {
        $objSwitch = " -o '$dir'";
      }

      if ($builddir ne $dir) {
        chdir($builddir) || die "can't cd to '$builddir' (Reason: $!)\n";;
      }
      my $cmd = "gcov '$fullcode' $objSwitch";

      $verbose==0 || print "[Action: $cmd]\n";

      open(CMD,$cmd.'|') || die "can't execute '$cmd' (Reason: $!)";

      if ($builddir ne $dir) {
        chdir($dir) || die "can't cd to '$dir' (Reason: $!)\n";;
      }

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
              # print "Skipping '$gcov'\n";
            }
            else {
              my $fullgcov = $gcov;
              if ($dir ne $builddir) {
                $fullgcov = $builddir.'/'.$gcov;
              }
              collect_gcov_data($source,$fullgcov);
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
