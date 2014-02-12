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

my $loaded_log = undef;
my @log = ();

sub loadLog($) {
  my ($log) = @_;
  if (defined $loaded_log) {
    if ($log ne $loaded_log) {
      $loaded_log = undef;
      @log = ();
    }
  }
  if (not defined $loaded_log) {
    open(LOG,'<'.$log) || die "can't load '$log' (Reason: $!)";
    my $line;
    while (defined($line=<LOG>)) {
      chomp($line);
      push @log, $line;
    }
    close(LOG);
  }
}

sub firstLogLineMatching($$) {
  my ($log,$regexp) = @_;
  loadLog($log);
  foreach my $line (@log) {
    if ($line =~ $regexp) { return ($line,$1); }
  }
  return (undef,undef);
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

sub treeInfo_normal($$) {
  my ($name,$run) = @_;

  my $result       = raxml_filename('result',$name,$run);
  my $bipartitions = raxml_filename('bipartitions',$name,$run);
  my $parsimony    = raxml_filename('parsimonyTree',$name,$run);
  my $log          = raxml_filename('log',$name,$run);

  if (not -f $result) { # no result tree, try bipartitions or parsimonyTree
    if (-f $bipartitions) { return ($bipartitions,'unknown'); }
    if (-f $parsimony) { return ($parsimony,'unknown'); }
  }

  my ($line) = firstLogLineMatching($log,qr/./); # first non-empty line
  if (not $line =~ / (.*)$/o) {
    die "can't parse likelyhood from '$log'";
  }

  my $likelyhood = $1;
  return ($result,$likelyhood);
}

my @splitted_trees = ();

sub split_treefile($$$) {
  my ($in,$out,$treesExpected) = @_;
  @splitted_trees = ();
  my $outcount = 0;
  open(IN,'<'.$in) || die "can't read '$in' (Reason: $!)";
  my $line;
  while (defined($line=<IN>)) {
    if ($line =~ /\(/o) {
      my $outname = "$out.$outcount"; $outcount++;
      open(OUT,'>'.$outname) || die "can't write '$outname' (Reason: $!)";
      print OUT $line;
      close(OUT);
      push @splitted_trees, $outname;
    }
    else {
      print "Unexpected line in '$in':\n";
      print $line;
    }
  }
  close(IN);
  my $treesFound = scalar(@splitted_trees);
  if ($treesFound!=$treesExpected) {
    die "Failed to split '$in' into single treefiles\n".
    "(expected to find $treesExpected trees, found $treesFound)";
  }
}

sub treeInfo_bootstrapped($$) {
  my ($name,$run) = @_;

  my $info = raxml_filename('info',$name,undef);
  if (defined $run) {
    my $treeCount = scalar(@splitted_trees);
    if ($run>=$treeCount) {
      die "Invalid run number $run - has to be in [0 .. ".($treeCount-1)."]";
    }
    my ($line,$likelyhood) = firstLogLineMatching($info,qr/^Bootstrap\[$run\]:\s.*\slikelihood\s+([^\s,]+),/);
    if (not defined $likelyhood) {
      die "Failed to parse likelyhood for 'Bootstrap[$run]' from '$info'";
    }
    return ($splitted_trees[$run],$likelyhood);
  }
  else {
    my $bestTree = raxml_filename('bestTree',$name,undef);
    my ($line,$likelyhood) = firstLogLineMatching($info,qr/^Final ML Optimization Likelihood:\s+(.*)$/);
    if (not defined $likelyhood) {
      arb_message("Failed to extract final likelyhood from '$info'");
      $likelyhood = 'unknown';
    }
    return ($bestTree,$likelyhood);
  }
}

sub treeInfo($$$) {
  my ($bootstrapped,$name,$run) = @_;
  if ($bootstrapped==1) {
    return treeInfo_bootstrapped($name,$run);
  }
  return treeInfo_normal($name,$run);
}

sub findTrees($$$$\%) {
  my ($name,$bootstrapped,$runs,$take,$likelyhood_r) = @_;

  %$likelyhood_r = ();

  if (($bootstrapped==0 and $runs==1) || ($bootstrapped==1 and $take==1)) {
    my ($tree,$likely) = treeInfo($bootstrapped,$name,undef);
    $$likelyhood_r{$tree} = $likely;
  }
  else {
    if ($bootstrapped==1) {
      my $raxml_out   = raxml_filename('bootstrap',$name,undef);
      split_treefile($raxml_out, 'raxml2arb.tree',$runs);
      print "Splitted '$raxml_out' into ".scalar(@splitted_trees)." treefiles\n";
    }
    for (my $r = 0; $r<$runs; $r++) {
      my ($tree,$likely) = treeInfo($bootstrapped,$name,$r);
      $$likelyhood_r{$tree} = $likely;
    }
  }
}

sub main() {
  eval {
    my $args = scalar(@ARGV);

    if ($args != 5) {
      die "Usage: raxml2arb.pl RUNNAME NUMBEROFRUNS [bootstrapped|normal] TAKETREES [import|consense]\n".
        "       import: Import the best TAKETREES trees of NUMBEROFRUNS generated trees into ARB\n".
        "       consense: Create and import consensus tree of best TAKETREES trees of NUMBEROFRUNS generated trees\n";
    }

    my ($RUNNAME,$NUMBEROFRUNS,$BOOTSTRAPPED,$TAKETREES,$CONSENSE) = @ARGV;

    if ($NUMBEROFRUNS<1) { die "NUMBEROFRUNS has to be 1 or higher (NUMBEROFRUNS=$NUMBEROFRUNS)"; }

    my %likelyhood  = (); # key=treefile, value=likelyhood
    my @treesToTake = (); # treefiles

    my $bootstrapped = 0;
    if ($BOOTSTRAPPED eq 'bootstrapped') { $bootstrapped = 1; }
    elsif ($BOOTSTRAPPED ne 'normal') { die "Unexpected argument '$BOOTSTRAPPED' (expected 'normal' or 'bootstrapped')"; }

    my $calc_consense = 0;
    if ($CONSENSE eq 'consense') { $calc_consense = 1; }
    elsif ($CONSENSE ne 'import') { die "Unknown value '$CONSENSE' (expected 'import' or 'consense')"; }

    findTrees($RUNNAME,$bootstrapped,$NUMBEROFRUNS,$TAKETREES,%likelyhood);

    my $createdTrees = scalar(keys %likelyhood);
    print "Found ".someWhat($createdTrees,'tree').":\n";

    if ($TAKETREES > $createdTrees) {
      arb_message("Cant take more trees ($TAKETREES) than calculated ($NUMBEROFRUNS) .. using all");
      $TAKETREES = $createdTrees;
    }

    my @sortedTrees = sort { $likelyhood{$b} <=> $likelyhood{$a}; } keys %likelyhood;
    foreach (@sortedTrees) { print "  $_ = ".$likelyhood{$_}."\n"; }

    @treesToTake = splice(@sortedTrees,0,$TAKETREES);

    my $treesToTake = scalar(@treesToTake);
    if ($treesToTake==0) { die "failed to detect RAxML output trees"; }

    if ($calc_consense and $treesToTake<2) {
      arb_message("Need to take at least 2 trees to create a consensus tree - importing..");
      $calc_consense = 0;
    }

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
