#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <arbdb.h>
#include <arbdbt.h>

struct timeval starttime;

void print_speed(const char *desc, int loops){
    struct timeval tp;
    gettimeofday(&tp,0);
    long usecs = tp.tv_sec - starttime.tv_sec;
    usecs *= 1000000;
    usecs += tp.tv_usec - starttime.tv_usec;
    printf("%s time %li   loops/second %i\n", desc, usecs,(int)( loops * 1000000.0 / (double)usecs) );
    starttime = tp;
}

int main(int argc, char **argv)
{
    GB_ERROR error = 0;

    if (argc == 0) {
        fprintf(stderr,"arb_2_ascii source.arb [dest.arb]: Converts a database to ascii format\n");
        fprintf(stderr,"        if dest.arb is set, try to fix problems  source.arb\n");
        fprintf(stderr,"        else replace source.arb by ascii version\n");

        error = "Missing arguments";
    }
    else {
        char   *in      = argv[1];
        GBDATA *gb_main = GBT_open(in,"rw",0);

        if (!gb_main){
            error = GB_await_error();
        }
        else {
            GBDATA *gbd;
            char *name;
            {
                GB_begin_transaction(gb_main);
                GBDATA *gb_species = GBT_first_species(gb_main);
                gbd = GB_search(gb_species,"name",GB_FIND);
                name = GB_read_string(gbd);
                GB_commit_transaction(gb_main);
            }

            int i;
            GB_begin_transaction(gb_main);
            gettimeofday(&starttime,0);

            GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_FIND);
            for (i= 0; i<10; i++){
                GB_find_string(gb_species_data,"full_name","asdfasdf", GB_IGNORE_CASE, down_2_level);
            }
            GB_commit_transaction(gb_main);
            print_speed("Transactions",i);

            GB_close(gb_main);
        }
    }

    if (error) {
        fprintf(stderr, "arb_perf_test: Error: %s\n", error);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
