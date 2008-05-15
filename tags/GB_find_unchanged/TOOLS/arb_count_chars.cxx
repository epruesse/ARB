#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <arbdb.h>
#include <arbdbt.h>

/*  count different kind of nucleotides for each alignment position */

#define MAXLETTER ('Z'-'A'+1)
#define RESULTNAME "COUNTED_CHARS"

int main(int argc, char **argv){
    const char *db_name = ":";      // default name -> link to server
    if (argc >1) db_name = argv[1];     // get the name of the database
    int is_amino = 0;

    printf("Counting the number of different chars of all marked sequences\n");

    GBDATA *gb_main = GB_open(":","rw");    // open database
    if (!gb_main){
        GB_print_error();
        return -1;
    }

    GB_begin_transaction(gb_main); // open transaction

    char *alignment_name = GBT_get_default_alignment(gb_main);  // info about sequences
    int alignment_len = GBT_get_alignment_len(gb_main,alignment_name);
    is_amino = GBT_is_alignment_protein(gb_main,alignment_name);
    char filter[256];
    if (is_amino){
        memset(filter,1,256);
        filter[(unsigned char)'B'] = 0;
        filter[(unsigned char)'J'] = 0;
        filter[(unsigned char)'O'] = 0;
        filter[(unsigned char)'U'] = 0;
        filter[(unsigned char)'Z'] = 0;
    }else{
        memset(filter,0,256);
        filter[(unsigned char)'A'] = 1;
        filter[(unsigned char)'C'] = 1;
        filter[(unsigned char)'G'] = 1;
        filter[(unsigned char)'T'] = 1;
        filter[(unsigned char)'U'] = 1;
    }

    // malloc and clear arrays for counting characters (only letters)
    int *counters[MAXLETTER];
    int i;
    for (i=0;i<MAXLETTER;i++){
        counters[i] = (int *)calloc(sizeof(int),alignment_len);
    }


    // loop over all marked species
    int slider = 0;
    int all_marked = GBT_count_marked_species(gb_main);

    for (GBDATA *gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species)){
        if ((slider++)%10 == 0) printf("%i:%i\n",slider,all_marked);

        GBDATA *gb_ali = GB_find(gb_species, alignment_name, 0, down_level); // search the sequence database entry ( ali_xxx/data )
        if (!gb_ali) continue;
        GBDATA *gb_data = GB_find(gb_ali, "data", 0, down_level);
        if(!gb_data) continue;

        const char *seq = GB_read_char_pntr(gb_data);
        if (!seq) continue;

        unsigned char c;
        for ( i=0; (i< alignment_len) && (c = *(seq)); i++, seq++){
            if (!isalpha(c)) continue;
            c = toupper(c);
            if (filter[c]){
                counters[ c - 'A' ][i] ++;
            }
        }
    }

    char *result = (char *)calloc(sizeof(char), alignment_len + 1);
    for (i=0; i<alignment_len; i++) { 
        int sum = 0;
        for (int l = 0; l < MAXLETTER; l++){
            if (counters[l][i]>0) sum++;
        }
        result[i] = sum<10 ? '0'+sum : 'A'-10+sum;
    }
    // save result as SAI COUNTED_CHARS
    {
        GBDATA *gb_sai = GBT_create_SAI(gb_main,RESULTNAME);
        if (!gb_sai) {
            GB_print_error();
            return -1;
        }
        GBDATA *gb_data = GBT_add_data(gb_sai, alignment_name, "data", GB_STRING);
        GB_write_string(gb_data, result);
    }

    GB_commit_transaction(gb_main); // commit it
    GB_close(gb_main);      // politely disconnect from server
    return 0;

}
