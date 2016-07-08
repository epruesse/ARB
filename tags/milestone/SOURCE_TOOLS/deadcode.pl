#!/usr/bin/perl

use strict;
use warnings;

my @symbol_priority = (
                       '::', # class members
                       '^TEST_', # ARB unit tests
                       '^XS_', # perl xsub interface
                       '^main$',
                      );

my $reg_file_exclude = qr/\/(GDE|EISPACK|READSEQ|PERL2ARB)\//;


sub findObjects(\@) {
  my ($libs_r) = @_;

  my $cmd = 'find . -name "*.o"';
  open(FOUND,$cmd.'|') || die "can't execute '$cmd' (Reason: $?)";
  foreach (<FOUND>) {
    chomp $_;
    if (not $_ =~ $reg_file_exclude) {
      push @$libs_r, $_;
    }
  }
  close(FOUND);
}

sub is_weak($) {
  my ($type) = @_;
  return ($type =~ /^[vVwW]$/o);
}

# ------------------------------ store symbols

my %def_loc = (); # key = symbol, value=ref to array [ file, line, type ]
my %dupdef_loc = (); # key = symbol, value=ref to array of refs to array [ file, line, type ]

my %referenced = (); # key=symbol, value=1 -> has been referenced

sub set_definition($$$$) {
  my ($sym,$file,$line,$type) = @_;
  my @array = ($file,$line,$type);
  $def_loc{$sym} = \@array;
}

sub add_dup_definition($$$$) {
  my ($sym,$file,$line,$type) = @_;

  my @array = ($file,$line,$type);
  my $dups_r = $dupdef_loc{$sym};

  if (not defined $dups_r) {
    my @dups = ( \@array );
    $dupdef_loc{$sym} = \@dups;
  }
  else {
    my $add = 1;
  LOOKUP: foreach my $duploc_r (@$dups_r) {
      my ($dfile,$dline,$dtype) = @$duploc_r;
      if (($dfile eq $file) and ($dline eq $line)) { # already have that location
        $add = 0;
        last LOOKUP;
      }
    }
    if ($add==1) {
      push @$dups_r, \@array;
    }
  }
}

sub definesSymbol($$$$$) {
  my ($obj,$file,$line,$sym,$type) = @_;

  my $loc_r = $def_loc{$sym};
  if (not defined $loc_r) { set_definition($sym,$file,$line,$type); }
  else {
    my ($pfile,$pline,$ptype) = @$loc_r;
    if (($file ne $pfile) and ($line != $pline)) { # locations differ
      if (is_weak($ptype) and not is_weak($type)) {
        set_definition($sym,$file,$line,$type);
        add_dup_definition($sym,$pfile,$pline,$ptype);
      }
      else { add_dup_definition($sym,$file,$line,$type); }
    }
  }
}

sub referencesSymbol($$$) {
  my ($obj,$sym,$type) = @_;
  $referenced{$sym} = $obj;
}

# ------------------------------ analyse

sub list_unreferenced_symbols() {
  print "Checking unreferenced symbols:\n";

  my @undefs = ();
  foreach my $sym (keys %def_loc) {
    my $ref_r = $referenced{$sym};
    if (not defined $ref_r) {
      my $def_r = $def_loc{$sym};
      my ($file,$line,$type) = @$def_r;
      if (not is_weak($type) and # ignore weak unrefs
          not $file =~ /^\/usr\/include\//o # ignore unrefs if /usr/include
          ) { 
        push @undefs, $sym;
      }
    }
  }

  @undefs = sort {
    my $la_r = $def_loc{$a};
    my $lb_r = $def_loc{$b};
    my $cmp = $$la_r[0] cmp $$lb_r[0];
    if ($cmp==0) { $cmp = $$la_r[1] <=> $$lb_r[1]; }
    $cmp;
  } @undefs;

  my %importance = map { $_ => 1; } @undefs; # key=sym, value=importance (lower = more important)

  my $regs = scalar(@symbol_priority);
  for (my $r = 0; $r<$regs; $r++) {
    my $expr = $symbol_priority[$r];
    my $imp = $r+2;
    my $reg = qr/$expr/;
    foreach my $sym (@undefs) {
      if ($sym =~ $reg) {
        $importance{$sym} = $imp;
      }
    }
  }
  my $max_imp = $regs+1;
  for (my $i=1; $i<=$max_imp; $i++) {
    print "Symbols for importance==$i:\n";
    foreach my $sym (@undefs) {
      if ($importance{$sym} == $i) {
        my $def_r = $def_loc{$sym};
        my ($file,$line,$type) = @$def_r;
        print "$file:$line: unreferenced '$sym' [$type]\n";
      }
    }
  }
}

