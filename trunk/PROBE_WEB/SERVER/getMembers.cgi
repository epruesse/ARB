#!/usr/bin/perl -w

use probe_server;
use strict;

sub run_request() {
  my $plength = $probe_server::params{"plength"};
  if (not $plength) { return "Missing parameter 'plength'"; }
  if ($plength < 15 || $plength > 20) { return "Illegal value for plength (allowed 15..20)"; }

  my ($request_name,$result_name) = probe_server::generate_filenames($plength,0);
  my $error = probe_server::write_std_request($request_name, 'getmembers');
  if (not $error) {
    probe_server::wait_answer($result_name);
    $error = probe_server::send_std_result($result_name);
  }
  return $error;
}

# main block:

&probe_server::print_header();
my $error = run_request();
if ($error && length $error) {
  probe_server::print_error($error);
}
