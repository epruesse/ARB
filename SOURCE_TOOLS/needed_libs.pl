#!/usr/bin/perl

use strict;
use warnings;

my $debug_verboose;
# BEGIN {
  # $debug_verboose = 1;
  # $SIG{__DIE__} = sub {
    # require Carp;
    # if ($debug_verboose>0) { Carp::confess(@_); } # with backtrace
    # else { Carp::croak(@_); }
  # }
# }

my $dieOnUndefEnvvar = 1; # use '-U' to toggle

my $libdepend_file = 'needs_libs';
my $exedepend_dir  = 'BINDEP';

# --------------------------------------------------------------------------------

my $ARBHOME = $ENV{ARBHOME};
defined $ARBHOME || die "Environmentvariable 'ARBHOME' is not defined";
-d $ARBHOME || die "ARBHOME does not point to a directory ($ARBHOME)";
if (not $ARBHOME =~ /\/$/o) { $ARBHOME .= '/'; }
my $AH_len = length($ARBHOME);

sub ARBHOME_relative($) {
  my ($dir) = @_;
  defined $dir || die;
  if (substr($dir,0,$AH_len) eq $ARBHOME) { substr($dir,$AH_len); }
  else { $dir; }
}

sub fullpath($) {
  my ($path) = @_;
  return $ARBHOME.ARBHOME_relative($path);
}

# --------------------------------------------------------------------------------

my $LINK_STATIC = $ENV{LINK_STATIC};
# LINK_STATIC==1 (used on OSX) ->
# - shared ARB-libraries (e.g. lib/libWINDOW.so) are created/linked statically (as lib/libWINDOW.a)
# - in contrast to "normal" static libs
#   - they do NOT reside in the source-code directory and
#   - they have a 'lib'-prefix

sub fixFileSuffix($) {
  my ($file) = @_;
  defined $LINK_STATIC || die "Environmentvariable 'LINK_STATIC' is not defined";
  if ($LINK_STATIC==1) { # true on OSX
    $file =~ s/\.so/\.a/; # correct lib-suffix
  }
  $file;
}

# --------------------------------------------------------------------------------

sub dirOf($) {
  my ($full) = @_;
  if ($full =~ /\/([^\/]+)$/o) { $`; }
  else { undef; }
}

sub filenameOf($) {
  my ($full) = @_;
  defined $full || die;
  if ($full =~ /\/([^\/]+)$/o) { $1; }
  else { $full; }
}

sub trim($) {
  my ($str) = @_;
  $str =~ s/^\s+//go;
  $str =~ s/\s+$//go;
  return $str;
}

# --------------------------------------------------------------------------------

sub legal_target_or_die($) {
  my ($target) = @_;

  my $ct = $target;
  $ct =~ s/ $//g;
  $ct =~ s/^ //g;

  if ($ct ne $target) {
    die "Invalid target '$target' (chomped='$ct')";
  }
}

my $reg_exedepend = qr/^$exedepend_dir\/$libdepend_file\./;
my $reg_depend = qr/\/$libdepend_file/;

sub dependencyFile2target($) {
  my ($depfile) = @_;

  $depfile = ARBHOME_relative($depfile);
  my $target = undef;
  if ($depfile =~ $reg_exedepend) {
    $target = $';
  }
  elsif ($depfile =~ $reg_depend) {
    my ($dir,$suffix) = ($`,$');
    my $libname = undef;
    if ($suffix eq '') { # default (DIR/DIR.a)
      my $lastDir = filenameOf($dir);
      $libname = $lastDir.'.a';
    }
    else {
      $libname = substr($suffix,1);
      $libname =~ s/\_(so|a|o)$/\.$1/o;
      if ($1 eq 'so') {
        $dir = 'lib';
      }
    }
    $target = $dir.'/'.$libname;
    if ($target eq 'lib/lib.dummy') {
      die "Generated invalid target '$target'";
    }
  }
  else {
    die "Illegal dependency file '$depfile'";
  }

  return $target;
}

