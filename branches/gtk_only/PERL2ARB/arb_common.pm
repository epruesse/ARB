# defines common settings for
#     LINUX.PL
# and DARWIN.PL

package arb_common;

our $name         = 'ARB';
our $version_from = 'ARB.pm';
our $inc          = '-I../INCLUDE';
our $libs;
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
  $libs .= ' -lCORE -lARBDB -lstdc++';
}



