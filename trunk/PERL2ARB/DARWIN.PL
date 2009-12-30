use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details on how to influence
# the contents of the Makefile that gets written.
WriteMakefile(
              'NAME'         => 'ARB',
              'VERSION_FROM' => 'ARB.pm',
              'CC'           => 'g++',
              'CCFLAGS'      => '-arch x86_64',
              'DEFINE'       => '',
              'INC'          => '-I../INCLUDE',
              'LIBS'         => '-L../ARBDB -lARBDB -lstdc++ -L/System/Library/Perl/lib/5.10/ -lperl', #System provides 64-bit Perl with Snow Leopard
              'LDDLFLAGS'    => '-bundle -flat_namespace',
             );