sub target2dependencyFile($) {
  my ($target) = @_;

  $target = ARBHOME_relative($target);
  legal_target_or_die($target);

  my $depfile = undef;
  my $dir     = dirOf($target);

  if (defined $dir) {
    if ($dir eq 'lib') {
      if ($target =~ /^lib\/lib([A-Z]+)\.(so|a)$/) {
        $depfile = $1.'/'.$libdepend_file.'.lib'.$1.'_so';
      }
      else {
        $depfile = "failed to parse libname from '$target'";
      }
    }
    else {
      my $lastdir    = filenameOf($dir);
      my $defaultLib = $dir.'/'.$lastdir.'.a';

      if ($target eq $defaultLib) {
        $depfile = $dir.'/'.$libdepend_file;
      }
      else {
        my $targetName =  filenameOf($target);
        $targetName    =~ s/\.(so|a|o)$/_$1/o;
        $depfile       =  $dir.'/'.$libdepend_file.'.'.$targetName;
      }
    }
  }
  else { # no dir -> must be executable
    $depfile = $exedepend_dir.'/'.$libdepend_file.'.'.$target;
  }
  return $depfile;
}

# --------------------------------------------------------------------------------

my $errors = 0;

sub if_errors_abortNow() {
  if ($errors>0) { die "Errors occurred - abort!\n"; }
}

sub show_location($$$) {
  my ($location,$message,$type) = @_;
  if (defined $location) { print STDERR $location.': '.$type.': '.$message."\n"; }
  else { print STDERR '(unknown source): '.$type.': '.$message."\n"; }
}
sub location_error($$) {
  my ($location,$error) = @_;
  show_location($location,$error,'Error');
  $errors++;
}
sub location_warning($$) {
  my ($location,$warning) = @_;
  show_location($location,$warning,'Warning');
}

# --------------------------------------------------------------------------------

# values for %target_type:
my $DYNAMIC_LIB  = 0;
my $STATIC_LIB   = 1;
my $EXECUTABLE   = 2;
my $EXTRA_PARAMS = 3;

my %inheritants_of      = (); # key=target; value=ref to hash (key=inheritant; value=location)
my %target_type         = (); # key=target; value: see above
my %dependencies_of     = (); # key=target; value=ref to hash (containing direct dependencies)
my %all_dependencies_of = (); # key=target; value=ref to hash (containing all dependencies)

sub get_inheritance_location($$) {
  my ($inheritor,$dependency) = @_;
  my $inh_r = $inheritants_of{$dependency};
  return $$inh_r{$inheritor};
}

sub warn_bad_dependency($) {
  my ($target) = @_;

  my $directly_depends_on_r = $dependencies_of{$target};
  foreach my $sub (keys %$directly_depends_on_r) {
    my $sub_depends_r = $all_dependencies_of{$sub};

    # check multi dependencies:
    foreach my $some_dep (keys %$sub_depends_r) {
      if (defined $$directly_depends_on_r{$some_dep}) { # multi dependency
        my $some_dep_location = get_inheritance_location($target,$some_dep);
        location_warning($some_dep_location, "Duplicated dependency on '$some_dep'");

        my $sub_location = get_inheritance_location($target,$sub);
        location_warning($sub_location, "(already depends on '$some_dep' via '$sub'");
      }
    }

    # check cyclic dependencies:
    if (defined $$sub_depends_r{$target}) {
      my $sub_location = get_inheritance_location($target,$sub);
      location_warning($sub_location, "Cyclic dependency between $target ..");
      my $target_location = get_inheritance_location($sub,$target);
      location_warning($sub_location, ".. and $sub");
    }
  }
}

sub warn_bad_dependencies() {
  foreach (keys %inheritants_of) {
    warn_bad_dependency($_);
  }
}

# --------------------------------------------------------------------------------

sub dump_dependencies() {
  foreach my $dependency (sort keys %inheritants_of) {
    print STDERR "------------------------------------ inheriting $dependency\n";
    my $inherit_locations_r = $inheritants_of{$dependency};
    foreach my $inheritant (keys %$inherit_locations_r) {
      my $location = $$inherit_locations_r{$inheritant};
      location_warning($location, "'$dependency' inherited by '$inheritant'");
    }
  }
}

