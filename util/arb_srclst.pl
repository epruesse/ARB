#!/usr/bin/perl

use strict;
use warnings;

my $debug_matching = 0;
my $ignore_unknown = 0;

# ------------------------------------------------------------
# skipped_directories and files inside are never examined:

my @skipped_directories = (
                           qr/_GEN$/o,
                           qr/_COM\/GEN[CH]$/o,
                           qr/_COM\/O$/o,
                           qr/_COM\/DUMP$/o,
                           qr/\/.+\/bin$/o,
                           qr/\/HELP_SOURCE\/Xml$/o,
                           qr/\/PERL2ARB\/blib$/o,
                           qr/\/UNIT_TESTER\/tests$/o,
                           qr/\/UNIT_TESTER\/logs$/o,
                           qr/^\.\/INCLUDE$/o,
                           qr/^\.\/PERL5$/o,
                           qr/^\.\/lib\/pts$/o,
                           qr/^\.\/lib\/help$/o,
                           qr/^\.\/lib\/help_html$/o,
                           qr/^\.\/ARB_SOURCE_DOC/o,
                           qr/^\.\/MAKEBIN$/o,
                           qr/^\.\/LIBLINK$/o,
                           qr/^\.\/patches.arb$/o,
                           qr/\/ignore\./o,
                          );

# first used/skipped match wins (exception see @3 below)

my %used_files = map { $_ => 1; } (
                                   'demo.arb',
                                   'export2sub',
                                   'needs_libs',
                                   'Doxyfile',
                                   'Makefile',
                                   'Makefile.org',
                                   'Makefile.test',
                                   'Makefile.setup',
                                   'Makefile.suite',
                                   '!BRANCH_STATE',
                                   'AUTHORS',
                                   'COPYING',
                                  );

my %skipped_files = map { $_ => 1; } (
                                      '.cvsignore',
                                      '.build.lst',
                                      '.depends',
                                      'config.makefile',
                                      'ChangeLog',
                                      'ARB_GDEmenus',
                                      'makeloc.here',
                                      'helpfiles.lst',
                                      'makeloc.here',
                                      'nt_date.h',
                                      'TAGS',
                                     );

my %used_extensions = map { $_ => 1; } (
                                        'c', 'cpp', 'cxx',
                                        'h', 'hpp', 'hxx',
                                        'f',
                                        'pl', 'pm', 'PL', 'cgi',
                                        'java', 'manifest',
                                        'inc',
                                        'sh',
                                        'aisc', 'pa',
                                        'template', 'default',
                                        'script',
                                        'txt', 'doc', 'ps', 'pdf',
                                        'bitmap',
                                        'source', 'menu',
                                        'head', 'header',
                                        'footer',
                                        'dtd', 'xsl',
                                        'makefile',
                                       );

my %skipped_extensions = map { $_ => 1; } (
                                           'o',
                                           'so',
                                           'a',
                                           'genmenu',
                                           'class',
                                           'jar',
                                           'stamp',
                                           'list',
                                           'log',
                                           'swp',
                                           'bak',
                                           'old',
                                           'last_gcc',
                                           'patch',
                                          );


# used_when_matches, skipped_when_matches and used_when_matchesFull are only tested,
# if above filters did not match:

my @used_when_matches = (
                         qr/^arb_.*\.txt$/o,
                         qr/license/io,
                         qr/disclaimer/io,
                         qr/needs_libs\..*/io,
                         qr/readme$/io,
                         qr/unused.*source.*\.tgz$/io,
                        );

my @skipped_when_matches = (
                            qr/^arbsrc\.lst$/o,
                            qr/^arbsrc\.lst\.tmp$/o,
                            qr/^arbsrc.*\.tgz$/o,
                            qr/\#.*\#$/o,
                            qr/\.\#.*$/o,
                            qr/.*~$/o, # backups
                           );

