use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'         => 'ARB',
    'VERSION_FROM' => 'ARB.pm',               # finds $VERSION
   #'LIBS'         => '-L../ARBDB2 -lARBDB -L/usr/local/lib/perl5/5.10.0/darwin-2level/CORE/ -lperl',  # e.g., '-lm' 
    'LIBS'         => '-L../ARBDB2 -lARBDB -L/System/Library/Perl/lib/5.10/ -lperl', #System provides 64-bit Perl with Snow Leopard
    'DEFINE'       => '',                     # e.g., '-DHAVE_SOMETHING' 
    'INC'          => '-I../INCLUDE',         # e.g., '-I/usr/include/other' 
    'CCFLAGS'      => '-arch x86_64',
    'LDDLFLAGS'    => '-bundle -flat_namespace',
);
