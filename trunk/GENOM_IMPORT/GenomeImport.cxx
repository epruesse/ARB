// ================================================================ //
//                                                                  //
//   File      : GenomeImport.cxx                                   //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "GenomeImport.h"
#include "FileBuffer.h"
#include "Importer.h"
#include "tools.h"
#include "DBwriter.h"

#include <errno.h>
#define AW_RENAME_SKIP_GUI
#include <AW_rename.hxx>
#include <aw_question.hxx>

using namespace std;

GB_ERROR GI_importGenomeFile(ImportSession& session, const char *file_name, const char *ali_name) {
    GB_ERROR error = 0;
    try {
        if (strcmp(ali_name, "ali_genom") != 0) throw "Alignment has to be 'ali_genom'";

        FILE *in = fopen(file_name, "rb");
        if (!in) throw GBS_global_string("Can't read file '%s' (Reason: %s)", file_name, strerror(errno));

        FileBuffer flatfile(file_name, in);
        flatfile.showFilenameInLineError(false);

        DBwriter db_writer(session, ali_name);

        SmartPtr<Importer> importer;
        {
            string line;
            if (!flatfile.getLine(line)) throw flatfile.lineError("File is empty");

            if      (beginsWith(line, "LOCUS")) importer = new GenebankImporter(flatfile, db_writer);
            else if (beginsWith(line, "ID")   ) importer = new EmblImporter    (flatfile, db_writer);
            else throw flatfile.lineError("Wrong format. Expected 'LOCUS' or 'ID'");

            flatfile.backLine(line);
        }

        importer->import();
    }
    catch (const string &err) { error = GBS_global_string("%s", err.c_str()); }
    catch (const char *err) { error = err; }
    catch (...) { error = "Unknown exception during import (program error)"; }
    return error;
}




ImportSession::ImportSession(GBDATA *gb_species_data_, int estimated_genomes_count)
    : gb_species_data(gb_species_data_)
{
    und_species                    = new UniqueNameDetector(gb_species_data, estimated_genomes_count);
    ok_to_ignore_wrong_start_codon = new AW_repeated_question();
}

ImportSession::~ImportSession() {
    delete ok_to_ignore_wrong_start_codon;
    delete und_species;
}
