#!/usr/bin/perl

use strict;
use warnings;

# -----------------------
# For all (marked) organisms, finds all (marked) genes and
# concatenates their $gene_source_field into $organism_dest_field
# -----------------------
# configure here:

my $organism_dest_field       = 'my_genes'; # or 'db_xref'
my $fail_if_dest_field_exists = 1;
my $organism_marked_only      = 1;

my $gene_source_field            = 'name'; # 'tax_xref_embl'
my $fail_if_source_field_missing = 1;
my $genes_marked_only            = 1;

my $field_seperator = ';';

# -----------------------

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  push @INC, "$arbhome/PERL_SCRIPTS/GENOME";
  1;
}

use ARB;
use GI;

my $source_fields_used  = 0;
my $dest_fields_written = 0;

my $collected;

sub collect_gene_info($) {
  my ($gb_gene) = @_;
  eval {
    my $gb_field = ARB::search($gb_gene,$gene_source_field,"NONE");
    my $content;
    if ($gb_field) {
      $content = ARB::read_as_string($gb_field);
    }
    else {
      if ($fail_if_source_field_missing==1) {
        die "field '$gene_source_field' missing\n";
      }
      $content = '';
    }
    if ($content ne '') {
      $source_fields_used++;
      if ($collected eq '') {
        $collected = $content;
      }
      else {
        $collected .= $field_seperator.$content;
      }
    }
  };
  if ($@) {
    my $name = BIO::read_name($gb_gene);
    die "@ gene '$name':\n$@";
  }
}

sub collect_to_dest_field($) {
  my ($gb_orga) = @_;
  eval {
    if ($fail_if_dest_field_exists==1) {
      if (ARB::search($gb_orga,$organism_dest_field,"NONE")) {
        die "field '$organism_dest_field' already exists\n";
      }
    }
    my $gb_dest = ARB::search($gb_orga,$organism_dest_field,"STRING");
    if (not defined $gb_dest) { die ARB::await_error(); }

    $collected = '';
    if ($genes_marked_only==1) {
      GI::with_marked_genes($gb_orga, &collect_gene_info);
    }
    else {
      GI::with_all_genes($gb_orga, &collect_gene_info);
    }
    ARB::write_string($gb_dest,$collected);
    $dest_fields_written++;
  };
  if ($@) {
    my $name = BIO::read_name($gb_orga);
    die "@ organism '$name':\n$@";
  }
}

sub handleOrganisms() {
  if ($organism_marked_only==1) {
    GI::with_marked_genomes(&collect_to_dest_field);
  }
  else {
    GI::with_all_genomes(&collect_to_dest_field);
  }
}

sub main() {
  GI::connectDB();
  eval {
    GI::message("Extracting '$gene_source_field' into '$organism_dest_field'"); 
    handleOrganisms();
    GI::message("Collected $source_fields_used '$gene_source_field' into $dest_fields_written '$organism_dest_field'");
  };
  my $err = $@;
  if ($err) { GI::message("Error in collect_gene_info.pl:\n$err"); }
  GI::disconnectDB();
}

main();
