#!/usr/bin/perl

use strict;
use warnings;

sub read_xml($);
sub read_xml($) {
  my ($xml_dir) = @_;

  # print "xml_dir='$xml_dir'\n";

  my @xml = ();
  my @sub = ();

  opendir(DIR,$xml_dir) || die "Failed to read '$xml_dir' (Reason: $!)";
  foreach (readdir(DIR)) {
    if ($_ ne '.' and $_ ne '..') {
      my $full = $xml_dir.'/'.$_;
      if (-d $full) {
        push @sub, $_;
      }
      elsif (/\.xml$/o) {
        push @xml, $_;
      }
    }
  }
  closedir(DIR);

  # foreach (@sub) { print "sub='$_'\n"; }

  foreach my $sub (@sub) {
    my @subxml = read_xml($xml_dir.'/'.$sub);
    foreach (@subxml) {
      push @xml, $sub.'/'.$_;
    }
  }

  return @xml;
}

sub print_index(\@) {
  my ($xml_r) = @_;

  my $header=<<HEADER;
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE PAGE SYSTEM 'arb_help.dtd' [
  <!ENTITY nbsp "&#160;">
  <!ENTITY acute "&#180;">
  <!ENTITY eacute "&#233;">
  <!ENTITY apostr "&#39;">
  <!ENTITY semi "&#59;">
]>
<!-- This file has been generated by ../generate_index.pl -->
<PAGE name="help_index" edit_warning="devel">
  <TITLE>ARB help index</TITLE>
  <SECTION name="List of existing helpfiles">
    <LIST>
HEADER
  my $footer=<<FOOTER;
    </LIST>
  </SECTION>
</PAGE>
FOOTER

  print $header;
  foreach my $xml (@$xml_r) {
    my $hlp  = $xml;
    $hlp =~ s/\.xml$/\.hlp/o;
    my $link = '      <T><LINK dest="'.$hlp.'" type="hlp" quoted="0"/></T>';
    print $link."\n";
  }
  print $footer;

}

sub find_indexed_xmls($$) {
  my ($index_name,$xml_dir) = @_;

  my @xml = read_xml($xml_dir);
  @xml = sort map {
    if ($_ eq $index_name) { ; } # dont index the index
    # else { $xml_dir.'/'.$_; } # prefix with xml_dir
    else { $_; }                # prefix with xml_dir
  } @xml;
  return @xml;
}

sub parse_titles($\@\%) {
  my ($xml_dir,$xml_r, $title_r) = @_;
  foreach my $name (@$xml_r) {
    my $xml = $xml_dir.'/'.$name;
    open(FILE,'<'.$xml) || die "can't read '$xml' (Reason: $!)";
    my $line;
  LINE: while (defined($line=<FILE>)) {
      if ($line =~ /<TITLE>(.*)<\/TITLE>/) {
        $$title_r{$name} = $1;
        last LINE;
      }
    }
    close(FILE);

    if (not defined $$title_r{$name}) {
      die "$xml:1: Failed to parse title\n ";
    }
  }
}

sub generate_index($$) {
  my ($index_name,$xml_dir) = @_;

  my @xml   = find_indexed_xmls($index_name,$xml_dir);
  my %title = ();
  parse_titles($xml_dir,@xml,%title);

  @xml = sort { $title{$a} cmp $title{$b}; } @xml;

  print_index(@xml);

  # foreach (@xml) { print "xml='$_'\n"; }
}

sub main() {
  my $args = scalar(@ARGV);
  if ($args != 2) { die "Usage: generate_index.pl NAME_OF_INDEX.xml XMLDIRECTORY\n "; }

  my $index_name = $ARGV[0];
  my $xml_dir    = $ARGV[1];

  if (not -d $xml_dir) { die "No such directory '$xml_dir'"; }

  generate_index($index_name,$xml_dir);
}
main();
