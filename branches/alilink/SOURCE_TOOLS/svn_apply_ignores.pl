#!/usr/bin/perl
# ============================================================ #
#                                                              #
#   File      : svn_apply_ignores.pl                           #
#   Purpose   : apply SVN default ignores for ARB              #
#                                                              #
#   Coded by Ralf Westram (coder@reallysoft.de) in July 2011   #
#   Institute of Microbiology (Technical University Munich)    #
#   http://www.arb-home.de/                                    #
#                                                              #
# ============================================================ #

# configure here
my $dump = 0;
# configure here


my $ARBHOME = $ENV{ARBHOME};
if (not -d $ARBHOME) { die "no ARBHOME"; }

my $SELF = $ARBHOME.'/SOURCE_TOOLS/svn_apply_ignores.pl';

sub apply_dir($$) {
  my ($full,$rel) = @_;

  my $cmd = "svn pe svn:ignore $rel --editor-cmd \"$SELF --dopropedit $full \"";
  system($cmd)==0 || die "can't execute '$cmd' (Reason: $?)";
}

sub apply_recursive($$);
sub apply_recursive($$) {
  my ($full,$rel) = @_;

  my $have_svn = 0;
  my @subdirs  = ();

  opendir(DIR,$full) || die "can't read directory '$full' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $sub = $full.'/'.$_;
      if (-d $sub and not -l $sub) { push @subdirs, $_; }
      if ($_ eq '.svn') { $have_svn = 1; }
    }
  }
  closedir(DIR);

  if ($have_svn==1) { apply_dir($full,$rel); }
  foreach (@subdirs) { apply_recursive($full.'/'.$_,$rel.'/'.$_); }
}

sub dumpContent($\@) {
  my ($tag,$content_r) = @_;
  print "------------------------------ [$tag start]\n";
  foreach (@$content_r) { print "$_\n"; }
  print "------------------------------ [$tag end]\n";
}

sub scanFilesExtensions($\%\%) {
  my ($dir,$files_r,$ext_r) = @_;
  opendir(DIR,$dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    my $full = $dir.'/'.$_;
    if (-f $full) {
      $$files_r{$_} = 1;
      if (/\.([^.]+)$/) { $$ext_r{$1} = 1; }
    }
  }
  closedir(DIR);
}

sub propedit_dir($$$) {
  # called as editor from svn-command

  my ($rel,$full,$propfile) = @_;

  open(IN,'<'.$propfile) || die "can't load '$propfile' (Reason: $!)";
  my @content = ();
  my $line;
  while (defined($line=<IN>)) { chomp($line); push @content, $line; }
  close(IN);

  my %have = ();
  sub add_missing($) {
    my ($want) = @_;
    if (not defined $have{$want}) {
      print "Added '$want'\n";
      push @content, $want;
    }
  }

  $dump==0 || dumpContent('old', @content);

  my %file = ();
  my %ext = ();
  scanFilesExtensions($full,%file,%ext);

  # ---------------------------------------- conditions

  my $creates_gcov = (defined $ext{c} or defined $ext{cpp} or defined $ext{cxx});
  my $creates_bak = (defined $file{Makefile});
  my $is_root = ($rel eq '.');

  %have = map { $_ => 1; } @content;

  # ---------------------------------------- remove ignores

  my @unwanted = (
                  'Makefile.bak',
                  '*.gcda',
                 );

  if (not $creates_gcov) { push @unwanted, '*.gcno'; }
  if (not $creates_bak) { push @unwanted, '*.bak'; }
  if (not $is_root) { push @unwanted, 'ChangeLog'; }

  # ---------------------------------------- remove ignores

  my %unwanted = map { $_ => 1; } @unwanted;

  foreach (@content) {
    if (defined $unwanted{$_}) { print "Removed '$_'\n"; }
  }
  @content = map {
    if (defined $unwanted{$_}) { ; }
    else { $_; }
  } @content;

  %have = map { $_ => 1; } @content;

  # ---------------------------------------- add ignores

  if ($creates_bak) { add_missing('*.bak'); }
  if ($creates_gcov) { add_missing('*.gcno'); }

  # ---------------------------------------- add ignores

  $dump==0 || dumpContent('new', @content);


  open(OUT,'>'.$propfile) || die "can't save '$propfile' (Reason: $!)";
  foreach (@content) { print OUT $_."\n"; }
  close(OUT);
}

sub show_usage() {
  print "Usage: svn_apply_ignores.pl --apply\n";
  print "       Apply all\n";
  print "Usage: svn_apply_ignores.pl --dopropedit DIR PROPFILE\n";
  print "       Edit property for directory 'DIR'\n";
}

sub main() {
  my $args = scalar(@ARGV);

  if ($args==0) {
    show_usage();
  }
  else {
    if ($ARGV[0] eq '--apply') {
      chdir($ARBHOME);
      apply_recursive($ARBHOME,'.');
    }
    elsif ($ARGV[0] eq '--dopropedit') {
      my $DIR = $ARGV[1];
      my $PROPFILE = $ARGV[2];
      my ($REL,$FULL);
      if ($DIR =~ /^$ARBHOME(\/)?/) {
        ($REL,$FULL) = ($',$DIR);
        if ($REL eq '') { $REL = '.'; }
      }
      else {
        ($REL,$FULL) = ($DIR,$ARBHOME.'/'.$DIR);
      }
      if (not -d $FULL) { die "No such directory '$FULL'"; }
      if (not -f $PROPFILE) { die "No such directory '$PROPFILE'"; }

      propedit_dir($REL,$FULL,$PROPFILE);
    }
  }
}
main();

