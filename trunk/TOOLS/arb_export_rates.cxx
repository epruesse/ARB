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
                "syntax: arb_export_rates [SAI_NAME|'--none'] [other_fastdnaml_args]*\n"
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

#define APPEARS_IN_HEADER(c) (strchr(not_in_header, (c)) == 0)

    const char *not_in_header = "Y"; // these flags don't appear in header and they should be written directly after header

    for (int arg = 2; arg<argc; ++arg) { // put all fastdnaml arguments to header
        char command_char = argv[arg][0];
        if (!command_char) continue; // skip empty arguments

        if (APPEARS_IN_HEADER(command_char)) {
            char *neu = GB_strdup(GBS_global_string("%s %c", firstline, command_char));
            free(firstline);
            firstline = neu;
        }
    }

    {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_sai  = 0;
        GBDATA         *gb_data = 0;
        long            ali_len = 0;

        if (SAI_name[0] == 0 || strcmp(SAI_name, "--none") != 0) {
            gb_sai = GBT_find_SAI(gb_main,SAI_name);
            if (gb_sai)  {
                char *ali_name = GBT_get_default_alignment(gb_main);
                ali_len        = GBT_get_alignment_len(gb_main,ali_name);
                gb_data        = GBT_read_sequence(gb_sai,ali_name);
            }
        }

        // print header
        if (gb_data) fputc('C', stdout); // prefix with categories
        fputs(firstline, stdout); // then other_fastdnaml_args

        // print other_fastdnaml_args in reverse order
        // (first those which do not appear in header, rest afterwards)
        for (int appears_in_header = 0; appears_in_header <= 1; ++appears_in_header) {
            for (int arg = 2; arg < argc; ++arg) { // print [other_fastdnaml_args]*
                if (!argv[arg][0]) continue; // skip empty arguments
                if (!argv[arg][1]) continue; // dont print single character commands again on a own line
                if (APPEARS_IN_HEADER(argv[arg][0]) != appears_in_header) continue;
                fputc('\n', stdout);
                fputs(argv[arg], stdout);
            }
        }

        if (gb_data) { // if SAI was found
            printf("\nC 35 ");
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
