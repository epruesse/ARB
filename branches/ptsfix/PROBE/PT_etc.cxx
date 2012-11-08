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
    PrefixProbabilities prob0(0);
    PrefixProbabilities prob1(1);
    PrefixProbabilities prob2(2);

    const double EPS = 0.00001;

    TEST_ASSERT_SIMILAR(prob0.of(0), 1.0000, EPS); // all

    TEST_ASSERT_SIMILAR(prob1.of(0), 0.0200, EPS); // PT_QU
    TEST_ASSERT_SIMILAR(prob1.of(1), 0.0200, EPS); // PT_N
    TEST_ASSERT_SIMILAR(prob1.of(2), 0.2400, EPS);
    TEST_ASSERT_SIMILAR(prob1.of(3), 0.2400, EPS);
    TEST_ASSERT_SIMILAR(prob1.of(4), 0.2400, EPS);
    TEST_ASSERT_SIMILAR(prob1.of(5), 0.2400, EPS);

    TEST_ASSERT_SIMILAR(prob2.of( 0), 0.0200, EPS); // PT_QU
    TEST_ASSERT_SIMILAR(prob2.of( 1), 0.0004, EPS); // PT_N PT_QU
    TEST_ASSERT_SIMILAR(prob2.of( 2), 0.0004, EPS); // PT_N PT_N
    TEST_ASSERT_SIMILAR(prob2.of( 3), 0.0048, EPS); // PT_N PT_A
    TEST_ASSERT_SIMILAR(prob2.of( 7), 0.0048, EPS); // PT_A PT_QU
    TEST_ASSERT_SIMILAR(prob2.of( 9), 0.0576, EPS); // PT_A PT_A
    TEST_ASSERT_SIMILAR(prob2.of(30), 0.0576, EPS); // PT_T PT_T

    TEST_ASSERT_SIMILAR(prob1.left_of(4), 0.5200, EPS);
    TEST_ASSERT_SIMILAR(prob1.left_of(6), 1.0000, EPS); // all prefixes together

    TEST_ASSERT_SIMILAR(prob2.left_of(19), 0.5200, EPS);
    TEST_ASSERT_SIMILAR(prob2.left_of(31), 1.0000, EPS); // all prefixes together

    TEST_ASSERT_EQUAL(prob0.find_index_near_leftsum(1.0), 1);

    TEST_ASSERT_EQUAL(prob1.find_index_near_leftsum(0.5), 4);
    TEST_ASSERT_SIMILAR(prob1.left_of(3), 0.2800, EPS);
    TEST_ASSERT_SIMILAR(prob1.left_of(4), 0.5200, EPS);

    TEST_ASSERT_EQUAL(prob2.find_index_near_leftsum(0.5), 19);
    TEST_ASSERT_SIMILAR(prob2.left_of(18), 0.4624, EPS);
    TEST_ASSERT_SIMILAR(prob2.left_of(19), 0.5200, EPS);
}

static int count_passes(Partition& p) {
    p.reset();
    int count = 0;
    while (!p.done()) {
        p.next();
        ++count;
    }
    p.reset();
    return count;
}

class Compressed : virtual Noncopyable {
    size_t  len;
    char   *data;
public:
    Compressed(const char *readable)
        : len(strlen(readable)+1),
          data(new char[len])
    {
        memcpy(data, readable, len);
        probe_compress_sequence(data, len);
    }
    ~Compressed() { delete [] data; }

    const char *seq() const { return data; }
};