# --------------------------------------------------------------------------------

sub prefix($$) {
  my ($maybePrefix,$path) = @_;
  if (defined $maybePrefix) { $maybePrefix.'/'.$path; }
  else { $path; }
}

sub dynamic_name($) {
  my ($target) = @_;
  my $name = filenameOf($target);
  if ($name =~ /^lib(.*)\.so$/o) { $1; }
  else { undef; }
}

sub assert_defined($) {
  my ($val) = @_;
  use Carp;
  defined $val || Carp::confess("undef");
  return $val;
}

sub is_dynamic_lib($) { my ($target) = @_; return $target_type{$target} == $DYNAMIC_LIB; }
sub is_static_lib($)  { my ($target) = @_; return assert_defined($target_type{$target}) == $STATIC_LIB; }
sub is_extra_param($) { my ($target) = @_; return $target_type{$target} == $EXTRA_PARAMS; }
sub is_executable($)  { my ($target) = @_; return $target_type{$target} == $EXECUTABLE; }

sub is_library($)  { my ($target) = @_; return is_static_lib($target) || is_dynamic_lib($target); }
sub is_file($)     { my ($target) = @_; return is_library($target) || is_executable($target); }

# --------------------------------------------------------------------------------

sub declare_initial_target($$) {
  my ($target,$type) = @_;

  if ($target ne '') {
    legal_target_or_die($target);

    my $inherit_locations_r = $inheritants_of{$target};
    if (not defined $inherit_locations_r) {
      my %new_hash = ();
      $inheritants_of{$target} = \%new_hash;
      $target_type{$target} = $type;
    }
    else {
      $target_type{$target}==$type || die "type mismatch for initial '$target' (".$target_type{$target}." vs $type)\n";
    }
  }
}

sub detect_target_type($) {
  my ($target) = @_;

  if ($target =~ /\.(so|a|o)$/o) {
    if ($1 eq 'so') { $DYNAMIC_LIB; }
    else { $STATIC_LIB; }
  }
  else { $EXECUTABLE; }
}

sub detect_type_and_declare_initial_target($) {
  my ($target) = @_;

  my $type = detect_target_type($target);
  declare_initial_target($target,$type);
}

# --------------------------------------------------------------------------------

sub scan_dependsfile_entry($$$) {
  my ($line, $inheritant, $location) = @_;

  my $analyse = 1;
  my $type = undef;

  if ($line =~ /^\$\((.*)\)$/o) {
    my $envvar = $1;
    my $env = $ENV{$envvar};
    if (not defined $env) {
      if ($dieOnUndefEnvvar==1) { die "Undefined environment variable '$envvar'\n"; }
      $type = $EXTRA_PARAMS;
      $analyse = 0;
    }
    else {
      $line = $env;
    }
  }

  if ($analyse==1) {
    if ($line =~ /-[lL]/o) {
      $type = $EXTRA_PARAMS;
    }
    else {
      $line = ARBHOME_relative($line);
      $type = detect_target_type($line);
    }
    $analyse = 0;
  }

  defined $type || die "could not detect line type\n";

  my $target = $line;
  declare_initial_target($target,$type);

  my $inherit_locations_r = $inheritants_of{$target};
  $$inherit_locations_r{$inheritant} = $location;
}


my %scanned = (); # key=dependsfile, value=1 -> already scanned