sub list_duplicate_defines() {
  print "Checking duplicate definitions:\n";
  foreach my $sym (keys %dupdef_loc) {
    my $main_def_r = $def_loc{$sym};
    my ($file,$line,$type) = @$main_def_r;
    if (not is_weak($type)) { # dont warn about weak symbols
      my $dup_def_r = $dupdef_loc{$sym};
      my $onlyWeakDups = 1;
      foreach my $dup_r (@$dup_def_r) {
        my ($dfile,$dline,$dtype) = @$dup_r;
        if (not is_weak($dtype)) {
          if ($onlyWeakDups==1) { # first non-weak dup -> start
            print "$file:$line: Multiple definition of '$sym' [$type]\n";
            $onlyWeakDups = 0;
          }
          print "$dfile:$dline: duplicate definition [$dtype]\n";
        }
      }
    }
  }
}

# ------------------------------ parse

my $reg_def = qr/^(.*)\s([BDRTVW])\s([0-9a-f]+)\s([0-9a-f]+)\s+([^\s]+):([0-9]+)$/;
my $reg_def_noloc = qr/^(.*)\s([BDRTVW])\s([0-9a-f]+)\s([0-9a-f]+)$/;
my $reg_def_noloc_oneadd = qr/^(.*)\s([AT])\s([0-9a-f]+)\s+$/;

my $reg_refer = qr/^(.*)\s([Uw])\s+([^\s]+):([0-9]+)$/;
my $reg_refer_noloc = qr/^(.*)\s([Uw])\s+$/;

sub scanObj($) {
  my ($obj) = @_;

  my $cmd = 'nm -l -P -p -C -g '.$obj;
  open(SYMBOLS,$cmd.'|') || die "can't execute '$cmd' (Reason: $?)";
  my $line;
  while (defined($line=<SYMBOLS>)) {
    chomp($line);

    if ($line =~ $reg_def) {
      my ($sym,$type,$add1,$add2,$file,$line) = ($1,$2,$3,$4,$5,$6);
      definesSymbol($obj,$file,$line,$sym,$type);
    }
    elsif ($line =~ $reg_def_noloc) { ; } # ignore atm
    elsif ($line =~ $reg_def_noloc_oneadd) { ; } # ignore atm
    elsif ($line =~ $reg_refer) {
      my ($sym,$type,$file,$line) = ($1,$2,$3,$4);
      referencesSymbol($obj,$sym,$type);
    }
    elsif ($line =~ $reg_refer_noloc) {
      my ($sym,$type) = ($1,$2);
      referencesSymbol($obj,$sym,$type);
    }
    else {
      die "can't parse line '$line'\n";
    }
  }
  close(SYMBOLS);
}

sub main() {
  print "DeadCode detector\n";
  print "- detects useless external linkage, that could go static\n";
  print "  (then the compiler will warn if code/data is unused)\n";
  print "- needs compilation with DEBUG information\n";
  print "- also lists\n";
  print "  - useless stuff like class-members, xsub-syms\n";
  print "  - duplicated global symbols\n";

  my @objs;
  findObjects(@objs);
  print 'Examining '.scalar(@objs)." objs\n";

  foreach my $obj (@objs) {
    scanObj($obj);
  }

  list_unreferenced_symbols();
  list_duplicate_defines();
}
main();
