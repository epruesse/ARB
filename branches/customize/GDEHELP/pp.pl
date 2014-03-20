#!/usr/bin/perl
# ============================================================ #
#                                                              #
#   File      : pp.pl                                          #
#   Purpose   : a simple pseudo-C-preprocessor                 #
#   Motivation:                                                #
#     The C-preprocessor behaves different on different        #
#     systems (e.g. clang, suse) while creating "arb.menu".    #
#     That resulted in various failures,                       #
#     some detected at compile-time, others at run-time.       #
#                                                              #
#   Coded by Ralf Westram (coder@reallysoft.de) in June 2012   #
#   Institute of Microbiology (Technical University Munich)    #
#   http://www.arb-home.de/                                    #
#                                                              #
# ============================================================ #

# Restrictions:
# - '#if' unsupported
# - comment parsing is error-prone

use strict;
use warnings;

sub parseOneParam(\$) {
  my ($code_r) = @_;

  my $inside = '';
  my @instack = ();

  my $param = '';

  while ($$code_r =~ /[()[\],\"\']/o) {
    my ($before,$sep,$after) = ($`,$&,$');
    my ($do_pop,$do_shift) = (0,0);

    if ($before =~ /\\$/o) { $do_shift = 1; }
    elsif ($inside eq '"' or $inside eq '\'') {
      if ($sep eq $inside) { $do_pop = 1; }
      else { $do_shift = 1; }
    }
    else {
      if ($sep eq ',') {
        $$code_r = $after;
        return $param.$before;
      }
      if ($sep eq '\'' or $sep eq '"' or $sep eq '(' or $sep eq '[') {
        push @instack, $inside;
        $inside = $sep;
        $do_shift = 1;
      }
      elsif ($sep eq ')') {
        if ($inside eq '') {
          $$code_r = $sep.$after;
          return $param.$before;
        }
        if ($inside ne '(') { die "Misplaced ')' in '$$code_r'\n"; }
        $do_pop = 1;
      }
      elsif ($sep eq ']') {
        if ($inside ne '[') { die "Misplaced ']' in '$$code_r'\n"; }
        $do_pop = 1;
      }
      else {
        die "unhandled separator: param='$param'\nbefore='$before'\nsep='$sep'\nafter='$after'\ncode_r='$$code_r'";
      }
    }

    if ($do_pop==1) {
      $inside = pop @instack;
      $do_shift = 1;
    }
    if ($do_shift==1) {
      $param .= $before.$sep;
      $$code_r = $after;
    }
  }

  $param .= $$code_r;
  $$code_r = '';

  return $param;
}

sub parseMacroParams($\@) {
  my ($code,$param_r) = @_;

  if (not $code =~ /^\(/o) { die "Expected '(', seen '$code'"; }
  $code = $';

 PARAM: while (1) {
    $code =~ s/^\s+//o;
    if ($code =~ /^\)/o) { $code = $'; last PARAM; }
    if ($code eq '') { die "Missing or misplaced ')'"; }

    my $param = parseOneParam($code);
    push @$param_r, $param;
  }
  return $code;
}

sub apply_define($\@);
sub apply_define($\@) {
  my ($line,$defr) = @_;

  my $name = $$defr[0];
  if ($line =~ /\b$name\b/) {
    my ($prefix,$suffix) = ($`,$');
    my $pcount = $$defr[1];
    if ($pcount==0) {
      return $prefix.$$defr[2].apply_define($suffix,@$defr);
    }

    my @param = ();
    $suffix = parseMacroParams($suffix,@param);

    my $paramCount = scalar(@param);
    if ($paramCount ne $pcount) {
      die "Expected $pcount arguments for macro '$name' (found $paramCount)\n";
    }

    my $expanded = $$defr[$pcount+2];
    for (my $p=0; $p<$pcount; $p++) {
      my $search = $$defr[$p+2];
      my $replace = $param[$p];
      $expanded =~ s#$search#$replace#g;
    }

    return $prefix.$expanded.apply_define($suffix,@$defr);
  }
  return $line;
}

my @define = (); # list of defines (applied in order). contains array refs to [ name, pcount, [ pnames...,] content ]
my %define = (); # known defines

sub apply_defines($) {
  my ($line) = @_;
  foreach my $defr (@define) {
    $line = apply_define($line, @$defr);
  }
  return $line;
}

sub def_define {
  my @def = @_;
  unshift @define, \@def;
  $define{$def[0]} = 1;
}

sub add_define($) {
  my ($rest) = @_;

  if ($rest =~ /^[A-Z0-9_]+/io) {
    my ($name,$param) = ($&,$');
    if ($param eq '') {
      def_define($name, 0, '');
    }
    elsif ($param =~ /^\s+/o) {
      def_define($name, 0, apply_defines($'));
    }
    elsif ($param =~ /^\(([a-z0-9,_]+)\)\s+/io) {
      my ($args,$def) = ($1,$');
      $args =~ s/\s+//oig;
      my @args = split /,/,$args;
      my $count = scalar(@args);

      my @array = ( $name, $count );
      foreach (@args) { push @array, $_; }
      push @array, apply_defines($def);
      def_define(@array);
    }
    else {
      die "invalid macro parameter '$param'";
    }
  }
  else {
    die "invalid define '$rest'\n";
  }
  
}
sub rm_define($) {
  my ($rest) = @_;
  if ($rest =~ /^[A-Z0-9_]+/io) {
    my $name = $&;
    if (exists $define{$name}) {
      @define = map {
        my $def_r = $_;
        if ($$def_r[0] eq $name) { ; }
        else { $def_r; }
      } @define;
      delete $define{$name};
    }
    else {
      die "'$name' has not been defined";
    }
  }
  else {
    die "invalid undef '$rest'\n";
  }
}
sub is_defined($) {
  my ($rest) = @_;
  if ($rest =~ /^[A-Z0-9_]+/io) {
    my $name = $&;
    exists $define{$name};
  }
  else {
    die "invalid ifdef '$rest'\n";
  }
}

my $inMultiLineComment = 0;

sub remove_comments($);
sub remove_comments($) {
  my ($line) = @_;
  if ($inMultiLineComment) {
    if ($line =~ /\*\//o) {
      $inMultiLineComment--;
      $line = $';
    }
    if ($inMultiLineComment) {
      return '';
    }
  }
  if ($line =~ /^[^'"]*\/\//o) {
    return $`."\n";
  }
  if ($line =~ /\/\*/o) {
    $inMultiLineComment++;
    return remove_comments($');
  }
  return $line;
}

sub preprocess($);

my @include = (); # list of include directories

sub include_via_ipath($) {
  my ($name) = @_;
  foreach (@include) {
    my $rel = $_.'/'.$name;
    if (-f $rel) {
      preprocess($rel);
      return;
    }
  }
  die "Could not find include file '$name'\n";
}

sub include($) {
  my ($spec) = @_;
  if ($spec =~ /^\"([^\"]+)\"/o) {
    my $name = $1;
    if (-f $name) { preprocess($name); }
    else { include_via_ipath($name); }
  }
  elsif ($spec =~ /^<([^>]+)>/o) {
    my $name = $1;
    include_via_ipath($name);
  }
  else { die "no idea how to include '$spec'\n"; }
}

sub preprocess($) {
  my ($src) = @_;

  my $skip = 0;
  my @skipstack = ();

  open(my $IN,'<'.$src) || die "can't read '$src' (Reason: $!)";
  my $line;
  while (defined($line=<$IN>)) {
    while ($line =~ /\\\n/o) { # concat multilines
      my ($body) = $`;
      my $nextLine = <$IN>;
      if (not defined $nextLine) { die "runaway multiline"; }
      $line = $body.$nextLine;
    }

    eval {
      if ($line =~ /^\s*[#]\s*([^\s]*)\s+/o) {
        my ($token,$rest) = ($1,$');
        chomp($rest);
        if ($token eq 'define') { add_define($rest); }
        elsif ($token eq 'undef') { rm_define($rest); }
        elsif ($token eq 'include') {
          my $oline = $.;
          eval { include($rest); };
          $. = $oline;
          if ($@) { die "included from here\n$@"; }
        }
        elsif ($token eq 'ifdef') {
          push @skipstack, $skip;
          $skip = is_defined($rest) ? 0 : 1;
        }
        elsif ($token eq 'else') {
          if (scalar(@skipstack)==0) { die "else w/o if\n"; }
          $skip = 1-$skip;
        }
        elsif ($token eq 'endif') {
          if (scalar(@skipstack)==0) { die "endif w/o if\n"; }
          $skip = pop @skipstack;
        }
        else { die "unknown preprocessor token='$token' rest='$rest'\n"; }
      }
      else {
        if ($skip==0) {
          $line = remove_comments($line);
          print apply_defines($line);
        }
      }
    };
    if ($@) { die "$src:$.: $@\n"; }
  }
  if (scalar(@skipstack)!=0) { die "EOF reached while inside if\n"; }
  close($IN);
}

sub addIncludePaths($) {
  my ($pathlist) = @_;
  my @paths = split /;/, $pathlist;
  foreach (@paths) { push @include, $_; }
}

sub main() {
  eval {
    my $src = undef;
    foreach (@ARGV) {
      if ($_ =~ /^-I/) {
        addIncludePaths($');
      }
      else {
        if (defined $src) { die "Multiple sources specified ('$src' and '$_')\n"; }
        $src = $_;
      }
    }

    preprocess($src);
  };
  if ($@) { die "$@ (in pp.pl)\n"; }
}
main();


