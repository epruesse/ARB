# Maus.pm - module
# Package holds function necessary for the MAUS
# (Management of ARB Updates System) System
# developed at the Lehrstuhl für Bioinformatik 
# and Lehrstuhl für Mikrobiologie
# (c) Lothar Richter June 2004


use diagnostics;
use warnings;

package Maus;
require Exporter;
@ISA       = qw(Exporter);

@EXPORT = qw (getOptions intializeTestFields parseEntry prepareExcludes);

# ## unshift (@EXPORT ,"helpMessage");
# @EXPORT_OK = qw(%options);


$version = 0.1;
%options = ("exff" => undef,	# exclude acc's for file
	    "exfl"  => undef,	# exclude acc's from quoted list
	    "kwl"   => undef,	# search for keywords from quoted list
	    "acl"   => undef,	# search for acc's from quoted list
	    "avl"   => undef,	# search for new acc versions from quoted list
	    "log"   => undef,	# write log messages to log file
	    "cdat"  => undef,	# sort according to date of creation
	    "udat"  => undef,	# sort according to date of last update
	    "if"    => undef,	# name of input file
	    "of"    => undef,	# name of output file
	    "min"   => undef,	# minimal sequence length
	    "max"   => undef,	# maximal sequence length
	    "spec"  => undef,	# species name components
	   );

sub getOptions{
  foreach my $option (@ARGV) {
    $option =~ m /-([a-z]{2,4})=(.*)/ or warn " Maus::getOptions: unknown option detected\n";
    print "key:  >>>$1<<<\n";
    print "value:>>>$2<<<\n";
    $options{$1} = $2;
  }
  return \%options;
}


sub intializeTestFields {
  my %selectionOptions = %{shift()}; # sort out command line options
  my %selection = ();
  foreach my $field (keys %selectionOptions) {
    if (defined $selectionOptions{$field}) {
      $selection{$field} = $selectionOptions{$field};
    }
  }


  delete $selection{"if"};	# not needed for further analysis
  delete $selection{"of"};	# not needed for further analysis
  delete $selection{"log"};	# not needed for further analysis

  ## handle exclude numbers in separate sub function prepareExcludes()

  delete $selection{"exff"};	# not needed for further analysis
  delete $selection{"exfl"};	# not needed for further analysis

  return \ %selection;
}

sub readAccString{
  my @accNumbers = ();
  my @rawNumbers = split(/[\s,]/, shift()); # split on white space
  foreach my $excl(@rawNumbers){
    if ($excl =~ /^\s*([A-Z]{1,2}[0-9]{4,}).*/){push (@accNumbers, $1)}
  }
  return \@accNumbers;
}

sub readAccFile{
  my $fileName = shift();
  my @accNumbers = ();
  open(ACC, "< $fileName") or warn "cannot open accession number file $fileName\n";
    while (<ACC>){
      if ($_ =~ /^\s*([A-Z]{1,2}[0-9]{4,}).*/){push (@accNumbers, $1)}
    }
  return \@accNumbers;
}


sub prepareExcludes{
  my %localArgs = %{shift()};
  my @excludes = ();
  if (defined $localArgs{exff}){
    push(@excludes,@{readAccFile($localArgs{exff})});
  }
  if (defined $localArgs{exfl}){
    push(@excludes,@{readAccString($localArgs{exfl})});
  }
  return \ @excludes;
}

sub analyseKeywords{}
sub analyseAccessionList{}
sub analyseVersionList{}
sub analyseCreationDate{}
sub analyseUpdateDate{}

sub analyseMinLength{
  my $line = shift();
  my $value = shift();
  die "Maus::analyseMinLength: analysis function and field id doesn't match" if not $line =~ /^ID.*/;
  if ($line =~ /.*\s([1-9][0-9]*)\sBP\.$/){
    $1 >= $value ? return 1: return 0;
  }else{
    warn "Maus::analyseMinLength: couldn't detect number of nucleotides\n";
    return 1;
  }
}

sub analyseMaxLength{
# $entryLine, $selection{$options}
  my $line = shift();
  my $value = shift();
  die "Maus::analyseMaxLength: analysis function and field id doesn't match" if not $line =~ /^ID.*/;
  if ($line =~ /.*\s([1-9][0-9]*)\sBP\.$/){
    $1 <= $value ? return 1: return 0;
  }else{
    warn "Maus::analyseMaxLength: couldn't detect number of nucleotides\n";
    return 1;
  }


}

sub analyseSpecies{
# $entryLine, $selection{$options}
  my $line = shift();
  my $value = shift();
  die "Maus::analyseSpecies: analysis function and field id doesn't match" if not $line =~ /^OS.*/;
  if ($line =~ /$value/i){return 1;}
  else{return 0;}
}



my %analysisFunctionsRef = (
			    "kwl"  => \&analyseKeywords,
			    "acl"  => \&analyseAccessionList,
			    "avl"  => \&analyseVersionList,
			    "cdat" => \&analyseCreationDate,
			    "udat" => \&analyseUpdateDate,
			    "min"  => \&analyseMinLength,
			    "max"  => \&analyseMaxLength,
			    "spec" => \&analyseSpecies
			   );



sub parseEntry{
  my $result;
    my @entryLines = split('\n', ${shift()});
  my %selection = %{shift()};	# contains command line options
  my %lists = %{shift()};
  my @exluded = @{$lists{"exclude"}};
  my $logFlag = shift();




  # information for which option can be found in which field
  my %opt2field = (
		   "kwl"  => "KW",
		   "acl"  => "AC",
		   "avl"  => "SV",
		   "cdat" => "DT",
		   "udat" => "DT",
		   "min"  => "ID",
		   "max"  => "ID",
		   "spec" => "OS"
		  );

  #  my %selectedLineLists = ();

  my @wantedFields = keys %selection;
  my %optionResults = ();# collect results for multi line sections

  foreach my $entryLine (@entryLines) { # check every line
#    print "line :$entryLine\n";
    foreach my $options (@wantedFields) { # for every selected option
#      print "option: " .$options."\t". $opt2field{$options} ."\n";
      if ($opt2field{$options} eq substr($entryLine, 0, 2)) {
	# call field specific analysis function
	# this is useful for multi line sections
	$optionResults{$options} ||=  &{$analysisFunctionsRef{$options}}($entryLine, $selection{$options});

      }
    }
  }
  return 1;
}
