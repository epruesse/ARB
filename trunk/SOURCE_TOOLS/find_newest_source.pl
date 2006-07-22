#!/usr/bin/perl

use strict;
use warnings;
use diagnostics;

# --------------------------------------------------------------------------------

# skip files with the following extensions:
my @boring_extensions = (
                         'o', 'a', 'so',
                         'gz', 'tgz',
                         'class', 'jar',
                         'elc',
                         'lnk',
                        );

# skip files with the following names:
my @boring_files = (
                    '.cvsignore',
                    'TAGS',
                   );

my $max_print = 1000; # max lines to print

# --------------------------------------------------------------------------------

my @files = ();
my %boring_extensions = map { $_ => 1; } @boring_extensions;
my %boring_files = map { $_ => 1;      } @boring_files;

sub scan_tree_recursive($);
sub scan_tree_recursive($) {
  my ($dir) = @_;
  # print "scan_tree_recursive '$dir'\n";
  opendir(DIR,$dir) || die "can't read directory '$dir'";
  my @subdirs = ();
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') { # ignore curr- and up-dir
      my $fullname = $dir.'/'.$_;
      if (not -l $fullname) { # ignore links
        if (-d $fullname) {
          if (not ( /^CVS$/ )) {
            push @subdirs, $fullname;
          }
        }
        else {
          my $skip = 0;
          if (exists $boring_files{$_}) {
            $skip = 1;
          }
          elsif (/\.([^\.]+)$/ and exists $boring_extensions{$1}) {
            $skip = 1;
          }
          else {
            if (/^\#.*\#$/ or # emacs autosave
                /^\.\#.*\.[0-9]+\.[0-9]+$/ # old cvs revisions
               )
              {
                $skip = 1;
              }
          }

          if ($skip==0) { push @files, $fullname; }
        }
      }
    }
  }
  closedir(DIR);
  foreach (@subdirs) {
    scan_tree_recursive($_);
  }
}

my $equality = '================';
my $reg_parse_part = qr/^([ :]*)([^ :]+)/;

sub diff2last_rec($$);
sub diff2last_rec($$) {
  my ($this,$last) = @_;

  if (not $this =~ $reg_parse_part) {
    if ($this =~ /^[ ]*$/) { return $this; }
    die "can't parse '$this'";
  }
  my ($this_space, $this_part, $this_rest) = ($1,$2,$');
  if (not $last =~ $reg_parse_part) { die "can't parse '$this'"; }
  my ($last_space, $last_part, $last_rest) = ($1,$2,$');

  my $part;
  if ($this_part eq $last_part) {
    $part = substr($equality, 1, length($this_part));
  }
  else {
    # print "'$this_part' ne '$last_part'\n";
    $part = $this_part;
  }

  return $this_space.$part.diff2last_rec($this_rest, $last_rest);
}

my $lastMod = undef;
sub diff2last($) {
  my ($mod) = @_;
  if (defined $lastMod) {
    my $newmod = diff2last_rec($mod, $lastMod);
    $lastMod = $mod;
    $mod = $newmod;
  }
  else {
    $lastMod = $mod;
  }
  return $mod;
}

sub max($$) {
  my ($a,$b) = @_;
  return $a if $a>$b;
  return $b;
}

sub readable_age($) {
  my ($modtime) = @_;
  my $age = time - $modtime;
  if ($age<60) { return $age.'s'; }
  $age = int($age/60); if ($age<60) { return $age.'m'; }
  $age = int($age/60); if ($age<24) { return $age.'h'; }
  $age = int($age/24); if ($age<30) { return $age.'d'; }
  my $months = int($age/30); if ($months<12) { return $months.'M'; }
  my $years = int($age/365);
  return $years.'Y';
}

sub perform_search() {
  my $root = `pwd`;
  chomp($root);
  $root =~ s/[\/\\]+$//g;

  scan_tree_recursive($root);
  my %filedate = ();

  my $del = length($root)+1;
  @files = map { substr($_,$del); } @files;

  foreach (@files) {
    my $modtime = (stat($_))[9];
    if (not defined $modtime) { die "Can't stat file '$_'"; }
    $filedate{$_} = $modtime;
    # print scalar(localtime($modtime))." $_\n";
  }

  my @sorted = sort {
    $filedate{$b} <=> $filedate{$a};
  } @files;

  if (scalar(@sorted)<$max_print) { $max_print=scalar(@sorted); }

  my $maxlen = 0;
  foreach (my $i=0; $i<$max_print; ++$i) {
    my $len = length($sorted[$i]);
    if ($len > $maxlen) { $maxlen = $len; }
  }

  my $spacer = "          ";
  while (length($spacer) < $maxlen) { $spacer .= $spacer; }

  print "find_newest_source: Entering directory `$root'\n";
  foreach (my $i=0; $i<$max_print; ++$i) {
    $_ = $sorted[$i];
    my $len = length($_);
    print "$_:1: ".substr($spacer,0,max($maxlen-$len,0)).
      "mod: ".diff2last(scalar(localtime($filedate{$_}))).
        "  [ ".sprintf("%3s",readable_age($filedate{$_}))." ]\n";
  }
  print "find_newest_source: Leaving directory `$root'\n";
}

perform_search();
