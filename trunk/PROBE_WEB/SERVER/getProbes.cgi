#!/usr/bin/perl -w

use probe_server;
use strict;

sub run_request() {
  my $plength = $probe_server::params{"plength"};
  if (not $plength) { return "Missing parameter 'plength'"; }
  if ($plength < 15 || $plength > 20) { return "Illegal value for plength (allowed 15..20)"; }

  # generate request/result file names

  my $request_name = $probe_server::requestdir.'/'.$plength.'_'.$$.'_0';
  my $result_name = $request_name.".res";
  $request_name.=".req";

  # write the request
  if (not open(REQUEST,">$request_name")) { return "Can't write '$request_name'"; }
  print REQUEST "command=getprobes\n"; # write command
  for (keys %probe_server::params) { # forward all parameters to request file
    print REQUEST "$_=$probe_server::params{$_}\n";
  }
  close REQUEST;

  # now wait for server to answer request
  while (not -f $result_name) { sleep 1; }

  # send answer to client software
  if (not open(RESULT,"<$result_name")) { return "Can't read '$result_name'"; }
  foreach (<RESULT>) { print $_; }
  close RESULT;
  unlink $result_name;    # remove the result file
  return;
}

# main block:

&probe_server::print_header();
my $error = run_request();
if ($error && length $error) {
  print "result=cgi-error\nmessage=$error\n";
}
