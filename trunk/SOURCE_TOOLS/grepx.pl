#!/usr/bin/perl
# ======================================================================== #
#                                                                          #
#   File      : grepx.pl                                                   #
#   Purpose   : Replacement for grep (used from emacs)                     #
#   Time-stamp: <Thu Jul/20/2006 11:33 MET Coder@ReallySoft.de>            #
#                                                                          #
#   (C) November 2005 by Ralf Westram                                      #
#                                                                          #
#   Permission to use, copy, modify, distribute and sell this software     #
#   and its documentation for any purpose is hereby granted without fee,   #
#   provided that the above copyright notice appear in all copies and      #
#   that both that copyright notice and this permission notice appear      #
#   in supporting documentation.                                           #
#                                                                          #
#   Ralf Westram makes no representations about the suitability of this    #
#   software for any purpose.  It is provided "as is" without express or   #
#   implied warranty.                                                      #
#                                                                          #
# ======================================================================== #
#
# Improvements compared with grep:
#
# * prints line column information
# * knows about groups of files belonging together (e.g. *.cxx *.hxx)
# * knows about special file locations (e.g. emacs lisp code, /usr/include, ...)
#
# --------------------------------------------------------------------------------

use strict;
use warnings;
use Cwd;

# --------------------------------------------------------------------------------
# group definitions (you may want to change here):
#
# Each element in groups defines a cluster of files.
# One cluster consists of:
#
# [0] = ref to array of header extensions
# [1] = ref to array of normal extensions
# [2] = ref to array of add. directories to search for
# [3] = ref to array of add. extensions to search
#
# If extension given is member of [0] (or [1] if not -h given) of a cluster,
# then the cluster gets activated (we call this an AC). Extensions in [3] do
# NOT activate clusters!
#
# If -h is given, only extensions from [0] of all ACs are searched
# otherwise those from [1] and [3] are added. ([3] is todo!)
#
# If -g is given the add. directories from [2] of all ACs are searched as well.


my @groups = (
              # C/C++
              [
               [ '.hxx', '.hpp', '.hh', '.h' ], # header files
               [ '.cxx', '.cpp', '.cc', '.c' ], # code files
               [ '/usr/include' ],              # additional header directories (used with -g)
               [ '.aisc', '.pa' ],
              ],
              # ARB code generation
              [
               [ ],
               [ '.aisc', '.pa' ],
               [ ],
               [ '.cxx', '.cpp', '.cc', '.c', '.hxx', '.hpp', '.hh', '.h' ],
              ],
              # perl
              [
               [ '.pm' ],                       # header files
               [ '.pl', '.cgi' ],               # code files
               [ '/usr/lib/perl5' ],            # additional header directories (used with -g)
              ],
              # java
              [
               [ ],            # java sucks headers
               [ '.java' ],
              ],
              # xml development
              [
               [ '.dtd' ],
               [ '.xml', '.xsl' ],
              ],
              # lisp
              [
               [ ],
               [ '.el' ],
               [
                '/usr/share/emacs/site-lisp',
                '/usr/share/xemacs',
               ],
              ],
              # shell-scripts etc.
              [
               [ ],
               [ '.sh', '.cmd', '.bat' ],
              ],
              # text files
              [
               [ ],
               [ '.txt', '.readme' ],
              ],
              # html etc
              [
               [ ],
               [ '.html', '.htm' ],
              ],
              # hamster scripts
              [
               [ '.hsm' ],
               [ '.hsc' ],
               [ ], # no add. directories
               [ '.ini' ], # search add. but don't add cluster if included
              ],
              # Euphoria
              [
               [ '.e' ],
               [ '.exw' ],
              ],
              # ARB specifics
              [
               [ ],
               [ '.menu' ],
              ],
              );

# files always searched (not by 'same' and 'header' search)
my @normally_searches = ( 'makefile' );

# files always searched by global search
my @global_always_searches = ( );

# --------------------------------------------------------------------------------

my $self = '~/emacs/bin/grepx';

my $global           = 0;
my $headers_only     = 0;
my $same_ext_only    = 0;
my $ignore_case      = 0;
my $recurse_subdirs  = 0;
my $one_hit_per_line = 0;
my $verbose          = 0;
my $matchFiles       = 1;
my $maxhits          = undef; # undef means unlimited

