#!/usr/bin/perl
# ============================================================ #
#                                                              #
#   File      : svn_undo_depends.pl                            #
#   Purpose   : undo all dependency-only changes to makefiles  #
#                                                              #
#   Coded by Ralf Westram (coder@reallysoft.de) in July 2013   #
#   Institute of Microbiology (Technical University Munich)    #
#   http://www.arb-home.de/                                    #
#                                                              #
# ============================================================ #

my $reg_changes_depends = qr/^[+-][a-z0-9_]+\.o:\s/i;
my $reg_is_header_start = qr/^Index:\s/i;
my $reg_is_chunk_start = qr/^\@\@\s/i;

use Carp confess;

sub save_array($\@) {
  my ($toname,$arr_r) = @_;

  open(FILE,'>'.$toname) || die "can't write to file '$toname' (Reason: $!)";
  foreach (@$arr_r) {
    print FILE $_."\n";
  }
  close(FILE);
}

sub chunk_contains(\@$\$) {
  my ($chunk_r, $reg_contains, $seen_other_changes_r) = @_;

  my $skipped_first = 0;
  my $contains = 0;
  foreach (@$chunk_r) {
    if ($skipped_first) {
      if ($_ =~ $reg_contains) {
        $contains = 1;
      }
      elsif (/^[+-]/o) {
        $$seen_other_changes_r = 1;
      }
    }
    else {
      $skipped_first = 1;
    }
  }
  return $contains;
}

sub is_section_end($) {
  my ($line) = @_;
  return ((not defined $line) or ($line =~ $reg_is_header_start) or ($line =~ $reg_is_chunk_start)) ? 1 : 0;
}

sub take($\@\@) {
  my ($amount,$in_r,$out_r) = @_;

  my $size = scalar(@$in_r);
  if ($amount>$size) { die "Tried to take to much"; }
  if ($amount==$size) {
    @$out_r = @$in_r;
    @$in_r = ();
  }
  else {
    @$out_r = @$in_r[0 .. $amount-1];
    @$in_r = @$in_r[$amount .. $size-1];
  }
}

sub find_next_section_end(\@) {
  my ($in_r) = @_;
  my $idx = 1;
  while (not is_section_end($$in_r[$idx])) {
    $idx++;
    die "overflow" if $idx>9999;
  }
  return $idx;
}

sub read_header(\@\@) {
  my ($in_r,$out_r) = @_;

  my $line = $$in_r[0];
  if (not $line =~ $reg_is_header_start) {
    Carp::confess "Expected header start in line '$line'";
  }

  my $length = find_next_section_end(@$in_r);
  take($length,@$in_r,@$out_r);
}

sub read_chunk(\@\@) {
  my ($in_r,$out_r) = @_;

  my $line = $$in_r[0];
  if (not $line =~ $reg_is_chunk_start) {
    Carp::confess "Expected chunk start in line '$line'";
  }

  my $length = find_next_section_end(@$in_r);
  take($length,@$in_r,@$out_r);
}

sub extract_depends(\@\@) {
  my ($in_r,$out_r) = @_;

  # drop lines before first header (e.g. mergeinfo)
  while (not $$in_r[0] =~ $reg_is_header_start) {
    die "no diff here" if not defined shift @$in_r;
  }

  my @header = ();
  read_header(@$in_r, @header);

  my @chunk = ();
  read_chunk(@$in_r, @chunk);

  my $count = 0;
  while (scalar(@chunk)) {
    my $seen_other_changes = 0;
    if (chunk_contains(@chunk, $reg_changes_depends, $seen_other_changes)) {
      if ($seen_other_changes==1) {
        die if (not $header[0] =~ $reg_is_header_start);
        print "Warning: skipped hunk with mixed changes ($')";
      }
      else {
        if (scalar(@header)) {
          push @$out_r, @header;
          @header = ();
        }
        push @$out_r, @chunk;
      }
    }
    @chunk = ();

    if (scalar(@$in_r)) {
      if ($$in_r[0] =~ $reg_is_header_start) {
        read_header(@$in_r, @header);
      }
      read_chunk(@$in_r, @chunk);
    }
  }
}

sub svn_undo_depends($) {
  my ($root) = @_;
  chdir($root) || die "can't cd to '$root' (Reason: $!)";

  my @diff = map { chomp; $_; } `svn diff`;

  my @diff_depends = ();
  extract_depends(@diff,@diff_depends);

  my $ARBHOME = $ENV{ARBHOME};
  if (defined $ARBHOME and -d $ARBHOME) {
    print "Creating safety patch..\n";
    my $cmd = "$ARBHOME/SOURCE_TOOLS/arb_create_patch.sh b4undodepends";
    system($cmd)==0 || die "error executing '$cmd' (result=$?)";
  }

  {
    my $rev_patch = 'reverted_depends.patch';
    save_array($rev_patch, @diff_depends);

    print "Reverting dependency changes..\n";
    my $patch_options = '--strip=0 --reverse --unified --force';
    my $cmd = "patch $patch_options";
    open(REVERT, '|'.$cmd) || die "can't execute '$cmd' (Reason: $!)";
    foreach (@diff_depends) { print REVERT $_."\n"; }
    close(REVERT);
    
    print "Reverts saved in $rev_patch\n";
  }
}

sub show_usage($) {
  my ($err) = @_;
  print "Usage: svn_undo_depends.pl rootdir\n";
  print "Reverts all dependency updates\n";
  print "Error: $err\n";
  exit(0);
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args!=1) {
    show_usage("expected argument 'rootdir'");
  }
  my $root = shift @ARGV;
  if (not -d $root) {
    show_usage("No such directory '$root'");
  }

  svn_undo_depends($root);
}

main();
