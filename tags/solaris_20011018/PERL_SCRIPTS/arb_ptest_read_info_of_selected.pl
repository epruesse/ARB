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

# see WINDOW/aw_awars for details
$selected_species_name = ARBT::read_string2($gb_main,"tmp/focus/species_name","none");
print("selected species name is $selected_species_name\n");
$gb_species = ARBT::find_species($gb_main,$selected_species_name);


if ($gb_species) {
    print "SPECIES *************************************\n";

    $gb_any_field = ARB::find($gb_species,"","","down");
    while ($gb_any_field){
	$type = ARB::read_type($gb_any_field);
	if ($type eq "STRING") {
	    $val = ARB::read_string($gb_any_field);
	    $key = ARB::read_key($gb_any_field);
	    print("$key $val\n");
	}
	$gb_any_field = ARB::find($gb_any_field,"","","this_next");
    }
}
ARB::commit_transaction($gb_main);
ARB::close($gb_main);



















