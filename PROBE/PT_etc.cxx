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

void pt_export_error(PT_local *locs, const char *error) {
    freedup(locs->ls_error, error);
}
void pt_export_error_if(PT_local *locs, ARB_ERROR& error) {
    if (error) pt_export_error(locs, error.deliver());
    else error.expect_no_error();
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

const char *virt_name(const PT_probematch *ml) 
{
    // get the name with a virtual function
    if (gene_flag) {
        const gene_struct *gs = get_gene_struct_by_internal_gene_name(psg.data[ml->name].get_shortname());
        return gs ? gs->get_arb_species_name() : "<cantResolveName>";
    }
    else {
        pt_assert(psg.data[ml->name].get_shortname());
        return psg.data[ml->name].get_shortname();
    }
}

const char *virt_fullname(const PT_probematch * ml) 
{
    if (gene_flag) {
        const gene_struct *gs = get_gene_struct_by_internal_gene_name(psg.data[ml->name].get_shortname());
        return gs ? gs->get_arb_gene_name() : "<cantResolveGeneFullname>";
    }
    else {
        return psg.data[ml->name].get_fullname() ?  psg.data[ml->name].get_fullname() : "<undefinedFullname>";
    }
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

char *ptpd_read_names(PT_local *locs, const char *names_list, const char *checksums, ARB_ERROR& error) {
    /* read the name list separated by '#' and set the flag for the group members,
     + returns a list of names which have not been found
     */

    // clear 'is_group'
    for (int i = 0; i < psg.data_count; i++) {
        psg.data[i].set_group_state(0); // Note: probes are designed for species with is_group == 1
    }
    locs->group_count = 0;

    error = 0;

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
                found                = atol(checksum) == psg.data[idx].get_checksum();
            }
            else {
                found = true;
            }

            if (found) {
                psg.data[idx].set_group_state(1); // mark
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

bytestring *PT_unknown_names(const PT_pdc *pdc) {
    PT_local *locs = (PT_local*)pdc->mh.parent->parent;
    static bytestring unknown = { 0, 0 };
    delete unknown.data;

    ARB_ERROR error;
    unknown.data = ptpd_read_names(locs, pdc->names.data, pdc->checksums.data, error);
    if (unknown.data) {
        unknown.size = strlen(unknown.data) + 1;
        pt_assert(!error);
    }
    else {
        unknown.data = strdup("");
        unknown.size = 1;
    }
    pt_export_error_if(locs, error);
    return &unknown;
}

