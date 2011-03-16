#!/usr/bin/perl
# ============================================================ #
#                                                              #
#   File      : beautifyNewick.pl                              #
#   Purpose   : Beautify a newick tree                         #
#                                                              #
#   Coded by Ralf Westram (coder@reallysoft.de) in June 2009   #
#   Institute of Microbiology (Technical University Munich)    #
#   www.arb-home.de                                            #
#                                                              #
# ============================================================ #

use strict;
use warnings;

use Bio::TreeIO;

# ----------------------------------------

my $indent_incr     = 1; # how much indentation to use
my $inTreeComments = 50; # indentation of in-tree-comments (0=none)

# ----------------------------------------

sub make_indent($) {
  my ($indlev) = @_;
  my $s = '';
  for (my $i = 0; $i<$indlev; $i++) { $s .= ' '; }
  return $s;
}

my $depth = 0;

sub indent() {
  my $indlev = $depth*$indent_incr;
  return make_indent($indlev);
}

sub node2string($);
sub node2string($) {
  my ($node) = @_;

  if ($node->is_Leaf) {
    return indent().'"'.$node->id.'"';
  }

  my $str = indent()."(";
  $depth++;

  my @children = ();
  foreach my $child ($node->each_Descendent()) {
    push @children, node2string($child).':'.$child->branch_length;
  }

  if ($inTreeComments) {
    my $len = length($str);

    $str .= make_indent($inTreeComments-$len)."[children=".scalar(@children);
    if ($indent_incr==0) { $str .= ', level='.$depth; }
    $str .= "]";
  }
  $str .= "\n";

  $str .= join(",\n", @children)."\n";

  $depth--;
  $str .= indent().")";

  return $str;
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args<2) {
    die(
        "Usage: beautifyNewick.pl [--indent] [--intreecmt] in.tree out.tree\n".
        "       --indent=inc          indent subnodes by 'inc' spaces\n".
        "       --intreecmt=col       insert info-comments inside tree at column 'col'\n".
        "       --overwrite           overwrite existing output file\n".
        "\n"
       );
  }

  my $infile    = undef;
  my $outfile   = undef;
  my $overwrite = 0;

  foreach (@ARGV) {
    if (/^--indent=/) { $indent_incr = $'; }
    elsif (/^--intreecmt=/) { $inTreeComments = $'; }
    elsif ($_ eq '--overwrite') { $overwrite = 1; }
    else {
      if (not defined $infile) { $infile = $_; }
      elsif (not defined $outfile) { $outfile = $_; }
      else {
        print "Warning: ignoring superfluous argument '$_'\n";
      }
    }
  }

  if (not defined $outfile) { die "Missing arguments!\n"; }

  if (not -f $infile) { die "File not found: '$infile'\n"; }
  if ($overwrite==0 and -f $outfile) { die "File already exists: '$outfile'\n"; }

  my $in   = new Bio::TreeIO(-format => 'newick', -file => $infile);
  my $tree = $in->next_tree;

  open(TREE,'>'.$outfile) || die "can't write '$outfile' (Reason: $!)";
  my $root = $tree->get_root_node($tree);
  print TREE node2string($root);
  close(TREE);
}
main();
