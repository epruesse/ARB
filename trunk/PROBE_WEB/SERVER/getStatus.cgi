#!/usr/bin/perl -w

use probe_server;
use strict;
use POSIX qw(ctime);

my $treefile = probe_server::treefilename();
probe_server::print_header();
print "result=ok\n";

if (-f $treefile) {
  print "treefile=$treefile\n";
  my $mtime       = (stat($treefile))[9];
  my $time_string = ctime($mtime);
  chomp $time_string;
  print "treemod=$time_string\n";
}
else {
  print "error_tree=file not found '$treefile'\n";
}

my $workers = `ps -auxw | grep bin/arb_probe_server_worker | grep -v grep | wc -l`;
$workers =~ s/ //ig;
chomp $workers;
print "running_workers=$workers\n";

my @servers = probe_server::available_servers();
my $lengths = '';
foreach (@servers) {
  $lengths .= "$_,";
}
chop $lengths;
print "probe_lengths=$lengths\n";
