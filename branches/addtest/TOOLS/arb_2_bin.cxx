// =============================================================== //
//                                                                 //
//   File      : arb_2_bin.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include <arbdbt.h>

int ARB_main(int argc, char *argv[]) {
    GB_ERROR error = 0;

    fprintf(stderr, "arb_2_bin - ARB database ascii to binary converter\n");

    if (argc <= 1 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr,
                "\n"
                "Purpose: Converts a database to binary format\n"
                "Syntax:  arb_2_bin [Options] database [newdatabase]\n"
                "Options: -m            create map file too\n"
                "         -r            try to repair destroyed database\n"
                "         -c[tree_xxx]  optimize database using tree_xxx or largest tree\n"
                "\n"
                "database my be '-' in which case arb_2_bin reads from stdin.\n"
                "\n"
            );

        if (strcmp(argv[1], "--help") != 0) { error = "Missing arguments"; }
    }
    else {
        char rtype[256];
        char wtype[256];
        int  ci   = 1;
        int  nidx = 0;

        const char *opt_tree = 0;

        {
            char *rtypep = rtype;
            char *wtypep = wtype;

            memset(rtype, 0, 10);
            memset(wtype, 0, 10);
            *(wtypep++) = 'b';
            *(rtypep++) = 'r';
            *(rtypep++) = 'w';

            while (argv[ci][0] == '-' && argv[ci][1] != 0) {
                if (!strcmp(argv[ci], "-m")) { ci++; *(wtypep++) = 'm'; }
                if (!strcmp(argv[ci], "-r")) { ci++; *(rtypep++) = 'R'; }
                if (!strncmp(argv[ci], "-c", 2)) { opt_tree = argv[ci]+2; ci++; }
                if (!strncmp(argv[ci], "-i", 2)) { nidx = atoi(argv[ci]+2); ci++; }
            }
        }

        const char *in  = argv[ci++];
        const char *out = ci >= argc ? in : argv[ci++];

        printf("Reading database...\n");
        GB_shell  shell;
        GBDATA   *gb_main = GBT_open(in, rtype);
        if (!gb_main) {
            error = GB_await_error();
        }
        else {
            if (opt_tree) {
                char *ali_name;
                {
                    GB_transaction ta(gb_main);
                    ali_name = GBT_get_default_alignment(gb_main);
                }
                if (!strlen(opt_tree)) opt_tree = 0;

                printf("Optimizing database...\n");
                error = GBT_compress_sequence_tree2(gb_main, opt_tree, ali_name);
                if (error) error = GBS_global_string("Error during optimize: %s", error);
                free(ali_name);
            }

            if (!error) {
                GB_set_next_main_idx(nidx);
                printf("Saving database...\n");
                error = GB_save(gb_main, out, wtype);
            }
            GB_close(gb_main);
        }
    }

    if (error) {
        fprintf(stderr, "arb_2_bin: Error: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
