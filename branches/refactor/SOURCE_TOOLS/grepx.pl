#!/usr/bin/perl
# ======================================================================== #
#                                                                          #
#   File      : grepx.pl                                                   #
#   Purpose   : Replacement for grep (used from emacs)                     #
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
# * able to search complete CVS/SVN trees
# * some ARB specific specials
#
# --------------------------------------------------------------------------------

use strict;
use warnings;
use Cwd;

# --------------------------------------------------------------------------------

my $tabsize = 4; # specify your emacs tabsize here (used to correct column position)

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
               [
                '/usr/include',
                '/usr/include/X11',
                '/usr/include/g++',
                '/usr/include/sys',
               ], # additional header directories (used with -g)
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
              [ # anything where aci/srt commands occur
               [ ],
               [ '.menu', '.source', '.hlp', '.eft', '.ift', '.mask', '.sellst' ],
               [ ],
               [ '.c', '.cxx' ],
              ],
              );

# files always searched (not by 'same' and 'header' search)
my @normally_searches = ( 'makefile' );

# files always searched by global search
my @global_always_searches = ( );

# --------------------------------------------------------------------------------

my $global           = 0;
my $headers_only     = 0;
my $same_ext_only    = 0;
my $ignore_case      = 0;
my $recurse_subdirs  = 0;
my $one_hit_per_line = 0;
my $verbose          = 0;
my $matchFiles       = 1;
my $arbSpecials      = 0;
my $maxhits          = undef; # undef means unlimited
my $searchNonCVS     = 0;

my $extension       = undef;
my $use_as_wildcard = 0;
my $regexpr         = undef;

my $calldir  = cwd();
my $startdir = undef;

# --------------------------------------------------------------------------------

my $GSM_NONE   = 0;
my $GSM_CVS    = 1; # scan a CVS/SVN tree
my $GSM_PARENT = 2; # do a simple parent scan

my $global_scan_mode = $GSM_NONE;

# --------------------------------------------------------------------------------

sub shall_skip_file($) {
  my ($file) = @_;
  die "arbSpecials not 1" if ($arbSpecials!=1);
  if ($file =~ /PERL2ARB\//o) {
    my $rest = $';
    if ($rest eq 'ARB.c' or $rest eq 'proto.h') { return 1; }
  }
  elsif ($file =~ /lib\/help\//o) {
    return 1;
  }
  return 0;
}

# --------------------------------------------------------------------------------

my @ignores = (); # directory local excludes (reg.expressions)
my $ignoreCount = 0; # overall ignore count

sub forget_grepxignore() { @ignores = (); }

sub load_grepxignore($) {
  my ($grepxignore) = @_;

  @ignores = ();
  open(IGNORE,'<'.$grepxignore) || die "can't open '$grepxignore' (Reason: $!)";
  foreach (<IGNORE>) {
    chomp;
    push @ignores, qr/^$_$/;
  }
  close(IGNORE);
}

sub is_ignored($) {
  my ($name) = @_;
  foreach (@ignores) {
    if ($name =~ $_) {
      $verbose==0 || print "Ignoring '$name' (by $_)\n";
      $ignoreCount++;
      return 1;
    }
  }
  return 0;
}

# --------------------------------------------------------------------------------

my $reg_nameOnly  = qr/\/([^\/]+)$/;
my $reg_extension = qr/(\.[^\.]+)$/;
# (\.[^\.]+)

my ($IS_HEADER,$IS_NORMAL,$IS_OTHER,$IS_ADDITIONAL) = (4,3,2,1);

my %wanted_extensions = ();
my %wanted_files      = (); # files that are always searched

my @add_header_dirs = ();

my $reg_is_cpp_std_dir = qr/^\/usr\/include\/g\+\+(\/|$)/;

