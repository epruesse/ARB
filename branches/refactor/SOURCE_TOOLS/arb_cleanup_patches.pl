#!/usr/bin/perl

use strict;

# --------------------------------------------------------------------------------

sub getModtime($) {
  my ($file_or_dir) = @_;
  if (-f $file_or_dir or -d $file_or_dir) {
    my @st = stat($file_or_dir);
    if (not @st) { die "can't stat '$file_or_dir' ($!)"; }
    return $st[9];
  }
  return 0; # does not exist -> use epoch
}
sub getAge($) { my ($file_or_dir) = @_; return time-getModtime($file_or_dir); }

# --------------------------------------------------------------------------------

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=3) {
    print "Usage: arb_cleanup_patches.sh name hours minkeep\n";
    print "       deletes all patches matching 'name_*.patch' if\n";
    print "       - they are older than 'hours' and\n";
    print "       - at least 'minkeep' patches remain present.\n";
    die "missing arguments";
  }

  my $mask           = $ARGV[0].'.*\.patch';
  my $olderThanHours = $ARGV[1];
  my $minKeep        = $ARGV[2];

  my $ARBHOME = $ENV{ARBHOME};
  if (not defined $ARBHOME) { die "environment variable 'ARBHOME' is not defined"; }

  my $patchDir = $ARBHOME.'/patches.arb';
  if (not -d $patchDir) { die "directory '$patchDir' does not exist"; }

  my $reg = qr/$mask/;

  my @patch = ();
  opendir(DIR,$patchDir) || die "can't read directory '$patchDir' (Reason: $!)";
  foreach (readdir(DIR)) { if ($_ =~ $reg) { push @patch, $_; } }
  closedir(DIR);

  my %age = map { $_ => getAge($patchDir.'/'.$_); } @patch;

  @patch = sort { $age{$a} <=> $age{$b}; } @patch; # sort newest first

  my $olderThanSeconds = $olderThanHours*60*60;
  my $patchCount = scalar(@patch);

  my %keep = map { $_ => 1; } @patch;

  for (my $i=$minKeep; $i<$patchCount; $i++) {
    my $patch = $patch[$i];
    if ($age{$patch}>$olderThanSeconds) { $keep{$patch} = 0; }
  }

  foreach (@patch) {
    if ($keep{$_}==0) {
      my $fullPatch = $patchDir.'/'.$_;
      print "Unlinking old patch '$_'\n";
      unlink($fullPatch) || die "Failed to unlink '$fullPatch' (Reason: $!)";
    }
  }
}

eval { main(); };
if ($@) { die "arb_cleanup_patches.pl: Error: $@\n"; }
