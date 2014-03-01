#!/usr/bin/perl

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

  my @childs = ();
  foreach (@dep_files) {
    my $pid = fork();
    while (not defined $pid) {
      print "Warning: could not fork\n";
      sleep(1);
      $pid = fork(); # retry
    }
    if ($pid == 0) { # child
      my $destgif = "$dest/".$gif_suffix{$_}.".gif";
      execute("SOURCE_TOOLS/needed_libs.pl -G $destgif -U -B -S $_");
      exit(0);
    }

    push @childs, $pid;
  }

  print "Waiting for childs...\n";
  foreach (@childs) {
    waitpid($_, 0);
  }
  print "All childs terminated\n";
}
main();