sub shall_search_file($$) {
  my ($file,$indir) = @_;

  if ($use_as_wildcard==0) {
    if ($file =~ $reg_nameOnly) { $file = $1; } # behind last /

    if ($file =~ /^\.?\#/ or $file =~ /~$/) { return 0; } # skip backup files etc.

    my $ext = '';
    if ($file =~ $reg_extension) { $ext = $1; }

    if ($ext eq '') {
      if ($indir =~ $reg_is_cpp_std_dir) {
        # print "hack: considering $file in $indir\n";
        $ext = '.h'; # special hack for new style C++ header (they suck an extension)
      }
      else {
        if (not $haveFile) { return 0; }
        my $full = $indir.'/'.$file;
        my $type = `file $full`; # detect filetype
        chomp $type;
        if ($type =~ /^[^:]+: (.*)/o) {
          $type = $1;
          if ($type =~ /shell.*script/o) { $ext = '.sh'; }
          elsif ($type =~ /perl.*script/o) { $ext = '.pl'; }
          elsif ($type =~ /ASCII.*text/o) { $ext = '.txt'; }
          elsif ($type =~ /ISO.*text/o) { $ext = '.txt'; }
          elsif ($type =~ /executable/o) { ; }
          elsif ($type =~ /symbolic.link.to/o) { ; }
          else {
            print "Unhandled file='$full'\n        type='$type'\n";
          }
        }
      }
    }

    $ext = lc($ext);
    if (exists $wanted_extensions{$ext}) { return NotIgnored($file,$wanted_extensions{$ext}); }

    $file = lc($file);
    if (exists $wanted_files{$file}) { return NotIgnored($file,$IS_OTHER); }
  }
  else {
    if ($file =~ /$extension/ig) {
      return NotIgnored($file,$IS_NORMAL);
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
    " -A -> do ARB specials if \$ARBHOME is defined\n".
    " -m xxx -> report max. xxx hits\n".
    " -c     -> search in non-CVS/SVN files as well (default is to search CVS/SVN controlled files only)".
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
      elsif ($option eq 'A') {
        if (exists $ENV{'ARBHOME'}) { $arbSpecials = 1; }
        else { print "grepx: Ignoring -A (ARBHOME not set)"; }
      }
      elsif ($option eq 'm') { $maxhits = int($ARGV[++$ap]); }
      elsif ($option eq 'c') { $searchNonCVS = 1; }
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

sub pos_correction($$) {
  my ($line,$pos) = @_;
  my $prematch = substr($line,0,$pos);
  $prematch =~ s/[^\t]//go;
  return length($prematch)*($tabsize-1);
}

my $lines_examined = 0;
my $reg_startdir = undef;

sub grepfile($$\$) {
  my ($file,$entering,$entering_shown_r) = @_;

  my $matches  = 0;
  my $reported = 0;
  my $show     = $file;

  if ($file =~ $reg_startdir) { $show = $'; }

  open(FILE,"<$file") || die "can't read file '$file' (Reason: $!)";
  while (my $line = <FILE>) {
    if ($line =~ $regexpr) {
      if ((not defined $maxhits) or ($maxhits>0)) {
        my $rest   = $';
        my $hitlen = $+[0] - $-[0];
        my $pos;

        $hitlen>0 || die "Non-positive hitlen (=$hitlen) [1]";

        if ($#+ > 0) { # regexpr has subgroups -> point to start of first subgroup
          $pos = $-[$#+] + 1; # start of first subgroup
        }
        else {
          $pos = $-[0] + 1; # start of regexpr
        }

        if ($matches==0 and $arbSpecials==1) {
          if (shall_skip_file($file)==1) {
            print "grepx: Unlisted occurrence(s) in $file\n";
            return (0,0);
          }
        }

        my $correct = pos_correction($line,$pos);
        $line =~ s/\r//o;
        $line =~ s/\n//o;
        chomp($line);
        $pos += $correct;
        $line =~ s/^([\s\t]+)//o;
        my $hits = 1;

        if ($one_hit_per_line==0) {
          if ($$entering_shown_r==0) { $$entering_shown_r=1; print $entering; }
          print "$show:$.:$pos:        $line\n";
          $rest =~ s/\r//o;
          $rest =~ s/\n//o;
          chomp($rest);

          while ($rest =~ $regexpr) {
            my $start_pos = $pos+$hitlen-1;

            $start_pos >= 0 || die "Negative start_pos(=$start_pos, pos=$pos, hitlen=$hitlen)";

            $hitlen = $+[0] - $-[0];
            $hitlen>0 || die "Non-positive hitlen (=$hitlen) [2]";

            if ($#+ > 0) {
              $pos = $-[$#+] + 1;
            }
            else {
              $pos = $-[0] + 1;
            }
            $correct = pos_correction($rest,$pos);
            $pos += $start_pos+$correct;

            $pos >= 0 || die "Negative pos";

            if ($$entering_shown_r==0) { $$entering_shown_r=1; print $entering; }
            print "$show:$.:$pos: [same] $line\n";
            $hits++;
            $rest = $';
          }
        }
        else {
          if ($$entering_shown_r==0) { $$entering_shown_r=1; print $entering; }
          print "$show:$.:$pos: $line\n";
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

my $versionControl = '<unknown version control>';

sub CVS_controlled($) {
  my ($dir)       = @_;
  my $SVN_entries = $dir.'/.svn/entries';
  if (-f $SVN_entries) {
    $versionControl = 'subversion';
    1;
  }
  else {
    my $CVS_Repository = $dir.'/CVS/Repository';
    if (-f $CVS_Repository) {
      $versionControl = 'CVS';
      1;
    }
    else {
      0;
    }
  }
}

sub parent_directory($) {
  my ($dir) = @_;
  if ($dir =~ /\/[^\/]+$/) {
    return $`;
  }
  return undef;
}

# --------------------------------------------------------------------------------

sub collect_files($\%$$);
sub collect_files($\%$$) {
  my ($dir,$files_r,$is_additional_directory,$follow_file_links) = @_;

  my @files   = ();
  my @subdirs = ();

  opendir(DIR, $dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $dir.'/'.$_;
      if (-l $full and ($follow_file_links==0 or -d $full)) { $verbose==0 || print "Skipping $full (symbolic link)\b"; }
      elsif (-f $full) { push @files, $full; }
      elsif (-d $full) { push @subdirs, $full; }
      else { $verbose==0 || print "Skipping $full (not a file or directory)\n"; }
    }
  }
  closedir(DIR);

  my $grepxignore = $dir.'/.grepxignore';
  if (-f $grepxignore) { load_grepxignore($grepxignore); }
  else { forget_grepxignore(); }

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
    my @descent_into = ();

    foreach (@subdirs) {
      my $descent = 1;
      my $reason = 'not specified';
      if ($global_scan_mode==$GSM_CVS and not $is_additional_directory and not CVS_controlled($_)) {
        if ($arbSpecials==1 and $_ =~ /\/GEN[CH]$/) {
          $verbose==0 || print "Descending non-$versionControl dir '$_' (caused by ARB mode)\n";
        }
        else {
          $descent = 0;
          $reason = 'not version-controlled';
        }
      }

      if ($descent==1) {
        $descent = NotIgnored($_,1);
        if ($descent==0) { $reason = 'Excluded by .grepxignore'; }
      }

      if ($descent==1) {
        push @descent_into, $_;
      }
      else {
        $verbose==0 || print "Skipping subdirectory '$_' ($reason)\n";
      }
    }

    foreach (@descent_into) {
      collect_files($_, %$files_r, $is_additional_directory,$follow_file_links);
    }
  }
}

sub grep_collected_files(\%$) {
  my ($files_r,$entering) = @_;

  my $entering_shown = 0;

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
        $cmp = $a cmp $b; # alphabetically
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
        if ($entering_shown==0) { $entering_shown=1; print $entering; }
        print "$show:0: <filename matched>\n";
      }
    }
  }

  # print "grepx: Searching $searched files..\n";
  foreach (@files) {
    $verbose==0 || print "searching '$_' (depth=$depth{$_}, importance=$$files_r{$_})\n";
    my ($m,$r) = grepfile($_,$entering,$entering_shown);
    $matches += $m;
    $reported += $r;
  }


  return ($searched,$matches,$reported);
}

sub perform_grep($$$) {
  my ($startdir, $is_additional_directory, $follow_file_links) = @_;
  my %files = (); # key=file, value=file-importance
  collect_files($startdir,%files,$is_additional_directory,$follow_file_links);

  my $max_importance = -1;
  foreach (values %files) {
    if ($_ > $max_importance) { $max_importance = $_; }
  }

  if ($max_importance<=$IS_OTHER) {
    print "grepx: Only found files with importance==$max_importance (aborting)\n";
    %files = ();
  }

  my ($searched,$matches,$reported) = (0,0,0);
  if (scalar(%files)) {
    my $entering                   = "grepx: Entering directory `$startdir'\n";
    ($searched,$matches,$reported) = grep_collected_files(%files,$entering);
    if ($reported>0) { print "grepx: Leaving directory `$startdir'\n"; }
  }
  return ($searched,$matches,$reported);
}

sub grep_add_directories() {
  my ($searched,$matches,$reported) = (0,0,0);
  foreach (@add_header_dirs) {
    my ($s,$m,$r) = perform_grep($_,1,0);
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
      print "grepx: Starting global search from root of $versionControl controlled directory-tree\n";
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

  my ($searched,$matches,$reported) = perform_grep($startdir,0,0);
  if ($matches==0) {
    print "grepx: No results - retry with links..\n";
    ($searched,$matches,$reported) = perform_grep($startdir,0,1); # retry following links
  }

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
    ($searched,$matches,$reported) = perform_grep($startdir,0,0);
    if ($matches==0) {
      print "grepx: No results - retry with links..\n";
      ($searched,$matches,$reported) = perform_grep($startdir,0,1);  # retry following links
    }
    if ($searched == 0) { print "grepx: No files matched.\n"; }
  }

  if ($searched>0) {
    my $info = "Searched $searched files (".megagiga($lines_examined)."LOC). ";
    if ($matches>0) {
      if ($reported == $matches) { $info .= "Found $matches"; }
      else { $info .= "Reported $reported (of $matches found)"; }
      $info .= " matches in ".(time-$start_time)." seconds.";
    }
    else { $info .= "No matches were found."; }
    print "grepx: $info\n";
  }

  if ($ignoreCount>0) {
    print "grepx: excluded by .grepxignore: $ignoreCount files/directories\n";
  }
};
if ($@) {
  print_usage();
  die "Error: $@";
}

# --------------------------------------------------------------------------------
