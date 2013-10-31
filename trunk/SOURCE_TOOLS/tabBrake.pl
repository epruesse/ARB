#!/usr/bin/perl
#
# checks files for TABs and raises error if TABs found
# (TABs are evil, cause they have no default width)
#
# As well checks for older ARB compile logs and removes them


use strict;
use warnings;

my $forceAll  = 0; # 1 -> force scan of all files if no stamp, 0 -> assume all ok, scan only new files
my $defsStart = 15; # lineno of start of definition

my $checkForOldLogs = 1; # whether to check for old logfile and delete them

my %scan_extension = map { $_ => 1; } (
                                       'c', 'h',
                                       'cxx', 'hxx',
                                       'cpp', 'hpp',
                                       'f', 'inc',
                                       'm4', 'status', 'in',
                                       'txt', 'doc', 'README', 'readme',
                                       'sh',
                                       'pl', 'pm', 'PL', 'cgi',
                                       'java',
                                       'head', 'header', 'default', 'footer',
                                       'makefile',
                                       'template',
                                       'dat',
                                       'dtd', 'xsl',
                                       'eft', 'ift', 'ift2', 'amc',
                                       'sellst',
                                       'mask',
                                       'aisc', 'pa',
                                       'source', 'menu',
                                       'mat', 'conf',
                                      );

my %auto_determine = map { $_ => 1; } (
                                       'x', 'jar',
                                      );

my %ignore_extension = map { $_ => 1; } (
                                         'a', 'o', 'so',
                                         'class',
                                         'html', 'bitmap', 'ps', 'icon', 'hlp', 'help', 
                                         'gif', 'png', 'fig', 'vfont', 'SAVE',
                                         'tgz', 'gz',
                                         'lst', 'depends',
                                         'md5', 'bs', 'xs',
                                         'exists', 'ignore', '3pm',
                                         'arb', 'seq', 'genmenu',
                                         'init', 'cvsignore', 'log', 'am', 'org',
                                         'last_gcc', 'last_compiler', 'csv',
                                         'b', 'n', 'helpfiles', 
                                         'ms', 'gon', 'pep', 'bla', 'tct', 'comm',
                                         'before', 'v1', 'data', 'pdb', 'embl', 'xpm', 'big',
                                         '2_manual_postscript', '2_manual_text',
                                         'stamp',
                                        );

my %ignored_subdirs = map { $_ => 1; } (
                                        'GDE',
                                        'READSEQ',
                                        'HEADERLIBS',
                                        'patches',
                                        'UNIT_TESTER/flags',
                                        'UNIT_TESTER/logs',
                                        'UNIT_TESTER/run',
                                        'UNIT_TESTER/sockets',
                                        'UNIT_TESTER/tests',
                                        'UNIT_TESTER/valgrind',
                                        'bin',
                                       );

my %ignored_relpath = map { $_ => 1; } (
                                        'NAMES_COM/names_client.h',
                                        'NAMES_COM/names_server.h',
                                        'PROBE_COM/PT_com.h',
                                        'PROBE_COM/PT_server.h',
                                       );

my $tab_count        = 0;
my $files_newer_than = 0;

sub getModtime($) {
  my ($file) = @_;
  my @st = stat($file);
  if (not @st) { die "can't stat '$file' ($!)"; }
  return $st[9];
}


sub scan_for_tabs($) {
  my ($file) = @_;
  my $has_tabs = `grep -m 1 -n -H '\t' $file`;

  # print "$file:0: has_tabs='$has_tabs'\n";

  if ($has_tabs ne '') {
    my $pos = "$file:0:";
    if ($has_tabs =~ /^[^:]+:[0-9]+:/) {
      $pos = $&;
    }
    print "$pos contains tabs. Tabs are not allowed.\n";
    $tab_count++;
    if ($tab_count>10) { die "Further scan skipped.\n"; }
  }
}

