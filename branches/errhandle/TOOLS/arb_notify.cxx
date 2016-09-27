// =============================================================== //
//                                                                 //
//   File      : arb_notify.cxx                                    //
//   Purpose   : report termination of external jobs to ARB        //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2007       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>

int ARB_main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: arb_notify ID \"message\"\n");
        fprintf(stderr, "Used by ARB itself to report termination of external jobs - see GB_notify()\n");
        return -1;
    }

    const char *progname = argv[0];
    if (!progname || progname[0] == 0) progname = "arb_notify";

    GB_shell shell;
    GBDATA *gb_main = GB_open(":", "r");
    if (!gb_main) {
        fprintf(stderr, "%s: Can't notify (connect to ARB failed)\n", progname);
    }
    else {
        char       *the_message = ARB_strdup(argv[2]);
        const char *idstr       = argv[1];
        int         id          = atoi(idstr);
        GB_ERROR    error       = 0;

        if (!id) {
            error = GBS_global_string("Illegal ID '%s'", idstr);
        }

        if (!error) {
            GB_transaction ta(gb_main);
            error = GB_notify(gb_main, id, the_message);
        }

        free(the_message);

        if (error) {
            GBT_message(gb_main, GBS_global_string("Error in %s: %s", progname, error));
        }
        GB_close(gb_main);
    }

    return 0;
}
