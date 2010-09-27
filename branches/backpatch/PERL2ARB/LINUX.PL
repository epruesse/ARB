use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details on how to influence
# the contents of the Makefile that gets written.

# called from Makefile.main@buildMakefile

my $args = scalar(@ARGV);
if ($args!=2) {
  die "Usage: LINUX.PL compiler-flags linker-flags\n".
    "Missing arguments";
}

my $cflags = shift @ARGV;
my $lflags = shift @ARGV;

WriteMakefile(
              'NAME'         => 'ARB',
              'VERSION_FROM' => 'ARB.pm',
              'CC'           => 'g++',
              'CCFLAGS'      => $cflags,
              'LD'           => 'g++ '.$lflags,
              'DEFINE'       => '',
              'INC'          => '-I../INCLUDE',
              'LIBS'         => '-L../ARBDB -lARBDB -lstdc++',
             );


