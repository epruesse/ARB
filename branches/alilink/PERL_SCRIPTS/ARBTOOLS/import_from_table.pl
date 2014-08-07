#!/usr/bin/perl
# =============================================================== #
#                                                                 #
#   File      : import_from_table.pl                              #
#   Purpose   : import data from comma- or tab-separated tables   #
#                                                                 #
#   Coded by Ralf Westram (coder@reallysoft.de) in January 2011   #
#   Institute of Microbiology (Technical University Munich)       #
#   http://www.arb-home.de/                                       #
#                                                                 #
# =============================================================== #

use strict;
use warnings;

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  push @INC, "$arbhome/PERL_SCRIPTS/lib";
  1;
}

use ARB;
use tools;

sub usage($) {
  my ($error) = @_;
  print(
        "Usage: perl import_from_table.pl --match CF --write CF [options] datafile [database outdb]\n".
        "\n".
        "Imports one column from the calc-sheet 'datafile' into an ARB database.\n".
        "\n".
        "'datafile'     should be a list of tab-separated values.\n".
        "'database'     if a name is specified, the modified DB will be saved as 'outdb'.\n".
        "               Otherwise the database running in ARB will be modified.\n".
        "\n".
        "--match CF     CF:=column,field\n".
        "               Define a 'column' in the 'datafile' and a species-'field' in the database.\n".
        "               For each row the content of the 'column' has to match the content of the\b".
        "               'field' for exactly one species in the 'database'.\n".
        "               Useful fields are 'acc' and 'name'.\n".
        "--write CF     CF:=column,field\n".
        "               For each row in 'datafile' write the content of 'column' into the\n".
        "               'field' of the species matched via --match\n".
        "\n".
        "Available 'options':\n".
        "--csv            expect 'datafile' is a list of comma-separated values (default: TAB-separated)\n".
        "--overwrite      overwrite 'field' specified via --write (default: abort if 'field' exists)\n".
        "--skip-unknown   silently skip rows that don't match any species (default: abort if no match found)\n".
        "--marked-only    only write to marked species (default: all species)\n".
        "--mark           mark species to which field has been imported (unmarks rest)\n".
        "--as-integer     use INTEGER database-type for field (default: STRING)\n"
       );

  if (defined $error) {
    die "\nError: $error";
  }
}

sub max($$) { my ($a,$b) = @_; return $a<$b ? $b : $a; }

sub parse_CF($$) {
  my ($switch,$CF) = @_;
  my ($column,$field);
  eval {
    if ($CF =~ /^([^,]+),(.*)$/o) {
      ($column,$field) = ($1,$2);
      my $int_column = int($column);
      if ($int_column<1) { die "'$column' is not a valid column\n"; }
      my $error = ARB::check_key($field);
      if (defined $error) { die "'$field' is not a valid DB field name\n"; }
    }
    else { die "',' expected in '$CF'\n"; }
  };
  if ($@) { die "in '$switch $CF': $@\n"; }
  return ($column,$field);
}

my $reg_column1 = undef;
my $reg_column2 = undef;

sub set_separator($) {
  my ($sep) = @_;
  my $rex1 = "^([^$sep\"]+)$sep"; # plain column
  my $rex2 = "^\"([^\"]+)\"$sep"; # quoted column
  $reg_column1 = qr/$rex1/;
  $reg_column2 = qr/$rex2/;
}

