#!/usr/bin/perl

use strict;
use warnings;
# use diagnostics;

# my $debug_verboose;

# BEGIN {
  # $debug_verboose = 1;
  # $SIG{__DIE__} = sub {
    # require Carp;
    # if ($debug_verboose>0) { Carp::confess(@_); } # with backtrace
    # else { Carp::croak(@_); }
  # }
# }


# --------------------------------------------------------------------------------

my $ARBHOME = $ENV{ARBHOME};
if (not defined $ARBHOME) { die "Environmentvariable ARBHOME has be defined"; }
if (not -d $ARBHOME) { die "ARBHOME ('$ARBHOME') does not point to a valid directory"; }

# --------------------------------------------------------------------------------

my @pictures  = (); # contains all .fig
my @pixmaps   = (); # contains all .xpm
my @helpfiles = (); # contains all .help, .pdf, .pdf.gz, .ps, .ps.gz

my %known   = (); # contains all files contained in arrays above
my %unknown = (); # contains all other files found in scanned directories

my %picture  = (); # key=subdir/name (as used in code), value=index into @pictures
my %pixmap   = (); # key=subdir/name (as used in code), value=index into @pixmaps
my %helpfile = (); # key=subdir/name (as used in code), value=index into @helpfiles

my %used = (); # key=file, value=1 -> used in code, value=2 -> used in helpfile

my %full2rel = (); # key=full ressource, value=relative ressource (w/o rootdir)
my %rel2full = (); # opposite

# --------------------------------------------------------------------------------

sub scanFiles(\@$$$$);
sub scanFiles(\@$$$$) {
  my ($files_r,$dir,$mask,$recurse,$ignoreLinks) = @_;

  my $reg = qr/$mask/;

  my @subdirs = ();
  opendir(DIR,$dir) || die "can't read directory '$dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $dir.'/'.$_;
      if (-d $full) {
        if (/unused/o) {
          print "Ignoring ressource directory '$full' (assuming it contains unused things)\n";
        }
        elsif (not (/^.svn$/o or /^CVS$/o)) {
          push @subdirs, $full;
        }
      }
      else {
        if ($ignoreLinks==0 || not -l $full) {
          if ($_ =~ $reg) {
            push @$files_r, $full;
            $known{$full} = 1;
            if (defined $unknown{$full}) { delete $unknown{$full}; }
          }
          elsif (/^Makefile$/o or /\.pl$/o) {
            ; # ignore
          }
          elsif (not defined $known{$full}) {
            $unknown{$full} = 1;
          }
        }
      }
    }
  }
  closedir(DIR);

  if ($recurse==1) {
    foreach (@subdirs) {
      scanFiles(@$files_r, $_, $mask, $recurse, $ignoreLinks);
    }
  }
}

sub scanFilesAndIndex(\%\@$$$$) {
  my ($index_r,$files_r,$dir,$mask,$recurse,$ignoreLinks) = @_;
  scanFiles(@$files_r,$dir,$mask,$recurse,$ignoreLinks);

  my $len   = length($dir)+1; # plus '/'
  my $count = scalar(@$files_r);
  for (my $c=0; $c<$count; $c++) {
    my $rel = substr($$files_r[$c], $len);
    $$index_r{$rel} = $c;
    # print "full='".$$files_r[$c]."' idx='$c' rel='$rel'\n";
  }
}

