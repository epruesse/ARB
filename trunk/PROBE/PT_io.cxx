#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <memory.h>
#include <string.h>
#include <PT_server.h>
#include "probe.h"
#include <arbdbt.h>
#include <BI_helix.hxx>
extern "C" { char *gbs_malloc_copy(char *,long); }


/* change a sequence with normal bases the PT_? format and delete all other signs */
int compress_data(char *probestring)
{
    char    c;
    char    *src,
        *dest;
    dest = src = probestring;

    while((c=*(src++)))
    {
        switch (c) {
            case 'A':
            case 'a': *(dest++) = PT_A;break;
            case 'C':
            case 'c': *(dest++) = PT_C;break;
            case 'G':
            case 'g': *(dest++) = PT_G;break;
            case 'U':
            case 'u':
            case 'T':
            case 't': *(dest++) = PT_T;break;
            case 'N':
            case 'n': *(dest++) = PT_N;break;
            default: break;
        }

    }
    *dest = PT_QU;
    return 0;
}

/* get a string with readable bases from a string with PT_? */
int PT_base_2_string(char *id_string, long len)
{
    char    c;
    char    *src,
        *dest;
    if (!len) len = strlen(id_string);
    dest = src = id_string;

    while((len--)>0){
        c=*(src++);
        switch (c) {
            case PT_A: *(dest++) = 'A';break;
            case PT_C: *(dest++) = 'C';break;
            case PT_G: *(dest++) = 'G';break;
            case PT_T: *(dest++) = 'U';break;
            case PT_N: *(dest++) = 'N';break;
            case 0: *(dest++) = '0'; break;
            default: *(dest++) = c;break;
        }

    }
    *dest = '\0';
    return 0;
}

void probe_read_data_base(char *name)
{
    // IDP Datenbank oeffnen


    GBDATA *gb_main;
    GBDATA *gb_species_data;

    GB_set_verbose();
    gb_main = GB_open(name,"r");
    if (!gb_main) {
        printf("Error reading file %s\n",name);
        exit(EXIT_FAILURE);
    }
    GB_begin_transaction(gb_main);
    gb_species_data = GB_find(gb_main,"species_data",0,down_level);
    if (!gb_species_data){
        printf("Database %s is empty\n",name);
        exit(EXIT_FAILURE);
    }
    psg.gb_main = gb_main;
    psg.gb_species_data = gb_species_data;
    psg.gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    GB_commit_transaction(gb_main);
}

