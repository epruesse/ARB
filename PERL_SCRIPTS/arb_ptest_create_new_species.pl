#!/usr/arb/bin/perl -I$ARBHOME/lib
use lib "$ENV{ARBHOME}/lib/";
use ARB;

$gb_main = &ARBT::open(":","r",0);
if (! $gb_main ){
    $error = ARB::get_error();
    print ("$error\n");
    exit;
}

ARB::begin_transaction($gb_main);
$gb_alignment_name     = ARB::search($gb_main,"presets/use","none");
$alignment_name = ARB::read_string($gb_alignment_name);



print ("Enter short name of species\n");
chop($name = <STDIN> );

$gb_species = ARBT::find_species($gb_main,$name);
if ($gb_species){
    print("Species $name already exists\n");
    exit(-1);
}

$gb_species = ARBT::create_species($gb_main,$name);
if (!$gb_species){
  ARB::print_error();
    exit(-1);
}

print ("Enter full name of species\n");
chop($full_name = <STDIN> );

$gb_full_name = ARB::create($gb_species,"full_name","STRING");  # create always, even if there is an entry of that name
# same as ARB::search($gb_species,"full_name","STRING"); # create only if new
if (!$gb_full_name){
  ARB::print_error();
    exit(-1);
}

$error = ARB::write_string($gb_full_name,$full_name);
if ($error){
  ARB::print_error();
    exit(-1);
}

print ("Enter sequence of species\n");
chop($sequence = <STDIN> );

$gb_sequence = ARB::search($gb_species, "$alignment_name/data", "STRING");
if (!gb_sequence){
  ARB::print_error();
    exit(-1);
}


$error = ARB::write_string($gb_sequence,$sequence);
if ($error){
  ARB::print_error();
    exit(-1);
}

ARB::commit_transaction($gb_main);
ARB::close($gb_main);