my @used_when_matchesFull = (
                             qr/\/EISPACK\/rg\.html$/o,
                             qr/\/CLUSTALW\/.*$/o,
                             qr/\/HGL_SRC\/plot\.icon$/o,
                             qr/\/PHYLIP\/doc\//o,
                             qr/\/GDE\/.*\.html$/o,
                             qr/GDE\/.*\/Makefile\.[^\/]+$/io,
                             qr/\/GDEHELP\/GDE.*/o,
                             qr/\/GDEHELP\/Makefile\.helpfiles/o,
                             qr/\/GDEHELP\/DATA_FILES/o,
                             qr/\/GDEHELP\/FASTA/o,
                             qr/\/GDEHELP\/HELP_PLAIN/o,
                             qr/\/GDEHELP\/HELP_WRITTEN/o,
                             qr/\/HELP_SOURCE\/.*\.gif$/o,
                             qr/\/HELP_SOURCE\/oldhelp\/.*\.hlp$/o,
                             qr/\/HELP_SOURCE\/oldhelp\/.*\.(ps|pdf)\.gz$/o,
                             qr/\/TREEPUZZLE\/.*\.gif$/o,
                             qr/\/PERL2ARB\/.*\.html$/o,
                             qr/\/PERL2ARB\/typemap$/o,
                             qr/\/PERL2ARB\/Makefile.main$/o,
                             qr/\/PERL2ARB\/ARB\.default\.xs$/o,
                             qr/\/PROBE_SERVER\/.*\.conf$/o,
                             qr/\/READSEQ\/Formats$/o,
                             qr/\/READSEQ\/.*\.help$/o,
                             qr/\/SH\/[^\/\.]*$/o,
                             qr/\/SOURCE_TOOLS\//o,
                             qr/\/UNIT_TESTER\/run\/.*\.arb$/o,
                             qr/\/UNIT_TESTER\/run\/.*\.a00$/o,
                             qr/\/UNIT_TESTER\/run\/.*\.expected$/o,
                             qr/\/UNIT_TESTER\/run\/.*\.input$/o,
                             qr/\/UNIT_TESTER\/run\/impexp\/.*\.exported$/o,
                             qr/\/UNIT_TESTER\/run\/TEST_.*\.pt$/o, # has to move to skipped_when_matchesFull (when merging test env)
                             qr/\/UNIT_TESTER\/run\/.*\.inp$/o,
                             qr/\/UNIT_TESTER\/run\/.*\.in$/o,
                             qr/\/UNIT_TESTER\/run\/.*\.out$/o,
                             qr/\/UNIT_TESTER\/run\/.*\.tree$/o,
                             qr/\/UNIT_TESTER\/valgrind\/arb_valgrind_logged$/o,
                             qr/^\.\/etc\//o,
                             qr/^\.\/lib\/arb_tcp_org\.dat$/o,
                             qr/^\.\/lib\/config\.[^\.]+$/io,
                             qr/^\.\/lib\/arb_default\/.*\.arb$/o,
                             qr/^\.\/lib\/export\/.*\.eft$/o,
                             qr/^\.\/lib\/import\/.*\.ift2?$/o,
                             qr/^\.\/lib\/inputMasks\/.*\.mask$/o,
                             qr/^\.\/lib\/macros\/.*\.amc$/o,
                             qr/^\.\/lib\/nas\/names\.dat\.template$/o,
                             qr/^\.\/lib\/pictures\/.*\.(fig|vfont)$/o,
                             qr/^\.\/lib\/pixmaps\/.*\.xpm$/o,
                             qr/^\.\/lib\/rna3d\/.*\.(pdb|data)$/o,
                             qr/^\.\/lib\/rna3d\/images\/.*\.png$/o,
                             qr/^\.\/lib\/sellists\/.*\.sellst$/o,
                             qr/^\.\/lib\/protein_2nd_structure\/.*\.dat$/o,
                             qr/^\.\/lib\/submit\//o,
                             qr/^\.\/util\/arb_.*$/o,
                             qr/^\.\/util\/config\..*$/o,
                            );

# skipped_when_matchesFull and forced_when_matchesFull are always tested! (@3)

my @skipped_when_matchesFull = (
                                qr/\/MOLPHY\/prot_tml\.h$/o,
                                qr/date\.xsl$/o,
                                qr/\/genhelp\/.*\.hlp$/o,
                                qr/^\.\/GDE\/CORE\/functions.h$/o,
                                qr/^\.\/PERL2ARB\/.*\.h$/o,
                                qr/^\.\/PERL2ARB\/ARB\.xs$/o,
                                qr/^\.\/PERL2ARB\/ARB\.c$/o,
                                qr/^\.\/PERL2ARB\/ARB\.bs$/o,
                                qr/^\.\/PERL2ARB\/pm_to_blib$/o,
                                qr/^\.\/PERL2ARB\/Makefile$/o,
                                qr/^\.\/PERL2ARB\/Makefile.PL$/o,
                                qr/^\.\/PERL2ARB\/perlmain.c$/o,
                                qr/^\.\/TEMPLATES\/arb_build\.h$/o,
                                qr/^\.\/lib\/ARB\.pm$/o,
                                qr/^\.\/lib\/nas\/names.*\.dat$/o,
                                qr/^\.\/lib\/arb_tcp\.dat$/o,
                                qr/^\.\/arb.*\.tgz$/o,
                                qr/^\.\/SOURCE_TOOLS\/valgrind2grep\.lst$/o,
                                qr/\/lib\/addlibs\/(lib.*\.so\..*)$/o,
                               );

