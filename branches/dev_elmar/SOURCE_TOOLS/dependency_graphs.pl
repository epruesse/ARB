#!/usr/bin/perl

# 0westram@waltz ...westram_waltz/ARB-waltz/ARB.relatives> [10:26]
# find . -name "needs_libs*" | grep -v '.svn' | sed -e 's/[\/\.]/_/g' | sed -e 's/needs_libs//' | sed -e 's/__*/_/g' | sed -e 's/^_*//'  | sed -e 's/_*$//' 

my $ARBHOME = $ENV{ARBHOME};
if (not -d $ARBHOME) {
  die "ARBHOME has to be defined and has to mark a directory";
}
chdir($ARBHOME);

sub make_suffix($) {
  my ($name) = @_;

  $name =~ s/[\/\.]/_/g;

  $name =~ s/needs_libs//;

  $name =~ s/__*/_/g;
  $name =~ s/^_//;
  $name =~ s/_$//;

  return $name;
}

sub execute($) {
  my ($cmd) = @_;
  system($cmd)==0 || die "error calling '$cmd'";
}

sub main() {
  my @dep_files = map {
    if (/\.svn/) { ; }
    else { chomp; s/^\.\///; $_; }
  } `cd $ARBHOME;find . -name "needs_libs*"`;

  my %gif_suffix = map { $_ => make_suffix($_); } @dep_files;

  my $dest = 'dep_graphs';
  if (not -d $dest) {
    mkdir($dest) || die "can't create dir '$dest' (Reason: $!)";
  }

  foreach (@dep_files) {
    my $destgif = "$dest/".$gif_suffix{$_}.".gif";
    execute("SOURCE_TOOLS/needed_libs.pl -G -U -B -S $_");
    execute("mv lib_dependency.gif $destgif");
  }
  unlink("lib_dependency.dot");
}
main();
