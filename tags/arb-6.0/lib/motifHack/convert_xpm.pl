#!/usr/bin/perl
# ============================================================= #
#                                                               #
#   File      : convert_xpm.pl                                  #
#   Purpose   : fix those .xpm that make motif fail             #
#                                                               #
#   Coded by Ralf Westram (coder@reallysoft.de) in March 2013   #
#   Institute of Microbiology (Technical University Munich)     #
#   http://www.arb-home.de/                                     #
#                                                               #
# ============================================================= #

# Reads xpm from STDIN.
# Writes fixed xpm to passed file (automatically creates target directories)
#
# Fixes done:
# - eliminates odd x- and y-sizes by reducing or
#   increasing the xpms size.
#   Only transparent pixel data will be removed.
# - removes additional comments
#
# see README for why this is done


use strict;
use warnings;

sub findTransparentColor(\@$) {
  my ($colors_r,$charsPerPixel) = @_;
  foreach (@$colors_r) {
    if (/c\s+none$/oi) {
      my $t = substr($_,0,$charsPerPixel);
      length($t) == $charsPerPixel || die;
      return $t;
    }
  }
  return undef;
}

sub transpose(\@$) {
  my ($pixels_r,$charsPerPixel) = @_;
  my @transposed = ();

  foreach (@$pixels_r) {
    my $org = $_;
    for (my $i = 0; $_ ne ''; ++$i) {
      $transposed[$i] .= substr($_,0,$charsPerPixel);
      $_ = substr($_,$charsPerPixel);
    }
  }
  @$pixels_r = @transposed;
}

sub createUniRow($$) {
  my ($pixel,$count) = @_;
  my $row = '';
  while ($count--) { $row .= $pixel; }
  return $row;
}
sub countUniRows($\@) {
  my ($unirow,$rows_r) = @_;
  my ($start,$end) = (0,0);
  my $rows = scalar(@$rows_r);
  for (my $i=0; $i<$rows; ++$i) {
    if ($$rows_r[$i] eq $unirow) {
      if ($start==$i) { $start++; }
      $end++;
    }
    else {
      $end = 0;
    }
  }

  return ($start,$end);
}
sub findMostUsedPixel(\@$) {
  my ($pixels_r,$charsPerPixel) = @_;
  my %count = ();
  foreach (@$pixels_r) {
    my $row = $_;
    while ($row ne '') {
      my $pixel = substr($row,0,$charsPerPixel);
      $row = substr($row,$charsPerPixel);
      if (defined $count{$pixel}) { $count{$pixel}++; }
      else { $count{$pixel} = 1; }
    }
  }
  my $maxCount = 0;
  my $mostUsedColor = undef;
  foreach (keys %count) {
    if ($count{$_} > $maxCount) {
      $maxCount = $count{$_};
      $mostUsedColor = $_;
    }
  }

  defined $mostUsedColor || die;
  return $mostUsedColor;
}

sub fixOddSize(\$\@$$) {
  my ($ysize_r,$pixels_r,$charsPerPixel,$transparent) = @_;
  ($$ysize_r % 2) || die;

  my $xsize = length($$pixels_r[0]) / $charsPerPixel;
  my $fixed = 0;

  # check for transparent rows and drop them
  if (defined $transparent) {
    my $unirow = createUniRow($transparent,$xsize);
    my ($uniStart,$uniEnd) = countUniRows($unirow,@$pixels_r);

    if ($uniStart or $uniEnd) {
      if ($uniStart>$uniEnd) { shift @$pixels_r; } # drop first row
      else { pop @$pixels_r; } # drop last row
      $$ysize_r--;
      $fixed = 1;
    }
  }

  # check for uni-colored rows and dup them
  if (not $fixed) {
    my $firstPixel = substr($$pixels_r[0], 0, $charsPerPixel);
    my $lastPixel  = substr($$pixels_r[scalar(@$pixels_r)-1], 0, $charsPerPixel);

    my $firstUniRow = createUniRow($firstPixel,$xsize);
    my $lastUniRow  = createUniRow($lastPixel,$xsize);

    my ($uniStart,$uniEnd,$ignored);

    ($uniStart,$ignored) = countUniRows($firstUniRow,@$pixels_r);
    ($ignored,$uniEnd)   = countUniRows($lastUniRow,@$pixels_r);

    if ($uniStart or $uniEnd) {
      if ($uniStart>$uniEnd) { push @$pixels_r, $lastUniRow; } # dup last row
      else { unshift @$pixels_r, $firstUniRow; } # dup first row
      $$ysize_r++;
      $fixed = 1;
    }
  }

  # try to add a transparent row
  if (not $fixed and defined $transparent) {
    my $emptyrow = createUniRow($transparent,$xsize);
    push @$pixels_r, $emptyrow;
    $$ysize_r++;
    $fixed = 1;
  }

  # add a row in the most used color
  # (always works, but result may be ugly)
  if (not $fixed) {
    my $mostUsedPixel = findMostUsedPixel(@$pixels_r,$charsPerPixel);
    my $colorrow      = createUniRow($mostUsedPixel,$xsize);
    push @$pixels_r, $colorrow;
    $$ysize_r++;
    $fixed = 1;
  }

  $fixed || die "could not fix odd size";
}

