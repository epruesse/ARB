#!/usr/bin/perl
# ================================================================= #
#                                                                   #
#   File      : postcompile.pl                                      #
#   Purpose   : filter gcc shadow spam                              #
#                                                                   #
#   Coded by Ralf Westram (coder@reallysoft.de) in September 2007   #
#   Institute of Microbiology (Technical University Munich)         #
#   http://www.arb-home.de/                                         #
#                                                                   #
# ================================================================= #

use strict;
use warnings;

# Note: g++ must be called with -fmessage-length=0

my $show_supressed_lines = 0;
my $show_filtered_lines  = 0;

# regexps for whole line:
my $reg_file = qr/^([^:]+):([0-9]+):(([0-9]+):)?\s/; # finds all messages
my $reg_file_noline = qr/^([^:]+):\s/; # finds all messages w/o linenumber (if not matched $reg_file)
my $reg_included = qr/^In\sfile\sincluded\sfrom\s(.*)[,:]/;
my $reg_included2 = qr/^\s+from\s(.*)[,:]/;
my $reg_location = qr/^[^:]+:\sIn\sfunction\s/;
my $reg_location2 = qr/^[^:]+:\sAt\stop\slevel:/;
my $reg_clang_dirt = qr/^ANALYZE:\s/;

# regexps for messages:
my $reg_is_error = qr/^error:\s/i;
my $reg_is_warning = qr/^warning:\s/i;
my $reg_is_note = qr/^note:\s/i;
my $reg_is_instantiated = qr/^\s\s+instantiated\sfrom\s/;
my $reg_is_required = qr/^\s\s+required\s(from|by)\s/;

# regexps for warning messages (for part behind 'warning: ')
my $reg_shadow_warning = qr/^declaration\sof\s.*\sshadows\s/;
my $reg_shadow_location = qr/^shadowed\s/;

# filter unwanted -Weffc++ warnings
my $filter_Weffpp = 1;
my @reg_Weffpp = (
                  qr/only\sdefines\sprivate\sconstructors\sand\shas\sno\sfriends/, # unwanted warning about singleton-class where the only instance exists as a static member of itself
                  qr/^base\sclass\s.*has\s*(a|accessible)\snon-virtual\sdestructor/,
                  qr/\sshould\sbe\sinitialized\sin\sthe\smember\sinitialization\slist/,
                  qr/boost::icl::(insert|add)_iterator<ContainerT>.*should\sreturn/, # filter boost-iterator postfix operators warnings
                  qr/^\s\sbut\sdoes\snot\soverride/, # belongs to reg_Weffpp_copyable
                  qr/^\s\sor\s\'operator=/, # belongs to reg_Weffpp_copyable
                 );

my $filter_Weffpp_copyable = 0; # 1 = filter copy-ctor/op=-warning, 0 = check for Noncopyable and warn
my $reg_Weffpp_copyable = qr/'(class|struct)\s([A-Za-z_0-9:]+).*'\shas\spointer\sdata\smembers/; # occurs also if derived from 'Noncopyable'

# regexps for files:
my $reg_user_include       = qr/^\/usr\/include\//;
my $reg_HEADERLIBS_include = qr/\/HEADERLIBS\//;

my $stop_after_first_error = 0;
my $hide_warnings          = 0;

# ----------------------------------------

sub getModtime($) {
  my ($fileOrDir) = @_;
  my $modtime = (stat($fileOrDir))[9];
  return $modtime;
}

my %derived_from_NC = (); # key=classname, value=1/0
my $NC_save_name = $ENV{ARBHOME}.'/SOURCE_TOOLS/postcompile.sav';
my $NC_loaded    = 0;
my $NC_loaded_timestamp;
my $NC_need_save = 0;

