# The ARB probe server cgi perl library
# (C) 2003 by Ralf Westram

package probe_server;

use CGI;
$CGI::POST_MAX=1024 * 100;  # max 100K posts
$CGI::DISABLE_UPLOADS = 1;  # no uploads
# use CGI::Carp qw(fatalsToBrowser warningsToBrowser set_message);
use CGI::Carp qw(fatalsToBrowser warningsToBrowser set_message);

BEGIN {
  use Exporter ();

  our (@ISA, @EXPORT, @EXPORT_OK);

  @ISA       = qw(Exporter);
  @EXPORT    = qw( &print_header );
  @EXPORT_OK = qw( $q %params $requestdir );
}

our @EXPORT_OK;
my $header_printed;

sub print_header() {
  print $probe_server::q->header(-type=>'text/plain');
  $header_printed=1;
}

#error handling:
BEGIN {
  sub handle_errors {
    my $msg = shift;
    if ($header_printed==0) { &probe_server::print_header(); }
    print "result=cgi-error\nmessage=$msg\n";
  }

  $header_printed = 0;
  $q              = new CGI;
  %params         = $q->Vars();
  $requestdir     = '/home/westram/ARB/PROBE_SERVER/ps_workerdir';
  set_message(\&handle_errors);
}

END { }                         # module clean-up code here (global destructor)
1;                              # don't forget to return a true value from the file
