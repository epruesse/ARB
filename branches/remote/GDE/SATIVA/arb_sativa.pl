#!/usr/bin/perl

#############################################################################
#                                                                           #
# File:    arb_sativa.pl                                                    #
# Purpose: Adapter script for SATIVA taxonomy validation pipeline           #
#                                                                           #
# Author: Alexey Kozlov (alexey.kozlov@h-its.org)                           # 
#         2015 HITS gGmbH / Exelixis Lab                                    #
#         http://h-its.org  http://www.exelixis-lab.org/                    #
#                                                                           #
#############################################################################

use strict;
use warnings;
# use diagnostics;

BEGIN {
  if (not exists $ENV{'ARBHOME'}) { die "Environment variable \$ARBHOME has to be defined"; }
  my $arbhome = $ENV{'ARBHOME'};
  push @INC, "$arbhome/lib";
  push @INC, "$arbhome/PERL_SCRIPTS/lib";
  1;
}

use ARB;
use tools;

my $sativa_home = $ENV{'ARBHOME'}.'/lib/sativa';
my ($start_time, $trainer_time, $mislabels_time);

# ------------------------------------------------------------

sub expectEntry($$) {
  my ($gb_father,$key) = @_;
  my $gb = ARB::search($gb_father, $key, 'NONE');
  defined $gb || die "Expected entry '$key' in '".ARB::get_db_path($gb_father)."'";
  return $gb;
}

# -----------------------

sub exportTaxonomy($$$$$\@) {
  my ($seq_file,$tax_file,$tax_field,$sp_field,$marked_only,@marklist_r) = @_;

  open(TAXFILE,'>'.$tax_file) || die "can't open '$tax_file' (Reason: $!)";
 # open(SEQFILE,'>'.$seq_file) || die "can't open '$seq_file' (Reason: $!)";

  my $gb_main = ARB::open(":","r");
  $gb_main || expectError('db connect (no running ARB?)');

  dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');

  for (my $gb_species = BIO::first_species($gb_main);
       $gb_species;
       $gb_species = BIO::next_species($gb_species))
       {
         my $marked = ARB::read_flag($gb_species);
         if ($marked==1 || $marked_only==0) {
           my $acc_no = BIO::read_string($gb_species, "name");          
           $acc_no || expectError('read_string acc');
           my $gb_field = ARB::entry($gb_species, $tax_field);
           if (not defined $gb_field) {      
             print "WARNING: field ".$tax_field." is missing for sequence ".$acc_no."\n";
             next;
           }     
           my $tax = BIO::read_string($gb_species, $tax_field);
           $tax || expectError('read_string '.$tax_field);
           my $lineage = $tax;
           if (substr($lineage, -1) eq ';') {
             chop($lineage);
           }
           if ($sp_field) {
             my $species_name = BIO::read_string($gb_species, $sp_field);
             if (defined $species_name) {
               $lineage = $lineage.";".$species_name;
             }
           }
           
           print TAXFILE $acc_no."\t".$lineage."\n";

#           my $gb_ali = expectEntry($gb_species, 'ali_16s');
#           my $gb_data = expectEntry($gb_ali, 'data');
#           my $seq = ARB::read_string($gb_data);

#           print SEQFILE ">".$acc_no."\n";
#           print SEQFILE $seq."\n";

           push(@marklist_r, $acc_no);
         }
       }

  ARB::commit_transaction($gb_main);
  ARB::close($gb_main);

  close(TAXFILE);
#  close(SEQFILE);
}

# ------------------------------------------------------------

sub cpu_has_feature($) {
    my ($feature) = @_;
    my $cmd;
    my $uname = `uname`;
    chomp($uname);

    if ($uname eq "Darwin") {
       $cmd = "sysctl machdep.cpu.features";
    }
    elsif ($uname eq "Linux") {
       $cmd = "grep flags /proc/cpuinfo";
    }
    else {
       return 0;
    }

    `$cmd | grep -qi "$feature"`;
    return $? + 1;
}

