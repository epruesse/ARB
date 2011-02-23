// =============================================================== //
//                                                                 //
//   File      : arb_2_ascii.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdb.h>

int main(int argc, char **argv) {
    GB_ERROR error = 0;

    fprintf(stderr, "arb_2_ascii - ARB database binary to ascii converter\n");

    if (argc<2 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr,
                "\n"
                "Usage:   arb_2_ascii <source.arb> [<dest.arb>|-]\n"
                "Purpose: Converts a database to ascii format\n"
                "\n"
                "if <dest.arb> is set, try to fix problems of <source.arb> and write to <dest.arb>\n"
                "if the second parameter is '-' print to console.\n"
                "else replace <source.arb> by ascii version (backup first)\n"
                "\n"
                );

        if (strcmp(argv[1], "--help") != 0) { error = "Missing arguments"; }
    }
    else {
        char *in  = argv[1];
        char *out = NULL;

        const char *readflags = "rw";
        const char *saveflags = "a";

        if (argc == 2) {
            out = in;
        }
        else {
            readflags = "rwR";      /* try to recover corrupt data */
            out       = argv[2];

            if (!out || strcmp(out, "-") == 0) {
                saveflags = "aS";
            }
        }

        GB_shell  shell;
        GBDATA   *gb_main = GB_open(in, readflags);
        if (!gb_main) {
            error = GB_await_error();
        }
        else {
            error = GB_save(gb_main, out, saveflags);
        }

        GB_close(gb_main);
    }

    if (error) {
        fprintf(stderr, "arb_2_ascii: Error: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
