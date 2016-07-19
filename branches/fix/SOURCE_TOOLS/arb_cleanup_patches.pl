#!/usr/bin/perl

use strict;

# --------------------------------------------------------------------------------

sub getModtimeAndSize($) {
  my ($file_or_dir) = @_;
  if (-f $file_or_dir or -d $file_or_dir) {
    my @st = stat($file_or_dir);
    if (not @st) { die "can't stat '$file_or_dir' ($!)"; }
    return ($st[9],$st[7]); # (stamp,size)
  }
  return (0,0); # does not exist -> use (epoch,zerosize)
}

sub getAgeAndSize($) {
  my ($file_or_dir) = @_;
  my ($mod,$size) = getModtimeAndSize($file_or_dir);
  return (time-$mod,$size);
}

# --------------------------------------------------------------------------------

sub scanPatches($$\@) {
  my ($patchDir,$mask,$patch_r) = @_;
  my $reg = qr/$mask/;
  opendir(DIR,$patchDir) || die "can't read directory '$patchDir' (Reason: $!)";
  foreach (readdir(DIR)) { if ($_ =~ $reg) { push @$patch_r, $_; } }
  closedir(DIR);
}

sub patchesDiffer($$$) {
  my ($patchDir,$patch,$otherpatch) = @_;
  my $diff = `diff $patchDir/$patch $patchDir/$otherpatch | wc -l`;
  return ($diff>0);
}

sub getOldDuplicates($\@\%\%) {
  my ($patchDir,$patch_r,$age_r,$size_r) = @_;
  my @oldDups = ();

  my %size2patch = ();
  foreach my $patch (@$patch_r) {
    my $size = $$size_r{$patch};
    my $otherpatch = $size2patch{$size};
    if (defined $otherpatch) { # got another patch with same size
      my $diff = patchesDiffer($patchDir,$patch,$otherpatch);

      if ($diff) {
      }
      else { # no diff -> removed older
        if ($$age_r{$otherpatch} > $$age_r{$patch}) {
          push @oldDups, $otherpatch;
          $size2patch{$size} = $patch;
        }
        else {
          push @oldDups, $patch;
        }
      }
    }
    else {
      $size2patch{$size} = $patch;
    }
  }
  return @oldDups;
}

sub readableSize($) {
  my ($size) = @_;
  if ($size<1024) { $size.'b'; }
  else {
    $size = int($size/1024);
    if ($size<1024) { $size.'k'; }
    else {
      $size = int($size/1024);
      if ($size<1024) { $size.'M'; }
      else {
        $size = int($size/1024);
        $size.'G';
      }
    }
  }
}

sub countAndSize($$$) {
  my ($name,$count,$size) = @_;
  return $count.' '.$name.' patches ('.readableSize($size).')  ';
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=3) {
    print "Usage: arb_cleanup_patches.sh name hours minkeep\n";
    print "       deletes all patches matching 'name_*.patch' if\n";
    print "       - the date-stamp created by arb_create_patch.sh is at end of name (ie. patch was not renamed manually),\n";
    print "       - they are older than 'hours' and\n";
    print "       - at least 'minkeep' patches remain present.\n";
    die "missing arguments";
  }

  my $mask           = $ARGV[0].'.*_[0-9]{8}_[0-9]{6}\.patch';
  my $olderThanHours = $ARGV[1];
  my $minKeep        = $ARGV[2];

  my $ARBHOME = $ENV{ARBHOME};
  if (not defined $ARBHOME) { die "environment variable 'ARBHOME' is not defined"; }

  my $patchDir = $ARBHOME.'/patches.arb';
  if (not -d $patchDir) { die "directory '$patchDir' does not exist"; }


  my @patch = ();
  scanPatches($patchDir,$mask,@patch);

  print "Matching patches=".scalar(@patch)."\n";

  my %age = ();
  my %size = ();
  foreach (@patch) { ($age{$_},$size{$_}) = getAgeAndSize($patchDir.'/'.$_); }

  my %unlink_patch = (); # key=patchname, value=why unlink
  {
    my @oldDups = getOldDuplicates($patchDir,@patch,%age,%size);
    foreach (@oldDups) { $unlink_patch{$_} = 'duplicate'; }
  }

  @patch = sort { $age{$a} <=> $age{$b}; } @patch;

  my $olderThanSeconds = $olderThanHours*60*60;
  my $patchCount = scalar(@patch);

  for (my $i=$minKeep; $i<$patchCount; $i++) {
    my $patch = $patch[$i];
    if ($age{$patch}>$olderThanSeconds) { $unlink_patch{$patch} = 'old'; }
  }

  my $all_size      = 0;
  my $unlinked_size = 0;

  foreach (@patch) { $all_size += $size{$_}; }

  foreach (sort keys %unlink_patch) {
    my $fullPatch = $patchDir.'/'.$_;
    unlink($fullPatch) || die "Failed to unlink '$fullPatch' (Reason: $!)";
    print "- unlinked ".$unlink_patch{$_}." '$_'\n";
    $unlinked_size += $size{$_};
  }

  my $summary = '';
  {
    my $unlinked_patches = scalar(keys %unlink_patch);
    my $all_patches      = scalar(@patch);

    if ($unlinked_patches>0) {
      my $left_patches = $all_patches - $unlinked_patches;
      my $left_size    = $all_size - $unlinked_size;

      $summary .= countAndSize('removed',$unlinked_patches,$unlinked_size);
      $summary .= countAndSize('kept',$left_patches,$left_size);
    }
    else {
      $summary .= countAndSize('existing',$all_patches,$all_size);
    }
  }
  print $summary."\n";
}

eval { main(); };
if ($@) { die "arb_cleanup_patches.pl: Error: $@\n"; }
