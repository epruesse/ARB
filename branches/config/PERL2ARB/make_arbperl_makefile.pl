use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details on how to influence
# the contents of the Makefile that gets written.

# called from Makefile.main@buildMakefile

my $name         = 'ARB';
my $version_from = 'ARB.pm';
my $inc          = '-I../INCLUDE';

my $compiler = $ENV{'A_CXX'};
my $execlibs = $ENV{'EXECLIBS'};

my $libs;
{
  # Explicitly add directories from LD_LIBRARY_PATH to linker command.
  #
  # otherwise generated Makefile overrides dynamic linking of libstdc++ via LD_RUN_PATH
  # and make it impossible to build with non-system gcc (e.g. in /opt/gcc-4.7.1/..)

  my $LD_LIBRARY_PATH = $ENV{LD_LIBRARY_PATH};
  if (not defined $LD_LIBRARY_PATH) { die "LD_LIBRARY_PATH is undefined"; }

  $libs = '';
  foreach (split /:/,$LD_LIBRARY_PATH) {
    if (($_ ne '') and (-d $_)) { $libs .= ' -L'.$_; }
  }
  $libs =~ s/^\s+//;
  $libs .= ' -lCORE -lARBDB -lstdc++ '.$execlibs.' ';
}

my $args = scalar(@ARGV);
if ($args!=2) {
  die "Usage: make_arbperl_makefile.pl compiler-flags linker-flags\n".
    "Error: missing arguments";
}

my $cflags = shift @ARGV;
my $lflags = shift @ARGV;

WriteMakefile(
              'NAME'         => $name,
              'VERSION_FROM' => $version_from,
              'CC'           => $compiler,
              'CCFLAGS'      => $cflags,
              'LD'           => $compiler,
              'LDDLFLAGS'    => $lflags,
              'DEFINE'       => '',
              'INC'          => $inc,
              'LIBS'         => $libs,
             );

