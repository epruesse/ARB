//  ==================================================================== //
//                                                                       //
//    File      : AWT_attributes.cxx                                     //
//    Purpose   : get attribute colors for species/genes                 //
//    Time-stamp: <Sun Jul/18/2004 18:43 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in July 2004             //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include <stdio.h>
#include <arbdb.h>

#include <aw_color_groups.hxx>
#include "awt_attributes.hxx"

using namespace std;

// faked attribute-interface (currently uses deprecated color groups)

int AWT_gene_get_dominant_color(GBDATA *gb_gene) {
    return AW_find_color_group(gb_gene, false);
}

int AWT_species_get_dominant_color(GBDATA *gb_species) {
    return AW_find_color_group(gb_species, false);
}

bool AWT_gene_has_attribute(GBDATA *gb_gene, int attribute_nr) {
    return AW_find_color_group(gb_gene, true) == attribute_nr;
}
bool AWT_species_has_attribute(GBDATA *gb_species, int attribute_nr) {
    return AW_find_color_group(gb_species, true) == attribute_nr;    
}


