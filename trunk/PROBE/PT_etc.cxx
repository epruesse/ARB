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

static const gene_struct *get_gene_struct_by_internal_gene_name(const char *gene_name) {
    gene_struct to_search(gene_name, "", "");
    
    gene_struct_index_internal::const_iterator found = gene_struct_internal2arb.find(&to_search);
    return (found == gene_struct_internal2arb.end()) ? 0 : *found;
}
static const gene_struct *get_gene_struct_by_arb_species_gene_name(const char *species_gene_name) {
    const char *slash = strchr(species_gene_name, '/');
    if (!slash) {
        fprintf(stderr, "Internal error: '%s' should be in format 'organism/gene'\n", species_gene_name);
        return 0;
    }

    int   slashpos     = slash-species_gene_name;
    char *organism     = strdup(species_gene_name);
    organism[slashpos] = 0;

    gene_struct to_search("", organism, species_gene_name+slashpos+1);
    free(organism);

    gene_struct_index_arb::const_iterator found = gene_struct_arb2internal.find(&to_search);
    return (found == gene_struct_arb2internal.end()) ? 0 : *found;
}

static const char *arb2internal_name(const char *name) {
    // convert arb name ('species/gene') into internal shortname
    const gene_struct *found = get_gene_struct_by_arb_species_gene_name(name);
    return found ? found->get_internal_gene_name() : 0;
}

/* get the name with a virtual function */
extern "C" const char *virt_name(PT_probematch *ml)
{
    if (gene_flag) {
        const gene_struct *gs = get_gene_struct_by_internal_gene_name(psg.data[ml->name].name);
        return gs ? gs->get_arb_species_name() : "<cantResolveName>";
    }
    else {
        pt_assert(psg.data[ml->name].name);
        return psg.data[ml->name].name;
    }
}

extern "C" const char *virt_fullname(PT_probematch * ml)
{
    if (gene_flag) {
        const gene_struct *gs = get_gene_struct_by_internal_gene_name(psg.data[ml->name].name);
        return gs ? gs->get_arb_gene_name() : "<cantResolveGeneFullname>";
    }
    else {
        return psg.data[ml->name].fullname ?  psg.data[ml->name].fullname : "<undefinedFullname>";
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

#define MAX_LIST_PART_SIZE 50

static const char *get_list_part(const char *list, int& offset) {
    // scans strings with format "xxxx#yyyy#zzzz"
    // your first call should be with offset == 0
    //
    // returns : static copy of each part or 0 when done
    // offset is incremented by this function and set to -1 when all parts were returned

    static char buffer[2][MAX_LIST_PART_SIZE+1];
    static int  curr_buff = 0;  // toggle buffer to allow 2 parallel gets w/o invalidation

    if (offset<0) return 0;     // already done
    curr_buff ^= 1; // toggle buffer

    const char *numsign = strchr(list+offset, '#');
    int         num_offset;
    if (numsign) {
        num_offset = numsign-list;
    }
    else { // last list part
        num_offset = offset+strlen(list+offset);
    }
    pt_assert(list[num_offset] == '#' || list[num_offset] == 0);

    int len = num_offset-offset;
    pt_assert(len <= MAX_LIST_PART_SIZE);

    memcpy(buffer[curr_buff], list+offset, len);
    buffer[curr_buff][len] = 0; // EOS

    offset = list[num_offset] ? num_offset+1 : -1; // set offset for next part

    return buffer[curr_buff];
}

#undef MAX_LIST_PART_SIZE

/* read the name list seperated by # and set the flag for the group members,
   + returns a list of names which have not been found */

char *ptpd_read_names(PT_local *locs, const char *names_list, const char *checksums) {
    /* clear 'is_group' */
    for (int i = 0; i < psg.data_count; i++) {
        psg.data[i].is_group = 0; // Note: probes are designed for species with is_group == 1
    }
    locs->group_count = 0;

    if (!names_list) {
        printf("Can't design probes for no species (species list is empty)\n");
        return 0;
    }

    int   noff      = 0;
    int   coff      = 0;
    void *not_found = 0;

    while (noff >= 0) {
        pt_assert(coff >= 0);   // otherwise 'checksums' contains less elements than 'names_list'
        const char *arb_name      = get_list_part(names_list, noff);
        const char *internal_name = arb_name; // differs only for gene pt server

        if (gene_flag) {
            const char *slash = strchr(arb_name, '/');

            pt_assert(slash); // ARB has to send 'species/gene'

            internal_name = arb2internal_name(arb_name);
            pt_assert(internal_name);
        }

        int  idx   = GBS_read_hash(psg.namehash, internal_name);
        bool found = false;

        if (idx) {
            --idx;              // because 0 means not found!

            if (checksums) {
                const char *checksum = get_list_part(checksums, coff);
                // if sequence checksum changed since pt server was updated -> not found
                found                = atol(checksum) == psg.data[idx].checksum;
            }
            else {
                found = true;
            }

            if (found) {
                psg.data[idx].is_group = 1; // mark
                locs->group_count++;
            }
        }

        if (!found) { // name not found -> put into result
            if (not_found == 0) not_found = GBS_stropen(1000);
            else GBS_chrcat(not_found, '#');
            GBS_strcat(not_found, arb_name);
        }
    }

    if (not_found) return GBS_strclose(not_found);
    return 0;
}

char *ptpd_read_names_old(PT_local * locs, char *names_listi, char *checksumsi)
{
    char *names_list;
    char *checksums = 0;
    long  i;
    char *name;

//     if (gene_flag) {
//         char *temp;
//         int   j = 0;
//         int   k = 0;
//         char *listi_copy;
//         char *temp_listi;
//         char bit;
//         list<gene_struct*>::iterator gene_iterator;
//         gene_iterator = names_list_idp.begin();
//         bit           = names_listi[k];
//         while (bit != '\0') {
//             if (bit=='#') j++;
//             k++;
//             bit = names_listi[k];
//         }
//         j++;
//         temp_listi = new char[j*9+2];
//         listi_copy = new char[strlen(names_listi)+1];
//         strcpy(temp_listi,"");
//         strcpy(listi_copy,names_listi);
//         temp = strtok(listi_copy,"#");
//         while (temp) {
//             while (strcmp((*gene_iterator)->arb_gene_name,temp) && gene_iterator != names_list_idp.end()) {
//                 gene_iterator++;
//             }

//             if (gene_iterator == names_list_idp.end()) {
//                 printf("Error in Name Resolution\n");
//                 exit(1);
//             }
//             else {
//                 strcat(temp_listi,(*gene_iterator)->gene_name);
//                 strcat(temp_listi,"#");
//             }
//             temp = strtok(NULL,"#");
//         }
//         //  temp_listi[strlen(temp_listi)] = '\0';
//         names_listi = temp_listi;
//     }

    char *to_free_names_list = names_list = strdup(names_listi);
    char *to_free_checksums  = 0;

    /* read single names */
    void *not_found = GBS_stropen(1000);
    int   nfound    = 0;

    if (checksumsi) to_free_checksums = checksums = strdup(checksumsi);

    char *checksum = 0;
    while (*names_list) {
        if (checksums){
            checksum = checksums;
            while (*checksums && *checksums != '#') checksums ++;
            if (*checksums) *(checksums++) = 0;
        }
        name = names_list;
        while (*names_list && *names_list != '#') names_list++;
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
        return GBS_strclose(not_found);
    }else{
        free(GBS_strclose(not_found));
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