my $extension       = undef;
my $use_as_wildcard = 0;
my $regexpr         = undef;

my $calldir  = cwd();
my $startdir = undef;

# --------------------------------------------------------------------------------

my $GSM_NONE   = 0;
my $GSM_CVS    = 1; # scan a CVS tree
my $GSM_PARENT = 2; # do a simple parent scan

my $global_scan_mode = $GSM_NONE;

# --------------------------------------------------------------------------------

my $reg_nameOnly  = qr/\/([^\/]+)$/;
my $reg_extension = qr/(\.[^\.]+)$/;
# (\.[^\.]+)

my ($IS_HEADER,$IS_NORMAL,$IS_OTHER,$IS_ADDITIONAL) = (3,2,1,4);

my %wanted_extensions = ();
my %wanted_files      = ();

my @add_header_dirs = ();

my $reg_is_cpp_std_dir = qr/^\/usr\/include\/g\+\+(\/|$)/;

sub shall_search_file($$) {
  my ($file,$indir) = @_;

  if ($use_as_wildcard==0) {
    if ($file =~ $reg_nameOnly) { $file = $1; } # behind last /

    if ($file =~ /^\.?#/ or $file =~ /~$/) { return 0; } # skip backup files etc.

    my $ext = '';
    if ($file =~ $reg_extension) { $ext = $1; }

    if ($ext eq '' and $indir =~ $reg_is_cpp_std_dir) {
      # print "hack: considering $file in $indir\n";
      $ext = '.h'; # special hack for new style C++ header (they suck an extension)
    }

    $ext = lc($ext);
    if (exists $wanted_extensions{$ext}) { return $wanted_extensions{$ext}; }

    $file = lc($file);
    if (exists $wanted_files{$file}) { return $IS_OTHER; }
  }
  else {
    if ($file =~ /$extension/ig) {
      return $IS_NORMAL;
    }
  }

  return 0;
}

sub memberOf($\@) {
  my ($ext, $extArray_r) = @_;
  foreach (@$extArray_r) {
    if ($ext eq $_) { return 1; }
  }
  return undef;
}

sub add_files(\@$) {
  my ($ext_array_r,$value) = @_;
  foreach (@$ext_array_r) { $wanted_extensions{$_} = $value; }
}

sub init_wanted() {
  %wanted_extensions = ();
  %wanted_files      = ();

  if ($same_ext_only==0 and $headers_only==0) {
    foreach (@normally_searches) { $wanted_files{$_} = 1; }
  }
  if ($global==1) {
    foreach (@global_always_searches)  { $wanted_files{$_} = 1; }
  }

  if ($same_ext_only) {
    $wanted_extensions{$extension} = $IS_NORMAL;
  }
  elsif ($extension eq '') {
    foreach my $group_r (@groups) {
      my $header_r  = $$group_r[0];
      my $nheader_r = $$group_r[1];

      add_files(@$header_r, $IS_HEADER);
      if ($headers_only==0) { add_files(@$nheader_r, $IS_NORMAL); }
    }
    my $which = '';
    if ($headers_only==1) { $which = 'header-'; }
    print "grepx: No extension given - searching all known ".$which."extensions.\n";
  }
  else {
    my $found_class = 0;
    my $group_count = 0;
    eval {
      foreach my $group_r (@groups) {
        my $group_defs = scalar(@$group_r);
        if ($group_defs<2) { die "Not enough entries (min. 2 are expected)"; }

        my $header_r  = $$group_r[0];
        my $nheader_r = $$group_r[1];

        if (memberOf($extension, @$header_r) or memberOf($extension, @$nheader_r)) { # is group active?
          $verbose==0 || print "'$extension' found in [@$header_r] or [@$nheader_r] - adding tables\n";
          $found_class = 1;

          add_files(@$header_r, $IS_HEADER);
          if ($headers_only==0) { add_files(@$nheader_r, $IS_NORMAL); }

          # 3rd entry is array of directories for -h -g
          if ($group_defs>=3) {
            my $add_dir_r = $$group_r[2];
            foreach my $adir (@$add_dir_r) {
              if (-d $adir) { push @add_header_dirs, $adir; }
              else { print "grepx: No such directory '$adir'\n"; }
            }

            if ($group_defs>=4) {
              my $add_extensions_r = $$group_r[3];
              if ($verbose>0) {
                print "Adding add. extensions:";
                foreach (@$add_extensions_r) { print " $_"; }
                print "\n";
              }
              add_files(@$add_extensions_r, $IS_ADDITIONAL);
            }
          }
        }
        $group_count++;
      }
    };
    if ($@) { die "Error parsing \@groups[$group_count]: $@"; }

    if ($found_class==0) {
      print "grepx: No class defined for '$extension' .. searching only '$extension' files\n";
      $wanted_extensions{$extension} = $IS_NORMAL;
    }
  }

  if ($verbose==1) {
    print "grepx: Searching";
    foreach (keys %wanted_extensions) { print " *$_"; }
    foreach (keys %wanted_files) { print " $_"; }
    print "\n";
  }
}

# --------------------------------------------------------------------------------

sub print_usage() {
  print "Usage: grepx 'ext' 'regexpr'\n".
    "Options:\n".
    " -g -> search globally (smart detect what global means)\n".
    " -h -> search in header files only (depends on 'ext')\n".
    " -s -> search in same fileextension only (default is to search file group)\n".
    " -i -> ignore case\n".
    " -r -> recurse subdirs\n".
    " -o -> one hit per line (default is to report multiple hits)\n".
    " -v -> be verbose (for debugging)\n".
    " -n -> don't match filenames\n".
    " -m xxx -> report max. xxx hits\n".
    "\n".
    " 'ext'     extension of file where grepx is called from\n".
    " 'regexpr' perl regular expression\n\n";
}

# --------------------------------------------------------------------------------

sub parse_args() {
  my $args         = scalar(@ARGV);
  my @non_opt_args = ();
  my $ap           = 0;

  while ($ap<$args) {
    if ($ARGV[$ap] =~ /^-/) {
      my $option = $';
      if ($option eq 'g') { $global = 1; }
      elsif ($option eq 'h') { $headers_only = 1; }
      elsif ($option eq 's') { $same_ext_only = 1; }
      elsif ($option eq 'i') { $ignore_case = 1; }
      elsif ($option eq 'r') { $recurse_subdirs = 1; }
      elsif ($option eq 'o') { $one_hit_per_line = 1; }
      elsif ($option eq 'v') { $verbose = 1; }
      elsif ($option eq 'n') { $matchFiles = 0; }
      elsif ($option eq 'm') { $maxhits = int($ARGV[++$ap]); }
      else { die "Unknown option '-$option'\n"; }
    }
    else {
      if ($ARGV[$ap] ne '/dev/null') {
        push @non_opt_args, $ARGV[$ap];
      }
    }
    $ap++;
  }

  my $restargs = scalar(@non_opt_args);
  # print "\@non_opt_args=@non_opt_args\n";
  if ($restargs!=2) { die "Expected exactly two normal arguments (non-switches), found $restargs\n"; }

  $extension = $non_opt_args[0];
  $regexpr   = $non_opt_args[1];
  $verbose==0 || print "grepx: Using regular expression '$regexpr'\n";

  if ($ignore_case==1) { $regexpr = qr/$regexpr/i; }
  else { $regexpr = qr/$regexpr/; }

  if ($headers_only==1 and $same_ext_only==1) { die "Options -s and -h may not be used together\n"; }
}

# --------------------------------------------------------------------------------

my $lines_examined = 0;
my $reg_startdir = undef;

sub grepfile($) {
  my ($file) = @_;

  my $matches  = 0;
  my $reported = 0;
  my $show     = $file;

  if ($file =~ $reg_startdir) { $show = $'; }

  open(FILE,"<$file") || die "can't read file '$file' (Reason: $!)";
  while (my $line = <FILE>) {
    if ($line =~ $regexpr) {
      if ((not defined $maxhits) or ($maxhits>0)) {
        my $rest   = $';
        my $hitlen = length($&);
        my $pos    = length($`)+1;

        chomp($line);
        $line =~ s/
//;
        print "$show:$.:$pos: $line\n";
        my $hits = 1;

        if ($one_hit_per_line==0) {
          chomp($rest);
          $rest =~ s/
//;

          while ($rest =~ $regexpr) {
            my $start_pos = $pos+$hitlen;
            $hitlen = length($&);
            $pos = $start_pos+length($`);
            print "$show:$.:$pos: $line\n";
            $hits++;
            $rest = $';
          }
        }

        $reported += $hits;
        if (defined $maxhits) { $maxhits -= $hits; }
      }
      $matches++;
    }
    $lines_examined++;
  }
  close(FILE);
  return ($matches,$reported);
}

