//  ==================================================================== //
//                                                                       //
//    File      : Genome_Import.h                                        //
//    Purpose   : Interface to ARB                                       //
//    Time-stamp: <Fri Sep/10/2004 13:34 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2004           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef GENOME_IMPORT_H
#define GENOME_IMPORT_H

// #include <GenomEmbl.h>
// #include <GenomArbConnection.h>

GB_ERROR Genom_read_embl_universal(GBDATA *gb_main, const char *filename, const char *ali_name);

#else
#error Genome_Import.h included twice
#endif

