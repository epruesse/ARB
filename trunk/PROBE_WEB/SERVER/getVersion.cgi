#!/usr/bin/perl -w

use probe_server;
use strict;

probe_server::print_header();
my $treefile = probe_server::treefilename();

if (-f $treefile) {
  my $mtime = (stat($treefile))[9];
  print "result=ok\n";
  print "tree_version=ARB_PS_TREE_$mtime\n";

  # search globally for 'CLIENT_SERVER_VERSIONS' (other occurance is in Toolkit.java in ../CLIENT)
  print "client_version=1.0\n"; # newest version of client available on out server
  print "interface_version=1.0\n"; # client-server interface version
}
else {
  probe_server::print_critical_error("no such file:'$treefile'");
}

