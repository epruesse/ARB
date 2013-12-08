#!/usr/bin/perl
# =============================================================== #
#                                                                 #
#   File      : raxml2arb.pl                                      #
#   Purpose   : Import XX best of YY calculated raxml trees into  #
#               ARB and generate comment containing likelyhood    #
#               and content of info file                          #
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
  print "Message: $msg\n";
  system("arb_message \"$msg\"");
}

sub error($) {
  my ($msg) = @_;
  if ($msg =~ /at.*\.pl/o) {
    my ($pe,$loc) = ($`,$&.$');
    chomp $loc;
    $msg = $pe."\n(".$loc.")\nPlease check console for additional error reasons";
  }
  arb_message($msg);
  die $msg;
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

  my $result       = raxml_filename('result',$name,$run);
  my $bipartitions = raxml_filename('bipartitions',$name,$run);
  my $parsimony    = raxml_filename('parsimonyTree',$name,$run);
  my $log          = raxml_filename('log',$name,$run);

  if (not -f $result) { # no result tree, try bipartitions or parsimonyTree
    if (-f $bipartitions) { return ($bipartitions,'unknown'); }
    if (-f $parsimony) { return ($parsimony,'unknown'); }
  }

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

    if ($args != 4) {
      die "Usage: raxml2arb.pl RUNNAME NUMBEROFRUNS TAKETREES [import|consense]\n".
        "       import: Import the best TAKETREES trees of NUMBEROFRUNS generated trees into ARB\n".
        "       consense: Create and import consensus tree of best TAKETREES trees of NUMBEROFRUNS generated trees\n";
    }

    my ($RUNNAME,$NUMBEROFRUNS,$TAKETREES,$CONSENSE) = @ARGV;

    if ($NUMBEROFRUNS<1) { die "NUMBEROFRUNS has to be 1 or more ($NUMBEROFRUNS)"; }

    my %likelyhood = ();
    findTrees($RUNNAME,$NUMBEROFRUNS,%likelyhood);

    my $createdTrees = scalar(keys %likelyhood);
    print "Found ".someWhat($createdTrees,'tree').":\n";

    if ($TAKETREES > $createdTrees) {
      arb_message("Cant take more trees ($TAKETREES) than calculated ($NUMBEROFRUNS) .. using all");
      $TAKETREES = $createdTrees;
    }

    my $calc_consense = 0;
    if ($CONSENSE eq 'consense') {
      if ($TAKETREES<2) {
        arb_message("Need to take at least 2 trees to create a consensus tree - importing..");
        $CONSENSE = 'import';
      }
      else {
        $calc_consense = 1;
      }
    }
    elsif ($CONSENSE ne 'import') {
      die "Unknown value '$CONSENSE' (expected 'import' or 'consense')";
    }

    my @sortedTrees = sort { $likelyhood{$b} <=> $likelyhood{$a}; } keys %likelyhood;
    foreach (@sortedTrees) { print "  $_ = ".$likelyhood{$_}."\n"; }

    my @treesToTake = splice(@sortedTrees,0,$TAKETREES);
    if (scalar(@treesToTake)<1) { die "No trees to $CONSENSE"; }

    my $treesToTake = scalar(@treesToTake);
    my $treename = 'tree_RAxML_'.$$;
    my $infofile = 'RAxML_info.'.$RUNNAME;

    if ($calc_consense==0) {
      print 'Importing '.someWhat($treesToTake,'tree').":\n";
      my $count = undef;
      if ($treesToTake>1) { $count = 1; }

      foreach (@treesToTake) {
        print "  $_ = ".$likelyhood{$_}."\n";
        my $currTreename = $treename;
        if (defined $count) {
          $currTreename .= '_'.$count;
          $count++;
        }
        my $command = 'arb_read_tree '.$currTreename.' '.$_.' "likelyhood='.$likelyhood{$_}.'"';
        if (-f $infofile) { $command .= ' -commentFromFile '.$infofile; }

        system($command)==0 || die "can't execute '$command' (Reason: $?)";
      }
    }
    else { # calc consense tree
      my $minLH  = undef;
      my $maxLH  = undef;
      my $meanLH = 0;

      print 'Consensing '.someWhat($treesToTake,'tree').":\n";
      open(INTREE,'>intree') || die "can't write 'intree' (Reason: $!)";
      foreach (@treesToTake) {
        my $LH = $likelyhood{$_};
        print "  $_ = $LH\n";
        if (not defined $minLH) {
          $minLH = $maxLH = $LH;
        }
        else {
          if ($LH < $minLH) { $minLH = $LH; }
          if ($LH > $maxLH) { $maxLH = $LH; }
        }
        $meanLH += $LH;

        open(RAXMLTREE,'<'.$_) || die "can't read '$_' (Reason: $!)";
        foreach (<RAXMLTREE>) {
          print INTREE $_;
        }
        close(RAXMLTREE);
        print INTREE "\n";
      }
      close(INTREE);

      $meanLH /= $treesToTake;

      my $command = 'arb_echo y | consense';
      system($command)==0 || die "can't execute '$command' (Reason: $?)";

      if (not -f 'outtree') {
        die "Consense failed (no 'outtree' generated)";
      }

      my $comment = "Consensus tree of $treesToTake trees\nLikelyhood: min=$minLH mean=$meanLH max=$maxLH";
      $command = 'arb_read_tree -consense '.$treesToTake.' '.$treename.' outtree "'.$comment.'"';
      if (-f $infofile) { $command .= ' -commentFromFile '.$infofile; }
      system($command)==0 || die "can't execute '$command' (Reason: $?)";
    }
  };
  if ($@) { error($@); }
}
main();
