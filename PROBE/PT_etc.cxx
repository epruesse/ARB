#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <PT_server.h>
#include <PT_server_prototypes.h>
#include "probe.h"
#include "pt_prototypes.h"
#include <struct_man.h>

/* get a weighted table for PT_N mismatches */
void set_table_for_PT_N_mis()
{
    int i;
    psg.w_N_mismatches[0] = 0;
    psg.w_N_mismatches[1] = 0;
    psg.w_N_mismatches[2] = 1;
    psg.w_N_mismatches[3] = 2;
    psg.w_N_mismatches[4] = 4;
    psg.w_N_mismatches[5] = 5;
    for (i=6; i<=PT_POS_TREE_HEIGHT; i++)
        psg.w_N_mismatches[i] = i;
}
void pt_export_error(PT_local *locs, const char *error)
{
    if (locs->ls_error) free(locs->ls_error);
    locs->ls_error = strdup(error);
}
/* get the name with a virtual function */
extern "C" char *virt_name(PT_probematch *ml)
{
    if (gene_flag) {
        list<gene_struct*>::iterator gene_iterator;
        gene_iterator = names_list_idp.begin();
    
        while (strcmp((*gene_iterator)->gene_name,psg.data[ml->name].name) && gene_iterator != names_list_idp.end()) {
            gene_iterator++;
        }

        if (gene_iterator == names_list_idp.end()) {
            printf("Error in Name Resolution\n");
            exit(1);
        }
        else {
            return (*gene_iterator)->arb_species_name;
        }
    }
    else {
        return psg.data[ml->name].name;
    }
}

extern "C" char *virt_fullname(PT_probematch * ml) {
    if (gene_flag) {
        list<gene_struct*>::iterator gene_iterator;
        gene_iterator = names_list_idp.begin();
    
        while (strcmp((*gene_iterator)->gene_name,psg.data[ml->name].name) && gene_iterator != names_list_idp.end()) {
            gene_iterator++;
        }

        if (gene_iterator == names_list_idp.end()) {
            printf("Error in Name Resolution\n");
            exit(1);
        }
        else {
            return (*gene_iterator)->arb_gene_name;
        }
    }
    else {

        if (psg.data[ml->name].fullname) {
            return psg.data[ml->name].fullname;
        } else {
            return (char*)"";
        }
    
    }
}

/* copy one mismatch table to a new one allocating memory */
int *table_copy(int *mis_table, int length)
{
    int	*result_table;
    int	i;

    result_table = (int *)calloc(length, sizeof(int));
    for (i=0; i<length; i++)
        result_table[i] = mis_table[i];
    return result_table;
}
/* add the values of a source table to a destination table */
void table_add(int *mis_tabled, int *mis_tables, int length)
{
    int i;
    for (i=0; i<length; i++)
        mis_tabled[i] += mis_tables[i];
}

/* read the name list seperated by # and set the flag for the group members, + returns a list of names which have not been found */
char *ptpd_read_names(PT_local * locs, char *names_listi, char *checksumsi)
{
    char *names_list;
    char *checksums = 0;
    long  i;
    char *name;
    
    if (gene_flag) {
        char *temp;
        int   j = 0;
        int   k = 0;
        char *listi_copy;
        char *temp_listi;
        char bit;        
        list<gene_struct*>::iterator gene_iterator;
        gene_iterator = names_list_idp.begin();
        bit           = names_listi[k];
        while (bit != '\0') {
            if (bit=='#') j++;
            k++;
            bit = names_listi[k];
        }
        j++;
        temp_listi = new char[j*9+2];
        listi_copy = new char[strlen(names_listi)+1];
        strcpy(temp_listi,"");
        strcpy(listi_copy,names_listi);
        temp = strtok(listi_copy,"#");
        while (temp) {
            while (strcmp((*gene_iterator)->arb_gene_name,temp) && gene_iterator != names_list_idp.end()) {
                gene_iterator++;
            }

            if (gene_iterator == names_list_idp.end()) {
                printf("Error in Name Resolution\n");
                exit(1);
            }
            else {
                strcat(temp_listi,(*gene_iterator)->gene_name);
                strcat(temp_listi,"#");
            }
            temp = strtok(NULL,"#");
        }
        //  temp_listi[strlen(temp_listi)] = '\0';
        names_listi = temp_listi;
    }

    /* delete is_group */
    for (i = 0; i < psg.data_count; i++)
        psg.data[i].is_group = 0;
    locs->group_count = 0;
    if (!names_listi) return 0;
    char *to_free_names_list = names_list =	strdup(names_listi);
    char *to_free_checksums = 0;
    /* einzelne Namen einlesen */
    void *not_found = GBS_stropen(1000);
    int nfound = 0;
    if (checksumsi) to_free_checksums = checksums = strdup(checksumsi);
    char	*checksum = 0;
    while (*names_list) {
        if (checksums){
            checksum = checksums;
            while (*checksums && *checksums != '#') checksums ++;
            if (*checksums)				*(checksums++) = 0;
        }
        name = names_list;
        while (*names_list && *names_list != '#')	names_list++;
        if (*names_list) *(names_list++) = 0;
        /* set is_group */
        i = GBS_read_hash(psg.namehash, name);
        if (i){
            i--;
	    // IDP Checksumme
	    if (checksum && atol(checksum)!= psg.data[i].checksum){
                psg.data[i].is_group = -1;
                goto not_found;	// sequence has changed meanwhile !!
	    }
            psg.data[i].is_group = 1;
            locs->group_count++;
        }else{
        not_found:
            if (nfound>0){
                GBS_chrcat(not_found,'#');
            }
            nfound++;
            GBS_strcat(not_found,name);
        }
    }
    free(to_free_names_list);
    if (to_free_checksums) free(to_free_checksums);
    if (nfound){
        return GBS_strclose(not_found,1);
    }else{
        free(GBS_strclose(not_found,0));
    }
    return 0;
}


extern "C" bytestring *PT_unknown_names(struct_PT_pdc *pdc){
    static bytestring un = {0,0};
    PT_local *locs = (PT_local*)pdc->mh.parent->parent;
    delete un.data;
    un.data    = ptpd_read_names(locs,pdc->names.data,pdc->checksums.data);
    if (un.data) {
        un.size		= strlen(un.data) + 1;
    }else{
        un.data		= strdup("");
        un.size		= 1;
    }
    return &un;
}

/* compute clip max using the probe length */
int get_clip_max_from_length(int length)
{
    int             data_size;
    int             i;
    double          hitperc_zero_mismatches;
    double          half_mismatches;
    data_size = psg.data_count * psg.max_size;
    hitperc_zero_mismatches = (double)data_size;
    for (i = 0; i < length; i++) {
        hitperc_zero_mismatches *= .25;
    }
    for (half_mismatches = 0; half_mismatches < 100; half_mismatches++) {
        if (hitperc_zero_mismatches > 1.0 / (3.0 * length))
            break;
        hitperc_zero_mismatches *= (3.0 * length);
    }
    return (int) (half_mismatches);
}


void PT_init_base_string_counter(char *str,char initval,int size)
{
    memset(str,initval,size+1);
    str[size] = 0;
}

void PT_inc_base_string_count(char *str,char initval, char maxval, int size)
{
    register int i;
    if (!size) {
        str[0]=255;
        return;
    }
    for (i=size-1; i>=0; i--) {
        str[i]++;
        if (str[i] >= maxval) {
            str[i] = initval;
            if (!i) str[i]=255;	/* end flag */
        }else{
            break;
        }
    }
}
