//  ==================================================================== //
//                                                                       //
//    File      : awt_attributes.hxx                                     //
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

#ifndef AWT_ATTRIBUTES_HXX
#define AWT_ATTRIBUTES_HXX

// find the color of the most dominant attribute of a gene/species
// returns 0 if no such color exists (or if display of attributes is disabled)
// returns 1..n otherwise

int AWT_gene_get_dominant_color(GBDATA *gb_gene);
int AWT_species_get_dominant_color(GBDATA *gb_species);


// check whether a gene/species matches the requirement of an attribute

bool AWT_gene_has_attribute(GBDATA *gb_gene, int attribute_nr);
bool AWT_species_has_attribute(GBDATA *gb_gene, int attribute_nr);

#else
#error awt_attributes.hxx included twice
#endif // AWT_ATTRIBUTES_HXX

