#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_awars.hxx>

/* Input: SAI name: argv[1]
 *        fastdnaml-args: argv[2]...
 *        ALINAME:  AWAR "presets/use"
 *        FILTER:   AWAR_GDE_EXPORT_FILTER
 *
 * Output:
 *        If SAI + Sequence is found: rates in fastdnaml-format (can be piped into arb_convert_aln)
 *        Otherwise : just forward args
 *
 *        If flag '-r' is used, weights are always printed. If no SAI given, every alignment-column  
 *        is given the same weight (1).
 */

#define CATSCALE 0.71           // downscale factor for rates
#define MIO      1000000        // factor to scale rate-values to integers (for RAxML)

int main(int argc, char **argv){
    argc--;argv++;
    
    if (argc<1 || strcmp(argv[0],"--help") == 0) {
        fprintf(stderr,
                "\n"
                "arb_export_rates: Add a line to phylip format which can be used by fastdnaml for rates\n"
                "syntax: arb_export_rates [SAI_NAME|'--none'] [other_fastdnaml_args]*\n"
                "if other_fastdnaml_args are given they are inserted to the output\n"
                "\n"
                "or\n"
                "\n"
                "arb_export_rates: Write a weightsfile for RAxML\n"
                "syntax: arb_export_rates -r [SAI_NAME|'--none']\n"
                "\n"
                );
        return EXIT_FAILURE;
    }

    bool RAxML_mode = false;

    if (argc >= 2) {
        if (strcmp(argv[0], "-r") == 0) {
            RAxML_mode = true;
            argc--;argv++;
        }
    }

    GBDATA *gb_main = GBT_open(":","r",0);
    if (!gb_main){
        GB_print_error();
        return EXIT_FAILURE;
    }

    char *SAI_name  = argv[0];
    argc--;argv++;

    {
        char *seq        = 0;
        char *filter     = 0;
        int   seq_len    = 0;
        int   filter_len = 0;
        long  ali_len    = 0;
        {
            GB_transaction  ta(gb_main);
            GBDATA         *gb_sai  = 0;
            GBDATA         *gb_data = 0;

            char *ali_name = GBT_get_default_alignment(gb_main);
            ali_len        = GBT_get_alignment_len(gb_main, ali_name);

            filter     = GBT_read_string(gb_main,AWAR_GDE_EXPORT_FILTER);
            filter_len = strlen(filter);

            if (SAI_name[0] != 0 || strcmp(SAI_name, "--none") != 0) {
                gb_sai = GBT_find_SAI(gb_main,SAI_name);
                if (gb_sai)  {
                    gb_data = GBT_read_sequence(gb_sai,ali_name);

                    if (gb_data) {
                        seq     = GB_read_string(gb_data);
                        seq_len = strlen(seq);
                    }
                }
            }
            free(ali_name);
        }

        if (!RAxML_mode) {
#define APPEARS_IN_HEADER(c) (strchr(not_in_header, (c)) == 0)
            const char *not_in_header = "Y"; // these flags don't appear in header and they should be written directly after header

            {
                char *firstline = strdup("");
                for (int arg = 0; arg<argc; ++arg) { // put all fastdnaml arguments to header
                    char command_char = argv[arg][0];
                    if (!command_char) continue; // skip empty arguments

                    if (APPEARS_IN_HEADER(command_char)) {
                        freeset(firstline, GBS_global_string_copy("%s %c", firstline, command_char));
                    }
                }

                // print header
                if (seq) fputc('C', stdout); // prefix with categories
                fputs(firstline, stdout); // then other_fastdnaml_args
                free(firstline);
            }

            // print other_fastdnaml_args in reverse order
            // (first those which do not appear in header, rest afterwards)
            for (int appears_in_header = 0; appears_in_header <= 1; ++appears_in_header) {
                for (int arg = 0; arg < argc; ++arg) { // print [other_fastdnaml_args]*
                    if (!argv[arg][0]) continue; // skip empty arguments
                    if (!argv[arg][1]) continue; // dont print single character commands again on a own line
                    if (APPEARS_IN_HEADER(argv[arg][0]) != appears_in_header) continue;
                    fputc('\n', stdout);
                    fputs(argv[arg], stdout);
                }
            }
#undef APPEARS_IN_HEADER

            if (seq) { // if SAI was found
                printf("\nC 35 ");
                double rate = 1.0;
                int    i;
                for (i=0;i<35;i++){
                    printf("%f ",rate);
                    rate *= CATSCALE;
                }
                printf("\nCategories ");

                for (i=0; i<seq_len; i++) {
                    if (i>filter_len || filter[i] != '0') {
                        int c = seq[i];
                        
                        arb_assert(c != '0'); // only 35 cats (1-9 and A-Z)
                        
                        if ( (c < '0' || c>'9' ) &&  (c < 'A' || c>'Z')) c = '1';
                        putchar(c);
                    }
                }
                for (; i<ali_len; i++) {
                    putchar('1');
                }
            }
        }
        else {
            // write RAxML weightsfile content
            int cnt = 0;

            char *weight['Z'+1];
            int i;

            for (i = 0; i <= 'Z'; i++) weight[i] = 0;

            double rate = 1.0;

            for (i = '1'; i <= '9'; i++) {
                weight[i] = GBS_global_string_copy(" %i", int(rate*MIO));
                rate *= CATSCALE;
            }
            for (i = 'A'; i <= 'Z'; i++) {
                weight[i] = GBS_global_string_copy(" %i", int(rate*MIO));
                rate *= CATSCALE;
            }

            if (!seq) { // no SAI selected -> unique weights
                seq          = (char*)malloc(ali_len+1);
                memset(seq, '1', ali_len);
                seq[ali_len] = 0;

                seq_len = ali_len;

                freedup(weight[int('1')], " 1");
            }

            for (i=0; i<seq_len; i++) {
                if (i>filter_len || filter[i] != '0') {
                    int c = seq[i];
                    if ( (c < '0' || c>'9' ) &&  (c < 'A' || c>'Z')) c = '1';
                    fputs(weight[c], stdout);
                    if (++cnt>30) {
                        fputc('\n', stdout);
                        cnt = 0;
                    }
                }
            }

            for (; i<ali_len; i++) {
                if (i>filter_len || filter[i] != '0') {
                    fputs(weight[int('1')], stdout);
                    if (++cnt>30) {
                        fputc('\n', stdout);
                        cnt = 0;
                    }
                }
            }
                
            for (i = 0; i <= 'Z'; i++) free(weight[i]);
            
            fputc('\n', stdout);
        }

        free(filter);
        free(seq);
    }
    GB_close(gb_main);
    return EXIT_SUCCESS;
}
