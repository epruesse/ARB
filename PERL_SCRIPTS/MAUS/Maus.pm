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

@EXPORT = qw (getOptions intializeTestFields parseEntry prepareList);

# ## unshift (@EXPORT ,"helpMessage");
# @EXPORT_OK = qw(%options);


$version = 0.1;
%options = ("exff" => undef,	# exclude acc's for file
	    "exfl"  => undef,	# exclude acc's from quoted list
	    "kwl"   => undef,	# search for keywords from quoted list
	    "acfl"  => undef,	# search for acc's from quoted list
	    "acff"  => undef,	# search for acc's from file
	    "avfl"   => undef,	# search for new acc versions from quoted list
	    "avff"   => undef,	# search for new acc versions from quoted file
	    "log"   => undef,	# write log messages to log file
	    "cdat"  => undef,	# sort according to date of creation
	    "udat"  => undef,	# sort according to date of last update
	    "if"    => undef,	# name of input file
	    "of"    => undef,	# name of output file
	    "min"   => undef,	# minimal sequence length
	    "max"   => undef,	# maximal sequence length
	    "spec"  => undef,	# species name components
	    "force" => undef
	   );
sub isInList{
my @list = @{shift()};
my $value = shift();
foreach $listValue(@list){
  if ($listValue eq $value) {return 1;}
}
return 0;
}

