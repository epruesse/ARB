#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>

int main(int argc, char **argv)
{
    GB_ERROR error;
    char *in;
    char *out;
    const char *readflags = "rw";
    GBDATA *gb_main;
    if (argc == 0) {
	fprintf(stderr,"arb_2_ascii source.arb [dest.arb]: Converts a database to ascii format\n");
	fprintf(stderr,"	if dest.arb is set, try to fix problems  source.arb\n");
	fprintf(stderr,"	else replace source.arb by ascii version\n");
	exit(-1);
    }
    in = argv[1];
    if (argc == 1) {
	out = in;
    }else{
	readflags = "rwR";		/* try to recover corrupt data */
	out = argv[2];
    }
    gb_main = GB_open(in,readflags);
    if (!gb_main){
	GB_print_error();
	return (-1);
    }
    error = GB_save(gb_main,out,"a");
    if (error){
	GB_print_error();
	return -1;
    }
    return 0;
}