sub recurse_dirs($$);
sub recurse_dirs($$) {
  my ($dir,$basedir) = @_;

  my @subdirs = ();
  my $subdir  = '';
  if ($dir ne $basedir) {
    $subdir = substr($dir,length($basedir)+1);
    if (defined $ignored_subdirs{$subdir}) {
      # print "Ignoring '$subdir' (dir='$dir')\n";
      return;
    }
  }

  opendir(DIR, $dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $dir.'/'.$_;
      my $rel  = $subdir.'/'.$_;
      if (-l $full) {
        # print "$full:0: link -- skipped\n";
      }
      elsif (-d $full) {
        if ($_ ne 'CVS' and
            $_ ne '.svn' and
            $_ ne 'ARB_SOURCE_DOC' and
            $_ ne 'old_help' and
            $_ ne 'PERL2ARB' and
            $_ ne 'GENC' and $_ ne 'GENH'
           ) {
          push @subdirs, $full;
        }
      }
      elsif (-f $full) {
        my $scan      = -1;
        my $determine = 0;
        my $modtime   = getModtime($full);

        if ($modtime<$files_newer_than) {
          # file was created before last run of tabBrake.pl
          $scan = 0;

          # check if it's a log from an aborted compile
          if (/^[^.]+\.([0-9]+)\.log$/o) {
            if ($checkForOldLogs and $modtime<($files_newer_than-3*60)) {
              print "Old log file: $full -- removing\n";
              unlink($full) || print "$full:0: can't unlink (Reason: $!)\n";
            }
          }
        }
        elsif (defined $ignored_relpath{$rel}) {
          # print "$full:0: excluded by relpath '$rel'\n";
          $scan = 0;
        }
        else {
          if (/\.([^.]+)$/) {   # file with extension
            my $ext = $1;
            if (exists $ignore_extension{$ext}) {
              # print "$full:0: excluded by extension '$ext'\n";
              $scan = 0;
            }
            elsif (exists $scan_extension{$ext}) {
              # print "$full:0: scan!\n";
              $scan = 1;
            }
            elsif (exists $auto_determine{$ext}) {
              # print "$full:0: scan!\n";
              $determine = 1;
            }
            else {
              if ($ext =~ /^[0-9]+$/ 
                 ) {
                $scan = 0;
              }
              elsif ($full =~ /util\/config\./ or
                     $full =~ /lib\/config\./
                    ) {
                $scan = 1;
              }
              else {
                                # print "$full:0: a '$ext' file\n";
              }
            }
          }
          else {
            if (/Makefile/ or
                /ChangeLog/
               ) {
              $scan = 0;
            }
            else {
              $determine = 1;
            }
          }

          if ($determine==1) {
            $scan==-1 || die "logic error";
            my $file_says = `file $full`;
            if ($file_says =~ /^[^:]+:[ ]*(.*)$/) {
              $file_says = $1;
            }
            else {
              die "Can't parse output from 'file'-command";
            }

            if ($file_says =~ /executable/ or
                $file_says eq 'empty' or
                $file_says eq 'data' or
                $file_says eq 'IFF data' or
                $file_says =~ /archive.*data/
               ) {
              $scan = 0;
            }
            elsif ($file_says =~ /shell.*script/ or
                   $file_says =~ /ASCII.*text/ or 
                   $file_says =~ /ISO.*text/ or
                   $file_says =~ /perl.*script/ 
                  ) {
              $scan = 1;
            }
            else {
              print "$full:0: file_says='$file_says'\n";
            }
          }
        }
        if ($scan==-1) {
          if (/^#.*#$/) {
            print "$full:1: a lost file?\n";
          }
          else {
            # die "Don't know what to do with '$full'";
          }
        }
        elsif ($scan==1) {
          scan_for_tabs($full);
          # print "$full:0: scanning..\n";
        }
      }
    }
  }
  closedir(DIR);

  foreach (@subdirs) {
    recurse_dirs($_,$basedir);
  }
}

# --------------------------------------------------------------------------------

print "--- TAB brake ---\n";

my $ARBHOME = $ENV{'ARBHOME'};

if (not defined $ARBHOME) {
  die "'ARBHOME' not defined";
}

my $time_stamp = $ARBHOME.'/SOURCE_TOOLS/stamp.tabBrake';
my $exitcode   = 0;
if (-f $time_stamp) {
  $files_newer_than = getModtime($time_stamp);
  print "Checking files newer than ".localtime($files_newer_than)."\n";
}
else {
  if ($forceAll==1) {
    print "Initial call - checking all files\n";
    $files_newer_than = 0;
  }
  else {
    print "Initial call - assuming everything is TAB-free\n";
    $files_newer_than = time;
    $checkForOldLogs  = 0; # do not check for old logs (sometimes fails on fresh checkouts, e.g. in jenkins build server)
  }
}

if ($exitcode==0) {
  `touch $time_stamp`;          # mark time of last try
  # i.e. user inserts TAB to file -> fail once
}
recurse_dirs($ARBHOME,$ARBHOME);
if ($tab_count>0) {
  $exitcode = 1;
  print $ARBHOME.'/SOURCE_TOOLS/tabBrake.pl:'.$defsStart.': Warning: what may contain TABs is defined here'."\n";
}

exit($exitcode);