void TEST_MarkedPrefixes() {
    MarkedPrefixes mp0(0);
    MarkedPrefixes mp1(1);
    MarkedPrefixes mp2(2);

    mp0.predecide();
    TEST_ASSERT_EQUAL(mp0.isMarked(Compressed(".").seq()), false);
    TEST_ASSERT_EQUAL(mp0.isMarked(Compressed("T").seq()), false);

    mp0.mark(0, 0);
    mp0.predecide();
    TEST_ASSERT_EQUAL(mp0.isMarked(Compressed(".").seq()), true);
    TEST_ASSERT_EQUAL(mp0.isMarked(Compressed("T").seq()), true);

    mp1.mark(3, 5);
    mp1.predecide();
    TEST_ASSERT_EQUAL(mp1.isMarked(Compressed(".").seq()), false);
    TEST_ASSERT_EQUAL(mp1.isMarked(Compressed("N").seq()), false);
    TEST_ASSERT_EQUAL(mp1.isMarked(Compressed("A").seq()), false);
    TEST_ASSERT_EQUAL(mp1.isMarked(Compressed("C").seq()), true);
    TEST_ASSERT_EQUAL(mp1.isMarked(Compressed("G").seq()), true);
    TEST_ASSERT_EQUAL(mp1.isMarked(Compressed("T").seq()), true);

    mp2.mark(1, 7);
    mp2.predecide();
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed(".").seq()),  false);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("N.").seq()), true);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("NN").seq()), true);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("NA").seq()), true);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("NC").seq()), true);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("NG").seq()), true);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("NT").seq()), true);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("A.").seq()), true);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("AN").seq()), false);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("AC").seq()), false);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("AG").seq()), false);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("AT").seq()), false);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("GG").seq()), false);
    TEST_ASSERT_EQUAL(mp2.isMarked(Compressed("TA").seq()), false);
}

