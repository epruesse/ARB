use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details on how to influence
# the contents of the Makefile that gets written.
WriteMakefile(
              'NAME'	     => 'ARB',
              'VERSION_FROM' => 'ARB.pm',
              'CC'	     => 'g++',
              'DEFINE'	     => '',
              'INC'	     => '-I../INCLUDE',
              'LIBS'	     => '-L../ARBDB -lARBDB -lstdc++',
             );


