package arb_build_common;

# meant to be used from perl scripts used during build of ARB

use strict;
use warnings;

BEGIN {
  use Exporter ();
  our ($VERSION,@ISA,@EXPORT,@EXPORT_OK,%EXPORT_TAGS);
  $VERSION     = 1.00;
  @ISA         = qw(Exporter);
  @EXPORT      = qw(
                    &format_asan_line
                   );
  %EXPORT_TAGS = qw();
  @EXPORT_OK   = qw();
}

# ----------------------------------------

sub format_asan_line($$) {
  my ($line,$topdir) = @_;
  # if $line is output from AddressSanitizer/LeakSanitizer -> reformat into 'file:lineno: msg' format
  # return undef if isnt (or source file is unknown)
  # if $topdir!=undef => remove topdir-prefix from path

  my $result = undef;

  if ($line =~ /^\s+(\#[0-9]+\s.*)\s+(.*):([0-9]+)$/o) {
    my ($msg,$file,$lineNo) = ($1,$2,$3);
    if (-f $file) {
      if ((defined $topdir) and ($file =~ /^$topdir\//)) {
        $file = $';
      }
      $result = "$file:$lineNo: $msg";
    }
    elsif ($file =~ /^C\//o) {
      $result  = "PROBE_COM/$file:$lineNo: $msg\n";
      $result .= "NAMES_COM/$file:$lineNo: [or here]";
    }
  }
  return $result;
}


# ----------------------------------------
# cleanup (if needed)
END { }

1; # module initialization is ok