my @forced_when_matchesFull = (
                               qr/\/PROBE_WEB\/SERVER\/.*\.jar$/o,
                              );

# files that are even packed when generated and not in VC
my @pack_fullGenerated = (
                          qr/\/TEMPLATES\/svn_revision\.h$/o,
                         );

# ------------------------------------------------------------
# sanity checks

foreach (keys %used_extensions) {
  if (exists $skipped_extensions{$_}) { die "'$_' in \$used_extensions and \$skipped_extensions"; }
}
foreach (keys %used_files) {
  if (exists $skipped_files{$_}) { die "'$_' in \$used_files and \$skipped_files"; }
}

# ------------------------------------------------------------

sub useDir($) {
  my ($dir) = @_;

  if ($dir =~ /.svn$/o) { return 0; }
  if ($dir =~ /CVS$/o) { return 0; }
  foreach (@skipped_directories) {
    if ($dir =~ $_) { return 0; }
  }
  return 1;
}

sub matchingExpr($\@) {
  # return 0 if no regexp matched, return index+1 otherwise
  my ($str,$regexp_arr_r) = @_;

  my $regexps = scalar(@$regexp_arr_r);
  for (my $r=0; $r<$regexps; $r++) {
    my $reg = $$regexp_arr_r[$r];
    if ($str =~ $reg) {
      return $r+1;
    }
  }
  return 0;
}

sub useIfMatching($\@\$) {
  my ($str,$regexp_arr_r,$use_r) = @_;
  my $matches = matchingExpr($str,@$regexp_arr_r);
  if ($matches>0) {
    if ($debug_matching!=0) { print "'$str' matches '".$$regexp_arr_r[$matches-1]."' => use!\n"; }
    $$use_r = 1;
  }
}
sub dontUseIfMatching($\@\$) {
  my ($str,$regexp_arr_r,$use_r) = @_;
  my $matches = matchingExpr($str,@$regexp_arr_r);
  if ($matches>0) {
    if ($debug_matching!=0) { print "'$str' matches '".$$regexp_arr_r[$matches-1]."' => don't use!\n"; }
    $$use_r = 0;
  }
}

sub useFile($$) {
  my ($dir,$file) = @_;

  my $use = undef;
  if (exists $used_files{$file}) { $use = 1; }
  elsif (exists $skipped_files{$file}) { $use = 0; }

  my $hasExt = 0;
  if (not defined $use) {
    if ($file =~ /\.([^\.]+)$/o) {
      my $ext = $1;
      $hasExt = 1;
      if (exists $used_extensions{$ext}) { $use = 1; }
      elsif (exists $skipped_extensions{$ext}) { $use = 0; }
    }
  }

  if (not defined $use) {
    useIfMatching($file,@used_when_matches, $use);
  }

  if (not defined $use) {
    dontUseIfMatching($file,@skipped_when_matches, $use);
  }

  my $full;
  if (not defined $use) {
    $full = $dir.'/'.$file;

    useIfMatching($full,@used_when_matchesFull, $use);
    if (not defined $use) {
      if (-X $full and $hasExt==0) { $use = 0; } # exclude binaries by default (wrong for scripts)
    }
  }

  if (not defined $use or $use==1) {
    if (not defined $full) { $full = $dir.'/'.$file; }

    dontUseIfMatching($full,@skipped_when_matchesFull, $use);
  }
  if (not defined $use or $use==0) {
    if (not defined $full) { $full = $dir.'/'.$file; }
    useIfMatching($full,@forced_when_matchesFull, $use);
  }

  if (not defined $use) {
    if ($ignore_unknown==0) {
      die "Don't know whether to use or skip '$file' (in $dir)";
    }
    $use = 1;
  }

  return $use;
}

# ------------------------------------------------------------