sub scan_target($) {
  my ($target) = @_;

  my $depfile = target2dependencyFile($target);

  if (not defined $scanned{$depfile}) {
    $scanned{$depfile} = 1;

    my $fulldep = fullpath($depfile);

    if (not -f $fulldep) {
      my $dir = dirOf($fulldep);
      my $err = "Missing dependency file for '$target'";
      if (not -d $dir) { $err .= " (No such directory '$dir')"; }
      my $location = $fulldep.':0';
      location_error($location,$err);
    }
    else {
      open(IN,'<'.$fulldep) || die "can't read '$fulldep' (Reason: $!)";
      my $lineno = 0;
      foreach (<IN>) {
        $lineno++;
        chomp;
        if ($_ =~ /\#/) { $_ = $`; } # remove comments
        $_ = trim($_); # remove whitespace

        if ($_ ne '') {  # skip empty lines
          my $location = $fulldep.':'.$lineno;
          eval {
            scan_dependsfile_entry($_, $target, $location);
          };
          if ($@) {
            location_error($location, $@);
          }
        }
      }
      close(IN);
    }
  }
}

sub rec_add_depends_to_hash($\%);
sub rec_add_depends_to_hash($\%) {
  my ($target,$hash_r) = @_;

  my $all_dep_r = $all_dependencies_of{$target};
  if (defined $all_dep_r) {
    foreach (keys %$all_dep_r) {
      $$hash_r{$_} = 1;
    }
  }
  else {
    my $dep_r = $dependencies_of{$target};
    foreach (keys %$dep_r) {
      if (not defined $$hash_r{$_}) {
        $$hash_r{$_} = 1;
        rec_add_depends_to_hash($_,%$hash_r);
      }
    }
  }
}

sub resolve_dependencies() {
  my $done = 0;
  my $target;
  while ($done==0) {
    my %unscanned = ();
    foreach my $target (keys %inheritants_of) {
      if ($target_type{$target} != $EXTRA_PARAMS) { # recurse dependency
        my $depfile = target2dependencyFile($target);
        if (not defined $scanned{$depfile}) {
          if ($target eq '') {
            my $inh_r = $inheritants_of{$target};
            foreach (keys %$inh_r) {
              my $loc = $$inh_r{$_};
              location_error($loc, "Empty target inherited from here");
            }
          }
          else {
            $unscanned{$target} = 1;
          }
        }
      }
    }

    my @unscanned = sort keys %unscanned;
    if (scalar(@unscanned)) {
      foreach $target (@unscanned) { scan_target($target); }
    }
    else {
      $done = 1;
    }
  }

  # build %dependencies_of

  foreach $target (keys %inheritants_of) {
    my %new_hash = ();
    $dependencies_of{$target} = \%new_hash;
  }
  foreach $target (keys %inheritants_of) {
    my $inherit_locations_r = $inheritants_of{$target};
    foreach my $inheritant (keys %$inherit_locations_r) {
      if ($target eq $inheritant) {
        my $self_include_loc = get_inheritance_location($inheritant, $target);
        location_error($self_include_loc, "Illegal self-dependency");
      }
      my $dep_r = $dependencies_of{$inheritant};
      $$dep_r{$target} = 1;
    }
  }

  if_errors_abortNow();

  # build %all_dependencies_of

  my @targets = keys %dependencies_of;
  my %depends_size = map {
    my $dep_r = $dependencies_of{$_};
    $_ => scalar(keys %$dep_r);
  } @targets;

  @targets = sort { $depends_size{$a} <=> $depends_size{$b}; } @targets; # smallest first

  foreach $target (@targets) {
    my %new_hash = ();
    rec_add_depends_to_hash($target,%new_hash);
    $all_dependencies_of{$target} = \%new_hash;
  }
}

# --------------------------------------------------------------------------------

my $reg_is_libdepend_file = qr/^($libdepend_file|$libdepend_file\..*)$/;

sub find_dep_decl_files($\@);
sub find_dep_decl_files($\@) {
  my ($dir,$found_r) = @_;
  $dir =~ s/\/\//\//og;
  my @subdirs = ();
  opendir(DIR,$dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $dir.'/'.$_;
      if (not -l $full) {
        if (-d $full) {
          if (not $_ =~ /\.svn$/o) { push @subdirs, $full; }
        }
        elsif ($_ =~ $reg_is_libdepend_file) { push @$found_r, ARBHOME_relative($full); }
      }
    }
  }
  closedir(DIR);
  foreach (@subdirs) { find_dep_decl_files($_,@$found_r); }
}

