#!/usr/bin/perl

use strict;
use warnings;

my $old_extension = 'last_gcc';
my $extension     = 'last_compiler';

my $args = scalar(@ARGV);
if ($args!=2) { die "Usage: check_same_compiler_version.pl name version\n"; }

my $name    = $ARGV[0];
my $version = $ARGV[1];

my $arbhome = $ENV{'ARBHOME'};
(-d $arbhome) || die "\$arbhome has to contain the name of a directory.\n";

opendir(ARBHOME,$arbhome) || die "can't read directory '$arbhome' (Reason: $!)";
my @found_version_files = ();
foreach (readdir(ARBHOME)) {
  if (/\.$extension$/ig) { push @found_version_files, $_; }
  if (/\.$old_extension$/ig) { push @found_version_files, $_; }
}
closedir(ARBHOME);

my $result = 0;
my $found  = scalar(@found_version_files);

my $currVersion     = $version.'.'.$name.'.'.$extension;
my $obsoleteVersion = $version.'.'.$old_extension;

if ($found == 0) { # first compilation -> create file
  my $flagfile = $arbhome.'/'.$currVersion;
  open(FLAG,">$flagfile") || die "can't create '$flagfile' (Reason: $!)";
  print FLAG "- The last compilation was done using '$name $version'.\n";
  close(FLAG);
}
elsif ($found != 1) {
  die "Multiple compiler version files were found -- 'make rebuild' is your friend";
}
else {
  my $lastVersion = $found_version_files[0];
  if ($lastVersion eq $obsoleteVersion) {
    print "[ renaming $obsoleteVersion -> $currVersion ]\n";
    my $command = "mv $obsoleteVersion $currVersion";
    system("$command") ==0 || die "can't execute '$command' (Reason: $?)";
  }
  elsif ($lastVersion ne $currVersion) {
    my $command = "cat $lastVersion";
    system("$command") ==0 || die "can't execute '$command' (Reason: $?)";
    print "- Your current compiler version is '$name $version'.\n";
    print "Use 'make rebuild' to recompile with a different compiler version or\n";
    print "use 'make clean' to cleanup build and then compile again.\n";
    $result = 1;
  }
}

exit($result);