sub getSVNEntries($\%) {
  my ($dir,$SVN_r) = @_;

  my $svnentries = $dir.'/.svn/entries';
  if (-f $svnentries) {
    open(SVN,'<'.$svnentries) || die "can't read '$svnentries' (Reason: $!)";
    # print "reading $svnentries\n";

    my $line;
  LINE: while (defined($line=<SVN>)) {
      if (length($line)==2 and ord($line)==12) { # entrymarker (^L)
        my $name=<SVN>;
        my $type=<SVN>;

        defined $name or last LINE;
        defined $type or die "Expected two or no lines after ^L";

        chomp($name);
        chomp($type);

        if ($type eq 'file') {
          $$SVN_r{$name} = 1;
        }
        elsif ($type eq 'dir') {
          $$SVN_r{$name} = 2;
        }
        else {
          die "Unknown type '$type' for '$name' in $svnentries";
        }
        # print "name='$name' type='$type'\n";
      }
    }

    close(SVN);
    return 1;
  }
  print "No such file: '$svnentries'\n";
  return 0;
}

sub getCVSEntries($\%) {
  my ($dir,$CVS_r) = @_;

  my $cvsentries = $dir.'/CVS/Entries';
  if (-f $cvsentries) {
    open(CVS,'<'.$cvsentries) || die "can't read '$cvsentries' (Reason: $!)";
    eval {
      foreach (<CVS>) {
        chomp;
        if (/^D\/([^\/]+)\//o) { # directory
          $$CVS_r{$1} = 2;
        }
        elsif (/^\/([^\/]+)\//o) { # file
          $$CVS_r{$1} = 1;
        }
        elsif (/^D$/o) {
          ;
        }
        else {
          die "can't parse line '$_'";
        }
      }
    };
    if ($@) { die "$@ while reading $cvsentries"; }
    close(CVS);
    return 1;
  }
  return 0;
}

my $VC = '<no VC>';

sub getVCEntries($\%) {
  my ($dir,$VC_r) = @_;

  my $res = 1;
  if (getSVNEntries($dir,%$VC_r)==0) {
    if (getCVSEntries($dir,%$VC_r)==0) {
      $VC = '<no VC>';
      $res = 0;
    }
    else {
      $VC = 'CVS';
    }
  }
  else {
    $VC = 'SVN';
  }

  if (0) {
    print "$VC entries for $dir:\n";
    foreach (sort keys %$VC_r) {
      print " ".$$VC_r{$_}.": $_\n";
    }
  }

  return $res;
}

# ------------------------------------------------------------

sub expectVCmember($$\%) {
  my ($full,$item,$VC_r) = @_;
  if ((not defined $$VC_r{$item}) and ($ignore_unknown==0)) {
    if (not matchingExpr($full,@pack_fullGenerated)) {
      die "'$full' ($_) included, but not in $VC (seems to be generated)";
    }
  }
}

my %unpackedCVSmember = map { $_ => 1; } (
                                          '.cvsignore',
                                          'ChangeLog',
                                         );

sub unexpectVCmember($$\%) {
  my ($full,$item,$VC_r) = @_;
  if (defined $$VC_r{$item}) {
    if (not exists $unpackedCVSmember{$item} and $ignore_unknown==0) {
      die "'$full' excluded, but in $VC";
    }
  }
}

sub dumpFiles($);
sub dumpFiles($) {
  my ($dir) = @_;

  my @subdirs;
  my @files;

  my %CVS;
  getVCEntries($dir,%CVS);

  opendir(DIR,$dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $dir.'/'.$_;
      if (not -l $full) {
        if (-d $full) {
          if (useDir($full)==1) {
            expectVCmember($full,$_,%CVS);
            push @subdirs, $full;
          }
          else { unexpectVCmember($full,$_,%CVS); }
        }
        elsif (-f $full) {
          if (useFile($dir,$_)==1) {
            expectVCmember($full,$_,%CVS);
            push @files, $full;
          }
          else { unexpectVCmember($full,$_,%CVS); }
        }
        else { die "Unknown: '$full'"; }
      }
    }
  }
  closedir(DIR);

  foreach (@files) { print $_."\n"; }
  foreach (@subdirs) { dumpFiles($_); }
}

my $args = scalar(@ARGV);
if ($args==0) {
  dumpFiles('.');
}
else {
  my $arg = $ARGV[0];
  if ($arg eq 'ignore') {
    $ignore_unknown = 1;
    dumpFiles('.');
  }
  elsif ($arg eq 'matching') {
    $debug_matching = 1;
    dumpFiles('.');
  }
  else {
    die "Usage: arb_srclst.pl [ignore|matching]\n";
  }
}
