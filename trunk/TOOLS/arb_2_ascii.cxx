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
#include <arb_handlers.h>

int ARB_main(int argc, char *argv[]) {
    GB_ERROR error = 0;

    ARB_redirect_handlers_to(stderr, stderr);

    fprintf(stderr, "arb_2_ascii - ARB database binary to ascii converter\n");

    bool recovery          = false; // try to recover corrupt data ?
    char compressionFlag[] = "\0x"; // extra space for compression flag
    bool help_requested    = false;

    while (argv[1]) {
        if (strcmp(argv[1], "-r") == 0) {
            recovery = true;
            argv++; argc--;
        }
        else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            help_requested = true;
            argv++; argc--;
        }
        else if (strncmp(argv[1], "-C", 2) == 0) {
            compressionFlag[0] = argv[1][2];
            compressionFlag[1] = 0;
            argv++; argc--;
        }
        else break;
    }

    if (!argv[1]) help_requested = true;

    if (help_requested) {
        fprintf(stderr,
                "\n"
                "Usage:   arb_2_ascii [-r] [-c<type>] <source.arb> [<dest.arb>|-]\n"
                "Purpose: Converts a database to ascii format\n"
                "\n"
                "if '-r' (=recovery) is specified, try to fix problems while reading <source.arb>.\n"
                "\n"
                "if '-C' (=Compress) is specified, use extra compression\n"
                "        known <type>s: %s\n"
                "\n"
                "if <dest.arb> is set, write to <dest.arb>\n"
                "if the second parameter is '-' write to console.\n"
                "else replace <source.arb> by ascii version (backup first)\n"
                "\n",
                GB_get_supported_compression_flags(true));
    }
    else {
        const char *readflags = recovery ? "rwR" : "rw";
        char saveflags[]      = "a\0xx"; // extra optional space for: compressionFlag, 'S'

        strcat(saveflags, compressionFlag);
        if (strchr(GB_get_supported_compression_flags(false), compressionFlag[0]) == 0) {
            error = GBS_global_string("Unknown compression flag '%c'", compressionFlag[0]);
        }
        else {
            const char *in  = argv[1];
            const char *out = NULL;

            if (argc == 2) {
                out = in;
            }
            else {
                out = argv[2];

                if (!out || strcmp(out, "-") == 0) {
                    strcat(saveflags, "S");
                }
            }

            if (!in) {
                error = "Missing parameters";
            }
            else {
                GB_shell  shell;
                GBDATA   *gb_main = GB_open(in, readflags);
                if (!gb_main) {
                    error = GB_await_error();
                }
                else {
                    error = GB_save(gb_main, out, saveflags);
                    GB_close(gb_main);
                }
            }
        }
    }

    if (error) {
        fprintf(stderr, "arb_2_ascii: Error: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
