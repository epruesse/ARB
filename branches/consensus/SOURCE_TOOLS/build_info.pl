#!/usr/bin/perl

use strict;
use warnings;

# create/update build info into
#     ../TEMPLATES/arb_build.h
#     ../TEMPLATES/svn_revision.h
# and
#     ../lib/revision_info.txt

# --------------------------------------------------------------------------------

my $dumpFiles = 1;

my $RC_BRANCH     = 'rc';
my $STABLE_BRANCH = 'stable';

# --------------------------------------------------------------------------------

my $ARBHOME = $ENV{ARBHOME};
if (not defined $ARBHOME) { die "ARBHOME undefined"; }
if ((not -d $ARBHOME) or (not -f $ARBHOME.'/arb_LICENSE.txt')) {
  die "ARBHOME ('$ARBHOME') must point to ARB build directory";
}

my $TEMPLATES    = $ARBHOME.'/TEMPLATES';       if (not -d $TEMPLATES)    { die "no such directory '$TEMPLATES'"; }
my $SOURCE_TOOLS = $ARBHOME.'/SOURCE_TOOLS';    if (not -d $SOURCE_TOOLS) { die "no such directory '$SOURCE_TOOLS'"; }
my $lib          = $ARBHOME.'/lib';             if (not -d $lib)          { die "no such directory '$lib'"; }
my $mv_if_diff   = $SOURCE_TOOLS.'/mv_if_diff'; if (not -x $mv_if_diff)   { die "no such script '$mv_if_diff'"; }

# upgrade version?
my $inc_major = $SOURCE_TOOLS.'/inc_major.stamp';
my $inc_minor = $SOURCE_TOOLS.'/inc_minor.stamp';
my $inc_candi = $SOURCE_TOOLS.'/inc_candi.stamp';
my $inc_patch = $SOURCE_TOOLS.'/inc_patch.stamp';

# --------------------------------------------------------------------------------

sub execAndGetFirstNonemptyLine($) {
  my ($infocmd) = @_;
  # returns first nonempty line from infocmd-output
  # or undef (if output is empty)
  # or dies (if command fails)

  my $content = undef;
  print "[executing '$infocmd']\n";
  open(INFO,$infocmd.'|') || die "failed to fork '$infocmd' (Reason: $!)";
 LINE: foreach (<INFO>) {
    if ($_ ne '') {
      chomp;
      $content = $_;
      last LINE;
    }
  }
  close(INFO) || die "failed to execute '$infocmd' (Reason: $!)";;
  if (defined $content) { print "output='$content'\n"; }
  else { print "no output :-(\n"; }
  return $content;
}

sub getHost() {
  my $arbbuildhost = $ENV{ARBBUILDHOST};
  my @hosts = ();
  if (defined $arbbuildhost) { push @hosts, $arbbuildhost; }
  else {
    my $host = $ENV{HOST};
    my $hostname = $ENV{HOSTNAME};

    if (defined $host) { push @hosts, $host; }
    if (defined $hostname) { push @hosts, $hostname; }
  }
  if (scalar(@hosts)==0) {
    my $hostnameout = undef;
    eval { $hostnameout = execAndGetFirstNonemptyLine('hostname'); };
    if ($@) { print "Warning: buildhost is unknown ($@)\n"; }
    if (not defined $hostnameout) { $hostnameout = 'unknown'; }

    my $domainname = undef;
    eval { $domainname = execAndGetFirstNonemptyLine('domainname'); };
    if ($@) { print "Warning: domain is unknown ($@)\n"; $domainname = undef; }
    if ((not defined $domainname) or ($domainname eq '(none)')) { $domainname = 'somewhere'; }

    push @hosts, $hostnameout.'.'.$domainname;
  }

  @hosts = sort { length($b) <=> length($a); } @hosts; # sort longest first
  return $hosts[0];
}

sub getUser() {
  my $user = $ENV{ARBBUILDUSER};
  if (not defined $user) { $user = $ENV{USER}; }
  if (not defined $user) {
    eval { $user = execAndGetFirstNonemptyLine('whoami'); };
    if ($@) { print "Warning: user is unknown ($@)\n"; $user = undef; }
  }
  if (not defined $user) { $user = 'unknownUser'; }
  return $user;
}

