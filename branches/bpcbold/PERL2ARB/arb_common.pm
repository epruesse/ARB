# defines common settings for
#     LINUX.PL
# and DARWIN.PL

package arb_common;

our $name         = 'ARB';
our $version_from = 'ARB.pm';
our $inc          = '-I../INCLUDE';
our $libs         = '-L../CORE -L../ARBDB -lCORE -lARBDB -lstdc++';



