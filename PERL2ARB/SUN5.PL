use ExtUtils::MakeMaker WriteMakefile;
#use ExtUtils::MakeMaker;
print "\@INC:\n";
print @INC;
print "\n";
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'	=> 'ARB',
    'VERSION_FROM' => 'ARB.pm', # finds $VERSION
    'LIBS'	=> '-L../ARBDB2 -lARBDB -lgen',   # e.g., '-lm' 
    'DEFINE'	=> '',     # e.g., '-DHAVE_SOMETHING' 
    'INC'	=> '-I../INCLUDE',     # e.g., '-I/usr/include/other' 
);
