#!/usr/bin/perl
# =============================================================== #
#                                                                 #
#   File      : raxml2arb.pl                                      #
#   Purpose   : Import XX best of YY calculated raxml trees into  #
#               ARB and generate comment containing likelyhood    #
#   Time-stamp: <Wed Mar/05/2008 12:03 MET Coder@ReallySoft.de>   #
#                                                                 #
#   Coded by Ralf Westram (coder@reallysoft.de) in March 2008     #
#   Institute of Microbiology (Technical University Munich)       #
#   http://www.arb-home.de/                                       #
#                                                                 #
# =============================================================== #

use strict;
use warnings;

sub arb_message($) {
  my ($msg) = @_;
  system("arb_message \"$msg\"");
}

sub error($) {
  my ($msg) = @_;
  arb_message($msg);
  die $msg."\n";
}

sub raxml_filename($$$) {
  my ($type,$name,$run) = @_;
  my $fname = 'RAxML_'.$type.'.'.$name;
  if (defined $run) { $fname .= '.RUN.'.$run; }
  return $fname;
}

sub someWhat($$) {
  my ($count,$what) = @_;
  if ($count==1) { return '1 '.$what; }
  return $count.' '.$what.'s';
}

sub treeInfo($$) {
  my ($name,$run) = @_;

  my $log    = raxml_filename('log',$name,$run);
  my $result = raxml_filename('result',$name,$run);

  open(LOG,'<'.$log) || die "can't open '$log' (Reason: $!)";
  my $line = undef;
  foreach (<LOG>) { if ($_ ne '') { $line = $_; } }
  close(LOG);

  chomp($line);

  if (not $line =~ / (.*)$/o) {
    die "can't parse likelyhood from '$log'";
  }

  my $likelyhood = $1;
  return ($result,$likelyhood);
}

sub findTrees($$\%) {
  my ($name,$runs,$likelyhood_r) = @_;

  %$likelyhood_r = ();

  if ($runs==1) {
    my ($tree,$likely) = treeInfo($name,undef);
    $$likelyhood_r{$tree} = $likely;
  }
  else {
    for (my $r = 0; $r<$runs; $r++) {
      my ($tree,$likely) = treeInfo($name,$r);
      $$likelyhood_r{$tree} = $likely;
    }
  }
}

sub main() {
  eval {
    my $args = scalar(@ARGV);

    if ($args != 3) {
      die "Usage: raxml2arb.pl RUNNAME NUMBEROFRUNS TAKETREES\n".
        "       Reads the best TAKETREES trees of NUMBEROFRUNS generated trees into ARB";
    }

    my ($RUNNAME,$NUMBEROFRUNS,$TAKETREES) = @ARGV;

    if ($NUMBEROFRUNS<1) { die "NUMBEROFRUNS has to be 1 or more ($NUMBEROFRUNS)"; }

    my %likelyhood = ();
    findTrees($RUNNAME,$NUMBEROFRUNS,%likelyhood);

    my $createdTrees = scalar(keys %likelyhood);
    print "Found ".someWhat($createdTrees,'tree').":\n";

    if ($TAKETREES>$createdTrees) {
      arb_message("Cant take more trees ($TAKETREES) than calculated ($NUMBEROFRUNS)");
      $TAKETREES = $createdTrees;
    }

    my @sortedTrees = sort { $likelyhood{$b} <=> $likelyhood{$a}; } keys %likelyhood;
    foreach (@sortedTrees) { print "  $_ = ".$likelyhood{$_}."\n"; }

    my @treesToImport = splice(@sortedTrees,0,$TAKETREES);
    if (scalar(@treesToImport)<1) { die "No trees to import"; }

    my $treesToImport = scalar(@treesToImport);
    print "Importing ".someWhat($treesToImport,'tree').":\n";

    my $count = undef;
    if ($treesToImport>1) { $count = 1; }
    my $treename = 'tree_RAxML_'.$$;

    foreach (@treesToImport) {
      print "  $_ = ".$likelyhood{$_}."\n";
      my $currTreename = $treename;
      if (defined $count) {
        $currTreename .= '_'.$count;
        $count++;
      }
      my $command = 'arb_read_tree '.$currTreename.' '.$_.' "likelyhood='.$likelyhood{$_}.'"';
      system($command)==0 || die "can't execute '$command' (Reason: $!)";
    }
  };
  if ($@) { error($@); }
}
main();
