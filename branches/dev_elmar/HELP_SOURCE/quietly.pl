#!/usr/bin/perl

use strict;
use warnings;

sub die_usage($) {
  my ($err) = @_;
  die ("Usage: ./quietly.pl XMLLINT xmlfile\n".
       "       Validates an XML file.\n".
       "       Error messages are converted into readable format\n".
       "       and superfluous output is suppressed\n".
       "Error: $err");
}

my $basedir = $ENV{ARBHOME}; if (not -d $basedir) { die "no such directory '$basedir'"; }
$basedir .= '/HELP_SOURCE';  if (not -d $basedir) { die "no such directory '$basedir'"; }

my $args = scalar(@ARGV);
if ($args != 2) { die_usage "Expected 2 arguments"; }

my $tool    = $ARGV[0];
my $xmlfile = $ARGV[1];

if ($tool ne 'XMLLINT') { die_usage "Expected 'XMLLINT' as 1st arg (not '$tool')"; }

my $tool_command = "xmllint --valid --noout $xmlfile 2>&1 >/dev/null";

my $tool_out = `$tool_command`;
if ($? == -1) { die "Failed to execute '$tool_command'" }
elsif ($? & 127) { die sprintf("Executed command '$tool_command'\ndied with signal %d", ($? & 127)); }
else {
  my $tool_exitcode = ($? >> 8);
  if ($tool_exitcode!=0) {
    my $sep = '------------------------------------------------------------';
    # print "$sep\n";
    # print "Error executing '$tool_command' (exitcode=$tool_exitcode):\n";

    # print "$sep plain:\n";
    # print $tool_out;
    # print "$sep end of plain\n";

    use Cwd;
    my $cwd         = getcwd();
    my $cwdlen      = length($cwd);
    my $file_prefix = 'file://'.$cwd.'/';

    # print "file_prefix='$file_prefix'\n";

    my @output = ();
    while ($tool_out =~ /\n/g) {
      $tool_out = $';
      push @output, $`;
    }
    if (length $tool_out) { push @output, $tool_out; }

    my $last_line = undef;
    foreach (@output) {
      if (/^([^:\s]+):([0-9]+):\s*(.*)$/o) { print "$basedir/$1:$2: $3\n"; }
      else { print "Add.info: '$_'\n"; }
    }
    die "$sep $tool failed!\n";
  }
}


