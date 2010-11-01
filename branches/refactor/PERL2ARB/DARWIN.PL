use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details on how to influence
# the contents of the Makefile that gets written.

print "DARWIN.PL:5: Warning: Please inspect comments here\n";
# DARWIN.PL is now called with two parameters containing compiler- and linker-flags.
# It would be better to use them instead of hardcoding values below
# (see LINUX.PL for recommended usage).
# Please remove this comment when done.

use arb_common;

WriteMakefile(
              'NAME'         => $arb_common::name,
              'VERSION_FROM' => $arb_common::version_from,
              'CC'           => 'g++',
              'CCFLAGS'      => '-arch x86_64',
              'DEFINE'       => '',
              'INC'          => $arb_common::inc,
              'LIBS'         => $arb_common::libs.' -L/System/Library/Perl/lib/5.10/ -lperl', #System provides 64-bit Perl with Snow Leopard
              'LDDLFLAGS'    => '-bundle -flat_namespace',
             );
