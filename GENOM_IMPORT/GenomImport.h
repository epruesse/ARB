//  ==================================================================== //
//                                                                       //
//    File      : GenomImport.h                                          //
//    Purpose   : Interface to GenomImport library                       //
//    Time-stamp: <Sat Sep/11/2004 12:50 MET Coder@ReallySoft.de>        //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2004        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef GENOME_IMPORT_H
#define GENOME_IMPORT_H

GB_ERROR Genom_read_embl_universal(GBDATA *gb_main, const char *filename, const char *ali_name);

#else
#error Genome_Import.h included twice
#endif

