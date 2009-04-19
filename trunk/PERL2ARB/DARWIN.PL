use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'         => 'ARB',
    'VERSION_FROM' => 'ARB.pm',               # finds $VERSION
    'LIBS'         => '-L../ARBDB2 -lARBDB',  # e.g., '-lm' 
    'DEFINE'       => '',                     # e.g., '-DHAVE_SOMETHING' 
    'INC'          => '-I../INCLUDE',         # e.g., '-I/usr/include/other' 
);
