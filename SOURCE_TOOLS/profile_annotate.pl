#!/usr/bin/perl

use strict;
use warnings;

# --------------------------------------------------------------------------------

sub scanLine($) {
  my ($line) = @_;
  if ($line =~ /^[ ]*([0-9,]+) [ ]*/o) {
    my ($num,$rest) = ($1,$');
    $num =~ s/,//ig;
    return ($num,$rest);
  }
  return (undef,undef);
}

sub prefix_percent($$$) {
  my ($line,$total,$partial) = @_;
  if ($partial>$total) { die "Invalid partial=$partial (total=$total)"; }
  my $percent = int($partial/$total*1000+0.5)/10;
  return sprintf("%5.1f",$percent).'% | '.$line;
}

sub percentize($$) {
  my ($line,$totals) = @_;
  my ($num,$rest) = scanLine($line);
  if (defined $num) {
    (prefix_percent($line,$totals,$num),$rest);
  }
  else {
    ($line,$rest);
  }
}

sub percentize_cluster(\@$) {
  my ($cluster_r, $cluster_total) = @_;

  eval {
    if (defined $cluster_total) {
      my @new = ();
      foreach (@$cluster_r) {
        if (/ \| /o) {
          my ($prefix,$orgline) = ($`.$&, $');
          my ($size,$rest) = scanLine($orgline);

          push @new, prefix_percent($_,$cluster_total,$size);
        }
      }

      @$cluster_r = @new;
    }
  };
  if ($@) {
    print "Error: $@\nat cluster:\n";
    foreach (@$cluster_r) {
      print "'$_'\n";
    }
  }
}

sub add_percentages(\@) {
  my ($lines_r) = @_;

  my @out       = ();
  my $nr        = 0;
  my $maxnr     = scalar(@$lines_r);
  my $totals    = undef;
  my $dummy;
  my $seperator = undef;

  while (not defined $totals && $nr<$maxnr) {
    my $line = $$lines_r[$nr++];
    push @out, $line;
    if ($line =~ /PROGRAM TOTALS$/o) {
      ($totals,$dummy) = scanLine($line);
    }
    elsif (not defined $seperator) {
      if ($line =~ /^-+$/o) { $seperator = $line; }
    }
  }

  if (not defined $totals) { die "Could not parse PROGRAM TOTALS"; }
  if (not defined $seperator) { die "No separator found"; }

  my $last_line = pop @out;
  ($last_line,$dummy) = percentize($last_line,$totals);
  push @out, $last_line;
  push @out, "";

  my @rest          = ();
  my @cluster       = ();
  my $cluster_total = undef;
  my $percentize_cluster = 1;

  while ($nr<$maxnr) {
    my $line = $$lines_r[$nr++];
    my ($pline,$rest) = percentize($line,$totals);
    if (defined $rest) { # percentage added
      if ($rest =~ /^\*/o) {
        push @out, $pline;
        if ($pline =~ /\| /o) {
          ($cluster_total,$dummy) = scanLine($');
        }
      }
      push @cluster, $pline;
    }
    else {
      if (scalar(@cluster)) {
        if ($percentize_cluster==1) { percentize_cluster(@cluster,$cluster_total); }
        push @rest, @cluster;
        @cluster       = ();
        $cluster_total = undef;
      }

      if ($line =~ /Auto-annotated source/o) { $percentize_cluster = 0; }
      push @rest, $pline;
    }
  }

  if (scalar(@cluster)) {
    if ($percentize_cluster==1) { percentize_cluster(@cluster,$cluster_total); }
    push @rest, @cluster;
  }

  @$lines_r = @out;
  push @$lines_r, @rest;
}

# --------------------------------------------------------------------------------

sub setModtime($$) {
  my ($file,$modtime) = @_;
  utime($modtime,$modtime,$file) || die "can't set modtime of '$file' (Reason: $!)";
}

sub getModtime($) {
  my ($fileOrDir) = @_;
  my $modtime = (stat($fileOrDir))[9];
  return $modtime;
}

sub annotate_one($$$) {
  my ($outfile,$force,$dir) = @_;
  if (not -f $outfile) { die "No such file '$outfile'"; }
  if (not $outfile =~ /^callgrind\.out\./o) {
    die "Illegal name (expected 'callgrind.out.xxx' not '$outfile')";
  }

  my $annotated   = 'callgrind.annotated.'.$';
  my $perform     = $force;
  my $modtime_out = getModtime($outfile);

  if (not -f $annotated) { $perform = 1; }
  elsif ($modtime_out>getModtime($annotated)) { $perform = 1; }

  if ($perform==1) {
    print "* Updating $annotated\n";

    my $command = "callgrind_annotate --tree=both --inclusive=yes ";
    if (defined $dir) { $command .= '--auto=yes --include=./'.$dir.' '; }
    $command .= $outfile;
    my $line;
    my @lines  = ();

    open(CMD, $command.'|') || die "can't execute '$command' (Reason: $!)";
    while (defined ($line=<CMD>)) {
      chomp($line);
      push @lines, $line;
    }
    close(CMD);

    add_percentages(@lines);

    open(ANNO,'>'.$annotated) || die "can't write '$annotated' (Reason: $!)";
    print ANNO "Command was '$command'\n\n";
    foreach (@lines) {
      print ANNO $_."\n";
    }
    close(ANNO);
    # setModtime($annotated,$modtime_out+2);
  }
}

sub annotate_all() {
  opendir(DIR,".") || die "can't read directory '.' (Reason: $!)";
  foreach (readdir(DIR)) {
    if (/^callgrind\.out\./o) {
      annotate_one($_,0, undef);
    }
    elsif (/^callgrind\.annotate\./o) {
      my $out = 'callgrind.out.'.$';
      if (not -f $out) {
        print "* $out disappeared => remove $_\n";
        unlink($_) || die "Can't unlink '$_' (Reason: $!)";
      }
    }
  }
  closedir(DIR);
}

# --------------------------------------------------------------------------------

sub die_usage($) {
  my ($err) = @_;

  die("Usage: profile_annotate.pl  all | callgrind.out.xxx [DIR]\n".
      "       Annotates all or one callgrind.out.xxx\n".
      "       Annotations are written to callgrind.annotated.xxx\n".
      "       If 'all' is specified, all callgrind.annotated.xxx files without source get deleted.\n".
      "       If DIR is given it's used for auto source annotation.\n".
      "Error: $err\n"
       );
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args<1 || $args>2) { die_usage "Wrong number of arguments"; }

  my $arg = $ARGV[0];
  if ($arg eq 'all') { annotate_all(); }
  elsif (-f $arg) {
    my $dir = undef;
    if ($args==2) {
      $dir = $ARGV[1];
      if (not -d $dir) { die "No such directory '$dir'"; }
    }
    annotate_one($arg,1,$dir);
  }
  else { die_usage("No such file '$arg'"); }
}

main();


