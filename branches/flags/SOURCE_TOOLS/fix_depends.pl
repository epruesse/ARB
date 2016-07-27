#!/usr/bin/perl -w
# 
# This script parses and fixes dependency lines in Makefiles:
# 1. Searches for a line containing '# DO NOT DELETE'
# 2. Lines after that point are modified like follows:
#    a. hardcoded directory path to $ARBHOME (environment variable)
#       is replaced by '$(ARBHOME)'
#    b. split lines with multiple dependencies
#    c. sort lines
#
# Goal of this script is to unify the result of 'make depends'
# to avoid CVS/SVN changes caused by formatting.

use strict;
use warnings;

my $arbhome_quoted = quotemeta($ENV{ARBHOME});
my $arbhome = qr/$arbhome_quoted/;

sub fix_name($) {
  my ($name) = @_;
  $name =~ s/^$arbhome/\$\(ARBHOME\)/ig; # translate $ARBHOME
  $name =~ s/^.\///ig; # remove './' at start

  # ensure there's a / behind '$(ARBHOME)'
  if ($name =~ /\$\(ARBHOME\)[^\/]/) {
    $name =~ s/\$\(ARBHOME\)/\$\(ARBHOME\)\//ig;
  }

  $name;
}

sub arb_dependency_order($$) {
  # sorts files alphabetically (ign. case)
  # sorts local dependencies first (for each file)
  my ($a,$b) = @_;
  my ($ap,$bp) = ('','');
  ($a,$b) = (lc($a),lc($b));

  if ($a =~ /^[^:]*:/) { $ap = $&; }
  if ($b =~ /^[^:]*:/) { $bp = $&; }

  my $res = $ap cmp $bp;

  if ($res == 0) {
    if ($a =~ /\$/) {
      if ($b =~ /\$/) { $a cmp $b; }
      else { 1; }
    }
    else {
      if ($b =~ /\$/) { -1; }
      else { $a cmp $b; }
    }
  }
  else {
    $res;
  }
}

sub add_dependency(\@$$) {
  my ($depends_r,$file,$depends_on) = @_;
  $depends_on = fix_name($depends_on);

  # skip outside dependencies
  if (not $depends_on =~ m@/usr/(include|lib)/@) {
    push @$depends_r, "$file: $depends_on";
  }
}

sub read_input_stream(\@) {
  my ($depends_r) = @_;
  my $makedependlineseen = 0;

  foreach (<STDIN>) {
    if ($makedependlineseen==0) { # simply forward lines until 'DO NOT DELETE'
      print "$_";
      if (/^\# DO NOT DELETE/) { $makedependlineseen = 1; }
    }
    else { # put lines behind into '@depends'
      chomp;
      if (/^ *[\/\$a-z]/i) { # skip empty lines
        if (/^([^:]*): *(.*)$/) {
          my $file       = $1;
          my $depends_on = $2;
          $file = fix_name($file);

          while ($depends_on =~ / /) { # split lines with multiple dependencies
            my $name = $`;
            my $rest = $';

            add_dependency(@$depends_r,$file,$name);
            $depends_on = $rest;
          }
          add_dependency(@$depends_r,$file,$depends_on);
        }
        else {
          push @$depends_r,$_; # what lines go here?
        }
      }
    }
  }
}

sub main() {
  my $comment = join(' ',@ARGV);
  if ($comment ne '') { $comment = ' '.$comment }
  @ARGV = ();

  my @depends=();
  read_input_stream(@depends);

  @depends = sort arb_dependency_order @depends;

  print "\n# Do not add dependencies manually - use 'make depend' in \$ARBHOME\n";
  print "# For formatting issues see SOURCE_TOOLS/fix_depends.pl$comment\n";

  # print dependency lines
  my $prefix = '';
  foreach (@depends) {
    my $tprefix = '';
    if (/^([^:]*):/) { $tprefix = $1; }
    if ($tprefix ne $prefix) {
      print "\n"; # empty line between different files
      $prefix = $tprefix;
    }
    print "$_\n";
  }
}
main();
