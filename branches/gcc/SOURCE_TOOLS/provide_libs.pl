# ============================================================ #
#                                                              #
#   File      : provide_libs.pl                                #
#   Purpose   :                                                #
#                                                              #
#   Coded by Ralf Westram (coder@reallysoft.de) in June 2009   #
#   Institute of Microbiology (Technical University Munich)    #
#   www.arb-home.de                                            #
#                                                              #
# ============================================================ #

use strict;
use warnings;

my %needed_by = (
                 # 'libXm.so' => 'arb_ntree', # this would also link the motif lib (not done atm)
                );

my %needed_by_opengl = (
                        'libGLEW.so' => 'arb_edit4',
                        'libGLw.so' => 'arb_edit4',
                       );

my $verbose = 0; # for debugging this script

#  libs get copied to ../lib/addlibs

sub getModtime($) {
  my ($fileOrDir) = @_;
  my $modtime = (stat($fileOrDir))[9];
  return $modtime;
}

sub update_lib($$$) {
  my ($libname, $binary, $addlibsdir) = @_;

  if (not -f $binary) {
    print "Can't use '$binary' to detect used '$libname' (no such file)\n";
  }
  else {
    my ($linked_symbolic,$linked_real) = (undef,undef);
    my $command = 'ldd '.$binary;

    $verbose==0 || print "$command\n";
    open(LDD,$command.'|') || die "can't execute '$command' (Reason: $!)";
    foreach (<LDD>) {
      chomp;
      if (/^\s+([^\s]+)\s+=>\s+([^\s]*)\s+\(.*\)$/) {
        my ($symbolic,$real) = ($1,$2);
        $verbose==0 || print "ldd-out: symbolic='$symbolic' real='$real'\n";
        if ($symbolic =~ /^$libname\./) {
          ($linked_symbolic,$linked_real) = ($symbolic,$real);
        }
      }
      else {
        $verbose==0 || print "cant parse ldd-out: '$_'\n";
      }
    }
    close(LDD);

    my $keep = undef;
    if (not defined $linked_symbolic) {
      print "Warning: $binary is NOT linked against $libname ($libname will not be provided)\n";
    }
    else {
      print "$binary links against '$linked_symbolic' ($linked_real)\n";

      my $source = $linked_real;
      my $dest   = $addlibsdir.'/'.$linked_symbolic;
      my $doCopy = 0;

      if (-f $dest) {
        my $mod_source = getModtime($source);
        my $mod_dest   = getModtime($dest);

        if ($mod_dest != $mod_source) {
          print "Modification time of $source differs -> force update\n";
          $doCopy = 1;
        }
        else {
          print "$dest is up-to-date\n";
        }
      }
      else { $doCopy = 1; }

      if ($doCopy==1) {
        $command = "cp -p '$source' '$dest'";
        print "Updating $dest\n";
        system($command)==0 || die "Failed to execute '$command' (Reason: $?)";
      }

      if (not -f $dest) { die "oops .. where is $dest?"; }
      $keep = $linked_symbolic;
    }

    # delete older copies
    opendir(DIR,$addlibsdir) || die "can't read directory '$addlibsdir' (Reason: $!)";
    foreach (readdir(DIR)) {
      if (/^$libname/) {
        # print "Matching file: '$_'\n";
        if ((not defined $keep) or ($_ ne $keep)) {
          my $toRemove = $addlibsdir.'/'.$_;
          print "removing obsolete file '$toRemove'\n";
          unlink($toRemove) || die "Failed to unlink '$toRemove' (Reason: $!)";
        }
      }
    }
    closedir(DIR);
  }
}
sub provide_libs($$$) {
  my ($arbhome,$addlibsdir,$opengl) = @_;
  my $bindir = $arbhome.'/bin';

  chdir($arbhome) || die "can't cd to '$arbhome' (Reason: $!)";
  print "Updating additional libs\n";

  foreach my $lib (keys %needed_by) {
    update_lib($lib, $bindir.'/'.$needed_by{$lib}, $addlibsdir);
  }
  if ($opengl==1) {
    foreach my $lib (keys %needed_by_opengl) {
      update_lib($lib, $bindir.'/'.$needed_by_opengl{$lib}, $addlibsdir);
    }
  }
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args==0) {
    die 'Usage: provide_libs.pl "opengl=$(OPENGL)" "link_static=$(LINK_STATIC)" "arbhome=$(ARBHOME)"';
  }

  my %arg = ();
  foreach (@ARGV) {
    if (/^([^=]+)=(.*)$/) { $arg{$1} = $2; }
    else { die "can't handle argument '$_'"; }
  }

  if ($verbose>0) { foreach (sort keys %arg) { print "$_='".$arg{$_}."'\n"; } }

  my $opengl = $arg{opengl};
  my $arbhome = $arg{arbhome};
  my $link_static = $arg{link_static};

  if (defined $opengl and defined $arbhome and defined $link_static) {
    if (not -d $arbhome) { die "Expected directory '$arbhome'"; }

    my $addlibsdir = $arbhome.'/lib/addlibs';
    if (not -d $addlibsdir) { die "Expected directory '$addlibsdir'"; }

    if ($link_static == 1) {
      print "No libs provided to $addlibsdir (we are linking static)\n";
    }
    else {
      provide_libs($arbhome,$addlibsdir,$opengl);
    }
  }
  else {
    die "Expected arguments are 'devel=', 'arbhome=' and 'link_static='";
  }
}
main();
