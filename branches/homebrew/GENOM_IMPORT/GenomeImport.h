// ================================================================ //
//                                                                  //
//   File      : GenomeImport.h                                     //
//   Purpose   : Genome flat file import                            //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef GENOMEIMPORT_H
#define GENOMEIMPORT_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif

class UniqueNameDetector;
class AW_repeated_question;

struct ImportSession : virtual Noncopyable { // valid during complete import of multiple files
    GBDATA               *gb_species_data;
    UniqueNameDetector   *und_species;                    // extended when creating new species
    AW_repeated_question *ok_to_ignore_wrong_start_codon;

    ImportSession(GBDATA *gb_species_data_, int estimated_genomes_count);
    ~ImportSession();
};


GB_ERROR GI_importGenomeFile(ImportSession& session, const char *file_name, const char *ali_name);

#else
#error GenomeImport.h included twice
#endif // GENOMEIMPORT_H

