// ================================================================ //
//                                                                  //
//   File      : DBwriter.h                                         //
//   Purpose   :                                                    //
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

class UniqueNameDetector;

class DBerror {
    // error class used for DB errors

    string err;                 // error message

    void init(const string& msg, GB_ERROR gberror);

public:
    DBerror();
    DBerror(const char *msg);
    DBerror(const string& msg);
    DBerror(const char *msg, GB_ERROR gberror);
    DBerror(const string& msg, GB_ERROR gberror);

    const string& getMessage() const { return err; }
};

typedef std::map<std::string, int> NameCounter;
struct Translator;

class DBwriter : virtual Noncopyable {
    const char     *ali_name;
    ImportSession&  session;

    // following data is valid for one organism write :
    GBDATA      *gb_organism;   // current organism
    GBDATA      *gb_gene_data;  // current gene data
    NameCounter  generatedGenes; // helper to create unique gene names (key = name, value = count occurrences)

    void testAndRemoveTranslations(Importer& importer); // test and delete translations (if test was ok). warns via Importer
    void hideUnwantedGenes();

    static Translator *unreserve;
    static const string& getUnreservedQualifier(const string& qualifier);

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

    void writeFeature(const Feature& feature, long seqLength);
    void writeSequence(const SequenceBuffer& seqData);

    void renumberDuplicateGenes();
    void finalizeOrganism(const MetaInfo& meta, const References& refs, Importer& importer);

    static void deleteStaticData();
};


#else
#error DBwriter.h included twice
#endif // DBWRITER_H



