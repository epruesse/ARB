#!/usr/arb/bin/perl
use lib "$ENV{'ARBHOME'}/lib/";
use ARB;

# syntax:   $gb_ali, clientdata, "DELETED"/"CHANGED"
sub CALC_GC {
    local($gb_data,$cl,$mode) = @_;
    if ($mode eq "DELETED" ){
	print "Sequence deleted\n";
	return;
    }
    local($seq)    = ARB::read_string($gb_data);
    local($gb_ali) = ARB::get_father($gb_data);
    local($gb_gc)  = ARB::search($gb_ali,"GC","STRING");
    local($error)  = ARB::write_string($gb_gc,$seq);
    if ($error) {
	&ARB::print_error();
    }
    local($gb_name) = ARB::find($gb_ali,"name","","this");
    local($name)    = ARB::read_string($gb_name);
    print ("Sequence of species $name has changed\n");
}

$gb_main = ARB::open(":","r");
if (! $gb_main ) {
    $error = ARB::get_error();
    print ("$error\n");
    exit 0;
}

ARB::begin_transaction($gb_main);
$gb_alignment_name     = ARB::search($gb_main,"presets/use","none");
$alignment_name = ARB::read_string($gb_alignment_name);

$gb_species = ARBT::first_marked_species($gb_main);
while ($gb_species) {
    $gb_data = ARB::search($gb_species, "$alignment_name/data","none");
    &ARB::add_callback($gb_data,"CALC_GC", "This is the callback");
    CALC_GC($gb_data,"","CHANGED");
    $gb_species = ARBT::next_marked_species($gb_species);
}
ARB::commit_transaction($gb_main);

while ( 1) {
    sleep (1);
    &ARB::begin_transaction($gb_main);
    &ARB::commit_transaction($gb_main);
}

ARB::close($gb_main);



















