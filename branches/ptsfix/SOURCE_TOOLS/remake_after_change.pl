#!/usr/bin/perl

use strict;
use warnings;

# ----------------------------------------

my @shared = (
              'AWT',
              'ARBDB',
              'CORE',
              'WINDOW',
             );
my %shared = map { $_ => 1; } @shared;

# ----------------------------------------

my %dir2dirs = (

                'AISC'     => 'NAMES_COM PROBE_COM',
                'AISC_COM' => 'AISC',
                'GL'       => 'GL/glAW GL/glpng',

               );

my %dir2libs = (

                'NAMES_COM' => 'NAMES_COM/client.a NAMES_COM/common.a NAMES_COM/server.a',
                'PROBE_COM' => 'PROBE_COM/client.a PROBE_COM/common.a PROBE_COM/server.a',
                'GL/glAW'   => 'GL/glAW/libglAW.a',
                'GL/glpng'  => 'GL/glpng/libglpng_arb.a',
                'ptpan'     => 'ptpan/PROBE.a',

               );

my %dir2targets = (

                   'AISC_MKPTPS'  => 'all',
                   'bin'          => 'all',
                   'BINDEP'       => 'all',
                   'BUGEX'        => 'all',
                   'dep_graphs'   => 'dep_graph',
                   'GDE'          => 'gde',
                   'GDEHELP'      => 'help menus',
                   'HELP_SOURCE'  => 'help',
                   'INCLUDE'      => 'all',
                   'lib'          => 'help libs perl',
                   'PERL2ARB'     => 'perl',
                   'PERL_SCRIPTS' => 'testperlscripts',
                   'PROBE_SET'    => 'pst',
                   'READSEQ'      => 'readseq',
                   'SL'           => 'all',
                   'SOURCE_TOOLS' => 'links valgrind_update',
                   'TEMPLATES'    => 'all',
                   'TOOLS'        => 'tools',
                   'UNIT_TESTER'  => 'ut',

                  );

# ----------------------------------------

my $ARBHOME = $ENV{ARBHOME};
if ((not defined $ARBHOME) or (not -d $ARBHOME)) {
  die "Need correct \$ARBHOME";
}
my $SOURCE_TOOLS = $ARBHOME.'/SOURCE_TOOLS';

sub dir2name($$$) {
  my ($dir,$prefix,$suffix) = @_;

  if ($dir eq '') { return undef; }

  my $lastname = $dir;
  if ($dir =~ /\/([^\/]+)$/) { $lastname = $1; }

  my $useddir = ($suffix eq '.so') ? 'lib' : $dir;
  my $name    = $useddir.'/'.$prefix.$lastname.$suffix;

  return $name;
}

sub dirs2changedlibs($);
sub dirs2changedlibs($) {
  my ($dirs) = @_;

  my @dir = split / /,$dirs;
  my %libs = ();

  foreach my $dir (@dir) {
    if (defined $dir2dirs{$dir}) {
      my $dirs2 = $dir2dirs{$dir};
      my $libs2 = dirs2changedlibs($dirs2);
      foreach (split / /,$libs2) { $libs{$_} = 1; }
    }
    elsif (defined $dir2libs{$dir}) {
      my $libs = $dir2libs{$dir};
      foreach (split / /,$libs) { $libs{$_} = 1; }
    }
    else {
      my $prefix = '';
      my $suffix = '.a';

      if (defined $shared{$dir}) {
        $prefix = 'lib';
        $suffix = '.so';
      }

      my $libname = dir2name($dir,$prefix,$suffix);
      $libs{$libname} = 1;
    }
  }

  return join(' ',keys %libs);
}

sub changedlibs2targets($) {
  my ($changed_lib) = @_;

  my %targets = ();
  foreach (split / /,$changed_lib) {
    # all client.a and common.a libs are identical (the libs in PROBE_COM are used in dependencies)
    if ($_ eq 'NAMES_COM/client.a') { $_ = 'PROBE_COM/client.a'; }
    if ($_ eq 'NAMES_COM/common.a') { $_ = 'PROBE_COM/common.a'; }

    my $cmd = $SOURCE_TOOLS.'/needed_libs.pl -F -U -I -T '.$_;
    my $base_targets = `$cmd`;
    chomp($base_targets);
    foreach (split / /,$base_targets) { $targets{$_} = 1; }
  }

  # modify targets
  my @targets = map {
    if ($_ =~ /\//) {
      if ($_ =~ /\.a$/o) {
        $`.'.dummy';
      }
      elsif ($_ =~ /\/([^\/]+)\.o$/o) {
        my $dir = $`;
        dir2name($dir,'','.dummy');
      }
      else {
        $_;
      }
    }
    else {
      'rac_'.$_;
    }
  } keys %targets;

  return(join(' ', @targets));
}

sub dirs2targets($) {
  my ($dirs) = @_;
  my %targets = ();
  foreach (split / /,$dirs) {
    if (defined $dir2targets{$_}) {
      my $targets = $dir2targets{$_};
      foreach (split / /,$targets) { $targets{$_} = 1; }
    }
  }
  return join(' ',keys %targets);
}

