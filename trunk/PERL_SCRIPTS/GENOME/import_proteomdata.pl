#!/usr/bin/perl

use strict;
use warnings;

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  push @INC, "$arbhome/PERL_SCRIPTS/GENOME";
  1;
}

use ARB;
use GI;

# -----------------------
# configure here:

my $create_ORFs = 0; # 0 -> error if ORF not found; 1 -> auto-create gene
my $verbose     = 0; # 0 -> be quiet; 1 -> be noisy
my $overwrite   = 0; # 0 -> if entry exists -> error; 1 -> silently overwrite entry

# -----------------------


my $importfilename;

{
  my $argcount = $#ARGV + 1;
  if ($argcount == 1) {
    $importfilename = $ARGV[0];
  }
  else {
    print "\nUsage: import_proteomdata.pl datafile\n";
    print "Imports a proteom datafile in CSV format.\n";
    GI::show_csv_info();
    print "The columns in datafile should contain:\n";
    print '"ORF-Name","Substrat","Ausstiche","Mittelw Score","Stabw Score","best hit","x","y"'.
      "\n\n";
    exit(1);
  }
}

GI::connectDB();
GI::define_tokenizer_columns(8); # datafile is expected to contain 8 columns

GI::message("Reading '$importfilename'..");
open(IMPORT,"<$importfilename") || GI::error("Can't read '$importfilename'");
my $lineno = 1;
my @head   = GI::tokenize_columns(<>,"$lineno of $importfilename");

my ($gb_genome,$genome_name) = GI::findCurrentGenome();
my $gb_gene_data = ARB::search($gb_genome, "gene_data", "CONTAINER");
if (!$gb_gene_data) {
  my $reason = ARB::get_error();
  GI::error("Couldn't find or create container 'gene_data' for organism '$genome_name' ($reason)");
}
GI::unmarkGenesOfGenome($gb_genome);

GI::message("Importing data to organism '$genome_name' ..");
my $gene_count = 0;

ORF: foreach (<>) { # loop over all lines from inputfile
  $lineno++;
  my @elems = GI::tokenize_columns($_,"$lineno of $importfilename");
  my ($orf,$substrate,$spots,$mean_score,$sd_score,$best_hit,$coordx,$coordy) = @elems;

  # find (or create) the orf gene:
  my ($gb_orf, $error) = GI::findORF($gb_gene_data,$genome_name,$orf,$create_ORFs,$verbose);
  if (!$error) {
    my $substrate_field = "proteome/$substrate";
    my $gb_substrate = ARB::search($gb_orf, $substrate_field, "NONE");

    if (!$gb_substrate) {
      $gb_substrate = ARB::search($gb_orf, $substrate_field, "CONTAINER");
    }

    if (!$gb_substrate) {
      my $reason = ARB::get_error();
      $error = "Could not create container '$substrate_field' ($reason)";
    }
    else {
      $error = GI::write_entry($gb_substrate, "spots", "STRING", $spots, $overwrite, $verbose);
      if (!$error) { $error = GI::write_entry($gb_substrate, "score", "INT",    $mean_score, $overwrite, $verbose); }
      if (!$error) { $error = GI::write_entry($gb_substrate, "sd",    "INT",    $sd_score, $overwrite, $verbose); }
      if (!$error) { $error = GI::write_entry($gb_substrate, "id",    "STRING", $best_hit, $overwrite, $verbose); }
      if (!$error) { $error = GI::write_entry($gb_substrate, "coordx","INT",    $coordx, $overwrite, $verbose); }
      if (!$error) { $error = GI::write_entry($gb_substrate, "coordy","INT",    $coordy, $overwrite, $verbose); }
      if (!$error) {
        my $marked = ARB::read_flag($gb_orf);
        if ($marked == 0) {
          ARB::write_flag($gb_orf,1); # mark changed genes
          $gene_count++;
        }
      }
    }
  }

  if ($error) { GI::error("$error (while parsing $lineno of $importfilename)"); }
}
close IMPORT;
GI::message("$gene_count genes modified and marked.");

GI::disconnectDB();
