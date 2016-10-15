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

my @alternative = ();
my $ARBHOME = $ENV{ARBHOME};

sub format_asan_line($$) {
  my ($line,$topdir) = @_;
  # if $line is output from AddressSanitizer/LeakSanitizer -> reformat into 'file:lineno: msg' format
  # return undef if isnt (or source file is unknown)
  # if $topdir!=undef => remove topdir-prefix from path

  my $result = undef;

  my $dump_alternatives = 1;

  if ($line =~ /^\s+(\#[0-9]+\s.*)\s+(.*):([0-9]+)$/o) {
    my ($msg,$file,$lineNo) = ($1,$2,$3);
    if (-f $file) {
      if ((defined $topdir) and ($file =~ /^$topdir\//)) { $file = $'; }
      $result = "$file:$lineNo: $msg\n";
    }
    elsif ($file =~ /^(C|GENC|GENH)\//o) {
      my $PROBE_file = 'PROBE_COM/'.$file;
      my $NAMES_file = 'NAMES_COM/'.$file;

      my $base = (defined $topdir) ? $topdir : $ARBHOME;
      $PROBE_file = $base.'/'.$PROBE_file;
      $NAMES_file = $base.'/'.$NAMES_file;

      $dump_alternatives = 0; # do later

      if (-f $PROBE_file) {
        if ((defined $topdir) and ($PROBE_file =~ /^$topdir\//)) { $PROBE_file = $'; }
        $result = "$PROBE_file:$lineNo: $msg\n";
      }
      if (-f $NAMES_file) {
        if ((defined $topdir) and ($NAMES_file =~ /^$topdir\//)) { $NAMES_file = $'; }
        push @alternative, "$NAMES_file:$lineNo: $msg";
      }
    }
  }

  if ($dump_alternatives==1 and scalar(@alternative)>0) {
    my $pre_result .= "alternative callstack [start]\n";
    foreach (@alternative) {
      $pre_result .= $_."\n";
    }
    $pre_result .= "alternative callstack [end]\n";
    @alternative = ();
    $result = $pre_result.$result;
  }

  return $result;
}


# ----------------------------------------
# cleanup (if needed)
END { }

1; # module initialization is ok