sub config2env() {
  my $conf = $ARBHOME.'/config.makefile';

  if (not -f $conf) {
    print "$conf not found.. generating\n";
    my $cmd = "cd $ARBHOME;make";
    system($cmd);
    exit(1);
  }

  open(CONF,'<'.$conf) || die "can't read '$conf' (Reason: $!)";

  my $OPENGL     = 0;
  my $PTPAN      = 0;
  my $UNIT_TESTS = 0;

  foreach (<CONF>) {
    chomp;
    if (/^OPENGL\s*:=\s*([0-9]+)/) { $OPENGL = $1; }
    if (/^PTPAN\s*:=\s*([0-9]+)/) { $PTPAN = $1; }
    if (/^UNIT_TESTS\s*:=\s*([0-9]+)/) { $UNIT_TESTS = $1; }
  }
  close(CONF);

  print "Using:\n";
  print "- OPENGL=$OPENGL\n";
  print "- PTPAN=$PTPAN\n";
  print "- UNIT_TESTS=$UNIT_TESTS\n";

  if ($OPENGL==1) {
    $ENV{RNA3D_LIB} = 'RNA3D/RNA3D.a';
  }
  if ($PTPAN==1) { $ENV{ARCHS_PT_SERVER_LINK} = 'ptpan/PROBE.a'; }
  else { $ENV{ARCHS_PT_SERVER_LINK} = 'PROBE/PROBE.a'; }

  return $UNIT_TESTS;
}

sub system_no_error_or_die($) {
  my ($cmd) = @_;
  if (system($cmd)!=0) { die "could not execute '$cmd'"; }
  if ($? == -1) { die "could not execute '$cmd'"; }
  if ($? & 127) { die sprintf("child died with signal %d", ($? & 127)); }
  my $exitcode = ($? >> 8);
  if ($exitcode!=0) { die "command '$cmd' failed (exitcode=$exitcode)"; }
}

sub main() {
  my $dir = `pwd`;
  chomp($dir);

  if (not defined $ENV{LINK_STATIC}) {
    print "Warning: LINK_STATIC not defined (assuming 0 - which is wrong for OSX)\n";
    $ENV{LINK_STATIC} = '0'; # wrong for OSX
  }

  my $UNIT_TESTS = config2env();

  if (not $dir =~ /^$ARBHOME/) {
    print "Usage: simply call in any directory inside ARBHOME\n";
    print "It will assume you changed something there and remake all depending executables\n";
    print "(In fact this is a lie, it won't work everywhere:)\n";

    die "Not called from inside $ARBHOME";
  }

  print "remake[3]: Entering directory `$ARBHOME'\n";
  chdir($ARBHOME);

  $dir = $';
  $dir =~ s/^\///;

  my $changed_libs = dirs2changedlibs($dir);
  my $targets;
  if ($changed_libs eq '') {
    print "Called in ARBHOME\n";
    $targets = 'all';
  }
  else {
    $targets = '';
    while ($targets eq '') {
      print "Assuming $changed_libs changed\n";
      $targets = changedlibs2targets($changed_libs);
      if ($targets eq '') { $targets = dirs2targets($dir); }

      if ($targets eq '') {
        if ($dir =~ /\//) {
          my $updir = $`;
          print "No known targets for $dir - trying $updir\n";
          $dir = $updir;
          $changed_libs = dirs2changedlibs($dir);
        }
        else {
          die "No idea what to remake for '$dir'\n ";
        }
      }
    }
  }

  if ($targets ne '') {
    my $cores = `cat /proc/cpuinfo | grep processor | wc -l`;
    if ($cores<1) { $cores = 1; }
    my $jobs = $cores+1;
    print "Remaking\n";

    foreach (split / /,$targets) { print " - $_\n"; }

    print "\n";

    my $premake = "make -j$jobs up_by_remake";
    print "Silent premake: '$premake'\n";
    my $log = 'silent_premake.log';
    $premake = "cd $ARBHOME;$premake > $log 2>&1";
    print "Silent premake: '$premake' (real)\n";

    eval {
      system_no_error_or_die($premake);
    };
    if ($@) {
      my $err = "Silent premake failed (".$@.")";
      print "\nError: $err\n";
      print "---------------------------------------- [$log start]\n";
      open(LOG,'<'.$log) || die "can't read '$log' (Reason: $!)";
      print <LOG>;
      close(LOG);
      print "---------------------------------------- [$log end]\n";
      die $err;
    }
    print "\n";

    my $targets_contain_unittests = 0;
    my $targets_are_timed = 0;

    my %targets = map { $_ => 1; } split / /,$targets;
    if (defined $targets{all}) {
      $targets_contain_unittests = 1;
      $targets_are_timed         = 1;
    }
    if (defined $targets{ut}) {
      $targets_contain_unittests = 1;
    }

    my $makecmd;
    if ($targets_are_timed==1) {
      $makecmd = "cd $ARBHOME;make -j$jobs $targets";
    }
    else {
      my $timed_target = (($UNIT_TESTS==0) or ($targets_contain_unittests==1)) ? 'timed_target' : 'timed_target_tested';
      $makecmd = "cd $ARBHOME;make \"TIMED_TARGET=$targets\" -j$jobs $timed_target";
    }

    print "[Make: '$makecmd']\n";
    system($makecmd)==0 || die "error executing '$makecmd' (exitcode=$?)\n";
    print "remake[3]: Leaving directory `$ARBHOME'\n";

    unlink('silent_premake.log');
  }
}

main();
