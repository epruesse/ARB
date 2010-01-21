#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

#define ASRS_BUFFERSIZE 256

struct arb_asrs_struct {
    GBDATA *gb_main;
    char    source[ASRS_BUFFERSIZE];
    char    dest[ASRS_BUFFERSIZE];
    char   *sp_name;
    char   *sequence;
} asrs;

void arb_asrs_menu()
{
    printf("please select source file name\n");
    fgets(asrs.source, ASRS_BUFFERSIZE, stdin);
    printf("please select dest file name\n");
    fgets(asrs.dest, ASRS_BUFFERSIZE, stdin);
}
#define NEXTBASE(p) while (*p && ((*p<'A') || (*p>'Z')) && ((*p<'a') || (*p>'z'))) p++;
void arb_asrs_swap()
{
    FILE *in, *out;
    char buffer[1024];
    char *p;
    char *ok;

    in = fopen(asrs.source, "r");
    if (!in) {
        printf("source file not found\n");
        exit (0);
    }
    out = fopen(asrs.dest, "w");
    if (!out) {
        printf("dest file could not be written\n");
        exit (0);
    }
    p = asrs.sequence;
    NEXTBASE(p);
    while ((ok = fgets(buffer, 1020, in))) {
        if (!strchr(buffer, '#')) {
            ok = strchr(buffer, ' ');
            if (ok && ok[1]) {
                if (*p) {
                    ok[1] = *(p++);
                    NEXTBASE(p);
                }
                else {
                    ok[1] = '-';
                }
            }
            fputs(buffer, out);
        }
        else {
            fputs(buffer, out);
            break;
        }
    }
    while ((ok = fgets(buffer, 1020, in))) {
        fputs(buffer, out);
    }
    fclose(in);
    fclose(out);
}

int main(int argc, char ** /* argv */) {
    GB_ERROR error = 0;
    if (argc != 1) {
        error = "no parameters";
    }
    else {
        asrs.gb_main = GB_open(":", "rwt");
        if (!asrs.gb_main) {
            error = "cannot find ARB server";
        }
        else {
            GB_begin_transaction(asrs.gb_main);
            GBDATA *gb_species = GBT_first_marked_species(asrs.gb_main);
            if (!gb_species) {
                error = "please mark exactly one species";
            }
            else if (GBT_next_marked_species(gb_species)) {
                error = "more than one species marked";
            }
            else {
                asrs.sp_name   = strdup(GBT_read_name(gb_species));
                GBDATA *gb_use = GB_search(asrs.gb_main, "presets/use", GB_FIND);
                char   *use    = GB_read_string(gb_use);
                GBDATA *gb_ali = GB_entry(gb_species, use);
                
                if (!gb_ali) {
                    error = GBS_global_string("Species '%s' has no data in alignment '%s'", asrs.sp_name, use);
                }
                else {
                    GBDATA *gb_data = GB_entry(gb_ali, "data");
                    
                    asrs.sequence = GB_read_string(gb_data);
                }
            }

            error = GB_end_transaction(asrs.gb_main, error);
            GB_close(asrs.gb_main);

            if (!error) {
                arb_asrs_menu();
                arb_asrs_swap();
            }
        }
    }

    int result = EXIT_SUCCESS;
    if (error) {
        fprintf(stderr, "Error in arb_swap_rnastr: %s\n", error);
        result = EXIT_FAILURE;
    }
    return result;
}
