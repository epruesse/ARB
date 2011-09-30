// =============================================================== //
//                                                                 //
//   File      : PT_etc.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "probe.h"

#include <PT_server_prototypes.h>
#include "pt_prototypes.h"
#include <struct_man.h>
#include <arb_strbuf.h>

void set_table_for_PT_N_mis(int ignored_Nmismatches, int when_less_than_Nmismatches) {
    // calculate table for PT_N mismatches
    // 
    // 'ignored_Nmismatches' specifies, how many N-mismatches will be accepted as
    // matches, when overall number of N-mismatches is below 'when_less_than_Nmismatches'.
    //
    // above that limit, every N-mismatch counts as mismatch

    if ((when_less_than_Nmismatches-1)>PT_POS_TREE_HEIGHT) when_less_than_Nmismatches = PT_POS_TREE_HEIGHT+1;
    if (ignored_Nmismatches >= when_less_than_Nmismatches) ignored_Nmismatches = when_less_than_Nmismatches-1;

    psg.w_N_mismatches[0] = 0;
    int mm;
    for (mm = 1; mm<when_less_than_Nmismatches; ++mm) {
        psg.w_N_mismatches[mm] = mm>ignored_Nmismatches ? mm-ignored_Nmismatches : 0;
    }
    pt_assert(mm <= (PT_POS_TREE_HEIGHT+1));
    for (; mm <= PT_POS_TREE_HEIGHT; ++mm) {
        psg.w_N_mismatches[mm] = mm;
    }
}

void pt_export_error(PT_local *locs, const char *error) {
    freedup(locs->ls_error, error);
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

const char *virt_name(PT_probematch *ml)
{
    // get the name with a virtual function
    if (gene_flag) {
        const gene_struct *gs = get_gene_struct_by_internal_gene_name(psg.data[ml->name].name);
        return gs ? gs->get_arb_species_name() : "<cantResolveName>";
    }
    else {
        pt_assert(psg.data[ml->name].name);
        return psg.data[ml->name].name;
    }
}

const char *virt_fullname(PT_probematch * ml)
{
    if (gene_flag) {
        const gene_struct *gs = get_gene_struct_by_internal_gene_name(psg.data[ml->name].name);
        return gs ? gs->get_arb_gene_name() : "<cantResolveGeneFullname>";
    }
    else {
        return psg.data[ml->name].fullname ?  psg.data[ml->name].fullname : "<undefinedFullname>";
    }
}

int *table_copy(int *mis_table, int length)
{
    // copy one mismatch table to a new one allocating memory
    int *result_table;
    int i;

    result_table = (int *)calloc(length, sizeof(int));
    for (i=0; i<length; i++)
        result_table[i] = mis_table[i];
    return result_table;
}
void table_add(int *mis_tabled, int *mis_tables, int length)
{
    // add the values of a source table to a destination table
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
        pt_assert(list[num_offset] == '#');
    }
    else { // last list part
        num_offset = offset+strlen(list+offset);
        pt_assert(list[num_offset] == 0);
    }

    // now num_offset points to next '#' or to end-of-string

    int len = num_offset-offset;
    pt_assert(len <= MAX_LIST_PART_SIZE);

    memcpy(buffer[curr_buff], list+offset, len);
    buffer[curr_buff][len] = 0; // EOS

    offset = (list[num_offset] == '#') ? num_offset+1 : -1; // set offset for next part

    return buffer[curr_buff];
}

#undef MAX_LIST_PART_SIZE

char *ptpd_read_names(PT_local *locs, const char *names_list, const char *checksums, const char*& error) {
    /* read the name list separated by '#' and set the flag for the group members,
     + returns a list of names which have not been found
     */

    // clear 'is_group'
    for (int i = 0; i < psg.data_count; i++) {
        psg.data[i].is_group = 0; // Note: probes are designed for species with is_group == 1
    }
    locs->group_count = 0;
    error             = 0;

    if (!names_list) {
        error = "Can't design probes for no species (species list is empty)";
        return 0;
    }

    int noff = 0;
    int coff = 0;

    GBS_strstruct *not_found = 0;

    while (noff >= 0) {
        pt_assert(coff >= 0);   // otherwise 'checksums' contains less elements than 'names_list'
        const char *arb_name      = get_list_part(names_list, noff);
        const char *internal_name = arb_name; // differs only for gene pt server

        if (arb_name[0] == 0) {
            pt_assert(names_list[0] == 0);
            break; // nothing marked
        }

        if (gene_flag) {
            const char *slash = strchr(arb_name, '/');

            if (!slash) {
                // ARB has to send 'species/gene'.
                // If it did not, user did not mark 'Gene probes ?' flag

                error = GBS_global_string("Expected '/' in '%s' (this PT-server can only design probes for genes)", arb_name);
                break;
            }

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

    char *result = not_found ? GBS_strclose(not_found) : 0;
    if (error) freenull(result);
    return result;
}

bytestring *PT_unknown_names(PT_pdc *pdc) {
    static bytestring un = { 0, 0 };
    PT_local *locs = (PT_local*)pdc->mh.parent->parent;
    delete un.data;

    const char *error;
    un.data = ptpd_read_names(locs, pdc->names.data, pdc->checksums.data, error);
    if (un.data) {
        un.size = strlen(un.data) + 1;
        pt_assert(!error);
    }
    else {
        un.data = strdup("");
        un.size = 1;
        if (error) pt_export_error(locs, error);
    }
    return &un;
}

int get_clip_max_from_length(int length) {
    // compute clip max using the probe length

    int    data_size;
    int    i;
    double hitperc_zero_mismatches;
    double half_mismatches;
    data_size               = psg.data_count * psg.max_size;
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


void PT_init_base_string_counter(char *str, char initval, int size)
{
    memset(str, initval, size+1);
    str[size] = 0;
}

void PT_inc_base_string_count(char *str, char initval, char maxval, int size)
{
    int i;
    if (!size) {
        str[0]=255;
        return;
    }
    for (i=size-1; i>=0; i--) {
        str[i]++;
        if (str[i] >= maxval) {
            str[i] = initval;
            if (!i) str[i]=255; // end flag
        }
        else {
            break;
        }
    }
}
