#!/usr/bin/perl -w

## next line mandatory
## maybe sometimes other type like gzip or similar useful

#use CGI qw(:standard);
use strict;

my $now = localtime(time());

my %codes = ("0" => "0000",
             "1" => "0001",
             "2" => "0010",
             "3" => "0011",
             "4" => "0100",
             "5" => "0101",
             "6" => "0110",
             "7" => "0111",
             "8" => "1000",
             "9" => "1001",
             "A" => "1010",
             "B" => "1011",
             "C" => "1100",
             "D" => "1101",
             "E" => "1110",
             "F" => "1111"
            );

# print header;
print STDOUT "Content-type: text/plain\n\n";
my $nodePath = shift();

my $hexLength = "0x".substr($nodePath, 0, 4);
my $pathLength = hex($hexLength);

my $codedPath = substr($nodePath, 4);
print "Coded Path: ".$codedPath."\n";
my $binaryPath = "";
for (my $pos = 0; $pos < length($codedPath);$pos++)
{
$binaryPath.=$codes{substr($codedPath, $pos, 1)};


}

print "number of visited nodes: ".$pathLength."\n";
print "Decoded Path: ".substr($binaryPath, 0, $pathLength)."\n";
# variable text output goes here:
print  "Test Response: $nodePath\n";
