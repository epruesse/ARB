// ================================================================ //
//                                                                  //
//   File      : DBwriter.h                                         //
//   Purpose   :                                                    //
//   Time-stamp: <Tue Mar/10/2009 15:32 MET Coder@ReallySoft.de>    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef DBWRITER_H
#define DBWRITER_H

#ifndef METAINFO_H
#include "MetaInfo.h"
#endif
#ifndef FEATURE_H
#include "Feature.h"
#endif
#ifndef SEQUENCEBUFFER_H
#include "SequenceBuffer.h"
#endif
#ifndef IMPORTER_H
#include "Importer.h"
#endif
#ifndef GENOMEIMPORT_H
#include "GenomeImport.h"
#endif


#ifndef ARBDB_H
#include <arbdb.h>
#endif

class UniqueNameDetector;

class DBerror {
    // error class used for DB errors

    string err;                 // error message

    void init(const string& msg, GB_ERROR gberror);

public:
    DBerror(const char *msg);
    DBerror(const string& msg);
    DBerror(const char *msg, GB_ERROR gberror);
    DBerror(const string& msg, GB_ERROR gberror);

    const string& getMessage() const { return err; }
};

typedef map<string, int> NameCounter;

class DBwriter : public Noncopyable {
    // GBDATA         *gb_species_data;
    const char     *ali_name;
    // UniqueNameDetector&  UND_species; // // passed from outside, extended when creating new species
    ImportSession&  session;

    // following data is valid for one organism write :
    GBDATA      *gb_organism;   // current organism
    GBDATA      *gb_gene_data;  // current gene data
    NameCounter  generatedGenes; // helper to create unique gene names (key = name, value = count occurances)

    void testAndRemoveTranslations(Importer& importer); // test and delete translations (if test was ok). warns via Importer
    void hideUnwantedGenes();

public:
    DBwriter(ImportSession& session_, const char *Ali_name)
    // : gb_species_data(Gb_species_data)
        : ali_name(Ali_name)
        // , UND_species(Und_species)
        , session(session_)
        , gb_organism(0)
        , gb_gene_data(0)
    {}

    void createOrganism(const string& flatfile, const char *importerTag);

    void writeFeature(const Feature& feature);
    void writeSequence(const SequenceBuffer& seqData);

    void renumberDuplicateGenes();
    void finalizeOrganism(const MetaInfo& meta, const References& refs, Importer& importer);
};


#else
#error DBwriter.h included twice
#endif // DBWRITER_H