sub parse_row($\@) {
  my ($line,$column_r) = @_;
  @$column_r = ();
  while (($line =~ $reg_column1) or ($line =~ $reg_column2)) {
    my ($col,$rest) = ($1,$');
    push @$column_r, $col;
    $line = $rest;
  }
  push @$column_r, $line;
}

my $inform_ARB = 0;
sub main() {
  my $datafile;
  my $database     = ':';
  my $database_out;

  my ($matchcolumn,$matchfield);
  my ($writecolumn,$writefield);
  my ($skip_unknown,$overwrite,$marked_only,$mark) = (0,0,0,0);
  my $int_type = 0;

  my @no_option = ();

  set_separator("\t");

  eval {
    while (scalar(@ARGV)>0) {
      my $arg = shift @ARGV;
      if    ($arg eq '--match') { ($matchcolumn,$matchfield) = parse_CF($arg, shift @ARGV); }
      elsif ($arg eq '--write') { ($writecolumn,$writefield) = parse_CF($arg, shift @ARGV); }
      elsif ($arg eq '--csv') { set_separator(','); }
      elsif ($arg eq '--overwrite') { $overwrite = 1; }
      elsif ($arg eq '--skip-unknown') { $skip_unknown = 1; }
      elsif ($arg eq '--marked-only') { $marked_only = 1; }
      elsif ($arg eq '--mark') { $mark = 1; }
      elsif ($arg eq '--as-integer') { $int_type = 1; }
      else { push @no_option, $arg; }
    }

    $datafile = shift @no_option;
    if (not defined $datafile) { die "Missing argument 'datafile'\n"; }
    if ($datafile =~ /^--/) { die "Unknown switch '$datafile'\n"; }

    if (scalar(@no_option)) {
      $database     = shift @no_option;
      $database_out = shift @no_option;
      if (not defined $database_out) { die "Missing argument 'outdb'\n"; }
    }

    if (scalar(@no_option)) { die "Unexpected arguments: ".join(',', @no_option)."\n"; }

    if (not defined $matchcolumn) { die "Mandatory option '--match CF' missing\n"; }
    if (not defined $writecolumn) { die "Mandatory option '--write CF' missing\n"; }
  };
  if ($@) {
    usage($@);
  }

  my $gb_main = ARB::open($database, "rw");
  if ($database eq ':') {
    if ($gb_main) { $inform_ARB = 1; }
    else { expectError('db connect (no running ARB)'); }
  }
  else {
    $gb_main || expectError('db connect (wrong \'database\' specified?)');
  }

  if (not -f $datafile) { die "No such file '$datafile'\n"; }
  open(TABLE,'<'.$datafile) || die "can't open '$datafile' (Reason: $!)\n";

  my %write_table = (); # key=matchvalue, value=writevalue
  my %source_line = (); # key=matchvalue, value=source-linenumber

  eval {
    my $min_elems = max($matchcolumn,$writecolumn);
    my $line;
    while (defined($line=<TABLE>)) {
      eval {
        chomp $line;
        $line =~ s/\r+$//;
        my @row = ();
        parse_row($line,@row);

        my $relems = scalar(@row);
        if ($relems<$min_elems) {
          die "need at least $min_elems columns per table-line\n".
            "(seen only $relems column. Maybe wrong separator chosen?)\n";
        }

        my $matchvalue = $row[$matchcolumn-1];
        my $writevalue = $row[$writecolumn-1];

        if (exists $write_table{$matchvalue}) {
          die "duplicated value '$matchvalue' in column $matchcolumn (first seen in line ".$source_line{$matchvalue}.")\n";
        }
        $write_table{$matchvalue} = $writevalue;
        $source_line{$matchvalue} = $.;
      };
      if ($@) { die "in line $. of '$datafile': $@"; }
    }

    # match and write to species
    dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');

    my $report = '';

    eval {
      my %written = (); # key=matchvalue, value: 1=written, 2=skipped cause not marked
      for (my $gb_species = BIO::first_species($gb_main);
           $gb_species;
           $gb_species = BIO::next_species($gb_species)) {
        eval {
          my $species_value = BIO::read_as_string($gb_species, $matchfield);
          my $wanted_mark = 0;
          if ($species_value) {
            if (exists $write_table{$species_value}) { # found species matching table entry
              if ($marked_only==1 and ARB::read_flag($gb_species)==0) {
                $written{$species_value} = 2;
              }
              else {
                my $existing_entry = BIO::read_as_string($gb_species, $writefield);
                if ($existing_entry and not $overwrite) {
                  die "already has an existing field '$writefield'.\n".
                    "Use --overwrite to allow replacement.\n";
                }
                my $error = undef;
                if ($int_type==1) {
                  $error = BIO::write_int($gb_species, $writefield, int($write_table{$species_value}));
                }
                else {
                  $error = BIO::write_string($gb_species, $writefield, $write_table{$species_value});
                }
                if ($error) { die $error; }
                $wanted_mark = 1;
                $written{$species_value} = 1;
              }
            }
          }
          else {
            die "No such DB-entry '$matchfield'\n";
          }
          if ($mark==1) {
            my $error = ARB::write_flag($gb_species,$wanted_mark);
            if ($error) { die $error; }
          }
        };
        if ($@) {
          my $name = BIO::read_name($gb_species);
          die "species '$name': $@";
        }
      }
      my $not_found  = 0;
      my $not_marked = 0;
      {
        my %missing = ();
        my $what = $skip_unknown ? 'Warning' : 'Error';
        foreach (keys %write_table) {
          my $wr = $written{$_};
          if (not defined $wr) {
            $missing{$_} = 1;
            $not_found++;
          }
          elsif ($wr==2) { # was not marked
            $not_marked++;
          }
        }
        foreach (sort { $source_line{$a} <=> $source_line{$b}; } keys %missing) {
          print "$what: Found no matching species for line ".$source_line{$_}." ($matchfield='$_')\n";
        }
      }
      if ($not_found>0 and $skip_unknown==0) {
        die "Failed to find $not_found species - aborting.\n".
          "(Note: use --skip-unknown to allow unknown references)\n";
      }
      $report = "Entries imported: ".(scalar(keys %written)-$not_marked)."\n";
      if ($not_found>0) { $report .= "Unmatched (skipped) entries: $not_found\n"; }
      if ($not_marked>0) { $report .= "Entries not imported because species were not marked: $not_marked\n"; }

      print "\n".$report;
    };
    if ($@) {
      ARB::abort_transaction($gb_main);
      die $@;
    }
    ARB::commit_transaction($gb_main);
    if ($database ne ':') { # database has been loaded
      print "Saving modified database to '$database_out'\n";
      my $error = ARB::save_as($gb_main, $database_out, "b");
      if ($error) { die $error; }
    }
    ARB::close($gb_main);

    if ($inform_ARB==1) {
      $report =~ s/\n$//;
      `arb_message "$report"`;
    }
  };
  if ($@) {
    ARB::close($gb_main);
    die $@;
  }
  close(TABLE);
}

eval {
  main();
};
if ($@) {
  my $error = "Error in import_from_table.pl: $@";
  print $error;
  if ($inform_ARB==1) {
    $error =~ s/: /:\n/g;       # wrap error to multiple lines for ARB
    `arb_message "$error";`;
  }
  exit(-1);
}
exit(0);
