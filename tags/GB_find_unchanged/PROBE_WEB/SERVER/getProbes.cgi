#!/usr/bin/perl -w

use probe_server;
use strict;

sub run_request() {
  my $plength = $probe_server::params{"plength"};
  if (not $plength) { return "Missing parameter 'plength'"; }
  my $exact = $probe_server::params{"exact"};

  my @requested_lengths = ();
  if ($plength eq 'all') { @requested_lengths = probe_server::available_servers(); }
  else {
    $plength = int($plength);
    if ($plength < 15 || $plength > 20) { return "Illegal value for plength (allowed '15' .. '20' or 'all')"; }
    $requested_lengths[0] = $plength;
  }

  my $diff_lens = @requested_lengths;
  my @request_name;
  my @result_name;
  my $dl;
  my $error;
  my $request_command = 'getnonexactprobes';
  if ($exact == 1) { $request_command = 'getexactprobes'; }

  # write all requests:
  for ($dl=0; $dl<$diff_lens and not $error; $dl++) {
    ($request_name[$dl],$result_name[$dl]) = probe_server::generate_filenames($requested_lengths[$dl],0);
    $error = probe_server::write_std_request($request_name[$dl], $request_command);
  }

  if ($error) { # error occurred while writing request $dl
    my $dl2;
    for ($dl2 = 0; $dl2<$dl; $dl2++) {
      probe_server::wait_answer($result_name[$dl2]); # wait for all requests written before requests $dl ..
      unlink $result_name[$dl2]; # .. and delete the result files.
    }
  }
  else {
    # all requests were written successfully
    for ($dl=0; $dl<$diff_lens; $dl++) { # wait for answers
      probe_server::wait_answer($result_name[$dl]);
    }

    if ($diff_lens == 1) { # only one result
      $error = probe_server::send_std_result($result_name[0]); # simply forward it to client
    }
    else { # merge multiple probe result files
      my @merged_result;
      my $all_found=0;

      for ($dl=0; $dl<$diff_lens; $dl++) { # wait for answers
        my %result=probe_server::get_std_result($result_name[$dl]);
        my $err;

        if ($result{'result'} eq 'ok') {
          my $found = int($result{'found'});
        PROBE: for (my $f=0; $f<$found; $f++) {
            my $probe_tag="probe$f";
            if (exists $result{$probe_tag}) {
              $merged_result[$all_found] = $result{$probe_tag};
              $all_found++;
            }
            else {
              $err = 'probe entry '.$f.' not found';
              last PROBE;
            }
          }
        } else {
          my $msg = $result{'message'};
          if ($msg and length $msg) { $err = $msg; }
          else { $err = "Unknown error"; } # strange
        }
        if ($err and not $error) { # skip subsequent errors
          $error = $err.' [length='.$requested_lengths[$dl].']';
        }
      }

      if (not $error) {
        print "result=ok\nfound=$all_found\n";
        for (my $a=0;$a<$all_found;$a++) {
          print "probe$a=$merged_result[$a]\n";
        }
      }
    }
  }

  return $error;
}

# main block:

&probe_server::print_header();
my $error = run_request();
if ($error && length $error) {
  probe_server::print_error($error);
}
