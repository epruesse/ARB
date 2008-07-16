#!/usr/bin/perl

use strict;
use warnings;

sub die_usage($) {
    my ($err) = @_;
    die
      "Usage: ./quietly.pl RXP xmlfile\n".
      "       Validates an XML file.\n".
      "       Error messages are converted into readable format\n".
      "       and superfluous output is suppressed\n".
      "Error: $err";
}

my $args = scalar(@ARGV);
if ($args != 2) { die_usage "Expected 2 arguments"; }

my $tool    = $ARGV[0];
my $xmlfile = $ARGV[1];

if ($tool ne 'RXP') { die_usage "Expected 'RXP' as 1st arg (not '$tool')"; }

my $tool_command = "rxp -v -V -V $xmlfile 2>&1 >/dev/null";

my $tool_out = `$tool_command`;
if ($? == -1) { die "Failed to execute '$tool_command'" }
elsif ($? & 127) { die sprintf("Executed command '$tool_command'\ndied with signal %d", ($? & 127)); }
else {
    my $tool_exitcode = ($? >> 8);
    if ($tool_exitcode!=0) {
        my $sep = '------------------------------------------------------------';
        # print "$sep\n";
        # print "Error executing '$tool_command' (exitcode=$tool_exitcode):\n";

        # print "$sep plain:\n";
        # print $tool_out;
        # print "$sep end of plain\n";

        use Cwd;
        my $cwd         = getcwd();
        my $cwdlen      = length($cwd);
        my $file_prefix = 'file://'.$cwd.'/';

        # print "file_prefix='$file_prefix'\n";

        my @output = ();
        while ($tool_out =~ /\n/g) {
            $tool_out = $';
            push @output, $`;
        }
        if (length $tool_out) { push @output, $tool_out; }

        my $last_line = undef;
        foreach (@output) {
            if (/Copyright.Richard/ or /Input.encoding.*output.encoding/) {
                ;
            }
            else {
                # print "line='$_'\n";
                if (/at.line.([0-9]+).char.([0-9]+).of.file:\/\/(.*)$/) {
                    my $lineno = $1;
                    my $column = $2;
                    my $file   = $3;
                    my $where = $`;
                    my $what;

                    if (substr($file,0,$cwdlen) eq $cwd) {
                        $file = substr($file,$cwdlen+1);
                    }

                    if (defined $last_line) { $what = $last_line; }
                    else { $what = 'Unknown error (parser quietly.pl failed)'; }

                    $last_line = undef;

                    if ($where =~ /^ +/) { $where = $'; }
                    if ($where =~ / +$/) { $where = $`; }

                    print "$file:$lineno:$column: $what ($where)\n";
                }
                else {
                    if (defined $last_line) { print "Unhandled output: '$last_line'\n"; }
                    $last_line = $_;
                }
            }
        }

        die "$sep $tool failed!\n";
    }
    # else {
        # print "Sucessfully executed '$tool_command'\n";
    # }
}


