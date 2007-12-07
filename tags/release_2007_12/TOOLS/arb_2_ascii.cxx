#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>

static void to_stderr(const char *msg) {
    fprintf(stderr, "arb_2_ascii: %s\n", msg);
}

int main(int argc, char **argv)
{
    GB_ERROR    error;
    char       *in;
    char       *out;
    const char *readflags      = "rw";
    const char *saveflags      = "a";
    GBDATA     *gb_main;
    int         dump_to_stdout = 0;

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
        exit(-1);
    }

    in = argv[1];
    if (argc == 1) {
        out = in;
    }
    else {
        readflags = "rwR";	/* try to recover corrupt data */
        out       = argv[2];

        if (strcmp(out, "-") == 0) {
            saveflags      = "aS";
            GB_install_information(to_stderr);
            GB_install_warning(to_stderr);
            dump_to_stdout = 1;
        }
    }

    gb_main = GB_open(in,readflags);
    if (!gb_main){
        GB_print_error();
        return (-1);
    }

    error = GB_save(gb_main,out, saveflags);
    if (error){
        fprintf(stderr, "arb_2_ascii: %s\n", error);
        return -1;
    }
    return 0;
}
