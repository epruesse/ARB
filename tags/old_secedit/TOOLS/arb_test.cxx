#include <stdio.h>
#include <arbdb.h>
#include <string.h>
int main(int argc, char **argv)
{
    GB_ERROR error;
    char *in;
//     const char *type = "b";
    int ci = 1;
    GBDATA *gb_main;

    if (argc <= 1)
    {
        GB_export_error(	"\narb_test: 	database test\n"
                            "		Syntax:	   arb_test database\n"
                            );
        GB_print_error();
        return(-1);
    }

    in = argv[ci++];

    gb_main = GB_open(in,"rw");
    if (!gb_main)
    {
        GB_print_error();
        return (-1);
    }

    error = GB_ralfs_test(gb_main);
    if (error)
    {
        GB_print_error();
        return -1;
    }

    return 0;
}


