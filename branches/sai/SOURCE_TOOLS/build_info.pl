#!/usr/bin/perl

use strict;
use warnings;

# create/update build info into
# $ARBHOME/TEMPLATES/arb_build.h and
# $ARBHOME/TEMPLATES/svn_revision.h

# --------------------------------------------------------------------------------

my $dump = 1;

# --------------------------------------------------------------------------------

my $ARBHOME = $ENV{ARBHOME};
if (not defined $ARBHOME) { die "ARBHOME undefined"; }
if ((not -d $ARBHOME) or (not -f $ARBHOME.'/arb_LICENSE.txt')) {
  die "ARBHOME ('$ARBHOME') must point to ARB build directory";
}

my $TEMPLATES    = $ARBHOME.'/TEMPLATES';       if (not -d $TEMPLATES)    { die "no such directory '$TEMPLATES'"; }
my $SOURCE_TOOLS = $ARBHOME.'/SOURCE_TOOLS';    if (not -d $SOURCE_TOOLS) { die "no such directory '$SOURCE_TOOLS'"; }
my $mv_if_diff   = $SOURCE_TOOLS.'/mv_if_diff'; if (not -x $mv_if_diff)   { die "no such script '$mv_if_diff'"; }

# --------------------------------------------------------------------------------

sub getHost() {
  my $host = $ENV{HOST};
  my $hostname = $ENV{HOSTNAME};

  my @hosts = ();
  if (defined $host) { push @hosts, $host; }
  if (defined $hostname) { push @hosts, $hostname; }

  if (scalar(@hosts)==0) { push @hosts, "unknownHost"; }

  @hosts = sort { length($b) <=> length($a); } @hosts; # sort longest first
  return $hosts[0];
}

sub getUser() {
  my $user = $ENV{USER};
  if (not defined $user) { $user = 'unknownUser'; }
  return $user;
}

sub update($\@) {
  my ($file,$content_r) = @_;
  my $tmp = $file.'.tmp';

  open(TMP,'>'.$tmp) || die "can't write to '$tmp' (Reason: $!)";
  foreach (@$content_r) { print TMP $_."\n"; }
  close(TMP);
  `$mv_if_diff '$tmp' '$file'`;
}

sub file2hash($\%$) {
  my ($file,$hash_r,$expectFile) = @_;
  if (open(FILE,'<'.$file)) {
    foreach (<FILE>) {
      chomp;
      if (/^([^=]+)=(.*)$/o) { $$hash_r{$1}=$2; }
    }
    close(FILE);
  }
  elsif ($expectFile==1) {
    die "can't read '$file' (Reason: $!)";
  }
}

sub hash2file(\%$) {
  my ($hash_r,$file) = @_;
  open(FILE,'>'.$file) or die "can't write '$file' (Reason: $!)";
  foreach (keys %$hash_r) {
    print FILE "$_=".$$hash_r{$_}."\n";
  }
  close(FILE);
}

# --------------------------------------------------------------------------------

my $dot_build_info = $ENV{HOME}.'/.arb_build_info';

# default build info (may be overridden by ~/.arb_build_info)
my %build_info =  (

                   user                => getUser(),
                   host                => getHost(),
                   tag                 => 'private',
                   allowVersionUpgrade => 0,
                   showWhereBuild      => 1,

                  );

# read local settings
file2hash($dot_build_info,%build_info,0);

my $arb_build_h    = $TEMPLATES.'/arb_build.h';
my $svn_revision_h = $TEMPLATES.'/svn_revision.h';

my $in_SVN = (-d $ARBHOME.'/.svn');
my $date = `date '+%d.%b.%Y'`;
chomp($date);
my $year = undef;
if ($date =~ /\.([^\.]+)$/o) {
  $year = $1;
}
else {
  die "error parsing year from '$date'";
}

# read version info
my $version_info = $SOURCE_TOOLS.'/version_info';
my %version_info = ();
file2hash($version_info,%version_info,1);

# upgrade version?
my $inc_major = $SOURCE_TOOLS.'/inc_major.stamp';
my $inc_minor = $SOURCE_TOOLS.'/inc_minor.stamp';

if ($in_SVN and $build_info{allowVersionUpgrade}==1) {
  if (-f $inc_major or -f $inc_minor) { # version upgrade requested
    my $last_version_upgrade = $version_info{last_upgrade};
    if (not defined $last_version_upgrade) { $last_version_upgrade = 0; }

    my $earliestNextUpgrade = $last_version_upgrade + (60*60);
    if (time>=$earliestNextUpgrade) { # do not upgrade version more than once per hour
        my $oldVersion = $version_info{MAJOR}.'.'.$version_info{MINOR};
        if (-f $inc_major) {
          $version_info{MAJOR}++;
          $version_info{MINOR} = 0;
        }
        else {
          $version_info{MINOR}++;
        }
        my $newVersion = $version_info{MAJOR}.'.'.$version_info{MINOR};
        print "Version upgraded from $oldVersion to $newVersion\n";
        $version_info{last_upgrade}=time; # upgrade timestamp

        hash2file(%version_info,$version_info);
        my $command = "cd '$ARBHOME/SOURCE_TOOLS'; ".
          "svn commit version_info --non-interactive ".
            "--message 'Auto version upgrade to $newVersion by ".$build_info{user}.'@'.$build_info{host}."';".
              "( cd '$ARBHOME' ; svn update )"; # update revision of checkout in $ARBHOME
        `( $command )`;
    }
    else {
      print "Skipping version upgrade (last upgrade was ".(time-$last_version_upgrade)." seconds ago)\n";
    }
  }
}
# remove requests
-f $inc_minor && unlink($inc_minor);
-f $inc_major && unlink($inc_major);

my @arb_build = (
                 '#define ARB_VERSION_MAJOR "'.$version_info{MAJOR}.'"',
                 '#define ARB_VERSION_MINOR "'.$version_info{MINOR}.'"',
                 '#define ARB_VERSION_TAG   "'.$build_info{tag}.'"',
                 '#define ARB_BUILD_DATE    "'.$date.'"',
                 '#define ARB_BUILD_YEAR    "'.$year.'"',
                 '#define ARB_BUILD_HOST    "'.$build_info{host}.'"',
                 '#define ARB_BUILD_USER    "'.$build_info{user}.'"',
                );

if ($build_info{showWhereBuild}!=0) {
  push @arb_build, '#define SHOW_WHERE_BUILD';
}

update($arb_build_h,@arb_build);

# update revision info?
if ($in_SVN) {
  # in SVN checkout -> update revision info
  my $revision = `svnversion -c -n $ARBHOME`;
  if ($revision =~ /^2:/) {
    # for some reason -c a "2:" prefix
    $revision = $';
  }
  my @svn_revision = (
                      '#define ARB_SVN_REVISION "'.$revision.'"',
                     );
  update($svn_revision_h,@svn_revision);
}
else {
  if (not -f $svn_revision_h) {
    die "Missing file '$svn_revision_h'";
  }
  # use revision info as in source tarball
}