sub fixContent {
  my @content = @_;
  my $header = $content[0];
  if (not $header =~ /^([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s*$/o) {
    die "can't parse header '$header'";
  }
  my ($xsize,$ysize,$colors,$charsPerPixel) = ($1,$2,$3,$4);

  my $elems = scalar(@content);
  my $expected = 1+$colors+$ysize;
  if ($elems != $expected) {
    die "expected $expected array entries (=1+$colors+$ysize), found $elems";
  }

  my @colors = splice(@content,1,$colors);
  my @pixels = splice(@content,1,$expected);

  scalar(@content)==1 || die;

  my $oddX = ($xsize % 2);
  my $oddY = ($ysize % 2);

  # for testing purposes uncomment the next lines (makes ARB-motif crash on 10.04 and b4)
  # $oddX = 0; # accept odd xsize
  # $oddY = 0; # accept odd ysize

  if ($oddX or $oddY) {
    my $transparent = findTransparentColor(@colors,$charsPerPixel);
    if ($oddY) {
      fixOddSize($ysize,@pixels,$charsPerPixel,$transparent);
    }
    if ($oddX) {
      transpose(@pixels,$charsPerPixel);
      fixOddSize($xsize,@pixels,$charsPerPixel,$transparent);
      transpose(@pixels,$charsPerPixel);
    }
  }

  return (
          "$xsize $ysize $colors $charsPerPixel ",
          @colors,
          @pixels
         );
}

sub parseArray($) {
  my ($content) = @_;

  my @array = ();
  while ($content =~ /^\s*\"([^\"]*)\"[\s,]*/o) {
    push @array, $1;
    $content = $';
  }
  return @array;
}

sub parentDir($) {
  my ($dirOrFile) = @_;
  if ($dirOrFile =~ /\/[^\/]+$/o) { return $`; }
  return undef;
}

sub ensureDir($);
sub ensureDir($) {
  my ($dir) = @_;
  if (defined $dir) {
    if (not -d $dir) {
      my $dparent = parentDir($dir);
      ensureDir($dparent);
      print "Creating directory '$dir'\n";
      if (not mkdir($dir)) {
        my $reason = $!;
        -d $dir || die "Failed to create '$dir' (Reason: $reason)";
      }
    }
  }
}

sub main() {
  my @source = <STDIN>;

  my $args = scalar(@ARGV);
  if ($args != 1) {
    die "Usage: convert_xpm.pl output.xpm\n".
      "Reads xpm from STDIN and writes result to 'output.xpm'\n";
  }
  my $outname = $ARGV[0];

  if ($outname =~ /\/[^\/]+$/o) {
    ensureDir(parentDir($outname));
  }

  # eliminate all comments (motif stumbles upon additional comments)
  my @noComment = map {
    chomp;
    if (/\/\*/o) {
      my ($prefix,$rest) = ($`,$');
      /\*\//o || die "expected closing comment";
      my $suffix = $';

      if ($prefix eq '') { $_ = $suffix; }
      elsif ($suffix eq '') { $_ = $prefix; }
      else { $_ = $prefix.' '.$suffix; }
    }
    if ($_ eq '') { ; } else { $_; }
  } @source;

  my $oneLine = join(' ', @noComment);

  if (not $oneLine =~ /^([^{]+{)(.+)(}[;\s]*)$/o) {
    die "failed to parse .xpm\n(oneLine='$oneLine')\n ";
  }
  my ($prefix,$content,$suffix) = ($1,$2,$3);

  my @content = fixContent(parseArray($content));

  # print out modified .xpm
  open(OUT,'>'.$outname) || die "Failed to write to '$outname' (Reason: $!)";
  print OUT "/* XPM */\n";
  print OUT "$prefix\n";

  my $lines = scalar(@content);
  for (my $i=0; $i<$lines; ++$i) {
    print OUT '"'.$content[$i].'"';
    print OUT ',' if ($i<($lines-1));
    print OUT "\n";
  }
  print OUT "$suffix\n";
  close(OUT);
}
main();
