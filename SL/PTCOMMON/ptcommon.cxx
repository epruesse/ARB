// =============================================================== //
//                                                                 //
//   File      : ptcommon.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ptcommon.h"
#include <arb_assert.h>
#include <arb_msg.h>
#include <arb_file.h>
#include <sys/stat.h>

#include <string>

using namespace std;

const char *readable_Servertype(PT_Servertype type) {
    switch (type) {
        case PTPAN: return "PTPAN";
        case PTSERVER: return "PTSERVER";
        case PTUNKNOWN: break;
    }
    return "<unknown PT-Server type>";
}

string indexfile_for(PT_Servertype type, const char *dbname) {
    string indexfile(dbname);

    switch (type) {
        case PTSERVER: indexfile += ".pt"; break;
        case PTPAN: indexfile    += ".pan"; break;
        case PTUNKNOWN: GBK_terminate("unknown servertype"); break;
    }

    return indexfile;
}

bool uptodate_index_exists_for(PT_Servertype type, const char *dbname) {
    string        indexfile = indexfile_for(type, dbname);
    unsigned long mod_db    = GB_time_of_file(dbname);
    unsigned long mod_index = GB_time_of_file(indexfile.c_str());

    arb_assert(mod_db>0); // database has to exist!
    return mod_index >= mod_db;
}

PT_Servertype servertype_of_uptodate_index(const char *dbname, GB_ERROR& error) {
    arb_assert(!error);
    if (!GB_is_regularfile(dbname)) {
        error = GBS_global_string("PT-server database is missing (%s)", dbname);
    }
    else {
        bool have_pan_index = uptodate_index_exists_for(PTPAN, dbname);
        bool have_pt_index  = uptodate_index_exists_for(PTSERVER, dbname);

        if (have_pan_index && have_pt_index) {
            error = GBS_global_string("Something is wrong: detected uptodate indices for %s __and__ %s\n(database=%s)",
                                      readable_Servertype(PTSERVER), 
                                      readable_Servertype(PTPAN), 
                                      dbname);
        }
        else {
            if (have_pan_index) return PTPAN;
            if (have_pt_index) return PTSERVER;
        }
    }
    return PTUNKNOWN;
}

