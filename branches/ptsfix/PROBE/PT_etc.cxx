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
#include "PT_partition.h"
#include <struct_man.h>
#include <arb_strbuf.h>

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

const char *virt_name(const PT_probematch *ml) 
{
    // get the name with a virtual function
    if (gene_flag) {
        const gene_struct *gs = get_gene_struct_by_internal_gene_name(psg.data[ml->name].get_name());
        return gs ? gs->get_arb_species_name() : "<cantResolveName>";
    }
    else {
        pt_assert(psg.data[ml->name].get_name());
        return psg.data[ml->name].get_name();
    }
}

const char *virt_fullname(const PT_probematch * ml) 
{
    if (gene_flag) {
        const gene_struct *gs = get_gene_struct_by_internal_gene_name(psg.data[ml->name].get_name());
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

char *ptpd_read_names(PT_local *locs, const char *names_list, const char *checksums, const char*& error) {
    /* read the name list separated by '#' and set the flag for the group members,
     + returns a list of names which have not been found
     */

    // clear 'is_group'
    for (int i = 0; i < psg.data_count; i++) {
        psg.data[i].set_group_state(0); // Note: probes are designed for species with is_group == 1
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

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

inline const char *concat_iteration(PrefixIterator& prefix) {
    static GBS_strstruct out(50);

    out.erase();

    while (!prefix.done()) {
        if (out.filled()) out.put(',');

        char *readable = probe_2_readable(prefix.copy(), prefix.length());
        out.cat(readable);
        free(readable);

        ++prefix;
    }

    return out.get_data();
}

void TEST_PrefixIterator() {
    // straight-forward permutation
    PrefixIterator p0(PT_A, PT_T, 0); TEST_ASSERT_EQUAL(p0.steps(), 1);
    PrefixIterator p1(PT_A, PT_T, 1); TEST_ASSERT_EQUAL(p1.steps(), 4);
    PrefixIterator p2(PT_A, PT_T, 2); TEST_ASSERT_EQUAL(p2.steps(), 16);
    PrefixIterator p3(PT_A, PT_T, 3); TEST_ASSERT_EQUAL(p3.steps(), 64);

    TEST_ASSERT_EQUAL(p1.done(), false);
    TEST_ASSERT_EQUAL(p0.done(), false);

    TEST_ASSERT_EQUAL(concat_iteration(p0), "");
    TEST_ASSERT_EQUAL(concat_iteration(p1), "A,C,G,U");
    TEST_ASSERT_EQUAL(concat_iteration(p2), "AA,AC,AG,AU,CA,CC,CG,CU,GA,GC,GG,GU,UA,UC,UG,UU");

    // permutation truncated at PT_QU
    PrefixIterator q0(PT_QU, PT_T, 0); TEST_ASSERT_EQUAL(q0.steps(), 1);
    PrefixIterator q1(PT_QU, PT_T, 1); TEST_ASSERT_EQUAL(q1.steps(), 6);
    PrefixIterator q2(PT_QU, PT_T, 2); TEST_ASSERT_EQUAL(q2.steps(), 31);
    PrefixIterator q3(PT_QU, PT_T, 3); TEST_ASSERT_EQUAL(q3.steps(), 156);
    PrefixIterator q4(PT_QU, PT_T, 4); TEST_ASSERT_EQUAL(q4.steps(), 781);

    TEST_ASSERT_EQUAL(concat_iteration(q0), "");
    TEST_ASSERT_EQUAL(concat_iteration(q1), ".,N,A,C,G,U");
    TEST_ASSERT_EQUAL(concat_iteration(q2),
                      ".,"
                      "N.,NN,NA,NC,NG,NU,"
                      "A.,AN,AA,AC,AG,AU,"
                      "C.,CN,CA,CC,CG,CU,"
                      "G.,GN,GA,GC,GG,GU,"
                      "U.,UN,UA,UC,UG,UU");
    TEST_ASSERT_EQUAL(concat_iteration(q3),
                      ".,"
                      "N.,"
                      "NN.,NNN,NNA,NNC,NNG,NNU,"
                      "NA.,NAN,NAA,NAC,NAG,NAU,"
                      "NC.,NCN,NCA,NCC,NCG,NCU,"
                      "NG.,NGN,NGA,NGC,NGG,NGU,"
                      "NU.,NUN,NUA,NUC,NUG,NUU,"
                      "A.,"
                      "AN.,ANN,ANA,ANC,ANG,ANU,"
                      "AA.,AAN,AAA,AAC,AAG,AAU,"
                      "AC.,ACN,ACA,ACC,ACG,ACU,"
                      "AG.,AGN,AGA,AGC,AGG,AGU,"
                      "AU.,AUN,AUA,AUC,AUG,AUU,"
                      "C.,"
                      "CN.,CNN,CNA,CNC,CNG,CNU,"
                      "CA.,CAN,CAA,CAC,CAG,CAU,"
                      "CC.,CCN,CCA,CCC,CCG,CCU,"
                      "CG.,CGN,CGA,CGC,CGG,CGU,"
                      "CU.,CUN,CUA,CUC,CUG,CUU,"
                      "G.,"
                      "GN.,GNN,GNA,GNC,GNG,GNU,"
                      "GA.,GAN,GAA,GAC,GAG,GAU,"
                      "GC.,GCN,GCA,GCC,GCG,GCU,"
                      "GG.,GGN,GGA,GGC,GGG,GGU,"
                      "GU.,GUN,GUA,GUC,GUG,GUU,"
                      "U.,"
                      "UN.,UNN,UNA,UNC,UNG,UNU,"
                      "UA.,UAN,UAA,UAC,UAG,UAU,"
                      "UC.,UCN,UCA,UCC,UCG,UCU,"
                      "UG.,UGN,UGA,UGC,UGG,UGU,"
                      "UU.,UUN,UUA,UUC,UUG,UUU");
}

void TEST_PrefixProbabilities() {
    PrefixProbabilities prob0(PrefixIterator(PT_QU, PT_T, 0));
    PrefixProbabilities prob1(PrefixIterator(PT_QU, PT_T, 1));
    PrefixProbabilities prob2(PrefixIterator(PT_QU, PT_T, 2));

    const double EPS = 0.00001;

    TEST_ASSERT_SIMILAR(prob0.get(0), 1.0000, EPS); // all

    TEST_ASSERT_SIMILAR(prob1.get(0), 0.0200, EPS); // PT_QU
    TEST_ASSERT_SIMILAR(prob1.get(1), 0.0200, EPS); // PT_N
    TEST_ASSERT_SIMILAR(prob1.get(2), 0.2400, EPS);
    TEST_ASSERT_SIMILAR(prob1.get(3), 0.2400, EPS);
    TEST_ASSERT_SIMILAR(prob1.get(4), 0.2400, EPS);
    TEST_ASSERT_SIMILAR(prob1.get(5), 0.2400, EPS);

    TEST_ASSERT_SIMILAR(prob2.get( 0), 0.0200, EPS); // PT_QU
    TEST_ASSERT_SIMILAR(prob2.get( 1), 0.0004, EPS); // PT_N PT_QU
    TEST_ASSERT_SIMILAR(prob2.get( 2), 0.0004, EPS); // PT_N PT_N
    TEST_ASSERT_SIMILAR(prob2.get( 3), 0.0048, EPS); // PT_N PT_A
    TEST_ASSERT_SIMILAR(prob2.get( 7), 0.0048, EPS); // PT_A PT_QU
    TEST_ASSERT_SIMILAR(prob2.get( 9), 0.0576, EPS); // PT_A PT_A
    TEST_ASSERT_SIMILAR(prob2.get(30), 0.0576, EPS); // PT_T PT_T

    TEST_ASSERT_SIMILAR(prob1.sum_left_of(4), 0.5200, EPS);
    TEST_ASSERT_SIMILAR(prob1.sum_left_of(6), 1.0000, EPS); // all prefixes together

    TEST_ASSERT_SIMILAR(prob2.sum_left_of(19), 0.5200, EPS);
    TEST_ASSERT_SIMILAR(prob2.sum_left_of(31), 1.0000, EPS); // all prefixes together

    TEST_ASSERT_EQUAL(prob0.find_index_near_leftsum(1.0), 1);

    TEST_ASSERT_EQUAL(prob1.find_index_near_leftsum(0.5), 4);
    TEST_ASSERT_SIMILAR(prob1.sum_left_of(3), 0.2800, EPS);
    TEST_ASSERT_SIMILAR(prob1.sum_left_of(4), 0.5200, EPS);

    TEST_ASSERT_EQUAL(prob2.find_index_near_leftsum(0.5), 19);
    TEST_ASSERT_SIMILAR(prob2.sum_left_of(18), 0.4624, EPS);
    TEST_ASSERT_SIMILAR(prob2.sum_left_of(19), 0.5200, EPS);
}

void TEST_Partitioner() {
    Partitioner p0(0); TEST_ASSERT_EQUAL(p0.steps(), 1);
    Partitioner p1(1); TEST_ASSERT_EQUAL(p1.steps(), 6);
    Partitioner p2(2); TEST_ASSERT_EQUAL(p2.steps(), 31);
    Partitioner p3(3); TEST_ASSERT_EQUAL(p3.steps(), 156);
    Partitioner p4(4); TEST_ASSERT_EQUAL(p4.steps(), 781);

    TEST_ASSERT_EQUAL(p0.select_passes(0), false);
    TEST_ASSERT_EQUAL(p0.select_passes(2), false);
    TEST_ASSERT_EQUAL(p0.select_passes(1), true);

    TEST_ASSERT_EQUAL(p1.select_passes(0), false);
    TEST_ASSERT_EQUAL(p1.select_passes(7), false);
    TEST_ASSERT_EQUAL(p1.select_passes(1), true);
    TEST_ASSERT_EQUAL(p1.select_passes(6), true);

    const int BASES_100k = 100000;

    TEST_ASSERT_EQUAL(p0.estimate_kb_for_pass(1, BASES_100k), 5371);

    // distributing memory to 6 passes on a level.1 Partitioner doesn't allow much choice:
    TEST_ASSERT_EQUAL(p1.select_passes(6), true);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(1, BASES_100k), 107);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(2, BASES_100k), 107);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(3, BASES_100k), 1289);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(4, BASES_100k), 1289);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(5, BASES_100k), 1289);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(6, BASES_100k), 1289);

    TEST_ASSERT_EQUAL(p1.select_passes(3), true);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(1, BASES_100k), 1503);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(2, BASES_100k), 2578);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(3, BASES_100k), 1289);

    TEST_ASSERT_EQUAL(p1.select_passes(2), true);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(1, BASES_100k), 2792);
    TEST_ASSERT_EQUAL(p1.estimate_kb_for_pass(2, BASES_100k), 2578);

    TEST_ASSERT_EQUAL(p1.max_kb_for_passes(1, BASES_100k), 5371);
    TEST_ASSERT_EQUAL(p1.max_kb_for_passes(2, BASES_100k), 2792);
    TEST_ASSERT_EQUAL(p1.max_kb_for_passes(3, BASES_100k), 2578);
    TEST_ASSERT_EQUAL(p1.max_kb_for_passes(4, BASES_100k), 1503);
    TEST_ASSERT_EQUAL(p1.max_kb_for_passes(5, BASES_100k), 1289);
    TEST_ASSERT_EQUAL(p1.max_kb_for_passes(6, BASES_100k), 1289);

    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 1, BASES_100k), 5371);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 2, BASES_100k), 2792);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 3, BASES_100k), 1907);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 4, BASES_100k), 1598);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 5, BASES_100k), 1289);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 6, BASES_100k), 979);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 7, BASES_100k), 979);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 8, BASES_100k), 928);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes( 9, BASES_100k), 670);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes(10, BASES_100k), 670);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes(15, BASES_100k), 618);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes(20, BASES_100k), 309);
    TEST_ASSERT_EQUAL(p2.max_kb_for_passes(30, BASES_100k), 309);

    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  1, BASES_100k), 5371);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  2, BASES_100k), 2718);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  3, BASES_100k), 1821);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  4, BASES_100k), 1363);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  5, BASES_100k), 1128);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  6, BASES_100k), 928);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  7, BASES_100k), 800);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  8, BASES_100k), 692);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(  9, BASES_100k), 618);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes( 10, BASES_100k), 596);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes( 15, BASES_100k), 383);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes( 20, BASES_100k), 309);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes( 30, BASES_100k), 235);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes( 40, BASES_100k), 160);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes( 50, BASES_100k), 160);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(100, BASES_100k), 107);
    TEST_ASSERT_EQUAL(p3.max_kb_for_passes(150, BASES_100k), 107);

    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  1, BASES_100k), 5371);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  2, BASES_100k), 2688);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  3, BASES_100k), 1797);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  4, BASES_100k), 1345);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  5, BASES_100k), 1089);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  6, BASES_100k), 905);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  7, BASES_100k), 779);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  8, BASES_100k), 675);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(  9, BASES_100k), 603);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes( 10, BASES_100k), 549);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes( 15, BASES_100k), 365);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes( 20, BASES_100k), 276);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes( 30, BASES_100k), 187);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes( 40, BASES_100k), 148);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes( 50, BASES_100k), 112);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(100, BASES_100k), 107);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(150, BASES_100k), 107);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(200, BASES_100k), 107);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(300, BASES_100k), 107);
    TEST_ASSERT_EQUAL(p4.max_kb_for_passes(600, BASES_100k), 107);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