sub getOptions{
  foreach my $option (@ARGV) {
    $option =~ m /-([a-z]{2,4})=(.*)/ or die " Maus::getOptions: unknown option detected\n";
#    print "key:  >>>$1<<<\n";
#    print "value:>>>$2<<<\n";
    if ( not isInList([ keys(%options)], $1)){die " Maus::getOptions: unknown option detected\n";}
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

  if (defined $selectionOptions{"acfl"} or defined $selectionOptions{"acff"}){
    my @acList = ();
    if(defined $selectionOptions{"acfl"}){push(@acList, @{readAccString($selectionOptions{"acfl"})});}
    if(defined $selectionOptions{"acff"}){push(@acList, @{readAccFile($selectionOptions{"acff"})});}
    $selection{"acpt"} = \@acList;
  }

  if (defined $selectionOptions{"exfl"} or defined $selectionOptions{"exff"}){
    my @acList = ();
    if(defined $selectionOptions{"exfl"}){push(@acList, @{readAccString($selectionOptions{"exfl"})});}
    if(defined $selectionOptions{"exff"}){push(@acList, @{readAccFile($selectionOptions{"exff"})});}
    $selection{"excl"} = \@acList;
  }

  if (defined $selectionOptions{"avfl"} or defined $selectionOptions{"avff"}){
    my @avList = ();
    if(defined $selectionOptions{"avfl"}){push(@avList, @{readVersionString($selectionOptions{"avfl"})});}
    if(defined $selectionOptions{"avff"}){push(@avList, @{readVersionFile($selectionOptions{"avff"})});}
    $selection{"avl"} = \@avList;
  }


  delete $selection{"if"};	# not needed for further analysis
  delete $selection{"of"};	# not needed for further analysis
  delete $selection{"log"};	# not needed for further analysis
  delete $selection{"exff"};	# not needed for further analysis
  delete $selection{"exfl"};	# not needed for further analysis
  delete $selection{"acfl"};	# not needed for further analysis
  delete $selection{"acff"};	# not needed for further analysis
  delete $selection{"acff"};	# not needed for further analysis
  delete $selection{"avfl"};	# not needed for further analysis
  delete $selection{"avff"};	# not needed for further analysis
  delete $selection{"force"};	# not needed for further analysis
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


sub readVersionString{
  my @versionNumbers = ();
  my @rawNumbers = split(/[\s,]/, shift()); # split on white space
  foreach my $number(@rawNumbers){
    if ($number =~ /^\s*([A-Z]{1,2}[0-9]{4,}\.[0-9]+).*/){push (@versionNumbers, $1)}
  }
  return \@versionNumbers;
}


sub readAccFile{
  my $fileName = shift();
  my @accNumbers = ();
  open(ACC, "< $fileName") or die "cannot open accession number file $fileName\n";
    while (<ACC>){
      if ($_ =~ /^\s*([A-Z]{1,2}[0-9]{4,}).*/){push (@accNumbers, $1)}
    }
  close ACC;
  return \@accNumbers;
}

sub readVersionFile{
  my $fileName = shift();
  my @versionNumbers = ();
  open(VER, "< $fileName") or die "cannot open accession number file $fileName\n";
    while (<VER>){
      if ($_ =~ /^\s*([A-Z]{1,2}[0-9]{4,}\.[0-9]+).*/){push (@accNumbers, $1)}
    }
  close VER;
  return \@versionNumbers;
}


sub analyseKeywords{
  my $entryLine = shift();
  my $option = shift();
  my %selection = %{shift()};
  my @keywords = split(/[\s,]/, shift()); # split on white space

#  foreach $w(@keywords){print ">>$w<<\n";}
  foreach $kw(@keywords){
    if($kw ne "" and ($entryLine =~ m/$kw/i)){ #print ">>>$kw<<<\n";
      return 1;}
  }
  return 0;
}


## to be adopted to combined lists in %selection{acpt,excl}
sub analyseAccessionList{
  my $entry = shift();
  my $option = shift();
  my %selection = %{shift()};
  if ((defined $selection{"excl"}) and (scalar @{$selection{"excl"}} != 0 )){
    for my $excl(@{$selection{"excl"}}){
      if ($entry =~ /$excl/){return 0;}
    }
  }

  if ((defined $selection{"acpt"}) and (scalar @{$selection{"acpt"}} != 0 )){
    for my $excl(@{$selection{"acpt"}}){
      if ($entry =~ /$excl/){return 1;}
    }
    return 0;
  }
  return 1; # fall back default
}

sub analyseVersionList{
  my $line = shift();
  my $option = shift();
  my %selection = %{shift()};

  if ((defined $selection{$option}) and ((scalar (my @versionList = @{$selection{$option}})) != 0)){
#    my @versionList = @{$selection{$option}};
    if($line =~ /SV[ ]{3}([A-Z]{1,2}[0-9]{4,})\.([0-9]+)/){
      my $versionBase = $1;
      my $versionNumber = $2;
      foreach my $oldVersion (@versionList){
	if($oldVersion =~ /($versionBase)\.([0-9]+)/){
	  return $versionNumber > $2 ?  1 : 0;
	}
      }
      return 1; # not yet in database

    }else{
      die "Not a version number line: >>$entry<<\n";
    }

  }else{       #specified version list empty, every sequence accepted
    return 1;
}


}

sub date2number{
  my $dateString = shift();
  my %calendar=("JAN" =>1,
		"FEB" =>2,
		"MAR" =>3,
		"APR" =>4,
		"MAY" =>5,
		"JUN" =>6,
		"JUL" =>7,
		"AUG" =>8,
		"SEP" =>9,
		"OCT" =>10,
		"NOC" =>11,
		"DEC" =>12
	       );
  if ($dateString =~ /([0-9]{1,2})-([A-Z]{3})-([0-9]{4})/){
    return ($1 + $calendar{$2}*35 + $3 *1000);
  }else{die "cannot convert date string to number\n";}
}

sub compareDates{
  my $date1 = date2number (shift());
  my $date2 = date2number (shift());

  #print "date1: >>$date1<< date2: >>$date2<<\n";
  return $date1 == $date2 ? 0 : $date1 > $date2 ?  1 : -1;
}

sub analyseCreationDate{
  my $line = shift();
  my $option = shift();
  my %selection = %{shift()};
  if($line =~ m/DT[ ]{3}([0-9]{1,2}-[A-Z]{3}-[0-9]{4}) \(Rel.*Created\)/){
#    print "creation date found\n";
    if ($option eq ""){return 1};
    my $cdate = $1;
#    print "option value: ".$selection{$option}."\n";
    if (!($selection{$option} =~ /([0-9]{1,2}-[A-Z]{3}-[0-9]{4})([+-]?)/))
      {die "no valid date format:\nPlease use e.g 27-AUG-1969\n"}
    my $cflag = $2;
#    print "dates to compare cdate: >>$cdate<< stamp:>>$1<<\n";
    my $comparation = compareDates($cdate, $1);
    if ((defined $cflag) and ($cflag eq "-")){return $comparation == -1 ? 1 : 0;}
    else{return $comparation == 1 ? 1 : 0;}
  }else{
    return 0;
  }
}


sub analyseUpdateDate{
  my $line = shift();
  my $option = shift();
  my %selection = %{shift()};
  if($line =~ m/DT[ ]{3}([0-9]{1,2}-[A-Z]{3}-[0-9]{4}) \(Rel.*Last updated.*\)/){
#    print "update date found\n";
    if ($option eq ""){return 1};
    my $cdate = $1;
    if (!($selection{$option} =~ /([0-9]{1,2}-[A-Z]{3}-[0-9]{4})([+-]?)/))
      {die "no valid date format:\nPlease use e.g 27-AUG-1969\n"}
    my $cflag = $2;
    my $comparation = compareDates($cdate, $1);
    if ((defined $cflag) and ($cflag eq "-")){return $comparation == -1 ? 1 : 0;}
    else{return $comparation == 1 ? 1 : 0;}
  }else{
    return 0;
  }
}

sub analyseMinLength{
  my $line = shift();
  my $option = shift();
  my %selection = %{shift()};

  my $value = $selection{$option};
  die "Maus::analyseMinLength: analysis function and field id doesn't match" if not $line =~ /^ID.*/;
  if ($line =~ /.*\s([1-9][0-9]*)\sBP\.$/){
    #    print "length detected: $1\n"; 
   $1 >= $value ? return 1: return 0;
  }else{
    warn "Maus::analyseMinLength: couldn't detect number of nucleotides\n";
    return 1;
  }
}

sub analyseMaxLength{
  # $entryLine, $selection{$options}
  my $line = shift();
  my $option = shift();
  my %selection = %{shift()};

  my $value = $selection{$option};
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
  my $option = shift();
  my %selection = %{shift()};

  my $value = $selection{$option};
  die "Maus::analyseSpecies: analysis function and field id doesn't match" if not $line =~ /^OS.*/;
  if ($line =~ /$value/i){return 1;}
  else{return 0;}
}



my %analysisFunctionsRef = (
			    "kwl"  => \&analyseKeywords,
			    "acpt" => \&analyseAccessionList,
			    "excl" => \&analyseAccessionList,
			    "avl"  => \&analyseVersionList,
			    "cdat" => \&analyseCreationDate,
			    "udat" => \&analyseUpdateDate,
			    "min"  => \&analyseMinLength,
			    "max"  => \&analyseMaxLength,
			    "spec" => \&analyseSpecies
			   );



sub parseEntry{
  my @entryLines = split('\n', ${shift()});
  my %selection = %{shift()};	# contains command line options
  my $logFlag = shift();




  # information for which option can be found in which field
  my %opt2field = (
		   "kwl"  => "DE", # changed from initial field value KW to DE
		   "acpt" => "AC",
		   "excl" => "AC",
		   "avl"  => "SV",
		   "cdat" => "DT",
		   "udat" => "DT",
		   "min"  => "ID",
		   "max"  => "ID",
		   "spec" => "OS"
		  );
  my @wantedFields = keys %selection;
  my %optionResults = ();# collect results for multi line sections

  foreach my $entryLine (@entryLines) { # check every line
#    print "line :$entryLine\n";
    foreach my $options (@wantedFields) { # for every selected option
#      print "option: " .$options."\t". $opt2field{$options} ."\n";

      # print "wantedFields: " .@wantedFields."\n";
      if ($opt2field{$options} eq substr($entryLine, 0, 2)) {
	# call field specific analysis function
	# this is useful for multi line sections
	$optionResults{$options} ||=  &{$analysisFunctionsRef{$options}}($entryLine, $options,\%selection);
      }
    }
  }

  # final evaluation summary
  my $result = 1;
  foreach my $sectionResult(keys(%optionResults)){
    $result = $result && $optionResults{$sectionResult};
#  print "$sectionResult: $optionResults{$sectionResult}\n";
  }
  ## combine evaluation results goes here
#  print "sectionResult: $sectionResult\n";
  return $result;
}
