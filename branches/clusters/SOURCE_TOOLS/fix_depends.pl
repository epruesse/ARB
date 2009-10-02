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

my $arbhome = qr/$ENV{ARBHOME}/;
my $makedependlineseen = 0;
my @depends;

if (0) {
  foreach (<>) {
    print "ALL: $_";
  }

  die "done";
}

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

# read input stream
foreach (<>) {
  if ($makedependlineseen==0) { # simply forward lines before 'DO NOT DELETE'
    print "$_";
    if (/^\# DO NOT DELETE/) { $makedependlineseen = 1; }
  }
  else { # put lines behind into '@depends'
    chomp;
    if (/^ *[\/\$a-z]/i) {
      if (/^([^:]*): *(.*)$/) {
        my $file       = $1;
        my $depends_on = $2;
        $file = fix_name($file);

        while ($depends_on =~ / /) { # split lines with multiple dependencies
          my $name = $`;
          my $rest = $';
          $name = fix_name($name);
          push @depends, "$file: $name";
          $depends_on = $rest;
        }
        $depends_on = fix_name($depends_on);
        $_ = "$file: $depends_on";
      }
      push @depends,$_;
    }
  }
}

print "\n# Do not add dependencies manually - use 'make depend' in \$ARBHOME\n";
print "# For formatting issues see SOURCE_TOOLS/fix_depends.pl\n";

# sort dependency lines

sub beautiful($$) {
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

@depends = sort beautiful @depends;

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
