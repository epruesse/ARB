#!/usr/bin/perl -w

use probe_server;
use strict;

probe_server::print_header();
my $treefile = probe_server::treefilename();

if (-f $treefile) {
  my $mtime = (stat($treefile))[9];
  print "result=ok\n";
  print "tree_version=ARB_PS_TREE_$mtime\n";
  print "client_version=1.0\n"; # CLIENT_SERVER_VERSION -- clients version has to match (see Toolkit.java in ../CLIENT)
}
else {
  probe_server::print_critical_error("no such file:'$treefile'");
}