sub guessSvnBranchInsideJenkins() {
  my $root = undef;
  my $url = $ENV{SVN_URL}; # is set inside jenkins

  if (defined $url) {
    if ($url eq '') { $url = undef; }
    else {
      my $suffix = undef;
      if ($url =~ /^(http:\/\/vc\.arb-home\.de\/(svn|readonly))\//o) {
        $suffix = $';
        $root = $1;
      }
      elsif ($url =~ /^(svn\+ssh:\/\/.*vc\.arb-home\.de\/home\/vc\/repos\/ARB)\//o) {
        $suffix = $';
        $root = $1;
      }
      if (not defined $suffix) { die "Unexpected value in SVN_URL ('$url')"; }
      die "root unset (url='$url')" if not defined $root;
    }
  }
  return ($root,$url);
}

sub getRevision() {
  my $jrevision = $ENV{SVN_REVISION}; # is set inside jenkins
  if (defined $jrevision and $jrevision eq '') { $jrevision = undef; }

  my $revision = undef;
  eval {
    $revision = execAndGetFirstNonemptyLine("svnversion -c -n '$ARBHOME'");
    if (defined $revision and $revision =~ /^2:/) { $revision = $'; }
  };
  if ($@) {
    if (defined $jrevision) {
      print "Accepting svn failure (apparently running inside jenkins)\n";
      $revision = $jrevision;
    }
    else { die $@."\n"; }
  }

  if (not defined $revision) { die "Failed to detect revision number"; }
  if (defined $jrevision) {
    if ($jrevision ne $revision) {
      die "Conflicting revision numbers (jrevision='$jrevision', revision='$revision')";
    }
  }
  return $revision;
}

sub getBranchOrTag() {
  # returns any of
  #   (0,trunk)
  #   (0,branchname)
  #   (1,tagname)
  # or dies

  my ($jroot,$jurl) = guessSvnBranchInsideJenkins();
  my ($root,$url)   = (undef,undef);
  eval {
    my $infocmd = "svn info '$ARBHOME'";
    print "[executing '$infocmd']\n";
    open(INFO,$infocmd.'|') || die "failed to fork '$infocmd' (Reason: $!)";
    foreach (<INFO>) {
      chomp;
      print "info[b]='$_'\n";
      if (/^Repository\sRoot:\s+/o) { $root = $'; }
      elsif (/^URL:\s+/o) { $url = $'; }
    }
    close(INFO) || die "failed to execute '$infocmd' (Reason: $!)";;
  };
  if ($@) {
    if (defined $jroot and defined $jurl) {
      print "Accepting svn failure (apparently running inside jenkins)\n";
      ($root,$url) = ($jroot,$jurl);
    }
    else { die $@."\n"; }
  }

  if (not defined $root) { die "Failed to detect repository root"; }
  if (not defined $url)  { die "Failed to detect repository URL"; }

  if (defined $jroot) {
    if (not defined $jurl)  { die "\$jroot defined, \$jurl undefined (bug in guessSvnBranchInsideJenkins?)"; }
    if ($jroot ne $root) { die "Conflicting SVN root detection:\n1='$root'\n2='$jroot'"; }
    if ($jurl  ne $url)  { die "Conflicting SVN url detection:\n1='$url'\n2='$jurl'"; }
  }
  elsif (defined $jurl) { die "\$jurl defined, \$jroot undefined (bug in guessSvnBranchInsideJenkins?)"; }

  my $rootlen = length($root);
  my $url_prefix = substr($url,0,$rootlen);
  if ($url_prefix ne $root) { die "Expected '$url_prefix' to match '$root'"; }

  my $rest = substr($url,$rootlen+1);
  my $is_tag = 0;
  if ($rest =~ /^branches\//) {
    $rest = $';
  }
  elsif ($rest =~ /^tags\//) {
    $rest = $';
    $is_tag = 1;
  }
  return ($is_tag,$rest);
}

sub getBranchOrTagFromHeader($) {
  my ($header) = @_;
  open(HEADER,'<'.$header) || die "Failed to read '$header' (Reason: $!)";
  my ($revision,$is_tag,$branch) = (undef,undef);
  foreach (<HEADER>) {
    chomp;
    if (/^\#define\s+ARB_SVN_BRANCH\s+\"([^\"]+)\"/o) { $branch = $1; }
    elsif (/^\#define\s+ARB_SVN_BRANCH_IS_TAG\s+([01])/o) { $is_tag = $1; }
    elsif (/^\#define\s+ARB_SVN_REVISION\s+\"([^\"]+)\"/o) { $revision = $1; }
  }
  close(HEADER);

  if (not defined $branch) { die "Failed to parse branch from $header"; }
  if (not defined $is_tag) { die "Failed to parse is_tag from $header"; }
  if (not defined $revision) { die "Failed to parse revision from $header"; }
  if ($is_tag != 0 and $is_tag != 1) { die "Invalid value is_tag='$is_tag'"; }
  return ($revision,$is_tag,$branch);
}

sub dumpFile($) {
  my ($file) = @_;
  print "---------------------------------------- [start $file]\n";
  system("cat $file");
  print "---------------------------------------- [end $file]\n";
}

sub update($\@) {
  my ($file,$content_r) = @_;
  my $tmp = $file.'.tmp';

  open(TMP,'>'.$tmp) || die "can't write to '$tmp' (Reason: $!)";
  foreach (@$content_r) { print TMP $_."\n"; }
  close(TMP);
  `$mv_if_diff '$tmp' '$file'`;
  if ($dumpFiles) { dumpFile($file); }
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
  foreach (sort keys %$hash_r) {
    print FILE "$_=".$$hash_r{$_}."\n";
  }
  close(FILE);
}

# --------------------------------------------------------------------------------

my $arb_build_h    = $TEMPLATES.'/arb_build.h';
my $svn_revision_h = $TEMPLATES.'/svn_revision.h';
my $revision_info  = $lib.'/revision_info.txt';

my $in_SVN = (-d $ARBHOME.'/.svn');

# update revision info?
my ($revision,$is_tag,$branch) = (undef,undef,undef);
if ($in_SVN) {
  # in SVN checkout -> update revision info
  $revision = getRevision();
  ($is_tag,$branch) = getBranchOrTag();

  # $branch = $RC_BRANCH; # @@@ fake
  # $branch = $STABLE_BRANCH; # @@@ fake
  # $branch = 'gtk_only'; # @@@ fake
  # ($is_tag,$branch) = (1, 'arb-5.20.1'); # @@@ fake
  # ($is_tag,$branch) = (1, 'arb-5.19'); # @@@ fake
  # ($is_tag,$branch) = (1, 'evalSomething'); # @@@ fake
  # ($is_tag,$branch) = (1, 'arb-5.20'); # @@@ fake
  # ($is_tag,$branch) = (1, 'arb-5.20-rc1'); # @@@ fake
  # ($is_tag,$branch) = (1, 'arb-5.20-rc2'); # @@@ fake

  my @svn_revision = (
                      '#define ARB_SVN_REVISION      "'.$revision.'"',
                      '#define ARB_SVN_BRANCH        "'.$branch.'"',
                      '#define ARB_SVN_BRANCH_IS_TAG '.$is_tag,
                     );

  update($svn_revision_h,@svn_revision);
}
else {
  if (not -f $svn_revision_h) {
    die "Missing file '$svn_revision_h'";
  }
  # use revision info as in source tarball
  ($revision,$is_tag,$branch) = getBranchOrTagFromHeader($svn_revision_h);
}

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

if (not defined $version_info{CANDIDATE}) { $version_info{CANDIDATE} = 1; }
if (not defined $version_info{PATCHLEVEL}) { $version_info{PATCHLEVEL} = 0; }

if (-f $inc_major or -f $inc_minor or -f $inc_candi or -f $inc_patch) { # version/rc-candidate/patchlevel upgrade requested?
  eval {
    print "\n";
    if ($in_SVN) {
      if ($is_tag==1) {
        die "Upgrading version information not possible in tag-checkout! (tag of this WC = '$branch')";
      }
      if (-f $inc_candi) {
        if ($branch ne $RC_BRANCH) {
          die "Upgrading RC-candidate number only possible in branch '$RC_BRANCH' (you are in '$branch')";
        }
        my $oldRC = $version_info{CANDIDATE};
        if (not defined $oldRC) { die "No CANDIDATE defined"; }
        $version_info{CANDIDATE}++;
        my $newRC = $version_info{CANDIDATE};
        print "RC-candidate number upgraded from $oldRC to $newRC\n";
      }
      elsif (-f $inc_patch) {
        if ($branch ne $STABLE_BRANCH) {
          die "Upgrading patchlevel only possible in branch '$STABLE_BRANCH' (you are in '$branch')";
        }
        my $oldPL = $version_info{PATCHLEVEL};
        if (not defined $oldPL) { die "No PATCHLEVEL defined"; }
        $version_info{PATCHLEVEL}++;
        my $newPL = $version_info{PATCHLEVEL};
        print "patchlevel upgraded from $oldPL to $newPL\n";
      }
      else {
        if ($is_tag==1 or $branch ne 'trunk') {
          die "Upgrading version only possible in 'trunk' (you are in '$branch')";
        }
        my $oldVersion = $version_info{MAJOR}.'.'.$version_info{MINOR};
        if (-f $inc_major) {
          $version_info{MAJOR}++;
          $version_info{MINOR} = 0;
        }
        else {
          $version_info{MINOR}++;
        }
        $version_info{CANDIDATE} = 1; # first release candidate will be rc1
        $version_info{PATCHLEVEL} = 0; # no patchlevel (yet)
        my $newVersion = $version_info{MAJOR}.'.'.$version_info{MINOR};
        print "Version upgraded from $oldVersion to $newVersion\n";
      }

      $version_info{last_upgrade}=time; # upgrade timestamp
      hash2file(%version_info,$version_info);
    }
    else {
      die "Upgrading version only possible in SVN WC";
    }
    print "\n";
  };
  my $failed = $@;

  # always remove requests
  -f $inc_major && unlink($inc_major);
  -f $inc_minor && unlink($inc_minor);
  -f $inc_candi && unlink($inc_candi);
  -f $inc_patch && unlink($inc_patch);

  if ($failed) { die "$failed\n"; }
}

# create valid svn-tag for this version

my $svn_tag              = undef;
my $short_version        = undef;
my $always_show_revision = 1;

my $orgbranch = $branch; # real branch or branch estimated from tag
if ($is_tag==1) {
  if ($branch =~ /^arb-[0-9]+\.[0-9]+/o) {
    if ($branch =~ /-rc[0-9]+$/o) { $orgbranch = $RC_BRANCH; }
    else                          { $orgbranch = $STABLE_BRANCH; }
  }
  else {
    $orgbranch = 'unknown';
  }
}

if ($orgbranch eq $STABLE_BRANCH or $orgbranch eq $RC_BRANCH) {
  $always_show_revision = 0;
  $svn_tag = 'arb-'.$version_info{MAJOR}.'.'.$version_info{MINOR};
  if ($orgbranch eq $RC_BRANCH) {
    $svn_tag .= '-rc'.$version_info{CANDIDATE};
  }
  else {
    if ($version_info{PATCHLEVEL} > 0) { $svn_tag .= '.'.$version_info{PATCHLEVEL}; }
  }
  $short_version = $svn_tag;

  if ($is_tag==1) {
    # check real SVN-tag vs generated SVN-tag
    if ($branch ne $svn_tag) {
      die "Version info and SVN-branch-tag mismatch:\n".
        "(version suggests svn-tag = '$svn_tag'\n".
        " real             svn-tag = '$branch')";
    }
  }
  print "SVN_URL='$ENV{SVN_URL}'\n";
  print "SVN_REVISION='$ENV{SVN_REVISION}'\n";
}
elsif ($is_tag==1) {
  $short_version = 'arb-special-'.$branch; # use custom tag
}
else {
  $short_version = 'arb-devel';
  if ($branch ne 'trunk') { $short_version .= '-'.$branch; }
  $short_version .= '-'.$version_info{MAJOR}.'.'.$version_info{MINOR};
}

defined $short_version || die "expected known short_version!";
defined $revision || die "expected known revision!";
my $long_version  = $short_version.'.rev'.$revision;

if ($always_show_revision==1) {
  $short_version = $long_version;
}

my $ARB_64 = $ENV{ARB_64};
if (not defined $ARB_64) {
  my $config_makefile = $ARBHOME.'/config.makefile';
  if (open(CONFIG, '<'.$config_makefile)) {
    $ARB_64 = 1; # default to 64 bit -- see ../Makefile@64bit
    foreach (<CONFIG>) {
      if (/^\s*ARB_64\s*:=\s*([01]).*/) {
        $ARB_64 = $1;
      }
    }
    close(CONFIG);
  }
  else {
    die "Either environment variable ARB_64 has to be defined or $config_makefile has to exist!";
  }
}

if (not $ARB_64) {
  $short_version .= '-32bit';
  $long_version  .= '-32bit';
}

my @arb_build = (
                 '#define ARB_VERSION            "'.$short_version.'"',
                 '#define ARB_VERSION_DETAILED   "'.$long_version.'"',

                 '#define ARB_BUILD_DATE         "'.$date.'"',
                 '#define ARB_BUILD_YEAR         "'.$year.'"',

                 '#define ARB_BUILD_HOST         "'.getHost().'"',
                 '#define ARB_BUILD_USER         "'.getUser().'"',
                );

update($arb_build_h,@arb_build);


my @revision_info = (
                     $revision.'@'.$branch,
                    );

update($revision_info,@revision_info);