# --------------------------------------------------------------------------------

sub CVS_controlled($) {
  my ($dir)          = @_;
  my $CVS_Repository = $dir.'/CVS/Repository';
  if (-f $CVS_Repository) { return 1; }
  return 0;
}

sub parent_directory($) {
  my ($dir) = @_;
  if ($dir =~ /\/[^\/]+$/) {
    return $`;
  }
  return undef;
}

# --------------------------------------------------------------------------------

sub collect_files($\%$);
sub collect_files($\%$) {
  my ($dir,$files_r,$is_additional_directory) = @_;

  my @files   = ();
  my @subdirs = ();

  opendir(DIR, $dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $dir.'/'.$_;
      if (-l $full) { $verbose==0 || print "Skipping $full (symbolic link)\b"; }
      elsif (-f $full) { push @files, $full; }
      elsif (-d $full) { push @subdirs, $full; }
      else { $verbose==0 || print "Skipping $full (not a file or directory)\n"; }
    }
  }
  closedir(DIR);

  # @files = sort @files;
  foreach (@files) {
    my $shall = shall_search_file($_,$dir);
    if ($shall) {
      $verbose==0 || print "Searching $_\n";
      # $matches += grepfile($_);
      # $searched++;
      $$files_r{$_} = $shall;
    }
    else {
      $verbose==0 || print "Skipping '$_' (unwanted)\n";
    }
  }

  if ($recurse_subdirs==1) {
    # @subdirs = sort @subdirs;
    foreach (@subdirs) {
      my $descent = 1;
      my $reason = 'not specified';
      if ($global_scan_mode==$GSM_CVS and not $is_additional_directory and not CVS_controlled($_)) {
        $descent = 0;
        $reason = 'not CVS controlled';
      }
      if ($descent==1) {
        collect_files($_, %$files_r, $is_additional_directory);
      }
      else {
        $verbose==0 || print "Skipping subdirectory '$_' ($reason)\n";
      }
    }
  }
}

sub grep_collected_files(\%) {
  my ($files_r) = @_;

  my %depth = map {
    my $d = $_;
    $d =~ s/[^\/\\]//ig;
    $_ => length($d);
  } keys %$files_r;

  my @files = sort {
    my $cmp = $$files_r{$b} <=> $$files_r{$a}; # file importance
    if ($cmp==0) {
      $cmp = $depth{$a} <=> $depth{$b}; # depth in directory tree
      if ($cmp==0) {
        $cmp = $a cmp $b; # alphabethically
      }
    }
    return $cmp;
  } keys %$files_r;

  my $searched = scalar(@files);
  my $matches  = 0;
  my $reported = 0;

  if ($matchFiles==1) {
    my @matching_files = ();    # files were regexp matches filename
    my $reg_name = qr/\/([^\/]+)$/;

    foreach (@files) {
      if ($_ =~ $reg_name) { # match the name part
        if ($1 =~ $regexpr) { push @matching_files, $_; }
      }
      else { die "can't parse name from '$_'"; }
    }
    my $matching_files = scalar(@matching_files);
    if ($matching_files>0) {
      print "grepx: Some filenames match your expression:\n";
      foreach (@matching_files) {
        my $show = $_;
        if ($_ =~ $reg_startdir) { $show = $'; }
        print "$show:0: <filename matched>\n";
      }
    }
  }

  print "grepx: Searching $searched files..\n";
  foreach (@files) {
    # print "searching '$_' (depth=$depth{$_})\n";
    my ($m,$r) = grepfile($_);
    $matches += $m;
    $reported += $r;
  }


  return ($searched,$matches,$reported);
}

sub perform_grep($$) {
  my ($startdir, $is_additional_directory) = @_;
  my %files = (); # key=file, value=file-importance
  collect_files($startdir,%files,$is_additional_directory);

  my $max_importance = -1;
  foreach (values %files) {
    if ($_ > $max_importance) { $max_importance = $_; }
  }

  if ($max_importance==$IS_OTHER) {
    print "grepx: Only found files with importance==$IS_OTHER (forgetting them)\n";
    %files = ();
  }

  my ($searched,$matches,$reported) = (0,0,0);
  if (scalar(%files)) {
    print "grepx: Entering directory `$startdir'\n";
    ($searched,$matches,$reported) = grep_collected_files(%files);
    print "grepx: Leaving directory `$startdir'\n";
  }
  return ($searched,$matches,$reported);
}

