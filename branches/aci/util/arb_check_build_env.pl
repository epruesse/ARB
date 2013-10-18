#!/usr/bin/perl
#
# This script checks whether all tools needed for ARB compilation are found in path.

use strict;
use warnings;

my @commands = (
                'bash',
                'cat',
                'chmod',
                'cp',
                'find',
                'grep',
                'gzip',
                'ln',
                'ls',
                'lynx',
                'mkdir',
                'mv',
                'perl',
                'rm',
                'sed',
                'sort',
                'tar',
                'test',
                'time',
                'touch',
                'uniq',
                'xmllint',
                'xsltproc',
               );

sub findPath($\@) {
  my ($command,$path_dirs_r) = @_;
  my ($found,$executable) = (undef,undef);

  if (-e $command) {
    $found = $command;
    if (-X $command) { $executable = $command; }
  }

  if (not defined $executable) {
    foreach (@$path_dirs_r) {
      my $full = $_.'/'.$command;
      if (-e $full) {
        $found = $full;
        if (-X $full) { $executable = $full; }
      }
    }
  }
  return ($found,$executable);
}

sub main() {
  foreach (@ARGV) {
    s/ .*$//og; # skip anything from first space to EOL
    push @commands, $_;
  }
  {
    my %commands = map { $_ => 1; } @commands;
    @commands = sort keys %commands;
  }

  my $path = $ENV{PATH};
  if (not defined $path) { die "Environmentvariable 'PATH' is undefined"; }
  my @path_dirs = split(':',$path);

  my @missing       = ();
  my @notExecutable = ();

  foreach (@commands) {
    my ($found,$executable) = findPath($_,@path_dirs);
    if (not defined $executable) { # not ok
      if (defined $found) { push @notExecutable, $_; }
      else { push @missing, $_; }
    }
  }

  my $missing = scalar(@missing);
  my $notExecutable = scalar(@notExecutable);

  if (($missing+$notExecutable)==0) {
    print "All tools needed for ARB compilation have been located.\n";
  }
  else {
    my $missingList       = join(', ', @missing);
    my $notExecutableList = join(', ', @notExecutable);

    my $msg = "The following tools are missing:\n";
    if ($missing>0)       { $msg .= "    $missingList\n"; }
    if ($notExecutable>0) { $msg .= "    $notExecutableList (found but not executable)\n"; }
    $msg .= "These tools are vital to compile ARB, so\n";
    $msg .= "please ensure that these tools are installed and that either\n";
    $msg .= "- their installation directory is in PATH or\n";
    $msg .= "- they are linked into the PATH\n\n";

    die $msg;
  }
}

main();

