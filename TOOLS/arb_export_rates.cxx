#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_awars.hxx>

/* Input	 SAI name = 'argv[1]'
 *		ALINAME = 	<presets/use>
 *		FILTER	= 	<gb_main/AWAR_GDE_EXPORT_FILTER>
 *
 *	Output:
 *		If SAI + Sequence is found
 */

int main(int argc, char **argv){
    //    GB_ERROR error;
//     if (argc <= 1) {
//         return EXIT_SUCCESS; // no arguments -> simply return
//     }
    if (argc<2 || strcmp(argv[1],"--help") == 0) {
        fprintf(stderr,
                "\n"
                "arb_export_rates:  Add a line to phylip format which can be used by fastdnaml for rates\n"
                "syntax: arb_export_rates SAI_NAME [other_fastdnaml_args]*\n"
                "if other_fastdnaml_args are given they are inserted to the output\n"
                "\n");
        return EXIT_FAILURE;
    }

    GBDATA *gb_main = GBT_open(":","r",0);
    if (!gb_main){
        GB_print_error();
        return EXIT_FAILURE;
    }

    char *firstline         = strdup("");
    char *SAI_name          = argv[1];

    for (int arg = 2; arg<argc; ++arg) {
        char        command_char  = argv[arg][0];
        const char *not_in_header = "Y";

        if (strchr(not_in_header, command_char) == 0) {
            char *neu = GB_strdup(GBS_global_string("%s %c", firstline, command_char));
            free(firstline);
            firstline = neu;
        }
    }

    {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_sai  = GBT_find_SAI(gb_main,SAI_name);
        GBDATA         *gb_data = 0;
        long            ali_len = 0;

        if (gb_sai)  {
            char *ali_name = GBT_get_default_alignment(gb_main);
            ali_len        = GBT_get_alignment_len(gb_main,ali_name);
            gb_data        = GBT_read_sequence(gb_sai,ali_name);
        }

        // print header
        fputc(gb_data ? 'C' : 'F', stdout);
        fputs(firstline, stdout);
        fputc('\n', stdout);

        for (int arg = 2; arg<argc; ++arg) { // print [other_fastdnaml_args]*
            if (argv[arg][1]) { // dont print single character commands again on a own line
                fputs(argv[arg], stdout);
                fputc('\n', stdout);
            }
        }

        if (gb_data) { // if SAI was found
            printf("C 35 ");
            double rate = 1.0;
            int    i;
            for (i=0;i<35;i++){
                printf("%f ",rate);
                rate *= 0.71;
            }
            printf("\nCategories ");

            char *seq = GB_read_string(gb_data);

            char *filter = GBT_read_string(gb_main,AWAR_GDE_EXPORT_FILTER);

            int flen = strlen(filter);
            int slen = strlen(seq);
            if (slen < ali_len) slen = ali_len;

            for (i=0;i<slen;i++){
                if (i>flen || filter[i] != '0') {
                    int c = seq[i];
                    if ( (c < '0' || c>'9' ) &&  (c < 'A' || c>'Z')) c = '1';
                    putchar(c);
                }
            }
            for (;i<ali_len;i++){
                putchar('1');
            }
        }
    }
    GB_close(gb_main);
    return EXIT_SUCCESS;
}
