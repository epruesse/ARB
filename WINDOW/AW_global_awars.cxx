//  ==================================================================== //
//                                                                       //
//    File      : AW_global_awars.cxx                                    //
//    Purpose   : Make some awars accessible from ALL arb applications   //
//    Time-stamp: <Thu Mar/11/2004 13:39 MET Coder@ReallySoft.de>        //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2003          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#define TEMP_DB_PATH "tmp/global_awars"

#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_global_awars.hxx>

static GBDATA *gb_main4awar = 0; // gb_main used for global awars

inline const char *get_db_path(const AW_awar *awar) {
    return GBS_global_string("%s/%s", TEMP_DB_PATH, awar->awar_name);
}

static bool in_global_awar_cb = false;

static void awar_updated_cb(AW_root */*aw_root*/, AW_CL cl_awar) {
    if (!in_global_awar_cb) {
        AW_awar        *awar    = (AW_awar*)cl_awar;
        char           *content = awar->read_as_string();
        const char     *db_path = get_db_path(awar);
        GB_transaction  dummy(gb_main4awar);
        GBDATA         *gbd     = GB_search(gb_main4awar, db_path, GB_FIND);

        aw_assert(gbd);             // should exists

        in_global_awar_cb = true;
        GB_write_string(gbd, content);
        in_global_awar_cb = false;

        free(content);
    }
}

static void db_updated_cb(GBDATA *gbd, int *cl_awar, GB_CB_TYPE /*cbtype*/) {
    if (!in_global_awar_cb) {
        AW_awar        *awar = (AW_awar*)cl_awar;
        GB_transaction  dummy(gb_main4awar);

        in_global_awar_cb = true;
        awar->write_as_string(GB_read_char_pntr(gbd));
        in_global_awar_cb = false;
    }
}

void AW_awar::make_global() {
#if defined(DEBUG)
    aw_assert(!is_global);      // don't make awars global twice!
    aw_assert(gb_main4awar);
    is_global = true;
#endif // DEBUG

    add_callback(awar_updated_cb, (AW_CL)this);

    GB_transaction  dummy(gb_main4awar);
    const char     *db_path = get_db_path(this);
    GBDATA         *gbd     = GB_search(gb_main4awar, db_path, GB_FIND);

    if (gbd) { // was already set by another ARB application
        // -> read db value and store in awar

        const char *content = GB_read_char_pntr(gbd);
        write_as_string(content);
    }
    else {
        // store awar value in db

        char *content = read_as_string();
        gbd           = GB_search(gb_main4awar, db_path, GB_STRING);
        GB_write_string(gbd, content);
        free(content);
    }

    GB_add_callback(gbd, GB_CB_CHANGED, db_updated_cb, (int*)this);
}

static bool initialized = false;

bool ARB_global_awars_initialized() {
    return initialized;
}

void ARB_init_global_awars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) {
    aw_assert(!initialized);    // don't call twice!

    initialized  = true;
    gb_main4awar = gb_main;

    aw_root->awar_string(AWAR_WWW_BROWSER, "(netscape -remote 'openURL($(URL))' || netscape '$(URL)') &", aw_def)->make_global();
}





