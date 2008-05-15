#  ==================================================================== #
#                                                                       #
#    File      : GI.pm                                                  #
#    Purpose   : Genome import library                                  #
#    Time-stamp: <Thu Aug/05/2004 15:16 MET Coder@ReallySoft.de>        #
#                                                                       #
#                                                                       #
#  Coded by Ralf Westram (coder@reallysoft.de) in December 2003         #
#  Copyright Department of Microbiology (Technical University Munich)   #
#                                                                       #
#  Visit our web site at: http://www.arb-home.de/                       #
#                                                                       #
#                                                                       #
#  ==================================================================== #

package GI;

use ARB;

my $gb_main;
my $columns;

sub connectDB() {
  print "Connecting to running ARB database\n";
  $gb_main = ARB::open(":","r");
  if (! $gb_main ) {
    $error = ARB::get_error();
    print("$error\n");
    exit 0;
  }
  ARB::begin_transaction($gb_main);
}

sub disconnectDB() {
  ARB::commit_transaction($gb_main);
  ARB::close($gb_main);
}

sub findCurrentGenome() {
  my $organism = BIO::read_string($gb_main, 'tmp/focus/organism_name'); # aka AWAR_ORGANISM_NAME
  if ($organism eq '') {
    error("You have to select the target organism in ARB!");
  }

  my $gb_orga = BIO::find_organism($gb_main,$organism);
  if (!$gb_orga) { error("'$organism' is not a organism"); }

  return ($gb_orga,$organism);
}

sub unmarkGenesOfGenome($) {
  my ($gb_genome) = @_;
  my $gb_gene = BIO::first_marked_gene($gb_genome);
  while ($gb_gene) {
    ARB::write_flag($gb_gene, 0); # unmark
    $gb_gene = BIO::next_marked_gene($gb_gene);
  }
}

sub findORF($$$$$) {
  my ($gb_gene_data,$genome_name,$orf,$create,$verbose) = @_;
  my $error;
  my $gb_orf;
  my $gb_locus_tag = ARB::find($gb_gene_data, "locus_tag", $orf, "down_2");
  if (!$gb_locus_tag) {
    if ($create==0) {
      $error = "no gene with locus_tag '$orf' found for organism '$genome_name'";
    }
    else {
      my $gb_genome = ARB::get_father($gb_gene_data);
      $gb_orf = BIO::create_gene($gb_genome, $orf);
      if (!$gb_orf) {
        my $reason = ARB::get_error();
        $error = "cannot create gene '$orf' ($reason)";
      }
      else {
        my $gb_locus_tag = ARB::search($gb_orf, "locus_tag", "STRING");
        if (!gb_locus_tag) {
          my $reason = ARB::get_error();
          $error = "cannot create field 'locus_tag' ($reason)";
        }
        else {
          $error = ARB::write_string($gb_locus_tag, $orf);
          if ($error) {
            $error = "Couldn't write to 'locus_tag' ($error)";
          }
        }
      }
      if (!$error and $verbose==1) { print "Created new gene '$orf'\n"; }
    }
  }
  else {
    $gb_orf = ARB::get_father($gb_locus_tag);
  }

  if (!$gb_orf) { if (!$error) { die "Internal error"; }}
  return ($gb_orf,$error);
}

sub write_entry($$$$$$) {
  my ($gb_container, $field_name, $field_type, $field_content, $overwrite, $verbose) = @_;
  my $gb_field = ARB::search($gb_container, $field_name, "NONE");
  my $error;
  if (!$gb_field) {
    $gb_field = ARB::search($gb_container, $field_name, $field_type);
    if (!$gb_field) {
      my $reason = ARB::get_error();
      $error = "Can't create '$field_name' ($reason)";
    }
  }
  else {
    if ($overwrite==0) {
      $error = "Field '$field_name' already exists";
    }
  }

  if (!$error) {
    if (!$gb_field) { die "internal error"; }
    $error = ARB::write_as_string($gb_field, $field_content);
    if ($error) { $error = "Cannot write to '$field_name' ($error)"; }
  }

  return $error;
}

# --------------------------------------------------------------------------------

sub show_csv_info() {
    print "  CSV may be saved with Excel and StarCalc. It simply is a\n".
          "  comma separated list with strings quoted in \"\". The first line\n".
          "  contains the column titles.\n";
}

sub message($) {
  my ($msg) = @_;
  BIO::message($gb_main, $msg);
  print "$msg\n";
}

sub error($) {
  my ($msg) = @_;
  $msg = "Error: ".$msg;
  ARB::commit_transaction($gb_main); # this undoes all changes made by this script
#   ARB::abort_transaction($gb_main); # this undoes all changes made by this script

  ARB::begin_transaction($gb_main);
  BIO::message($gb_main, $msg);
  BIO::message($gb_main, "Script aborted!");
  ARB::commit_transaction($gb_main);
  die $msg."\n";
}

# --------------------------------------------------------------------------------

sub define_tokenizer_columns($) {
  ($columns) = @_;
}

sub tokenize_columns($$) {
  my ($line,$errline) = @_;
  chomp $line;
  $line .= ',';

  my @array = ();

  while (not $line =~ /^[ ]*$/ig) {
    if ($line =~ /^[ ]*\"([^\"]*)\"[ ]*,/ig) {
      my $content = $1;
      $line = $';
      $content =~ s/^[ ]*//ig;
      $content =~ s/[ ]*$//ig;
      push @array, $content;
    }
    elsif ($line =~ /[ ]*([0-9]+)[ ]*,/ig) {
      push @array, $1;
      $line = $';
    }
    else {
      error("cannot parse line $errline (at '$line')");
    }
  }

  my $cols = @array;
  if ($cols != $columns) {
    error("expected $columns columns (found $cols) in line $errline");
  }

  return @array;
}

# --------------------------------------------------------------------------------

1; # result of module initialization

