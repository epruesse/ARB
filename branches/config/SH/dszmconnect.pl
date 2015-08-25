#!/usr/bin/perl -w
# switches for stringent coding and debugging

# This program is an adapter between the ARB program package and the DSZM website.
# It allows to run a taxonomic query using the web form offered by the DSZM via 
# the http-POST method implemented by the LWP package of perl.
# Subsequent modification of relative URLs into absolute one allows allows the browser
# started by ARB to connect with the DSZM website.
#  (c) Lothar Richter Oct. 2003

use strict;
use diagnostics;


# script for automated information retrieval from DSZM


# moduls in use
use LWP::Simple;
use HTTP::Request::Common qw(POST);
use LWP::UserAgent;

use File::Temp qw/ tempfile tempdir /;




#die"code successful parsed and compiled\n";




my $errordocument = 
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">
<html>
  <head>
    <title>DSMZ Failure</title>
  </head>

  <body>
    <h1>DSZM Access Error</h1>
<br/>
You have given no search items! Please give at least on search item to access the taxonomic search tool at the DSMZ!

    <hr>
    <address><a href=\"mailto:\">Lothar Richter</a></address>
<!-- Created: Mon Sep  8 14:23:58 CEST 2003 -->
<!-- hhmts start -->
Last modified: Mon Sep  8 14:25:42 CEST 2003
<!-- hhmts end -->
  </body>
</html>";

my $TMPOUT;
my $template = 'arbdsmz_XXXXXX';
(undef, $TMPOUT) = tempfile($template, OPEN => 0);
$TMPOUT = $TMPOUT . ".html";

open (OUTPUT , "> $TMPOUT") or die "cannot open temporary input file $TMPOUT";

if (scalar(@ARGV) == 0)
  {print OUTPUT $errordocument;
   die("no search items given ! Give at least one item!");}
##print length(@ARGV)."\n";
my $item1 = shift() || "";
##print $item1."\n";
my $item2 = shift() || "";

print STDERR "Searching for '$item1'\n";
print STDERR "Searching for '$item2'\n";

#--------------------------------------------------------------------------------
# begin of post-method emulations
#--------------------------------------------------------------------------------


 my $ua_selection = LWP::UserAgent ->new ;

 #$ua_selection -> agent ("UpdateAgent/0.1" . $ua_selection -> agent);

 ##my $req_selection = new HTTP::Request POST => $baseURL;
 my $req_selection =  HTTP::Request -> new( POST => 'http://www.dsmz.de/cgi-bin/dsmzfind.pl');
 ##my $req_selection =  HTTP::Request -> new( POST => $baseURL);

 $req_selection->content_type('application/x-www-form-urlencoded');
# my $selection_content = 'VAR_DATABASE=bact&VAR_HITS=25&VAR_DSMZITEM=Escherichia&VAR_DSMZITEM2=coli&B1=Search';
 my $selection_content = 'VAR_DATABASE=bact&VAR_HITS=25&VAR_DSMZITEM='."$item1".'&VAR_DSMZITEM2='."$item2".'&B1=Search';

 $req_selection->content($selection_content);

 # Pass request to the user agent and get a response back
 my $TMPUSEROUT;
 (undef, $TMPUSEROUT) = tempfile($template, OPEN => 0);
 $TMPUSEROUT = $TMPUSEROUT . ".htm";
 my $res_selection = $ua_selection -> request($req_selection, $TMPUSEROUT);



 # Check the outcome of the response
 if ($res_selection->is_success) {print $res_selection->content;} 
 else  {die "Bad luck this time, request failed\n";};


open (INPUT , "< $TMPUSEROUT") or die "cannot open input file $TMPUSEROUT";


 my $htmlcontent;
{
local $/;
$htmlcontent = <INPUT>;
}
##print "$htmlcontent\n";

##$htmlcontent =~ s{(.*HREF=")(\/w+)}{$1http:\/\/www.dszm.de$1}igm; ##"
$htmlcontent =~ s{HREF="}{HREF="http://www.dsmz.de}igm;
$htmlcontent =~ s{HREF=[^"]}{HREF=http://www.dsmz.de/}igm; ##"

print OUTPUT $htmlcontent ;

#exec ('netscape', $TMPOUT);
print "file://" . $TMPOUT;

##print "$htmlcontent\n";
