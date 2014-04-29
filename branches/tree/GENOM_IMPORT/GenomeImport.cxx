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

#include "tools.h"
#include "DBwriter.h"

#include <cerrno>
#define AW_RENAME_SKIP_GUI
#include <AW_rename.hxx>
#include <aw_question.hxx>
#include <arb_stdstr.h>
#include <arb_file.h>
#include <arb_diff.h>

using namespace std;

GB_ERROR GI_importGenomeFile(ImportSession& session, const char *file_name, const char *ali_name) {
    GB_ERROR error = 0;
    try {
        if (strcmp(ali_name, "ali_genom") != 0) throw "Alignment has to be 'ali_genom'";

        FILE *in = fopen(file_name, "rb");
        if (!in) throw GBS_global_string("Can't read file '%s' (Reason: %s)", file_name, strerror(errno));

        BufferedFileReader flatfile(file_name, in);
        flatfile.showFilenameInLineError(false);

        DBwriter db_writer(session, ali_name);

        SmartPtr<Importer> importer;
        {
            string line;
            if (!flatfile.getLine(line)) throw flatfile.lineError("File is empty");

            if      (beginsWith(line, "LOCUS")) importer = new GenebankImporter(flatfile, db_writer);
            else if (beginsWith(line, "ID")) importer = new EmblImporter       (flatfile, db_writer);
            else throw flatfile.lineError("Wrong format. Expected 'LOCUS' or 'ID'");

            flatfile.backLine(line);
        }

        importer->import();
    }
    catch (const string &err) { error = GBS_static_string(err.c_str()); }
    catch (const char *err) { error = err; }
    catch (...) { error = "Unknown exception during import (program error)"; }
    return error;
}




ImportSession::ImportSession(GBDATA *gb_species_data_, int estimated_genomes_count)
    : gb_species_data(gb_species_data_)
{
    und_species                    = new UniqueNameDetector(gb_species_data, estimated_genomes_count);
    ok_to_ignore_wrong_start_codon = new AW_repeated_question;
}

ImportSession::~ImportSession() {
    delete ok_to_ignore_wrong_start_codon;
    delete und_species;
    DBwriter::deleteStaticData();
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

// #define TEST_AUTO_UPDATE // uncomment to auto-update expected result-database

void TEST_SLOW_import_genome_flatfile() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("nosuch.arb", "wc");

    // import flatfile
    {
        GB_transaction  ta(gb_main);
        GBDATA *gb_species_data = GBT_find_or_create(gb_main, "species_data", 7);

        ImportSession isess(gb_species_data, 1);
        TEST_EXPECT_NO_ERROR(GI_importGenomeFile(isess, "AB735678.txt", "ali_genom"));
    }

    // save database and compare with expectation
    {
        const char *savename = "AB735678.arb";
        const char *expected = "AB735678_expected.arb";

        TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, savename, "a"));
#if defined(TEST_AUTO_UPDATE)
        TEST_COPY_FILE(savename, expected);
#else
        TEST_EXPECT_TEXTFILES_EQUAL(savename, expected);
#endif // TEST_AUTO_UPDATE
        TEST_EXPECT_ZERO_OR_SHOW_ERRNO(GB_unlink(savename));
    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