sub cpu_get_cores() {
    my $cores = 2;
    my $uname = `uname`;
    chomp($uname);
    if ($uname eq "Darwin") {
       $cores = `sysctl -n hw.ncpu`;
    }
    elsif ($uname eq "Linux") {
       $cores = `grep -c "^processor" /proc/cpuinfo`;
    }

    return $cores;
}

sub runPythonPipeline($$$$$$$$$) {
  my ($seq_file,$tax_file,$tax_code,$raxml_method,$num_reps,$conf_cutoff,$rand_seed,$rank_test,$verbose) = @_;

  my $refjson_file = 'arb_export.json';
  my $cfg_file = $sativa_home.'/epac.cfg.sativa';

  # auto-configure RAxML
  my $cores = cpu_get_cores();

  my $bindir = $ENV{'ARBHOME'}."/bin";
  my $tmpdir = `pwd`;
  chomp($tmpdir);

  my $stime = time;

  my $sativa_cmd = $sativa_home.'/sativa.py';
  my @args = ($sativa_cmd, '-t', $tax_file, '-s', $seq_file, '-c', $cfg_file, '-T', $cores, '-tmpdir', $tmpdir, 
               '-N', $num_reps, '-x', $tax_code, '-m', $raxml_method, '-p', $rand_seed, '-C', $conf_cutoff);
  if ($rank_test == 1) { push(@args, "-ranktest"); }
  if ($verbose == 1) { push(@args, "-v"); }
  system(@args);

#  $trainer_time = time - $stime;
}

# ------------------------------------------------------------

sub deleteIfExists($$) {
  my ($father,$key) = @_;

  my $gb_field = ARB::entry($father,$key);
  if (defined $gb_field) {  
    my $error = ARB::delete($gb_field);
    if ($error) { die $error; }
  }
}

sub importResults($$$$) {
  my ($res_file,$marked_only,$mark_misplaced,$field_suffix) = @_;

  open(FILE,'<'.$res_file) || die "can't open '$res_file' (Reason: $!)";

  my %mis_map = ();

  # skip the header
  my $header = <FILE>;

  while (my $line = <FILE>) {
    chomp $line;
    my @fields = split "\t" , $line;
    my $seq_id = $fields[0];
    my $lvl = $fields[1];
    my $conf = $fields[4];
    my $new_tax = $fields[6];
    $mis_map{$seq_id} = {conf => $conf, new_tax => $new_tax, mis_lvl => $lvl};
    if (scalar(@fields) == 9) { $mis_map{$seq_id}{'rank_conf'} = $fields[8]; }
  }
  close(FILE);

  my $gb_main = ARB::open(":","rw");
  $gb_main || expectError('db connect (no running ARB?)');

  dieOnError(ARB::begin_transaction($gb_main), 'begin_transaction');

  my $count = 0;
  for (my $gb_species = BIO::first_species($gb_main);
       $gb_species;
       $gb_species = BIO::next_species($gb_species))
       {
         my $error;
         my $name = BIO::read_string($gb_species, "name");          
         $name || expectError('read_string name');
         my $marked = ARB::read_flag($gb_species);
         if (defined $mis_map{$name}) {
           $error = BIO::write_int($gb_species, "sativa_mislabel_flag".$field_suffix, 1);
           if ($error) { die $error; }
           $error = BIO::write_string($gb_species, "sativa_tax".$field_suffix, $mis_map{$name}{'new_tax'});
           if ($error) { die $error; }
           $error = BIO::write_float($gb_species, "sativa_seqmis_conf".$field_suffix, $mis_map{$name}{'conf'});
           if ($error) { die $error; }
           $error = BIO::write_string($gb_species, "sativa_mislabel_level".$field_suffix, $mis_map{$name}{'mis_lvl'});
           if ($error) { die $error; }
           if (exists $mis_map{$name}{'rank_conf'}) {
               $error = BIO::write_string($gb_species, "sativa_rankmis_conf".$field_suffix, $mis_map{$name}{'rank_conf'});
               if ($error) { die $error; }
           }
           
           if ($mark_misplaced==1) { ARB::write_flag($gb_species, 1); }
           $count++; 
         }
         elsif ($marked==1 || $marked_only==0) {
           $error = BIO::write_int($gb_species, "sativa_mislabel_flag".$field_suffix, 0);
           if ($error) { die $error; }
 
#           deleteIfExists($gb_species, "sativa_tax");
#           deleteIfExists($gb_species, "sativa_conf");
#           deleteIfExists($gb_species, "sativa_seqmis_conf");
#           deleteIfExists($gb_species, "sativa_mislabel_level");
#           deleteIfExists($gb_species, "sativa_misrank_conf");

           if ($mark_misplaced==1) { ARB::write_flag($gb_species, 0); }
         }
       }

  print "\nMislabels found: $count\n\n";
  ARB::commit_transaction($gb_main);
  ARB::close($gb_main);
}


