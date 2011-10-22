#!/usr/bin/perl

use strict;
use warnings;

my $args = @ARGV;
if ($args==0) { die "Usage: binuptodate.pl target [sources]+\n"; }
my $target = $ARGV[0];
my $arbhome = $ENV{'ARBHOME'};
(-d $arbhome) || die "\$arbhome has to contain the name of a directory.\n";
my $target_date = -1;

sub filedate($) {
  my $filename = $_[0];
  my @stat_res = stat($filename);
  return($stat_res[9]); # the files modification date
}

sub ok_if_older($) {
  my $filename = $_[0];

}

sub first_existing(@) {
  my $idx = 0;
  my $count = @_;
  while ($idx<$count) {
    if (-f $_[$idx]) { return($_[$idx]); }
    $idx++;
  }
  return undef;
}

sub uptodate() {
  print STDERR "-------------------- Checking $target\n";
  (-f $target) || die "Target '$target' not found\n";

  my $source_idx = 1;
  $target_date = filedate($target);

  while ($source_idx<$args) {
    my $source = $ARGV[$source_idx];

    if ($source =~ /^\-l/) { # handle libs specified with -l
      my $libbase = $';
      my $fulllib = first_existing((
                                    $arbhome.'/LIBLINK/lib'.$libbase.'.a',
                                    $arbhome.'/LIBLINK/lib'.$libbase.'.so',
                                   ));

      if (not defined $fulllib) {
        # print STDERR "can't detect where '$source' resides -- ignoring this dependency\n";
        $source = undef;
      }
      else {
        # print STDERR "fulllib='$fulllib'\n";
        $source=$fulllib;
      }
    }
    elsif ($source =~ /^\-L/) {
      $source = undef;
    }

    if (defined $source) {
      (-f $source) || die "Source missing: $source\n";
      my $source_date = filedate($source);
      ($source_date <= $target_date) || die "Changed source: $source (".localtime($source_date).")\n";
    }
    $source_idx++;
  }

}

eval { uptodate(); };
if ($@) {
  print STDERR "$@-> rebuilding $target (".localtime($target_date).")\n";
  exit(1);
}

print STDERR "$target is up-to-date.\n";
exit(0);

