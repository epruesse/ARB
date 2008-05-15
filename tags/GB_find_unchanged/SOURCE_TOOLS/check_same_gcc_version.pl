#!/usr/bin/perl

use strict;
use warnings;

my $extension = 'last_gcc';

my $args = @ARGV;
if ($args==0) { die "Usage: check_same_gcc_version.pl version\n"; }

my $version = $ARGV[0];
my $arbhome = $ENV{'ARBHOME'};
(-d $arbhome) || die "\$arbhome has to contain the name of a directory.\n";

opendir(ARBHOME,$arbhome) || die "can't read directory '$arbhome' (Reason: $!)";
my @found_version_files = ();
foreach (readdir(ARBHOME)) {
    if (/\.$extension$/ig) { push @found_version_files, $_; }
}
closedir(ARBHOME);

my $result = 0;
my $found  = scalar(@found_version_files);

if ($found == 0) { # first compilation -> create file
    my $flagfile = $arbhome.'/'.$version.'.'.$extension;
    open(FLAG,">$flagfile") || die "can't create '$flagfile' (Reason: $!)";
    print FLAG "- The last compilation was done using gcc '$version'.\n";
    close(FLAG);
}
elsif ($found != 1) {
    die "Multiple gcc version files were found -- 'make rebuild' is your friend";
}
else {
    my $lastVersion = $found_version_files[0];
    if ($lastVersion ne "$version.$extension") {
        my $command = "cat $lastVersion";
        system("$command") ==0 || die "can't execute '$command' (Reason: $!)";
        print "- Your current gcc version is '$version'.\n";
        print "Use 'make rebuild' to recompile with a different gcc version\n";
        $result = 1;
    }
}

exit($result);