# ------------------------------------------------------------

sub die_usage($) {
  my ($err) = @_;
  print "Purpose: Run SATIVA taxonomy validation pipeline\n";
  print "and import results back into ARB\n";
  print "Usage: arb_sativa.pl [--marked-only] [--mark-misplaced] [--rank-test] tax_field tax_code num_reps raxml_method conf_cutoff rand_seed verbose species_field\n";
  print "       tax_field         Field contatining full (original) taxonomic path (lineage)\n";
  print "       species_field     Field containing species name\n";
  print "       --marked-only     Process only species that are marked in ARB (default: process all)\n";
  print "       --mark-misplaced  Mark putatively misplaced species in ARB upon completion\n";
  print "       --rank-test       Test for misplaced higher ranks\n";
  print "\n";
  die "Error: $err\n";
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args<1) { die_usage('Missing arguments'); }

  my $tax_file      = 'sativa_in.tax';
  my $seq_file      = 'sativa_in.phy';
  my $res_file      = 'sativa_in.mis';

  my $marked_only = 0;
  my $mark_misplaced = 0;
  my $rank_test = 0;
  my $verbose = 0;

  while (substr($ARGV[0],0,2) eq '--') {
    my $arg = shift @ARGV;
    if ($arg eq '--marked-only') { $marked_only = 1; }
    elsif ($arg eq '--mark-misplaced') { $mark_misplaced = 1; }
    elsif ($arg eq '--rank-test') { $rank_test = 1; }
    elsif ($arg eq '--verbose') { $verbose = 1; }
    else { die_usage("Unknown switch '$arg'"); }
    $args--;
  }

  my $tax_field  = shift @ARGV;
  my $tax_code = shift @ARGV;
  my $num_reps = shift @ARGV;
  my $raxml_method = shift @ARGV;
  my $conf_cutoff = shift @ARGV;
  my $rand_seed = shift @ARGV;
  my $species_field = shift @ARGV;
  
  my $field_suffix = $tax_field;
  $field_suffix =~ s/^tax\_//g;
  $field_suffix = "_".$field_suffix;
  
  my @marklist = {};
  
  $start_time = time;

  exportTaxonomy($seq_file,$tax_file,$tax_field,$species_field,$marked_only,@marklist);

  runPythonPipeline($seq_file,$tax_file,$tax_code,$num_reps,$raxml_method,$conf_cutoff,$rand_seed,$rank_test,$verbose);

  importResults($res_file,$marked_only,$mark_misplaced,$field_suffix);
  
  my $total_time = time - $start_time;
#  print "Elapsed time: $total_time s (trainer: $trainer_time s, find_mislabels: $mislabels_time s)\n\n";
  
  print "Done!\n\n";
}

main();