my %dep2lib = (
               'ARBDB/needs_libs.libARBDB_so'   => 'lib/libARBDB.so',
               'AWT/needs_libs.libAWT_so'       => 'lib/libAWT.so',
               'CORE/needs_libs.libCORE_so'     => 'lib/libCORE.so',
               'WINDOW/needs_libs.libWINDOW_so' => 'lib/libWINDOW.so',

               'GL/glAW/needs_libs.libglAW_a'       => 'GL/glAW/libglAW.a',
               'GL/glpng/needs_libs.libglpng_arb_a' => 'GL/glpng/libglpng_arb.a',

               'PROBE_COM/needs_libs.client_a' => 'PROBE_COM/client.a',
               'PROBE_COM/needs_libs.server_a' => 'PROBE_COM/server.a',
               'NAMES_COM/needs_libs.client_a' => 'NAMES_COM/client.a',
               'NAMES_COM/needs_libs.server_a' => 'NAMES_COM/server.a',

               'ptpan/needs_libs.PROBE_a' => 'ptpan/PROBE.a',
              );

sub libdepend_file_2_libname($) {
  my ($dep) = @_;
  if (defined $dep2lib{$dep}) {
    $dep = $dep2lib{$dep};
  }
  elsif ($dep =~ /^(.*)\/([^\/]*)/) {
    my ($dir,$name) = ($1,$2);
    if ($name =~ $reg_is_libdepend_file) {
      if ($dep =~ /BINDEP\/needs_libs\./) {
        $dep = $';
      }
      else {
        my ($prefix,$targetdir) = ('',undef);

        if ($dir =~ /^(.*)\/([^\/]*)/) {
          my ($path,$lastdir) = ($1,$2);
          $path =~ s/^.\///;
          $prefix = $path.'/';
          $targetdir = $lastdir;
        }
        else {
          $targetdir = $dir;
        }

        my $target = undef;
        if ($name =~ /^needs_libs\.(.*)_o$/) {
          $target = $targetdir.'/'.$1.'.o';
        }
        else {
          $target = $targetdir.'/'.$targetdir.'.a';
        }

        $dep = $prefix.$target;
      }
    }
  }
  return $dep;
}

sub declare_all_targets() {
  my @depfiles = ();
  find_dep_decl_files($ARBHOME,@depfiles);
  foreach my $depfile (sort @depfiles) {
    my $target = dependencyFile2target($depfile);
    detect_type_and_declare_initial_target($target);
  }
}

# --------------------------------------------------------------------------------

sub dot_label($) {
  my ($target) = @_;
  if (is_file($target)) { $target = filenameOf($target); }
  return '"'.$target.'"';
}

