package ali_dnapro;

# meant to be used from inside macros
# see ../../HELP_SOURCE/oldhelp/macro.hlp@ali_dnapro

use strict;
use warnings;

use lib "$ENV{'ARBHOME'}/lib/";
use ARB;

BEGIN {
  use Exporter ();
  our ($VERSION,@ISA,@EXPORT,@EXPORT_OK,%EXPORT_TAGS);
  $VERSION     = 1.00;
  @ISA         = qw(Exporter);
  @EXPORT      = qw(
                    &get_dnapro_alignments
                   );
  %EXPORT_TAGS = qw();
  @EXPORT_OK   = qw();
}

sub caps($);
sub caps($) {
  my ($reg) = @_;
  if ($reg =~ /\|/) {
    return join('|', map {
      die if /\|/;
      caps($_);
    } split(/\|/,$reg));
  }
  return uc(substr($reg,0,1)).substr($reg,1);
}

sub mod_name($$$) {
  my ($name,$old,$new) = @_;
  if ($name =~ /$old/) { return $`.$new.$'; }
  $old = caps($old);
  $new = caps($new);
  if ($name =~ /$old/) { return $`.$new.$'; }
  $old = uc($old);
  $new = uc($new);
  if ($name =~ /$old/) { return $`.$new.$'; }
  return undef;
}

sub cant_decide($) {
  my ($ali_selected) = @_;
  die "can't decide whether '$ali_selected' is a protein or a DNA alignment.\nPlease select an alignment with 'pro' OR 'dna' in name.";
}

sub get_dnapro_alignments($) {
  my ($gb_main) = @_;

  my $ali_selected = BIO::remote_read_awar($gb_main,'ARB_NT','presets/use');
  if ((not defined $ali_selected) or ($ali_selected =~ /\?/)) {
    die "Please select a valid alignment";
  }

  my $ali_dna = mod_name($ali_selected,'protein|prot|pro', 'dna');
  my $ali_pro = mod_name($ali_selected,'dna', 'pro');

  if (defined $ali_pro) {
    if (defined $ali_dna) { cant_decide($ali_selected); }
    $ali_dna = $ali_selected;
  }
  else {
    if (not defined $ali_dna) { cant_decide($ali_selected); }
    $ali_pro = $ali_selected;
  }

  return ($ali_dna,$ali_pro);
}

# ----------------------------------------
# cleanup (if needed)
END { }

1; # module initialization is ok
