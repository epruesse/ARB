// =============================================================== //
//                                                                 //
//   File      : ptwrapper.cxx                                     //
//   Purpose   : wrapper around ptserver and ptpan                 //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2011   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <ptcommon.h>
#include <arb_strbuf.h>
#include <arb_str.h>
#include <arb_msg.h>
#include <ut_valgrinded.h>

int main(int argc, char *argv[]) {
    GBS_strstruct arguments(1000);

    const char   *PREFER_PREFIX     = "-prefer=";
    const size_t  PREFER_PREFIX_LEN = strlen(PREFER_PREFIX);
    
    const char   *DB_PREFIX     = "-D";
    const size_t  DB_PREFIX_LEN = strlen(DB_PREFIX);

    PT_Servertype  preferred_type       = PTUNKNOWN;
    GB_ERROR       error                = NULL;
    char          *db_name              = NULL;
    bool           dont_mind_servertype = false;

    for (int a = 1; a<argc; a++) {
        const char *arg = argv[a];
        if (strncmp(arg, PREFER_PREFIX, PREFER_PREFIX_LEN) == 0) {
            const char *preferred = arg+PREFER_PREFIX_LEN;

            if (strcmp(preferred, "PAN")     == 0) preferred_type = PTPAN;
            else if (strcmp(preferred, "PT") == 0) preferred_type = PTSERVER;
            else {
                error = GBS_global_string("Unknown type in -prefer=%s (known: PAN, PT)", preferred);
            }
        }
        else {
            if (strncmp(arg, DB_PREFIX, DB_PREFIX_LEN) == 0) {
                freeset(db_name, strdup(arg+DB_PREFIX_LEN));
            }
            else if (strcmp(arg, "-kill") == 0) dont_mind_servertype = true;
            arguments.put(' ');
            arguments.cat(arg);
        }
    }

    // if database is passed, check for existing index
    if (!error) {
        PT_Servertype saved_type = PTUNKNOWN;
        if (db_name) saved_type  = servertype_of_uptodate_index(db_name, error);
        if (!error) {
            if (saved_type == PTUNKNOWN) {
                if (preferred_type == PTUNKNOWN) {
                    if (dont_mind_servertype) {
                        preferred_type = PTSERVER; // use any server (e.g. for -kill)
                    }
                    else {
                        error = GBS_global_string("no idea if you like PTPAN or PTSERVER (arguments='%s')", arguments.get_data());
                    }
                }
                // else use preferred_type
            }
            else {
                if (preferred_type == PTUNKNOWN) preferred_type = saved_type;
                else if (preferred_type != saved_type) {
                    fprintf(stderr, "Ignoring preferred type '%s', since index exists for '%s'\n",
                            readable_Servertype(preferred_type),
                            readable_Servertype(saved_type));
                    preferred_type = saved_type;
                }
            }
        }
    }

    // now call the server
    int exitcode = EXIT_FAILURE;
    if (!error) {
        arb_assert(preferred_type != PTUNKNOWN);

        GBS_strstruct cmdline(50+arguments.get_position());
        cmdline.cat(preferred_type == PTSERVER ? "arb_ptserver" : "arb_ptpan --verbose");
        cmdline.ncat(arguments.get_data(), arguments.get_position());

        char *cmd = cmdline.release();
        fprintf(stderr, "ptwrapper calls '%s'\n", cmd); fflush(stderr);

        make_valgrinded_call(cmd);
        exitcode = system(cmd);
        fprintf(stderr, "ptwrapper returns with ec=%i from '%s'\n", exitcode, cmd); fflush(stderr);
        free(cmd);
    }
    else {
        fprintf(stderr, "Error in ptwrapper: %s\n", error);
    }

    return exitcode;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_wrapper() {
    // TEST_ASSERT(0);
    MISSING_TEST(TEST_wrapper);
    TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");
    // TEST_SETUP_GLOBAL_ENVIRONMENT("ptpan");
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