sub load_from_NC() {
  if (-f $NC_save_name) {
    $NC_loaded_timestamp = getModtime($NC_save_name);
    my $age = $NC_loaded_timestamp - time;
    if ($age<3*60) { # never load data older than 3 min
      open(NC,'<'.$NC_save_name);
      foreach (<NC>) {
        chomp;
        if (/^([01]),/o) {
          $derived_from_NC{$'} = $1;
        }
      }
      close(NC);
    }
    else {
      $NC_loaded_timestamp = 0; # epoch
    }
  }
  else {
    $NC_loaded_timestamp = 0; # epoch
  }
  $NC_loaded = 1;
}

sub save_from_NC() {
  if ($NC_need_save==1) {
    my $mt = 0;
    if (-f $NC_save_name) { $mt = getModtime($NC_save_name); }
    if ($mt>$NC_loaded_timestamp) { # changed on disk
      load_from_NC(); # does simple merge
    }
    my $NC_save_name_private = $NC_save_name.'.'.$$;
    open(NC,'>'.$NC_save_name_private);
    foreach (sort keys %derived_from_NC) {
      print NC $derived_from_NC{$_}.','.$_."\n";
    }
    close(NC);

    rename($NC_save_name_private,$NC_save_name) || die "can't rename '$NC_save_name_private' to '$NC_save_name' (Reason: $!)";
  }
}

sub advice_derived_from_Noncopyable($$$) {
  # Note: you can silence the Noncopyable-warning by
  # adding a comment containing 'Noncopyable' behind the 'class'-line
  my ($classname,$file,$linenr) = @_;

  if ($NC_loaded==0) { load_from_NC(); }
  my $is_a_NC = $derived_from_NC{$classname};
  if (defined $is_a_NC) {
    return 0; # do not warn twice
  }

  if (not -f $file) {
    die "no such file '$file'";
  }

  my $uq_classname = $classname;
  while ($uq_classname =~ /::/o) { $uq_classname = $'; } # skip namespace prefixes

  open(FILE,'<'.$file) || die "can't read '$file' (Reason: $!)";
  my $line;
  my $line_count = 0;
 LINE: while (defined($line=<FILE>)) {
    $line_count++;
    if ($line_count==$linenr) {
      if ($line =~ /(class|struct)\s+$uq_classname(.*)Noncopyable/) {
        my $prefix = $2;
        if (not $prefix =~ /\/\//) { # if we have a comment, assume it mentions that the class is derived from a Noncopyable
          if (not $prefix =~ /virtual/) {
            print $file.':'.$linenr.': inheritance from Noncopyable should be virtual'."\n";
          }
        }
        $is_a_NC = 1;
      }
      else { $is_a_NC = 0; }
      last LINE;
    }
  }
  close(FILE);
  $derived_from_NC{$classname} = $is_a_NC;
  $NC_need_save = 1;

  return 1-$is_a_NC;
}

# results for Weffpp_warning_wanted():
my $WANTED            = 0;
my $WANTED_NO_RELATED = 1;
my $UNWANTED          = 2;

sub Weffpp_warning_wanted($$\$) {
  my ($file,$line,$warning_text_r) = @_;

  if ($filter_Weffpp==1) {
    foreach my $reg (@reg_Weffpp) {
      return $UNWANTED if ($$warning_text_r =~ $reg);
    }
    if ($$warning_text_r =~ $reg_Weffpp_copyable) {
      my $classname = $2;
      return $UNWANTED if ($filter_Weffpp_copyable==1);
      return $UNWANTED if (advice_derived_from_Noncopyable($classname,$file,$line)==0);
      $$warning_text_r = $$warning_text_r.' (and is neighter derived from Noncopyable nor defines copy-ctor and op=)';
      return $WANTED_NO_RELATED;
    }
  }
  return $WANTED;
}

sub warning($\@) {
  my ($msg,$out_r) = @_;
  push @$out_r, '[postcompile]: '.$msg;
}

my $shadow_warning = undef;

sub store_shadow($\@) {
  my ($warn,$out_r) = @_;
  if (defined $shadow_warning) {
    warning('unprocessed shadow_warning:', @$out_r);
    push @$out_r, $shadow_warning;
  }
  $shadow_warning = $warn;
}

my $last_pushed_related = 0;

sub push_loc_and_related($$\@\@) {
  my ($location_info,$message,$related_r,$out_r) = @_;
  if (defined $location_info) {
    push @$out_r, $location_info;
    $location_info = undef;
  }
  push @$out_r, $message;
  $last_pushed_related = scalar(@$related_r);
  foreach (@$related_r) { push @$out_r, $_; }
}

sub drop_last_pushed_relateds(\@) {
  my ($out_r) = @_;
  if ($last_pushed_related>0) {
    my $before_related = scalar(@$out_r) - $last_pushed_related - 1;
    $before_related>=0 || die "impossible (out_r-elements=".scalar(@$out_r)."; last_pushed_related=$last_pushed_related)";
    @$out_r = @$out_r[0 .. $before_related];
  }
}

sub included_from_here($) {
  my ($loc) = @_;
  return $loc.': included from here';
}

sub suppress($\@) {
  my ($curr,$out_r) = @_;
  if ($show_supressed_lines==1) {
    warning('suppressed: '.$curr,@$out_r);
  }
  return undef;
}

sub is_system_or_builtin($) {
  my ($file) = @_;
  return (($file =~ $reg_user_include) or ($file eq '<built-in>') or ($file =~ $reg_HEADERLIBS_include));
}

sub suppress_shadow_warning_for($) {
  my ($file) = @_;
  return is_system_or_builtin($file);
}

sub parse_input(\@) {
  my ($out_r) = @_;
  my @related = ();
  my $location_info = undef;

  my @warnout = ();
  my @errout = ();

  my $did_show_previous = 0;
  my $is_error          = 0;
  my $curr_out_r = \@warnout;

 LINE: while (defined($_=<STDIN>)) {
    chomp;
    my $filter_current = 0;

    if ($_ =~ $reg_file) {
      my ($file,$line,$msg) = ($1,$2,$');

      if ($msg =~ $reg_is_warning) {
        my $warn_text = $';
        if ($warn_text =~ $reg_shadow_warning) {
          if (not $' =~ /this/) { # don't store this warnings (no location follows)
            store_shadow($_,@warnout);
            $_ = suppress($_,@warnout);
          }
          elsif (suppress_shadow_warning_for($file)) {
            $_ = suppress($_,@warnout);
            # $location_info = undef;
          }
        }
        elsif ($warn_text =~ $reg_shadow_location) {
          if (not defined $shadow_warning) { warning('no shadow_warning seen',@warnout); }
          else {
            if (suppress_shadow_warning_for($file)) {
              # don't warn about /usr/include or <built-in> shadowing
              $_ = suppress($_,@warnout);
              @related = ();
              $location_info = undef;
            }
            else {
              if (defined $location_info) {
                push @warnout, $location_info;
                $location_info = undef;
              }
              push @warnout, $shadow_warning;
            }
            $shadow_warning = undef;
          }
        }
        else {
          my $warn_is = Weffpp_warning_wanted($file,$line,$warn_text);
          if ($warn_is == $UNWANTED) {
            $filter_current = 1; # ignore this warning
          }
          elsif ($warn_is == $WANTED_NO_RELATED) {
            @related = (); # drop related messages
          }
          # rebuild warning (Weffpp_warning_wanted might modify the message)
          $_ = $file.':'.$line.': warning: '.$warn_text;
        }
        $is_error = 0;
        $curr_out_r = \@warnout;
      }
      elsif ($msg =~ $reg_is_error) {
        $is_error = 1;
        $curr_out_r = \@errout;
      }
      elsif ($msg =~ $reg_is_instantiated) {
        push @related, $_;
        $_ = suppress($_,@warnout);
      }
      elsif ($msg =~ $reg_is_required) {
        push @related, $_;
        $_ = suppress($_,@warnout);
      }
      elsif ($msg =~ $reg_is_note) {
        if ($did_show_previous==0) {
          $_ = suppress($_,@warnout);
        }
        else {
          if ($msg =~ /in\sexpansion\sof\smacro/o) {
            drop_last_pushed_relateds(@$curr_out_r);
          }
        }
      }
    }
    elsif ($_ =~ $reg_location or $_ =~ $reg_location2) {
      $location_info = $_;
      $_ = suppress($_,@warnout);
    }
    elsif ($_ =~ $reg_included) {
      push @related, included_from_here($1);
      $_ = suppress($_,@warnout);
    }
    elsif ($_ =~ $reg_clang_dirt) {
      $_ = undef;
    }
    elsif ($_ =~ $reg_file_noline) {
      if (/^cc1plus:.*error/) {
        ; # display normally
      }
      else {
        push @related, included_from_here($1);
        $_ = suppress($_,@warnout);
      }
    }
    elsif (@related) {
      if ($_ =~ $reg_included2) {
        push @related, included_from_here($1);
        $_ = suppress($_,@warnout);
      }
    }

    if (defined $_) {
      if ($filter_current==0) {
        push_loc_and_related($location_info,$_,@related,@$curr_out_r);
        $did_show_previous = 1;

        if (($is_error==1) and ($stop_after_first_error==1)) {
          @warnout = (); # drop warnings
          last LINE;
        }
      }
      else {
        die "can't filter errors (Error=$_)" if ($is_error==1);
        if ($show_filtered_lines==1) { warning('filtered: '.$_, @$curr_out_r); }
        $did_show_previous = 0;
      }
      @related = ();
    }
  }

  @$out_r = @errout;
  if ($hide_warnings==0) { push @$out_r, @warnout; }
}

sub main() {
  my $args = scalar(@ARGV);
  my $pass_through = 0;
  while ($args>0) {
    my $arg = shift(@ARGV);
    if ($arg eq '--no-warnings') { $hide_warnings = 1; }
    elsif ($arg eq '--only-first-error') { $stop_after_first_error = 1; }
    elsif ($arg eq '--original') { $pass_through = 1; }
    elsif ($arg eq '--show-useless-Weff++') { $filter_Weffpp = 0; }
    elsif ($arg eq '--hide-Noncopyable-advices') { $filter_Weffpp_copyable = 1; }
    else {
      die "Usage: postcompile.pl [--no-warnings] [--only-first-error] [--filter-Weff++] [--hide-Noncopyable-advices] [--original]\n";
    }
    $args--;
  }

  eval {
    if ($pass_through==1) {
      while (defined($_=<STDIN>)) { print $_; }
    }
    else {
      my @out = ();
      parse_input(@out);
      store_shadow(undef,@out);
      foreach (@out) { print "$_\n"; }
    }
  };
  my $err = $@;
  save_from_NC();
  if ($err) { die $err; }
}
main();
