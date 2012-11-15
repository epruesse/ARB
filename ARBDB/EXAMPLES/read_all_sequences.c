#include <stdio.h>
#include <arbdb.h>
#include <arbdbt.h>

int main(int argc, char **argv) {
    int     i;
    char   *path;
    GBDATA *gbmain;
    GBDATA *gbspec, *gbspecname;
    char   *ali_name;
    int     len = 0;
    
    if (argc < 2)
        path = ":";
    else
        path = argv[1];

    if (!(gbmain = GB_open(path, "r"))) {
        GB_print_error();
        return (-1);
    }
    GB_begin_transaction(gbmain);
    ali_name = GBT_get_default_alignment(gbmain);


    for (   gbspec = GBT_first_species_2(gbmain);
            gbspec;
            gbspec = GBT_next_species(gbspec)) {
        GBDATA *gb_data = GBT_read_sequence(gbspec,ali_name);
        if (!gb_data) continue;
        GB_read_char_pntr(gb_data);
        len += GB_read_string_count(gb_data);
    }

    GB_commit_transaction(gbmain);
    GB_close(gbmain);
    printf("%i bytes decompressed\n",len);
    return 0;
}
