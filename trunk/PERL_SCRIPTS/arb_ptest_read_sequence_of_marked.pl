#!/usr/arb/bin/perl
use lib "$ENV{'ARBHOME'}/lib/";
use ARB;

$gb_main = &ARB::open(":","r");
if (! $gb_main ) {
    $error = ARB::get_error();
    print ("$error\n");
    exit 0;
}

&ARB::begin_transaction($gb_main);
$gb_alignment_name     = &ARB::search($gb_main,"presets/use","none");
$alignment_name = ARB::read_string($gb_alignment_name);
    print "alignment name is $alignment_name\n\n";

$gb_species = ARBT::first_marked_species($gb_main);
while ($gb_species) {
    $gb_name = ARB::search($gb_species,"name","none");
    $name = ARB::read_string($gb_name);
    $gb_data = ARB::search($gb_species, "$alignment_name/data","none");
    $seq = ARB::read_string($gb_data);
    print "First selected species is $name\n";
    print "   it's sequence is $seq\n";
    $gb_species = ARBT::next_marked_species($gb_species);
}
ARB::commit_transaction($gb_main);
ARB::close($gb_main);



















