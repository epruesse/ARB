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
#define NEXTBASE(p) while ( *p && ((*p<'A') || (*p>'Z')) && ((*p<'a') || (*p>'z')) ) p++;
void arb_asrs_swap()
{
    FILE *in,*out;
    char buffer[1024];
    char *p;
    char *ok;

    in = fopen(asrs.source,"r");
    if (!in) {
        printf("source file not found\n");
        exit (0);
    }
    out = fopen(asrs.dest,"w");
    if (!out) {
        printf("dest file could not be written\n");
        exit (0);
    }
    p = asrs.sequence;
    NEXTBASE(p);
    while ((ok = fgets(buffer,1020,in))){
        if (!strchr(buffer,'#')) {
            ok = strchr(buffer,' ');
            if (ok && ok[1]) {
                if (*p){
                    ok[1] = *(p++);
                    NEXTBASE(p);
                }else{
                    ok[1] = '-';
                }
            }
            fputs(buffer,out);
        }else{
            fputs(buffer,out);
            break;
        }
    }
    while ((ok = fgets(buffer,1020,in))){
        fputs(buffer,out);
    }
    fclose(in);
    fclose(out);
}

int main(int argc, char **/*argv*/)
{
    //  char *error;
    const char *path;
    GBDATA *gb_species;
    GBDATA *gb_name;
    GBDATA *gb_use;
    GBDATA *gb_ali;
    GBDATA *gb_data;
    char    *use;
    if (argc != 1) {
        fprintf(stderr,"no parameters\n");
        exit(-1);
    }
    path = ":";
    asrs.gb_main = GB_open(path,"rwt");
    if (!asrs.gb_main){
        fprintf(stderr,"ERROR cannot find server\n");
        exit(-1);
    }
    GB_begin_transaction(asrs.gb_main);
    gb_species = GBT_first_marked_species(asrs.gb_main);
    if (!gb_species) {
        printf("please mark exactly one sequence\n");
        exit(0);
    }
    if (GBT_next_marked_species(gb_species)) {
        printf("more than one sequence marked\n");
        printf("please mark exactly one sequence\n");
        exit(0);
    }
    gb_name = GB_entry(gb_species,"name");
    asrs.sp_name = GB_read_string(gb_name);
    gb_use = GB_search(asrs.gb_main,"presets/use",GB_FIND);
    use = GB_read_string(gb_use);
    gb_ali = GB_entry(gb_species,use);
    if (!gb_ali) {
        printf("the selected species dont have the selected sequence\n");
        exit(0);
    }
    gb_data = GB_entry(gb_ali,"data");
    asrs.sequence = GB_read_string(gb_data);
    GB_commit_transaction(asrs.gb_main);
    GB_close(asrs.gb_main);

    arb_asrs_menu();
    arb_asrs_swap();
    return 0;
}
