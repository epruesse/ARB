// =============================================================== //
//                                                                 //
//   File      : arb_message.cxx                                   //
//   Purpose   : raise aw_message from external scripts            //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2003  //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>

int ARB_main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: arb_message \"the message\"\n");
        return EXIT_FAILURE;
    }

    const char *progname = argv[0];
    if (!progname || progname[0] == 0) progname = "arb_message";

    char *the_message  = strdup(argv[1]);
    char *unencoded_lf = 0;
    while ((unencoded_lf = strstr(the_message, "\\n")) != 0) {
        unencoded_lf[0] = '\n';
        strcpy(unencoded_lf+1, unencoded_lf+2);
    }

    {
        GB_shell shell;
        GBDATA *gb_main = GB_open(":", "r");
        if (!gb_main) {
            fprintf(stderr, "%s: %s\n", progname, the_message);
        }
        else {
            GBT_message(gb_main, the_message);
            GB_close(gb_main);
        }
    }
    free(the_message);
    return EXIT_SUCCESS;
}
