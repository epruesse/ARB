#include <stdio.h>
#include <arbdb.h>

int main(int argc, char **argv) {
    int     i;
    char   *path;
    GBDATA *gbmain;
    GBDATA *gbspec, *gbspecname;

    if (argc < 2)
        path = ":";
    else
        path = argv[1];

    if (!(gbmain = GB_open(path, "r"))) {
        GB_print_error();
        return (-1);
    }
    GB_begin_transaction(gbmain);

    if (!(gbspec = GB_search(gbmain, "species_data/species", GB_FIND))){
        GB_abort_transaction(gbmain);
        GB_close(gbmain);
        return (-1);
    }

    for (; gbspec; gbspec = GB_nextEntry(gbspec)) {
        gbspecname = GB_entry(gbspec, "name");
        fprintf(stdout, "%s\n", GB_read_char_pntr(gbspecname));
    }

    GB_commit_transaction(gbmain);
    GB_close(gbmain);

}
