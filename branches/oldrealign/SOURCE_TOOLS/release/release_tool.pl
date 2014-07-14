#!/usr/bin/perl

use strict;
use warnings;

my $ARBHOME = $ENV{ARBHOME};
die "ARBHOME has to be defined" if not defined $ARBHOME;
die "ARBHOME has specify a directory (ARBHOME='$ARBHOME')" if not -d $ARBHOME;

my %svn_info = ();

sub retrieve_svn_info() {
  %svn_info = ();
  my $cmd = "(cd '$ARBHOME'; svn info)";
  open(INFO,$cmd.'|') || die "failed to fork '$cmd' (Reason: $!)";
  foreach (<INFO>) {
    chomp;
    if (/^Repository\sRoot:\s+/o) { $svn_info{ROOT} = $'; }
    elsif (/^Revision:\s+/o) { $svn_info{REVISION} = $'; }
    elsif (/^URL:\s+/o) { $svn_info{URL} = $'; }
    # else { print "info='$_'\n"; }
  }
  close(INFO) || die "failed to execute '$cmd' (Reason: $!)";

  if (not defined $svn_info{ROOT}) { die "Failed to detect SVN root"; }

  {
    my $rootlen = length($svn_info{ROOT});
    my $prefix = substr($svn_info{URL},0,$rootlen);
    my $suffix = substr($svn_info{URL},$rootlen+1);
    if ($prefix ne $svn_info{ROOT}) {
      die "prefix!=ROOT ('$prefix' != '".$svn_info{ROOT}."')";
    }
    $svn_info{SUB} = $suffix;
  }

  print "-------------------- [WC info]\n";
  foreach (sort keys %svn_info) { print "$_='".$svn_info{$_}."'\n"; }
  print "--------------------\n";
}

sub getArbVersion() {
  my ($tag,$version) = (undef,undef);
  eval {
    my $arb_build    = $ARBHOME.'/TEMPLATES/arb_build.h';
    my $version_info = $ARBHOME.'/SOURCE_TOOLS/version_info';

    die "missing expected file '$arb_build'"    if not -f $arb_build;
    die "missing expected file '$version_info'" if not -f $version_info;

    open(BUILD,'<'.$arb_build) || die "can't read '$arb_build' (Reason: $!)";
    foreach (<BUILD>) {
      if (/define\s+ARB_VERSION\s+"(.*)"/o) { $tag = $1; }
    }
    close(BUILD);

    {
      my ($minor,$major) = (undef,undef);
      open(VERSION,'<'.$version_info) || die "can't read '$version_info' (Reason: $!)";
      foreach (<VERSION>) {
        if (/^MINOR=([0-9]+)$/o) { $minor = $1; }
        if (/^MAJOR=([0-9]+)$/o) { $major = $1; }
      }
      close(VERSION);
      if (not defined $minor) { die "Failed to retrieve MINOR from $version_info"; }
      if (not defined $major) { die "Failed to retrieve MAJOR from $version_info"; }
      $version = "$major.$minor";
    }

    if (not defined $tag) { die "Failed to retrieve ARB_VERSION from $arb_build"; }
    defined $version || die;
  };
  if ($@) {
    die "Note: maybe you forgot to 'make all'?\n".
      "Error while retrieving ARB version: $@";
  }
  return ($tag,$version);
}

sub getExisting($) {
  my ($baseUrl) = @_;

  my @existing = ();
  my $cmd = "svn list '$baseUrl'";
  open(LIST,$cmd.'|') || die "failed to fork '$cmd' (Reason: $!)";
  foreach (<LIST>) {
    chomp;
    if (/\/$/o) { push @existing, $`; }
    else { die "Unexpected content '$_' (received from '$cmd')"; }
  }
  close(LIST) || die "failed to execute '$cmd' (Reason: $!)";
  return @existing;
}

my %known_branches = ();
my %known_tags = ();

sub branch_exists($) {
  my ($branch) = @_;
  if (not %known_branches) {
    %known_branches = map { $_ => 1; } getExisting($svn_info{ROOT}.'/branches');
  }
  return exists $known_branches{$branch};
}
sub tag_exists($) {
  my ($tag) = @_;
  if (not %known_tags) {
    %known_tags = map { $_ => 1; } getExisting($svn_info{ROOT}.'/tags');
  }
  return exists $known_tags{$tag};
}

sub trunkURL()   { return $svn_info{ROOT}.'/trunk'; }
sub currentURL() { return $svn_info{ROOT}.'/'.$svn_info{SUB}; }
sub branchURL($) { my ($branch) = @_; return $svn_info{ROOT}.'/branches/'.$branch; }
sub tagURL($)    { my ($tag)    = @_; return $svn_info{ROOT}.'/tags/'.$tag; }

sub expectSUB($) {
  my ($expected) = @_;
  my $got = $svn_info{SUB};
  defined $got || die "SUB undefined";

  if ($got ne $expected) {
    die "Error: this is only possible in '$expected' (you are in '$got')";
  }
}

sub denySUB($) {
  my ($expected) = @_;
  my $got = $svn_info{SUB};
  defined $got || die "SUB undefined";

  if ($got eq $expected) {
    die "Error: this is NOT possible in '$expected'";
  }
}


sub expectTrunk()   { expectSUB('trunk'); }
sub expectBranch($) { my ($branch) = @_; expectSUB('branches/'.$branch); }
sub denyBranch($)   { my ($branch) = @_; denySUB  ('branches/'.$branch); }

sub tag_remove_command($$) {
  my ($tag,$action) = @_;
  return "svn delete '".tagURL($tag)."' -m \"[$action] delete tag '$tag'\"";
}
sub branch_remove_command($$) {
  my ($branch,$action) = @_;
  return "svn delete '".branchURL($branch)."' -m \"[$action] delete branch '$branch'\"";
}
sub die_due_to_tag($$) {
  my ($tag,$desc) = @_;
  my $remove_cmd = tag_remove_command($tag,$desc);
  die "tag '$tag' already exists.\nTo remove that tag use\n$remove_cmd\n ";
}

sub get_branches() { branch_exists('xxx'); return sort keys %known_branches; }
sub get_tags() { tag_exists('xxx'); return sort keys %known_tags; }

sub perform($$) {
  my ($action,$arg) = @_;
  retrieve_svn_info();

  my @commands = ();

  my ($tag,$version) = getArbVersion();

  if ($action eq 'branch_rc1') {
    expectTrunk();
    push @commands, "# check version and changelog in trunk are set correctly; see SOURCE_TOOLS/release/HOWTO.release";
    if (branch_exists('rc')) {
      push @commands, branch_remove_command('rc', $action);
    }
    push @commands, "svn copy '".trunkURL().'@'.$svn_info{REVISION}."' '".branchURL('rc')."' -m \"[$action] create rc1 for arb $version\"";
    push @commands, "# increment version in trunk; see SOURCE_TOOLS/release/HOWTO.release";
    push @commands, "# let jenkins build job 'ARB-rc' (check if job is enabled)";
    push @commands, "svn switch '".branchURL('rc')."'";
    push @commands, "make show_version";
    push @commands, "SOURCE_TOOLS/release/release_tool.pl tag_rc";
  }
  elsif ($action eq 'branch_stable') {
    expectBranch('rc');
    if (branch_exists('stable')) {
      push @commands, branch_remove_command('stable', $action);
    }
    push @commands, "svn copy '".branchURL('rc').'@'.$svn_info{REVISION}."' '".branchURL('stable')."' -m \"[$action] arb $version\"";
    push @commands, "# let jenkins build job 'ARB-stable'";
    push @commands, "svn switch '".branchURL('stable')."'";
    push @commands, "make show_version";
    push @commands, "SOURCE_TOOLS/release/release_tool.pl tag_stable";
  }
  elsif ($action eq 'tag_rc') {
    expectBranch('rc');
    if (($tag =~ /devel/oi) or ($tag =~ /rev/oi) or (not $tag =~ /^arb-/o)) { die "Invalid tag '$tag'"; }
    if (tag_exists($tag)) { die_due_to_tag($tag, 'invalid rc'); }
    push @commands, "svn copy '".branchURL('rc').'@'.$svn_info{REVISION}."' '".tagURL($tag)."' -m \"[$action] '$tag'\"";
  }
  elsif ($action eq 'tag_stable') {
    expectBranch('stable');
    if (($tag =~ /devel/oi) or ($tag =~ /rev/oi) or (not $tag =~ /^arb-/o)) { die "Invalid tag '$tag'"; }
    if (tag_exists($tag)) { die_due_to_tag($tag, 'invalid release'); }
    push @commands, "svn copy '".branchURL('stable').'@'.$svn_info{REVISION}."' '".tagURL($tag)."' -m \"[$action] release '$tag'\"";
  }
  elsif ($action eq 'tag_custom') {
    if (not defined $arg) {
      die "Expected additional argument 'tag'";
    }

    denyBranch('rc');
    denyBranch('stable');
    $tag = $arg; # use given arg as tagname

    if (($tag =~ /dev/oi) or ($tag =~ /rev/oi)) { die "Invalid tag '$tag'"; }
    if (tag_exists($tag)) {
      my $remove_cmd = "svn delete '".tagURL($tag)."' -m \"[$action] delete invalid tag '$tag'\"";
      die "tag '$tag' already exists.\nTo remove that tag use\n$remove_cmd\n ";
    }
    push @commands, "svn copy '".currentURL().'@'.$svn_info{REVISION}."' '".tagURL($tag)."' -m \"[$action] '$tag'\"";
  }
  elsif ($action eq 'rm') {
    if (not defined $arg) {
      die "Expected additional argument 'action'";
    }
    my $rm_action = $arg;
    print "To remove branches:\n"; foreach (get_branches()) { print branch_remove_command($_,$rm_action)."\n"; }
    print "To remove tags:\n";     foreach (get_tags())     { print tag_remove_command($_,$rm_action)."\n"; }
  }
  else {
    die "Unknown action '$action'";
  }

  if ($action =~ /tag/) {
    push @commands, "# 'Schedule Release Build' in jenkins job 'ARB-tagged-builder' to build and release the tagged version";
  }

  if (scalar(@commands)) {
    print "-------------------- [Commands to execute for '$action']\n";
    foreach (@commands) {
      if ($_ =~ /^#\s/o) { $_ = $&.'[MANUALLY] '.$'; }
      print $_."\n";
    }
    print "--------------------\n";
  }

  print "Note: Please check the above commands for errors,\n";
  print "      then copy & paste to execute them.\n";
  print "      (could be done automatically later, when everything is known to work)\n";
}

sub warnya() {
  print "--------------------------------------------------------------------------------\n";
  print "IMPORTANT: This script is for ARB adminstrators only and will modify ARB SVN!\n";
  print "           Please do not misuse this script!\n";
  print "--------------------------------------------------------------------------------\n";
}

sub show_usage($) {
  my ($err) = @_;
  warnya();
  print "\n";
  print "Usage: release_tool.pl [action [arg]]\n";
  print "known 'action's:\n";
  print "\n";
  print "    branch_rc1           branch a new release candidate from 'trunk'           (uses WC-revision!)\n";
  print "    branch_stable        branch a new release           from 'branches/rc'     (uses WC-revision!)\n";
  print "\n";
  print "    tag_rc               tag rc                         in   'branches/rc'     (uses WC-revision!)\n";
  print "    tag_stable           tag release                    in   'branches/stable' (uses WC-revision!)\n";
  print "    tag_custom tag       tag custom version             anywhere               (uses WC-revision!)\n";
  print "\n";
  print "    rm action            helper to get rid of unwanted branches/tags\n";
  print "\n";
  warnya();
  if (defined $err) { print "\nError: $err\n"; }
  exit 1;
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args < 1 or $args > 2) {
    show_usage(undef);
  }
  my $action = $ARGV[0];
  my $arg    = $ARGV[1];
  perform($action,$arg);
}
main();