int probe_compress_sequence(char *seq)
{
    static uchar *tab = 0;
    if (!tab){
        tab = (uchar *)malloc(256);
        memset(tab,PT_N,256);
        tab['A'] = tab['a'] = PT_A;
        tab['C'] = tab['c'] = PT_C;
        tab['G'] = tab['g'] = PT_G;
        tab['T'] = tab['t'] = PT_T;
        tab['U'] = tab['u'] = PT_T;
        tab['.'] = PT_QU;
    }

    char *dest;
    uchar c;
    char *source;
    int last_is_q = 0;
    dest = source = seq;
    while((c=(uchar)*(seq++)))
    {
        if (c == '-') continue;
        c = tab[c];
        if (c == PT_QU){
            if (last_is_q) continue;
            last_is_q = 1;
        }else{
            last_is_q = 0;
        }
        *(dest)++ = c;
    }

    if (dest[-1] != PT_QU){
        *(dest++) = PT_QU;
        printf("error\n");
    }
    return dest-source;
}
char *probe_read_string_append_point(GBDATA *gb_data,int *psize)
{
    char *data;
    char *buffer;
    int len;
    len = (int)GB_read_string_count(gb_data);
    data = GB_read_string(gb_data);
    if (data[len-1] != '.') {
        buffer = (char *)calloc(sizeof(char),len+2);
        strcpy(buffer,data);
        buffer[len++] = '.';
        free(data);
        data = buffer;
    }
    *psize = len;
    return data;
}
char           *
probe_read_alignment(int j, int *psize)
{
    static char    *buffer = 0;
    GBDATA  *gb_ali,*gb_data;
    buffer = 0;
    gb_ali = GB_find(psg.data[j].gbd, psg.alignment_name, 0, down_level);
    if (!gb_ali)
        return 0;
    /* this alignment dont exists */
    gb_data = GB_find(gb_ali, "data", 0, down_level);
    if (!gb_data) {
        return 0;
    }
    buffer = probe_read_string_append_point(gb_data,psize);
    return buffer;
}
void probe_read_alignments()
    /* liest die Alignments in psg.data */
{
    GBDATA     *gb_species;
    GBDATA     *gb_name;
    GBDATA     *gb_ali;
    GBDATA     *gb_data;
    char       *data;
    int         count;
    static int  size,hsize;

    count = 0;
    GB_begin_transaction(psg.gb_main);

    /* count all data */
    char *def_ref = GBT_get_default_ref(psg.gb_main);
    gb_species    = GBT_find_SAI_rel_exdata(psg.gb_extended_data, def_ref);
    free(def_ref);
    psg.ecoli     = 0;
    if (gb_species) {
        gb_data = GBT_read_sequence(gb_species,psg.alignment_name);
        if (gb_data) {
            psg.ecoli = GB_read_string(gb_data);
        }
    }
    for (gb_species = GBT_first_species_rel_species_data(psg.gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species) )
    {
        count ++;
    }
    psg.data       = (struct probe_input_data *) calloc(sizeof(struct probe_input_data),count);
    psg.data_count = 0;
    printf("Database contains %i species.\nPreparing sequence data:\n", count);

    int data_missing  = 0;
    int species_count = count;

    count = 0;

    for (gb_species = GBT_first_species_rel_species_data(psg.gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species) )
    {
        gb_name = GB_find(gb_species,"name",0,down_level);
        if (!gb_name) continue;

        psg.data[count].name = GB_read_string(gb_name);

        gb_name = GB_find(gb_species,"full_name",0,down_level);
        if (gb_name) {
            psg.data[count].fullname = GB_read_string(gb_name);
        }
        else {
            static char empty[] = "";
            psg.data[count].fullname = empty;
        }
        psg.data[count].is_group = 1;
        psg.data[count].gbd = gb_species;
        /* get alignments */
        gb_ali = GB_find(gb_species, psg.alignment_name, 0, down_level);
        if (!gb_ali) continue;
        /* this alignment doesnt exist */
        gb_data = GB_find(gb_ali,"data",0,down_level);
        if (!gb_data) {
            fprintf(stderr,"Species '%s' has no data in '%s'\n", psg.data[count].name, psg.alignment_name);
            data_missing++;
            continue;
        }
        psg.data[count].checksum = GBS_checksum(GB_read_char_pntr(gb_data),1,".-");
        data                     = probe_read_string_append_point(gb_data,&hsize);
        size                     = probe_compress_sequence(data);
        psg.data[count].data     = (char *)gbs_malloc_copy(data,size);
        psg.data[count].size     = size;
        free(data);
        count ++;

        if (count%10 == 0) {
            if (count%500) {
                printf(".");
                fflush(stdout);
            }
            else {
                printf(".%6i (%5.1f%%)\n", count, ((double)count/species_count)*100);
            }
        }
    }
    printf("\n");
    psg.data_count = count;
    GB_commit_transaction(psg.gb_main);
    if (data_missing) {
        printf("%i species were ignored because of missing data.\n", data_missing);
    }
    else {
        printf("All species contain data in alignment '%s'.\n", psg.alignment_name);
    }
}

void PT_build_species_hash(void){
    long i;
    psg.namehash = GBS_create_hash(PT_NAME_HASH_SIZE,0);
    for (i=0;i<psg.data_count;i++){
        GBS_write_hash(psg.namehash, psg.data[i].name,i+1);
    }
    unsigned int    max_size;
    max_size = 0;
    for (i = 0; i < psg.data_count; i++){   /* get max sequence len */
        max_size = max(max_size, (unsigned)(psg.data[i].size));
        psg.char_count += psg.data[i].size;
    }
    psg.max_size = max_size;

    if ( psg.ecoli){
        BI_ecoli_ref *ref = new BI_ecoli_ref;
        const char *error = ref->init(psg.ecoli,strlen(psg.ecoli));
        if (error) {
            delete psg.ecoli;
            psg.ecoli = 0;
        }else{
            psg.bi_ecoli = ref;
        }
    }
}


long PT_abs_2_rel(long pos) {
    if (!psg.ecoli) return pos;
    return psg.bi_ecoli->abs_2_rel(pos);
}

long PT_rel_2_abs(long pos) {
    if (!psg.ecoli) return pos;
    return psg.bi_ecoli->rel_2_abs(pos);
}
