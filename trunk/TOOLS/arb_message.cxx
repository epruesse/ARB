#include <stdio.h>
#include <stdlib.h>
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

    GBDATA *gb_main = GB_open(":","r");
    if (!gb_main){
        fprintf(stderr, "%s: %s\n", argv[0], argv[1]);
    }
    else {
        GBT_message(gb_main, argv[1]);
        GB_close(gb_main);
    }
    return 0;
}
