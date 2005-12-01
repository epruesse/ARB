#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <string.h>
int main(int argc, char **argv)
{
    GB_ERROR  error;
    char     *in;
    char     *out;
    char      rtype[256];
    char     *rtypep   = rtype;
    char      wtype[256];
    char     *wtypep   = wtype;
    char     *opt_tree = 0;
    int       ci       = 1;
    int       nidx     = 0;
    int       test     = 0;
    GBDATA   *gb_main;
    memset(rtype,0,10);
    memset(wtype,0,10);
    *(wtypep++)        = 'b';
    *(rtypep++)        = 'r';
    *(rtypep++)        = 'w';

    fprintf(stderr, "arb_2_bin - ARB database ascii to binary converter\n");

    if (argc <= 1 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr,
                "\n"
                "Purpose: Converts a database to binary format\n"
                "Syntax:  arb_2_bin [-m] [-r] [-c][tree_xxx] database [newdatabase]\n"
                "Options: -m            create map file too\n"
                "         -r            try to repair destroyed database\n"
                "         -c[tree_xxx]	optimize database using tree_xxx or largest tree\n"
                "\n"
                "database my be '-' in which case arb_2_bin reads from stdin.\n"
                "\n"
                );
        return(-1);
    }
    while (argv[ci][0] == '-' && argv[ci][1] != 0){
        if (!strcmp(argv[ci],"-m")){
            ci++;
            *(wtypep++) = 'm';
        }
        if (!strcmp(argv[ci],"-r")){
            ci++;
            *(rtypep++) = 'R';
        }
        if (!strncmp(argv[ci],"-c",2)){
            opt_tree = argv[ci]+2;

            ci++;
        }
        if (!strncmp(argv[ci],"-i",2)){
            nidx = atoi(argv[ci]+2);
            ci++;
        }
        if (!strncmp(argv[ci],"-t",2)){
            test = 1;
            ci++;
        }
    }

    in = argv[ci++];
    if (ci >= argc) {
        out = in;
    }else{
        out = argv[ci++];
    }

    printf("Reading database...\n");
    gb_main = GBT_open(in,rtype,0);
    if (!gb_main){
        GB_print_error();
        return (-1);
    }
    if (opt_tree){
        char *ali_name;
        {
            GB_transaction dummy(gb_main);
            ali_name = GBT_get_default_alignment(gb_main);
        }
        if (!strlen(opt_tree)) opt_tree = 0;
        printf("Optimizing database...\n");
        error = GBT_compress_sequence_tree2(gb_main,opt_tree,ali_name);
        if (error) {
            printf("Error during optimize: ");
            GB_print_error();
        }
        free(ali_name);
    }
    GB_set_next_main_idx(nidx);

    if (test){
        GB_ralfs_test(gb_main);
    }

    printf("Saving database...\n");
    error = GB_save(gb_main,out,wtype);
    if (error){
        fprintf(stderr, "arb_2_bin: %s\n", error);
        return -1;
    }
    return 0;
}