sub scanExistingRessources() {
  scanFilesAndIndex(%picture,  @pictures,  $ARBHOME.'/lib/pictures',        '.*\.(fig|vfont)$',                 1, 0);
  scanFilesAndIndex(%pixmap,   @pixmaps,   $ARBHOME.'/lib/pixmaps',         '.*\.(xpm)$',                       1, 0);
  scanFilesAndIndex(%helpfile, @helpfiles, $ARBHOME.'/HELP_SOURCE/oldhelp', '.*\.(hlp|ps|pdf|ps\.gz|pdf\.gz)$', 1, 0);
  scanFilesAndIndex(%helpfile, @helpfiles, $ARBHOME.'/HELP_SOURCE/genhelp', '.*\.(hlp|ps|pdf|ps\.gz|pdf\.gz)$', 1, 0);

  foreach (sort keys %unknown) {
    if (/readme[^\/]*$/i)                    { ; } # ignore readme files
    elsif (/\/genhelp\/.*(footer|header)$/o) { ; } # ignore files used for help generation
    else {
      print "$_:0: Unhandled file in ressource directory\n";
    }
  }

  foreach (keys %picture)  { my $full = $pictures [$picture{$_}];  $full2rel{$full} = $_; }
  foreach (keys %pixmap)   { my $full = $pixmaps  [$pixmap{$_}];   $full2rel{$full} = $_; }
  foreach (keys %helpfile) { my $full = $helpfiles[$helpfile{$_}]; $full2rel{$full} = $_; }

  foreach (keys %full2rel) { $rel2full{$full2rel{$_}} = $_; }
}

# --------------------------------------------------------------------------------