sub grep_add_directories() {
  my ($searched,$matches,$reported) = (0,0,0);
  foreach (@add_header_dirs) {
    my ($s,$m,$r) = perform_grep($_,1);
    ($searched,$matches,$reported) = ($searched+$s,$matches+$m,$reported+$r);
  }
  return ($searched,$matches,$reported);
}

# --------------------------------------------------------------------------------


sub detect_wanted_startdir($) {
  my ($calldir) = @_;
  if ($global==1) {
    my $know_whats_global = 0;

    if (CVS_controlled($calldir)) {
      my $updir = parent_directory($calldir);
      while (defined $updir and -d $updir and CVS_controlled($updir)) {
        $calldir = $updir;
        $updir   = parent_directory($updir);
      }
      print "grepx: Starting global search from root of CVS controlled directory-tree\n";
      $global_scan_mode  = $GSM_CVS;
      $know_whats_global = 1;
    }

    if ($know_whats_global==0) {
      print "grepx: Don't know what 'global search' means here.. using parent directory\n";
      $global_scan_mode = $GSM_PARENT;
      my $updir         = parent_directory($calldir);
      if (defined $updir and -d $updir) { $calldir = $updir; }
    }
  }
  return $calldir;
}

sub megagiga($) {
  my ($val) = @_;
  if ($val<1024) { return "$val "; }

  my $pot = 0;
  while ($val>=1024) {
    $val = int($val/1024+0.5);
    $pot++;
  }
  return "$val ".substr("kMGTP", $pot-1, 1);
}

