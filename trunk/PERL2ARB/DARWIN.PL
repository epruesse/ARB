use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details on how to influence
# the contents of the Makefile that gets written.

# called from Makefile.main@buildMakefile

my $args = scalar(@ARGV);
if ($args!=2) {
  die "Usage: DARWIN.PL compiler-flags linker-flags\n".
    "Missing arguments";
}

my $cflags = shift @ARGV;
my $lflags = shift @ARGV;

use arb_common;

WriteMakefile(
              'NAME'         => $arb_common::name,
              'VERSION_FROM' => $arb_common::version_from,
              'CC'           => $ENV{'A_CXX'},
              'CCFLAGS'      => $cflags,
              'LD'           => $ENV{'A_CXX'}.' '.$lflags,
              'DEFINE'       => '',
              'INC'          => $arb_common::inc,
              'LIBS'         => $arb_common::libs,
             );


