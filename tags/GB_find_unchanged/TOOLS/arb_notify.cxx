#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

int main(int argc,char **argv)
{
    if (argc == 1)  {
        fprintf(stderr, "Usage: arb_notify ID \"message\"\n");
        fprintf(stderr, "Used by ARB itself to report termination of external jobs - see GB_notify()\n");
        return -1;
    }

    const char *progname = argv[0];
    if (!progname || progname[0] == 0) progname = "arb_notify";

    GBDATA *gb_main = GB_open(":","r");
    if (!gb_main){
        fprintf(stderr, "%s: Can't notify (connect to ARB failed)\n", progname);
    }
    else {
        char       *the_message = strdup(argv[2]);
        const char *idstr       = argv[1];
        int         id          = atoi(idstr);
        GB_ERROR    error       = 0;

        if (!id) {
            error = GBS_global_string("Illegal ID '%s'", idstr);
        }

        if (!error) {
            GB_transaction ta(gb_main);
            error = GB_notify(gb_main, id, the_message);
        }

        free(the_message);

        if (error) {
            GBT_message(gb_main, GBS_global_string("Error in %s: %s", progname, error));
        }
        GB_close(gb_main);
    }

    return 0;
}
