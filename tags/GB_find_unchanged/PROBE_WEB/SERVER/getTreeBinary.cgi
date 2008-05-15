#!/usr/bin/perl -w

use probe_server;
use strict;

my $treefile = probe_server::treefilename();

if (-f $treefile) {
  probe_server::print_special_header('application/octet-stream');
  system("cat $treefile");
}
else {
  probe_server::print_header();
  probe_server::print_critical_error("no such file:'$treefile'");
}