# --------------------------------------------------------------------------------

eval {
  my $start_time = time;
  parse_args();

  $startdir = detect_wanted_startdir($calldir);
  $reg_startdir = quotemeta($startdir.'/');
  $reg_startdir = qr/^$reg_startdir/;

  init_wanted();

  my ($searched,$matches,$reported) = perform_grep($startdir,0);

  if ($global==1 and scalar(@add_header_dirs)>0) {
    if ($reported==$matches) {
      print "grepx: ------------------------------ Searching in add. directories:\n";
      my ($s,$m,$r) = grep_add_directories();
      ($searched,$matches,$reported) = ($searched+$s,$matches+$m,$reported+$r);
    }
    else {
      print "grepx: Skipping search of add. directories - already got enough matches.\n";
    }
  }

  if ($searched == 0) {
    print "grepx: No files matched.\n";
    print "grepx: Retrying using '$extension' as wildcard.\n";

    $use_as_wildcard     = 1;
    ($searched,$matches,$reported) = perform_grep($startdir,0);
    if ($searched == 0) { print "grepx: No files matched.\n"; }
  }

  if ($searched>0) {
    print "grepx: Searched $searched files (".megagiga($lines_examined)."LOC)\n";
    if ($matches>0) {
      print "grepx: ";
      if ($reported == $matches) { print "Found $matches"; }
      else { print "Reported $reported (of $matches found)"; }
      print " matches in ".(time-$start_time)." seconds.\n";
    }
    else { print "grepx: No matches were found.\n"; }
  }

  print "\n";
};
if ($@) {
  print_usage();
  die "Error: $@";
}

# --------------------------------------------------------------------------------
