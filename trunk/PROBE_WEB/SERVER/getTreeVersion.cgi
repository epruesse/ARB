#!/usr/bin/perl -w

## next line mandatory
## maybe sometimes other type like gzip or similar useful

#use CGI qw(:standard);
use strict;

my $now = localtime(time());


# print header;
print STDOUT "Content-type: text/plain\n\n";
print "CURRENTVERSION_16S_27081969";
