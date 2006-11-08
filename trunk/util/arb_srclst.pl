#!/usr/bin/perl

use strict;
use warnings;

# ------------------------------------------------------------
# first used/skipped match wins:

my %used_files = map { $_ => 1; } (
                                   'demo.arb',
                                   'zcat',
                                   'Doxyfile',
                                   'Makefile',
                                   'Makefile.org',
                                   'AUTHORS',
                                   'COPYING',
                                  );

my %skipped_files = map { $_ => 1; } (
                                      '.cvsignore',
                                      '.depends',
                                      'config.makefile',
                                      'ChangeLog',
                                      'ARB_GDEmenus',
                                      'helpfiles.lst',
                                      '_index.html',
                                      'nt_date.h',
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
                                          );


my @used_when_matches = (
                         qr/^arb_.*\.txt$/,
                         qr/license/i,
                         qr/disclaimer/i,
                         qr/readme$/i,
                         qr/unused.*source.*\.tgz$/i,
                        );

my @skipped_when_matches = (
                            qr/^arbsrc.*\.lst$/,
                            qr/^arbsrc.*\.tgz$/,
                            qr/.*\.last_gcc$/,
                           );

my @used_when_matchesFull = (
                             qr/\/EISPACK\/rg\.html$/,
                             qr/\/CLUSTALW\/.*$/,
                             qr/\/HGL_SRC\/plot\.icon$/,
                             qr/\/PHYLIP\/doc\//,
                             qr/\/GDE\/.*\.html$/,
                             qr/\/GDEHELP\/GDE.*/,
                             qr/\/GDEHELP\/Makefile\.helpfiles/,
                             qr/\/GDEHELP\/DATA_FILES/,
                             qr/\/GDEHELP\/FASTA/,
                             qr/\/GDEHELP\/HELP_PLAIN/,
                             qr/\/HELP_SOURCE\/.*\.gif$/,
                             qr/\/HELP_SOURCE\/oldhelp\/.*\.hlp$/,
                             qr/\/HELP_SOURCE\/oldhelp\/.*\.(ps|pdf)\.gz$/,
                             qr/\/TREEPUZZLE\/.*\.gif$/,
                             qr/\/PERL2ARB\/.*\.html$/,
                             qr/\/PERL2ARB\/typemap$/,
                             qr/\/PROBE_SERVER\/.*\.conf$/,
                             qr/\/READSEQ\/Formats$/,
                             qr/\/READSEQ\/.*\.help$/,
                             qr/\/SH\/[^\/\.]*$/,
                             qr/\/SOURCE_TOOLS\//,
                             qr/^\.\/etc\//,
                             qr/^\.\/lib\/arb_tcp_org\.dat$/,
                             qr/^\.\/lib\/config\.[^\.]+$/i,
                             qr/^\.\/lib\/arb_default\/.*\.arb$/,
                             qr/^\.\/lib\/export\/.*\.eft$/,
                             qr/^\.\/lib\/import\/.*\.ift2?$/,
                             qr/^\.\/lib\/inputMasks\/.*\.mask$/,
                             qr/^\.\/lib\/macros\/.*\.amc$/,
                             qr/^\.\/lib\/nas\/names\.dat\.template$/,
                             qr/^\.\/lib\/pictures\/.*\.(fig|vfont)$/,
                             qr/^\.\/lib\/pixmaps\/.*\.xpm$/,
                             qr/^\.\/lib\/rna3d\/.*\.(pdb|data)$/,
                             qr/^\.\/lib\/rna3d\/images\/.*\.png$/,
                             qr/^\.\/lib\/sellists\/.*\.sellst$/,
                             qr/^\.\/lib\/submit\//,
                             qr/^\.\/util\/arb_.*$/,
                             qr/^\.\/util\/config\..*$/,
                            );