my $reg_parser = qr/([\(\),\\\"\'\;\{]|\/\*|\/\/)/;
my $reg_parse_dquotes = qr/(\\.|\")/;
my $reg_parse_squotes = qr/(\\.|\')/;
my $reg_parse_eoc = qr/\*\//;

sub scanNextToken(\$\@\$);
sub scanNextToken(\$\@\$) {
  my ($rest_r,$file_r,$lineNr_r) = @_;
  # scans for the next token (tokens are '(', ')', ',', ';' and '{')
  # and returns it together with it's prefix
  # modifies rest_r and lineNr_r
  # if no token is found until EOF, token will be 'undef'
  # reads over comments

  my $prefix = '';
  my $match = undef;
  if ($$rest_r =~ $reg_parser) {
    my ($preTok,$tok) = ($`,$&);
    $$rest_r = $';

    if ($tok eq '(' or $tok eq ')' or $tok eq ',' or $tok eq ';' or $tok eq '{') { # accept wanted tokens
      $prefix .= $preTok;
      $match = $tok;
    }
    elsif ($tok eq '\\') { # skip escaped chars
      $prefix .= $preTok.$tok.substr($$rest_r,0,1);
      $$rest_r = substr($$rest_r,1);
    }
    elsif ($tok eq '/*') { # skip /**/-comments
      $prefix .= $preTok;
      # print "prefix='$prefix' preTok='$preTok' rest_r='$$rest_r'\n";
      my $found = 0;
      while ($found==0) {
        if ($$rest_r =~ $reg_parse_eoc) {
          # print "\$`='$`' \$&='$&' \$'='$''\n";
          if (not $& eq '*/') { die "expected to see '*/', parsed '$&' from '$$rest_r' (this is a bug in check_ressources.pl!)"; }
          $$rest_r = $';
          $found = 1;
        }
        else {
          $$rest_r = $$file_r[$$lineNr_r++];
          chomp($$rest_r);
          # print "Continue in next line (while searching '*/'): '$$rest_r'\n";
          if (not defined $$rest_r) { die "Unclosed '/*'"; }
        }
      }
    }
    elsif ($tok eq '//') {
      $prefix .= $preTok;
      $$rest_r = $$file_r[$$lineNr_r++];
      chomp($$rest_r);
      # print "Continue in next line (skipping '//'): '$$rest_r'\n";
    }
    elsif ($tok eq '"') {
      $prefix .= $preTok.$tok;
      my $closed_dquote = 0;
      while ($closed_dquote==0) {
        if ($$rest_r =~ $reg_parse_dquotes) {
          $prefix .= $`.$&;
          $$rest_r = $';
          if ($& eq '"') { $closed_dquote = 1; }
        }
        else { die "Missing '\"'"; }
      }
    }
    elsif ($tok eq '\'') {
      $prefix .= $preTok.$tok;
      my $closed_squote = 0;
      while ($closed_squote==0) {
        if ($$rest_r =~ $reg_parse_squotes) {
          $prefix .= $`.$&;
          $$rest_r = $';
          if ($& eq '\'') { $closed_squote = 1; }
        }
        else { die "Missing '\''"; }
      }
    }
    else {
      die "Unknown token '$tok'";
    }

    if (not defined $match) { # continue
      my $nextPrefix;
      ($nextPrefix,$match) = scanNextToken($$rest_r, @$file_r, $$lineNr_r);
      $prefix .= $nextPrefix;
    }
  }
  else {
    $prefix .= $$rest_r;
    $$rest_r = $$file_r[$$lineNr_r++];
    chomp($$rest_r);
    # print "Continue in next line: '$$rest_r'\n";
    if (defined $$rest_r) { # not EOF yet
      my $p;
      ($p,$match) = scanNextToken($$rest_r,@$file_r,$$lineNr_r);
      $prefix .= $p;
    }
  }

  return ($prefix,$match);
}

sub scanParams($\@$\$) {
  my ($rest,$file_r,$lineNr,$calltype_r) = @_;

  $$calltype_r = 0; # no params

  my ($prefix,$token);
  my @params = ();
  eval {
    ($prefix,$token) = scanNextToken($rest,@$file_r,$lineNr);
  };
  if (!$@ and $token eq '(') {
    if (trim($prefix) ne '') {
      # print "Found prefix '$prefix' before potential parameter list - assume it's sth else\n";
    }
    else {
      my $openParens = 1;
      my $prevPrefix = '';
      while ($openParens>0) {
        ($prefix,$token) = scanNextToken($rest,@$file_r,$lineNr);
        my $seen_eop = 0;

        if (not defined $token) { die "EOF reached while scanning parameter list"; }
        elsif ($token eq ')') { $openParens--; if ($openParens==0) { $seen_eop = 1; } }
        elsif ($token eq '(') { $openParens++; }
        elsif ($token eq ',') { if ($openParens==1) { $seen_eop = 1; } }
        else { die "Unexpected token '$token' (behind '$prefix')"; }

        $prevPrefix .= $prefix;
        if ($seen_eop==1) { push @params, $prevPrefix; $prevPrefix = ''; }
        else { $prevPrefix .= $token; }
      }

      $$calltype_r = 1;
      eval {
        ($prefix,$token) = scanNextToken($rest,@$file_r,$lineNr);
      };
      if ($@) {
        @params = ();
      }
      else {
        if    ($token eq ';') { $$calltype_r = 2; } # accepted call
        elsif ($token eq '{') { $$calltype_r = 3; } # function def
        elsif ($token eq ')') { $$calltype_r = 2; } # accepted ("othercall(call())")
        elsif ($token eq ',') { $$calltype_r = 2; } # accepted ("othercall(call(),...)")
        else {
          if ($prefix =~ /__ATTR_/o) {
            $$calltype_r = 3; # function def
          }
          else {
            die "unknown token behind call: '$token' (possible call; ignored due to this error; prefix='$prefix')\n";
          }
        }
      }
    }
  }

  return @params;
}

sub trim($) {
  my ($str) = @_;
  $str =~ s/^\s+//g;
  $str =~ s/\s+$//g;
  return $str;
}

sub isQuoted($);
sub isQuoted($) {
  my ($str) = @_;
  if ($str =~ /^\"(.*)\"$/o) { return $1; }
  if ($str =~ /^\(\s*AW_CL\s*\)\s*/o) {
    return isQuoted($');
  }
  if ($str =~ 'AW_POPUP_HELP') { return 'AW_POPUP_HELP'; }
  return undef;
}

# --------------------------------------------------------------------------------

sub acceptAll($) {
  my ($res_param) = @_;
  return ($res_param);
}
sub isPixmapRef($) {
  my ($res_param) = @_;
  if ($res_param =~ /^#/) { return ($'); }
  return ();
}
sub isIconRes($) {
  my ($res_param) = @_;
  my $base = 'icons/'.$res_param;
  return ($base.'.xpm');
}
sub isHelpRef($) {
  my ($res_param) = @_;
  if ($res_param =~ /\.(hlp|ps|pdf)$/o) { return ($res_param); }
  return ();
}

my @defs =
  (
   # regexp for function,                  param numbers,         expectInIndex, isRessource,
   [ qr/\b(AW_help_popup)\b/,              [ 2 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(create_button)\b/,              [ 2 ],                 \%pixmap,      \&isPixmapRef,   ],
   [ qr/\b(create_mode)\b/,                [ 1 ],                 \%pixmap,      \&acceptAll,     ],
   [ qr/\b(create_mode)\b/,                [ 2 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(create_toggle)\b/,              [ -2, -3 ],            \%pixmap,      \&isPixmapRef,   ],
   [ qr/\b(help_text)\b/,                  [ 1 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(makeHelpCallback)\b/,           [ 1 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(AWT_create_root)\b/,            [ 2 ],                 \%pixmap,      \&isIconRes,     ],
   [ qr/\b(insert_help_topic)\b/,          [ 3 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(insert_menu_topic)\b/,          [ 2 ],                 \%pixmap,      \&isPixmapRef,   ],
   [ qr/\b(insert_menu_topic)\b/,          [ 4 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(insert_mark_topic)\b/,          [ 7 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(GEN_insert_extract_submenu)\b/, [ 6 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(GEN_insert_mark_submenu)\b/,    [ 6 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(AW_advice)\b/,                  [ -4 ],                \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(add_help)\b/,                   [ 1 ],                 \%helpfile,    \&isHelpRef,     ],
   [ qr/\b(insert_toggle)\b/,              [ 1 ],                 \%pixmap,      \&isPixmapRef,   ],
   [ qr/\b(load_xfig)\b/,                  [ 1 ],                 \%picture,     \&acceptAll,     ],

   # pseudos (used in comment to mark a ressource as used)
   [ qr/\b(uses_hlp_res)\b/,               [ 1, -2, -3, -4, -5 ], \%helpfile, \&isHelpRef,     ],
   # [ qr/\b(uses_pic_res)\b/,               [ 1, -2, -3, -4, -5 ], \%picture,  \&acceptAll,     ],
   [ qr/\b(uses_pix_res)\b/,               [ 1, -2, -3, -4, -5 ], \%pixmap,   \&acceptAll,     ],
  );

# - param numbers is [1..n] or [-1..-n] for optional params
# - isRessource gets the unquoted potential ressource (without possible '(AW_CL)'-cast) and
#   returns the plain ressource name or undef (if it's definitely no ressource)

my $defs                = scalar(@defs);
my $errors              = 0;
my $LOC                 = 0;
my $showSpecialWarnings = 0;

my @ruleMatched = ();

sub scanCodeFile($) {
  my ($file) = @_;
  open(FILE,'<'.$file) || die "can't read '$file' (Reason: $!)";

  my @file = <FILE>;
  my $flines = scalar(@file);
  unshift @file, undef; # line 0

  my $lineNr = 0;
  eval {
    for ($lineNr=1; $lineNr<=$flines; $lineNr++) {
      my $line = $file[$lineNr];
      # if ($line =~ /kernlin/io) { print "$file:$lineNr: Custom-Search: ".trim($line)."\n"; } # used to test this script

      for (my $d=0; $d<$defs; $d++) {
        my $def_r = $defs[$d];
        my $reg = $$def_r[0];
        if ($line =~ $reg) {
          my $rest = $';
          my $match = $&;
          # print "reg='$reg'\n";
          # print "$file:$lineNr: Match def #$d: $line";

          chomp($rest);
          my $calltype;
          my @params = scanParams($rest,@file,$lineNr+1,$calltype);

          if ($calltype==2) {
            my $pnum_r = $$def_r[1]; 
            my $pis = scalar(@$pnum_r);
            for (my $pi=0; $pi<$pis; $pi++) { # for all params referencing a ressource
              my $pnum = $$pnum_r[$pi];
              my $param = $params[$pnum<0 ? -$pnum-1 : $pnum-1];

              if (defined $param) {
                $param = trim($param);
                my $unquoted = isQuoted($param);
                if (defined $unquoted) {
                  my $test_r = $$def_r[3];
                  my @unquoted = &$test_r($unquoted); # test if definitely NO ressource

                  if (scalar(@unquoted)>0) { # possible ressource(s)
                    my $idx_r = $$def_r[2]; # ressource index
                    my $used = 0;
                  UNQUOTED: foreach my $unquoted (@unquoted) {
                      my $full_ressource_idx = $$idx_r{$unquoted};
                      if (not defined $full_ressource_idx and $unquoted =~ /\.(ps|pdf)$/o) {
                        $unquoted .= '.gz';  # try zipped version
                        $full_ressource_idx = $$idx_r{$unquoted};
                      }
                      if (defined $full_ressource_idx) { # existing ressource
                        my $full_ressource = $rel2full{$unquoted};
                        if (not defined $full_ressource) { die "expected ressource '$unquoted' to be defined"; }
                        $used{$full_ressource} = 1;
                        $ruleMatched[$d] = 1;
                        $used = 1;
                        last UNQUOTED;
                      }
                    }

                    if ($used==0) {
                      print "$file:$lineNr: Error: Ressource '".$unquoted[0]."' is missing\n";
                      $errors++;
                    }
                  }
                }
                else {
                  if ($showSpecialWarnings==1) {
                    print "$file:$lineNr: Warning: Param '$param' is not an explicit ressource, can't check\n";
                    # print "Params:\n"; foreach (@params) { print "- param='$_'\n"; }
                  }
                }
              }
              else {
                if ($pnum>0) {
                  print "$file:$lineNr: Warning: Param #$pnum is missing, can't check (maybe param should be optional)\n";
                }
              }
            }
          }
          else {
            if ($showSpecialWarnings==1 and $calltype!=3) { # don't warn about function definition
              print "$file:$lineNr: Warning: Matched '$match', but wrong calltype (=$calltype)\n";
            }
          }
        }
      }
    }
  };
  if ($@) {
    print "$file:$lineNr: Error: $@\n";
    $errors++;
    # if ($@ =~ /enough/) { die "enough"; }
  }
  close(FILE);
  $LOC += $flines;
}

sub autouse($) {
  my ($res) = @_;
  if (not defined $known{$res}) {
    print "Warning: Invalid autouse($res) -- unknown ressource\n";
  }
  else {
    $used{$res} = 1;
  }
}

sub scanCodeFile_forUnuseds($\$\%) {
  my ($file,$reg_r,$seen_r) = @_;
  open(FILE,'<'.$file) || die "can't read '$file' (Reason: $!)";

  my @file = <FILE>;
  my $flines = scalar(@file);
  unshift @file, undef;         # line 0

  my $lineNr = 0;
  for ($lineNr=1; $lineNr<=$flines; $lineNr++) {
    my $line = $file[$lineNr];
    while ($line =~ $$reg_r) {
      my $res = $&;
      # print "$file:$lineNr: Warning: Checker failed to detect potential usage of ressource '$res'\n";
      my $seen = $$seen_r{$res};
      if (defined $seen) { $seen .= ",$file:$lineNr"; }
      else { $seen = "$file:$lineNr"; }
      $$seen_r{$res} = $seen;
      $line = $';
    }
  }
  close(FILE);
}

my %helpScanned = ();
my $newHelpRef  = 0;

sub referenceHelp($);
sub referenceHelp($) {
  my ($referred) = @_;

  my $full_ressource_idx = $helpfile{$referred};
  if (defined $full_ressource_idx) { # existing ressource
    my $full_ressource = $rel2full{$referred};
    if (not defined $full_ressource) { die "expected ressource '$referred' to be defined"; }
    $used{$full_ressource} = 1;
    $newHelpRef++;
  }
  else {
    if ($referred =~ /\.(pdf|ps)$/) {
      referenceHelp($referred.'.gz');
    }
    elsif ($referred =~ /\@/) {
      ; # ignore mail addresses
    }
    elsif ($referred =~ /^(http|file):\/\//o) {
      ; # ignore urls
    }
    else {
      die "Ressource '".$referred."' is missing\n";
    }
  }
}

sub scanHelpFile($) {
  my ($file) = @_;
  if ($file =~ /\.hlp$/o) {
    if (defined $used{$file} and not defined $helpScanned{$file}) {
      open(FILE,'<'.$file) || die "can't read '$file' (Reason: $!)";
      my @file = <FILE>;
      my $flines = scalar(@file);
      unshift @file, undef;     # line 0
      my $lineNr = 0;
      for ($lineNr=1; $lineNr<=$flines; $lineNr++) {
        eval {
          $_ = $file[$lineNr];
          if (/#/) { $_ = $`; }  # skip comments
          if (/^\s*(SUB|UP)\s+(.*)$/o) {
            referenceHelp($2);
          }
          else {
            while (/LINK{([^\}]*)}/o) {
              my $rest = $';
              referenceHelp($1);
              $_ = $rest;
            }
          }
        };
        if ($@) {
          chomp($@);
          print "$file:$lineNr: Error: $@\n";
          $errors++;
          # if ($@ =~ /enough/) { die "enough"; }
        }
      }
      close(FILE);
      $helpScanned{$file} = 1;
      $LOC += $flines;
    }
  }
}


sub scanCode() {
  my @sources = ();
  {
    my %oldKnown = %known;
    scanFiles(@sources, $ARBHOME, '\.[ch](xx|pp){0,1}$', 1, 1); # destroys %known and %unknown
    print 'Checking '.scalar(@sources)." source files.\n";

    %known = %oldKnown;
    %unknown = ();
  }

  for (my $d=0; $d<scalar(@defs); ++$d) { $ruleMatched[$d] = 0; }
  @sources = sort @sources;
  foreach (@sources) { scanCodeFile($_); }

  {
    my $intro = 0;
    for (my $d=0; $d<scalar(@defs); ++$d) {
      if ($ruleMatched[$d] == 0) {
        if ($intro==0) { print "Some code-rules never applied:"; $intro = 1; }
        print " $d";
      }
    }
    if ($intro==1) { print "\n"; }
  }

  $newHelpRef = 1;
  while ($newHelpRef>0) {
    $newHelpRef = 0;
    foreach (@helpfiles) { scanHelpFile($_); }
  }

  print "Scanned $LOC LOC.\n";

  autouse($ARBHOME.'/HELP_SOURCE/oldhelp/unittest.hlp');

  my %unused = ();
  foreach (sort keys %known) {
    if (not defined $used{$_}) { $unused{$_} = 1; }
  }

  my $unused = scalar(keys %unused);

  if ($unused>0) {
    print "Detected $unused unused ressources.\nRunning brute force scan..\n";
    my $reg_unused = '';
    foreach (keys %unused) { $reg_unused .= '|'.quotemeta($full2rel{$_}); }
    $reg_unused = substr($reg_unused,1);
    print "reg_unused='$reg_unused'\n";
    $reg_unused = qr/$reg_unused/;

    my %seen_unused = ();
    foreach (@sources) {
      scanCodeFile_forUnuseds($_,$reg_unused, %seen_unused);
    }

    foreach (sort keys %unused) {
      my $rel = $full2rel{$_};
      my $seen = $seen_unused{$rel};
      if (defined $seen) {
        print "$_:0: Warning: Checker failed to detect ressource usage\n";
        my @seen = split(',',$seen);
        my %seen = map { $_ => 1; } @seen;
        foreach (sort keys %seen) {
          print "$_: Warning: '$rel' possibly used here\n";
        }
      }
      else {
        print "$_:0: Error: Ressource is most likely unused\n";
        $errors++;
      }
    }
  }
}

sub main() {
  print "Checking ARB ressources\n";

  scanExistingRessources();
  print ' - '.scalar(@pictures)." pictures\n";
  print ' - '.scalar(@pixmaps)." images\n";
  print ' - '.scalar(@helpfiles)." helpfiles\n";

  scanCode();
  if ($errors>0) { die "$errors errors detected by ressource checker\n"; }
}

main();
