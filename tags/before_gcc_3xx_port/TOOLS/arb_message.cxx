#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

void show_error(GBDATA *gb_main) {
    GBT_message(gb_main, GB_get_error());
    GB_print_error();
}

// ---------------------------------------
//      int main(int argc,char **argv)
// ---------------------------------------
int main(int argc,char **argv)
{
    if (argc == 1)  {
        fprintf(stderr, "Usage: arb_message \"the message\"");
        return -1;
    }

    const char *progname = argv[0];
    if (!progname || progname[0] == 0) {
        progname = "arb_message";
    }

    char *the_message  = strdup(argv[1]);
    char *unencoded_lf = 0;
    while ((unencoded_lf = strstr(the_message, "\\n")) != 0) {
        unencoded_lf[0] = '\n';
        strcpy(unencoded_lf+1, unencoded_lf+2);
    }

    GBDATA *gb_main = GB_open(":","r");
    if (!gb_main){
        fprintf(stderr, "%s: %s\n", progname, the_message);
    }
    else {
        GBT_message(gb_main, the_message);
        GB_close(gb_main);
    }
    free(the_message);
    return 0;
}