# skipped_when_matchesFull and forced_when_matchesFull are always tested!
my @skipped_when_matchesFull = (
                                qr/\/MOLPHY\/prot_tml\.h$/,
                                qr/date\.xsl$/,
                                qr/\/genhelp\/.*\.hlp$/,
                                qr/^\.\/PERL2ARB\/.*\.h$/,
                                qr/^\.\/PERL2ARB\/ARB\.xs$/,
                                qr/^\.\/PERL2ARB\/ARB\.c$/,
                                qr/^\.\/PERL2ARB\/ARB\.bs$/,
                                qr/^\.\/PERL2ARB\/pm_to_blib$/,
                                qr/^\.\/PERL2ARB\/Makefile$/,
                                qr/^\.\/lib\/ARB\.pm$/,
                                qr/^\.\/lib\/nas\/names\.dat$/,
                               );

my @forced_when_matchesFull = (
                               qr/\/PROBE_WEB\/SERVER\/.*\.jar$/,
                              );

my @skipped_directories = (
                           qr/_GEN$/,
                           qr/_COM\/GEN[CH]$/,
                           qr/_COM\/O$/,
                           qr/\/.+\/bin$/,
                           qr/\/HELP_SOURCE\/Xml$/,
                           qr/\/PERL2ARB\/blib$/,
                           qr/^\.\/INCLUDE$/,
                           qr/^\.\/PERL5$/,
                           qr/^\.\/lib\/pts$/,
                           qr/^\.\/lib\/help$/,
                           qr/^\.\/lib\/help_html$/,
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

  if ($dir =~ /CVS$/o) { return 0; }
  foreach (@skipped_directories) {
    if ($dir =~ $_) { return 0; }
  }
  return 1;
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
    foreach (@used_when_matches) {
      if ($file =~ $_) { $use = 1; }
    }
  }

  if (not defined $use) {
    foreach (@skipped_when_matches) {
      if ($file =~ $_) { $use = 0; }
    }
  }

  my $full;
  if (not defined $use) {
    $full = $dir.'/'.$file;

    foreach (@used_when_matchesFull) {
      if ($full =~ $_) { $use = 1; }
    }

    if (not defined $use) {
      if (-X $full and $hasExt==0) { $use = 0; } # exclude binaries by default (wrong for scripts)
    }
  }

  if (not defined $use or $use==1) {
    if (not defined $full) { $full = $dir.'/'.$file; }
    foreach (@skipped_when_matchesFull) {
      if ($full =~ $_) { $use = 0; }
    }
  }
  if (not defined $use or $use==0) {
    if (not defined $full) { $full = $dir.'/'.$file; }
    foreach (@forced_when_matchesFull) {
      if ($full =~ $_) { $use = 1; }
    }
  }

  if (not defined $use) {
    die "Don't know whether to use or skip '$file' (in $dir)";
  }

  return $use;
}

# ------------------------------------------------------------

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
  }

  # foreach (sort keys %$CVS_r) { print "CVS-entry: '$_'\n"; }
}

# ------------------------------------------------------------

sub expectCVSmember($$\%) {
  my ($full,$item,$CVS_r) = @_;
  if (not exists $$CVS_r{$item}) {
    die "'$full' ($_) included, but not in CVS (seems to be generated)";
  }
}

my %unpackedCVSmember = map { $_ => 1; } (
                                          '.cvsignore',
                                          'ChangeLog',
                                         );

sub unexpectCVSmember($$\%) {
  my ($full,$item,$CVS_r) = @_;
  if (exists $$CVS_r{$item}) {
    if (not exists $unpackedCVSmember{$item}) {
      die "'$full' excluded, but in CVS";
    }
  }
}

sub dumpFiles($);
sub dumpFiles($) {
  my ($dir) = @_;

  my @subdirs;
  my @files;

  my %CVS;
  getCVSEntries($dir,%CVS);

  opendir(DIR,$dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $dir.'/'.$_;
      if (not -l $full) {
        if (-d $full) {
          if (useDir($full)==1) {
            expectCVSmember($full,$_,%CVS);
            push @subdirs, $full;
          }
          else { unexpectCVSmember($full,$_,%CVS); }
        }
        elsif (-f $full) {
          if (useFile($dir,$_)==1) {
            expectCVSmember($full,$_,%CVS);
            push @files, $full;
          }
          else { unexpectCVSmember($full,$_,%CVS); }
        }
        else { die "Unknown: '$full'"; }
      }
    }
  }
  closedir(DIR);

  foreach (@files) { print $_."\n"; }
  foreach (@subdirs) { dumpFiles($_); }
}

dumpFiles('.');