void TEST_Partition() {
    PrefixProbabilities p0(0);
    PrefixProbabilities p1(1);
    PrefixProbabilities p2(2);
    PrefixProbabilities p3(3);
    PrefixProbabilities p4(4);

    const int BASES_100k = 100000;

    {
        Partition P01(p0, 1);
        TEST_ASSERT_EQUAL(P01.estimate_probes_for_pass(1, BASES_100k), 100000);
        TEST_ASSERT_EQUAL(P01.estimate_max_probes_for_any_pass(BASES_100k), 100000);
    }

    {
        // distributing memory to 6 passes on a level.1 Partitioner doesn't allow much choice:
        Partition P16(p1, 6);
        TEST_ASSERT_EQUAL(P16.number_of_passes(), 6);
        TEST_ASSERT_EQUAL(P16.estimate_probes_for_pass(1, BASES_100k), 2000);
        TEST_ASSERT_EQUAL(P16.estimate_probes_for_pass(2, BASES_100k), 2000);
        TEST_ASSERT_EQUAL(P16.estimate_probes_for_pass(3, BASES_100k), 24000);
        TEST_ASSERT_EQUAL(P16.estimate_probes_for_pass(4, BASES_100k), 24000);
        TEST_ASSERT_EQUAL(P16.estimate_probes_for_pass(5, BASES_100k), 24000);
        TEST_ASSERT_EQUAL(P16.estimate_probes_for_pass(6, BASES_100k), 24000);
        TEST_ASSERT_EQUAL(P16.estimate_max_probes_for_any_pass(BASES_100k), 24000);
        TEST_ASSERT_EQUAL(P16.estimate_max_kb_for_any_pass(BASES_100k), 1289);
    }

    {
        // 3 passes
        Partition P13(p1, 3);
        TEST_ASSERT_EQUAL(P13.number_of_passes(), 3);
        TEST_ASSERT_EQUAL(count_passes(P13), 3);

        TEST_ASSERT_EQUAL(P13.contains(Compressed(".").seq()), true);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("N").seq()), true);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("A").seq()), true);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("C").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("G").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("T").seq()), false);

        TEST_ASSERT_EQUAL(P13.next(), true);

        TEST_ASSERT_EQUAL(P13.contains(Compressed(".").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("N").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("A").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("C").seq()), true);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("G").seq()), true);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("T").seq()), false);

        TEST_ASSERT_EQUAL(P13.next(), true);

        TEST_ASSERT_EQUAL(P13.contains(Compressed(".").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("N").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("A").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("C").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("G").seq()), false);
        TEST_ASSERT_EQUAL(P13.contains(Compressed("T").seq()), true);

        TEST_ASSERT_EQUAL(P13.next(), false);

        TEST_ASSERT_EQUAL(P13.estimate_probes_for_pass(1, BASES_100k), 28000);
        TEST_ASSERT_EQUAL(P13.estimate_probes_for_pass(2, BASES_100k), 48000);
        TEST_ASSERT_EQUAL(P13.estimate_probes_for_pass(3, BASES_100k), 24000);
        TEST_ASSERT_EQUAL(P13.estimate_max_probes_for_any_pass(BASES_100k), 48000);
        TEST_ASSERT_EQUAL(P13.estimate_max_kb_for_any_pass(BASES_100k), 2578);
    }

    {
        // 2 passes
        Partition P12(p1, 2);
        TEST_ASSERT_EQUAL(P12.number_of_passes(), 2);
        TEST_ASSERT_EQUAL(count_passes(P12), 2);

        TEST_ASSERT_EQUAL(P12.contains(Compressed(".").seq()), true);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("N").seq()), true);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("A").seq()), true);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("C").seq()), true);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("G").seq()), false);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("T").seq()), false);

        TEST_ASSERT_EQUAL(P12.next(), true);

        TEST_ASSERT_EQUAL(P12.contains(Compressed(".").seq()), false);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("N").seq()), false);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("A").seq()), false);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("C").seq()), false);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("G").seq()), true);
        TEST_ASSERT_EQUAL(P12.contains(Compressed("T").seq()), true);

        TEST_ASSERT_EQUAL(P12.next(), false);

        TEST_ASSERT_EQUAL(P12.estimate_probes_for_pass(1, BASES_100k), 52000);
        TEST_ASSERT_EQUAL(P12.estimate_probes_for_pass(2, BASES_100k), 48000);
        TEST_ASSERT_EQUAL(P12.estimate_max_probes_for_any_pass(BASES_100k), 52000);
        TEST_ASSERT_EQUAL(P12.estimate_max_kb_for_any_pass(BASES_100k), 2792);
    }

    TEST_ASSERT_EQUAL(max_probes_for_passes(p1,   1, BASES_100k), 100000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p1,   2, BASES_100k), 52000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p1,   3, BASES_100k), 48000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p1,   4, BASES_100k), 28000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p1,   5, BASES_100k), 24000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p1,   6, BASES_100k), 24000);

    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   1, BASES_100k), 100000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   2, BASES_100k), 52000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   3, BASES_100k), 35520);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   4, BASES_100k), 29760);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   5, BASES_100k), 24000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   6, BASES_100k), 18240);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   7, BASES_100k), 18240);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   8, BASES_100k), 17280);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,   9, BASES_100k), 12480);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,  10, BASES_100k), 12480);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,  15, BASES_100k), 11520);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,  20, BASES_100k), 5760);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p2,  30, BASES_100k), 5760);

    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   1, BASES_100k), 100000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   2, BASES_100k), 50618);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   3, BASES_100k), 33907);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   4, BASES_100k), 25382);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   5, BASES_100k), 21005);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   6, BASES_100k), 17280);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   7, BASES_100k), 14899);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   8, BASES_100k), 12902);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,   9, BASES_100k), 11520);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,  10, BASES_100k), 11098);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,  15, BASES_100k), 7142);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,  20, BASES_100k), 5760);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,  30, BASES_100k), 4378);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,  40, BASES_100k), 2995);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3,  50, BASES_100k), 2995);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3, 100, BASES_100k), 2000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p3, 150, BASES_100k), 2000);

    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   1, BASES_100k), 100000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   2, BASES_100k), 50046);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   3, BASES_100k), 33474);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   4, BASES_100k), 25051);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   5, BASES_100k), 20286);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   6, BASES_100k), 16858);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   7, BASES_100k), 14515);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   8, BASES_100k), 12571);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,   9, BASES_100k), 11234);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,  10, BASES_100k), 10239);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,  15, BASES_100k), 6811);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,  20, BASES_100k), 5143);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,  30, BASES_100k), 3484);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,  40, BASES_100k), 2765);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4,  50, BASES_100k), 2101);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4, 100, BASES_100k), 2000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4, 150, BASES_100k), 2000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4, 200, BASES_100k), 2000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4, 300, BASES_100k), 2000);
    TEST_ASSERT_EQUAL(max_probes_for_passes(p4, 600, BASES_100k), 2000);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