sub generateDependencyGraph(\@\@$) {
  my ($depends_r,$targets_r,$gif) = @_;

  my $base;
  if ($gif =~ /\.gif$/) {
    $base = $`;
  }
  else {
    $base = $gif;
    $gif = $base.'.gif';
  }

  my $dotfile = $base.'.dot';
  my $maxsize = 17; # inch

  open(DOT,'>'.$dotfile) || die "can't write to '$dotfile' (Reason: $!)";
  print DOT "digraph ARBLIBDEP {\n";
  print DOT "  rankdir=LR;\n";
  print DOT "  concentrate=true;\n";
  print DOT "  searchsize=1000;\n";
  print DOT "  Damping=2.0;\n";
  print DOT "  size=\"$maxsize,$maxsize\";\n";
  print DOT "  orientation=portrait;\n";
  print DOT "\n";

  my %wanted_depend = map { $_ => 1; } @$depends_r;

  my $seen_binary = 0;

  my %execount = ();

  foreach my $exe (keys %inheritants_of) {
    if ($target_type{$exe}==$EXECUTABLE) {
      my $deps_r = $all_dependencies_of{$exe};
      foreach (keys %$deps_r) {
        my $count = $execount{$_};
        $count = (defined $count) ? $count+1 : 1;
        $execount{$_} = $count;
      }
    }
  }

  my %is_target = map { $_ => 1; } @$targets_r;

  foreach my $target (@$depends_r) {
    my $deps_r = $dependencies_of{$target};
    my $type = $target_type{$target};

    if (not defined $type) {
      use Carp;
      Carp::confess("no target_type for '$target'");
    }
    
    my $dtarget = dot_label($target);

    my $color = 'black';
    my $shape = 'box';
    my $style = '';

    if ($type==$EXECUTABLE)   { $color = 'green'; $shape = 'ellipse'; }
    if ($type==$DYNAMIC_LIB)  { $color = 'blue'; }
    if ($type==$STATIC_LIB)   { ; }
    if ($type==$EXTRA_PARAMS) { $color = 'red'; $shape = 'box'; }

    if ($is_target{$target}) {
      $color = '"#ffbb66"';
      $style = 'style=filled';
    }

    my $peri = $execount{$target};
    $peri = defined $peri ? $peri : 1;

    print DOT '    '.$dtarget.' [';
    print DOT ' color='.$color;
    print DOT ' shape='.$shape;
    print DOT ' peripheries='.$peri;
    print DOT ' '.$style;
    print DOT '];'."\n";

    if ($type==$EXECUTABLE) {
      if ($seen_binary==0) {
        print DOT '    executables [';
        print DOT ' color=yellow';
        print DOT '];'."\n";
        $seen_binary = 1;
      }
      print DOT '    executables -> '.$dtarget.';'."\n";
    }

    foreach my $depend (keys %$deps_r) {
      if (defined $wanted_depend{$depend}) {
        my $ddepend = dot_label($depend);
        print DOT '    '.$dtarget.' -> '.$ddepend.';'."\n";
      }
    }
  }
  print DOT "}\n";
  close(DOT);
  print STDERR "Wrote $dotfile\n";

  my $cmd = 'dot -Tgif -o'.$gif.' '.$dotfile;
  print STDERR $cmd."\n";
  system($cmd)==0 || die "Failed to execute '$cmd'";

  unlink($dotfile) || die "Failed to unlink '$dotfile' (Reason: $!)";
}

# --------------------------------------------------------------------------------

sub pushDirsTo($\@\@) {
  my ($pathPrefix,$depends_r,$out_r) = @_;
  my @out = ();
  foreach my $dep (@$depends_r) {
    if (is_static_lib($dep)) {
      my $dir  = dirOf($dep);
      defined $dir || die "no dir for '$dep'";
      my $name = filenameOf($dir);
      push @out, prefix($pathPrefix,$dir.'/'.$name);
    }
    elsif (is_dynamic_lib($dep)) {
      if ($dep =~ /^lib\/lib([A-Z]+)\.so/) {
        my $base = $1;
        push @out, prefix($pathPrefix,$base.'/'.$base);
      }
      else {
        die "malformed (unexpected) dependency name for shared lib ('$dep')";
      }
    }
  }

  foreach (sort @out) { push @$out_r, $_; }
}

sub pushFilesTo($\@\@) {
  my ($pathPrefix,$depends_r,$out_r) = @_;
  foreach my $dep (@$depends_r) {
    if (is_file($dep)) {
      push @$out_r, fixFileSuffix(prefix($pathPrefix,$dep));
    }
  }
}

sub sortLibsByDependency(\@) {
  # sort libs for linker CL (fullfilling libs behind requesting libs)
  my ($libs_r) = @_;
  my @order = ();
  my %added = map { $_ => 0; } @$libs_r;

  while (scalar(@order) < scalar(@$libs_r)) {
    my $didAdd = 0;
    foreach my $lib (@$libs_r) {
      if ($added{$lib}==0) { # not added yet
        my $all_dep_r = $all_dependencies_of{$lib};
        my $missing = 0;
      CHECK: foreach my $dep (keys %$all_dep_r) {
          my $was_added = $added{$dep};
          if (defined $was_added) {
            if ($was_added==0) { $missing = 1; last CHECK; }
          }
        }
        if ($missing==0) {
          push @order, $lib;
          $added{$lib} = 1;
          $didAdd = 1;
        }
      }
    }
    if ($didAdd==0) {
      die "internal error: did not add any lib in last loop (undetected nested dependency?)";
    }
  }
  @$libs_r = reverse @order;
}

sub pushStaticLibsTo($\@\@) {
  my ($pathPrefix,$depends_r,$out_r) = @_;
  my @libs = ();
  foreach my $dep (@$depends_r) { if (is_static_lib($dep)) { push @libs, $dep; } }
  sortLibsByDependency(@libs);
  foreach (@libs) { push @$out_r, prefix($pathPrefix,$_); }
}

sub pushDynamicLibsTo(\@\@) {
  my ($depends_r,$out_r) = @_;
  my @dep = ();
  foreach my $dep (@$depends_r) { if (is_dynamic_lib($dep)) { push @dep, $dep; } }
  sortLibsByDependency(@dep);
  foreach (@dep) { push @$out_r, '-l'.dynamic_name($_); }
  foreach my $dep (@$depends_r) { if (is_extra_param($dep)) { push @$out_r, $dep; } }
}

# --------------------------------------------------------------------------------

sub die_usage($) {
  my ($err) = @_;
  print "Usage: needed_libs.pl [options] [lib|obj|executable]\n";
  print "Scans directories of 'lib|obj's for files named '$libdepend_file' or '$libdepend_file.libname'\n";
  print "In case of an executable, it scans for a file named 'BINDEP/$libdepend_file.executable'.\n";
  print "\n";
  print "These files have to contain the name(s) of the libraries needed by 'lib', either\n";
  print "as ARBHOME-relative path to the lib file or\n";
  print "as link instruction (containing '-l') for non-ARB-libraries\n";
  print "\n";
  print "Libraries in directory '\$ARBHOME/lib' are handled special:\n";
  print "- they need to be specified as lib/libNAME.so\n";
  print "- their source directory is assumed to be \$ARBHOME/NAME\n";
  print "- they are called 'dynamic' in the context of this script (even if \$LINK_STATIC=1, where they aren't dynamic)\n";
  print "\n";
  print "They may as well contain \${ENVVAR}\n";
  print "If no '$libdepend_file' exists, the 'lib' is assumed to be independent.\n";
  print "\n";
  print "If the filename is 'DIR/$libdepend_file', the library is assumed to be 'DIR/DIR.a'\n";
  print "\n";
  print "options:\n";
  print "  -D          output dirs lib depends on (e.g. ARBDB/ARBDB SL/ARB_TREE/ARB_TREE ..)\n";
  print "  -F          output all libfiles the given lib depends on (e.g. ARBDB/libARBDB.so SL/ARB_TREE/ARB_TREE.a ..)\n";
  print "              (suffix used for \"dynamic\" libs depends on setting of \$LINK_STATIC)\n";
  print "  -l          output link params (e.g. -lARBDB SL/ARB_TREE/ARB_TREE.a ..)\n";
  print "  -s          output names of \"static\" libs (e.g. SL/ARB_TREE/ARB_TREE.a ..)\n";
  print "  -d          output names of \"dynamic\" libs (e.g. -lARBDB ..)\n";
  print "  -A dir      prefix paths with 'dir' (not for \"dynamic\" libs)\n";
  print "  -U          do not die on undefined environment variables\n";
  print "  -G outgif   Draw dependency graph (using dot)\n";
  print "  -V          Dump dependencies\n";
  print "  -n          do not print nested dependencies\n";
  print "  -I          Invert (print inheritants instead of dependencies)\n";
  print "  -B          Both (print inheritants and dependencies)\n";
  print "  -S          Include self (lib/exe given on command line)\n";
  print "  -T          only show terminals (=those w/o dependencies and/or inheritants)\n";
  die "Error: $err\n";
}


sub main() {
  my @targets = ();

  my $printDirs       = 0;
  my $printFiles      = 0;
  my $printStatic     = 0;
  my $printDynamic    = 0;
  my $pathPrefix      = undef;
  my $dependencyGraph = undef;
  my $dump            = 0;
  my $trackDepends    = 1;
  my $nestDepends     = 1;
  my $trackInherits   = 0;
  my $includeSelf     = 0;
  my $onlyTerminals   = 0;

  while (scalar(@ARGV)) {
    $_ = shift @ARGV;
    if ($_ =~ /^-/o) {
      my $switch = $';
      if    ($switch eq 'D') { $printDirs = 1; }
      elsif ($switch eq 'F') { $printFiles = 1; }
      elsif ($switch eq 'd') { $printDynamic = 1; }
      elsif ($switch eq 's') { $printStatic = 1; }
      elsif ($switch eq 'l') { $printStatic = 1; $printDynamic = 1; }
      elsif ($switch eq 'A') { $pathPrefix = shift @ARGV; }
      elsif ($switch eq 'U') { $dieOnUndefEnvvar = 0; }
      elsif ($switch eq 'G') { $dependencyGraph = shift @ARGV; }
      elsif ($switch eq 'V') { $dump = 1; }
      elsif ($switch eq 'n') { $nestDepends = 0; }
      elsif ($switch eq 'I') { $trackInherits = 1; $trackDepends = 0; }
      elsif ($switch eq 'B') { $trackInherits = 1; }
      elsif ($switch eq 'S') { $includeSelf = 1; }
      elsif ($switch eq 'T') { $onlyTerminals = 1; }
      elsif ($switch eq '?' or $switch eq 'help' or $switch eq 'h') { die_usage('help requested'); }
      else { die "unknown switch '-$switch'"; }
    }
    else {
      if ($_ ne '') { push @targets, $_; }
    }
  }

  my $targets = scalar(@targets);

  if ($targets>0) {
    if ($targets>1) {
      if (not defined $dependencyGraph) {
        die_usage("You may only specify ONE target! (only with -G multiple targets are possible)");
      }
    }
    @targets = map { $_ = libdepend_file_2_libname($_); } @targets;
    @targets = map { $_ = ARBHOME_relative($_); } @targets;
    if ($trackDepends==1 and $trackInherits==0) {
      foreach (@targets) {
        detect_type_and_declare_initial_target($_);
      }
    }
    else {
      declare_all_targets();
    }
  }
  else {
    declare_all_targets();
  }

  resolve_dependencies();
  warn_bad_dependencies();
  if_errors_abortNow();

  my %track = ();
  if ($trackDepends==1) {
    if ($nestDepends==1) {
      foreach (@targets) {
        my $dep_r = $all_dependencies_of{$_};
        foreach (keys %$dep_r) { $track{$_} = 1; }
      }
    }
    else {
      foreach (@targets) {
        my $dep_r = $dependencies_of{$_};
        foreach (keys %$dep_r) { $track{$_} = 1; }
      }
    }
  }
  if ($trackInherits==1) {
    if ($targets>0) {
      foreach my $inheritant (keys %inheritants_of) {
        my $dep_r = $all_dependencies_of{$inheritant};
        foreach (@targets) {
          if (defined $$dep_r{$_}) { $track{$inheritant} = 1; }
        }
      }
    }
  }

  if ($targets>0) {
    if ($includeSelf==1) {
      foreach (@targets) {
        $track{$_} = 1;
      }
    }
  }
  else {
    foreach (keys %inheritants_of) { $track{$_} = 1; }
  }

  if ($onlyTerminals==1) {
    if ($trackInherits==1) {
      # untrack all which have inheritants
      foreach (keys %track) {
        my $hash_r = $inheritants_of{$_};
        if ((defined $hash_r) and (scalar(keys %$hash_r)>0)) {
          delete $track{$_};
        }
      }
    }
    if ($trackDepends==1) {
      # untrack all which have dependencies
      foreach (keys %track) {
        my $hash_r = $dependencies_of{$_};
        if ((defined $hash_r) and (scalar(keys %$hash_r)>0)) {
          delete $track{$_};
        }
      }
    }
  }

  my @track = sort keys %track;

  if ($dump==1) { dump_dependencies(); }
  if (defined $dependencyGraph) { generateDependencyGraph(@track,@targets,$dependencyGraph); }

  my @out = ();
  if ($printDirs==1) { pushDirsTo($pathPrefix,@track,@out); }
  if ($printFiles==1) { pushFilesTo($pathPrefix,@track,@out); }

  {
    my @libs = ();
    if ($printStatic==1) { pushStaticLibsTo($pathPrefix,@track,@libs); }
    if ($printDynamic==1) { pushDynamicLibsTo(@track,@libs); }

    foreach (@libs) { push @out, $_; }
  }

  if (scalar(@out)>0) { print join(' ',@out)."\n"; }
}

main();
