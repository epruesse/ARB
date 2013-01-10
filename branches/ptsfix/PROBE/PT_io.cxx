// =============================================================== //
//                                                                 //
//   File      : PT_io.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#include "probe.h"
#include "pt_prototypes.h"

#include <arbdbt.h>
#include <BI_basepos.hxx>
#include <arb_progress.h>
#include <arb_file.h>
#include <cache.h>

int compress_data(char *probestring) {
    //! change a sequence with normal bases the PT_? format and delete all other signs
    char    c;
    char    *src,
        *dest;
    dest = src = probestring;

    while ((c=*(src++)))
    {
        switch (c) {
            case 'A':
            case 'a': *(dest++) = PT_A; break;
            case 'C':
            case 'c': *(dest++) = PT_C; break;
            case 'G':
            case 'g': *(dest++) = PT_G; break;
            case 'U':
            case 'u':
            case 'T':
            case 't': *(dest++) = PT_T; break;
            case 'N':
            case 'n': *(dest++) = PT_N; break;
            default: break;
        }

    }
    *dest = PT_QU;
    return 0;
}

ARB_ERROR probe_read_data_base(const char *name, bool readOnly) { // goes to header: __ATTR__USERESULT
    ARB_ERROR error;
    GB_set_verbose();

    psg.gb_shell = new GB_shell;

    if (!readOnly && !GB_is_writeablefile(name)) {
        error = GBS_global_string("Database '%s' is write-protected - aborting", name);
    }
    if (!error) {
        GBDATA *gb_main     = GB_open(name, readOnly ? "r" : "rw");
        if (!gb_main) error = GB_await_error();
        else {
            error = GB_begin_transaction(gb_main);
            if (!error) {
                psg.gb_main         = gb_main;
                psg.gb_species_data = GBT_get_species_data(gb_main);
                psg.gb_sai_data     = GBT_get_SAI_data(gb_main);
            }
            error = GB_end_transaction(gb_main, error);
        }
    }
    return error;
}

inline size_t count_uint_32(uint32_t *seq, size_t seqsize, uint32_t cmp) {
    size_t count = 0;
    while (count<seqsize && seq[count] == cmp) count++;
    return count*4;
}

inline size_t count_char(const char *seq, size_t seqsize, char c, uint32_t c4) {
    if (seq[0] == c) {
        size_t count = 1+count_uint_32((uint32_t*)(seq+1), (seqsize-1)/4, c4);
        for (; count<seqsize && seq[count] == c; ++count) ;
        return count;
    }
    return 0;
}

inline size_t count_dots(const char *seq, int seqsize) { return count_char(seq, seqsize, '.', 0x2E2E2E2E); }
inline size_t count_gaps(const char *seq, int seqsize) { return count_char(seq, seqsize, '-', 0x2D2D2D2D); }

inline size_t count_gaps_and_dots(const char *seq, int seqsize) {
    size_t count = 0;
    size_t count2;
    size_t count3;

    do {
        count2  = count_dots(seq+count, seqsize-count);
        count  += count2;
        count3  = count_gaps(seq+count, seqsize-count);
        count  += count3;
    }
    while (count2 || count3);
    return count;
}

// uncomment next line to count all bases compressed by ptserver and dump them when program terminates
// #define COUNT_COMPRESSES_BASES
#if defined(COUNT_COMPRESSES_BASES)
class BaseCounter {
    long count[PT_BASES];
public:
    BaseCounter() {
        for (int i = 0; i<PT_BASES; ++i) {
            count[i] = 0;
        }
    }
    ~BaseCounter() {
        fflush_all();
        fputs("\nBaseCounter:\n", stderr);
        for (int i = 0; i<PT_BASES; ++i) {
            fprintf(stderr, "count[%i]=%li\n", i, count[i]);
        }
    }

    void inc(uchar base) {
        pt_assert(base >= 0 && base<PT_BASES);
        ++count[base];
    }
};

static BaseCounter base_counter;
#endif

int probe_compress_sequence(char *seq, int seqsize) {
    // translates a readable sequence into PT_base
    // (see also: probe_2_readable)
    static SmartMallocPtr(uchar) smart_tab;
    uchar *tab = NULL;
    if (smart_tab.isNull()) {
        tab = (uchar *) malloc(256);
        memset(tab, PT_N, 256);
        tab['A'] = tab['a'] = PT_A;
        tab['C'] = tab['c'] = PT_C;
        tab['G'] = tab['g'] = PT_G;
        tab['T'] = tab['t'] = PT_T;
        tab['U'] = tab['u'] = PT_T;
        tab['.'] = PT_QU;
        tab[0] = PT_B_UNDEF;
        smart_tab = tab;
    }

    tab = &*smart_tab;
    char *dest = seq;
    size_t offset = 0;

    while (seq[offset]) {
        offset += count_gaps(seq + offset, seqsize - offset); // skip over gaps

        uchar c = tab[safeCharIndex(seq[offset++])];
        if (c == PT_B_UNDEF)
            break; // already seen terminal zerobyte

#if defined(COUNT_COMPRESSES_BASES)
        base_counter.inc(c);
#endif
        *dest++ = c;
        if (c == PT_QU) { // TODO: *seq = '.' ???
            offset += count_gaps_and_dots(seq + offset, seqsize - offset); // skip over gaps and dots
            // dest[-1] = PT_N; // @@@ uncomment this to handle '.' like 'N' (experimental!!!)
        }
    }

    if (dest[-1] != PT_QU) {
#if defined(COUNT_COMPRESSES_BASES)
        base_counter.inc(PT_QU);
#endif
        *dest++ = PT_QU;
    }

#ifdef ARB_64
    pt_assert(!((dest - seq) & 0xffffffff00000000)); // must fit into 32 bit
#endif

    return dest - seq;
}

char *readable_probe(const char *compressed_probe, size_t len, char T_or_U) {
    static SmartMallocPtr(uchar) smart_tab;
    uchar *tab = NULL;

    if (smart_tab.isNull()) {
        tab = (uchar *) malloc(256);
        memset(tab, '?', 256);

        tab[PT_A]  = 'A';
        tab[PT_C]  = 'C';
        tab[PT_G]  = 'G';
        tab[PT_QU] = '.';
        tab[PT_N]  = 'N';

        tab[PT_B_UNDEF] = '!';

        smart_tab = tab;
    }

    tab = &*smart_tab;
    tab[PT_T] = T_or_U;
    
    char *result = (char*)malloc(len+1);
    for (size_t i = 0; i<len; ++i) {
        result[i] = tab[safeCharIndex(compressed_probe[i])];
    }
    result[len] = 0;
    return result;
}

static char *probe_read_string_append_point(GBDATA *gb_data, int *psize) {
    long  len  = GB_read_string_count(gb_data);
    char *data = GB_read_string(gb_data);

    if (data) {
        if (data[len - 1] != '.') {
            char *buffer = (char *) malloc(len + 2);
            strcpy(buffer, data);
            buffer[len++] = '.';
            buffer[len] = 0;
            freeset(data, buffer);
        }
        *psize = len;
    }
    else {
        *psize = 0;
    }
    return data;
}

char *probe_input_data::read_alignment(int *psize) const {
    char           *buffer     = 0;
    GBDATA         *gb_species = get_gbdata();
    GB_transaction  ta(gb_species);
    GBDATA         *gb_ali     = GB_entry(gb_species, psg.alignment_name);

    if (gb_ali) {
        GBDATA *gb_data = GB_entry(gb_ali, "data");
        if (gb_data) buffer = probe_read_string_append_point(gb_data, psize);
    }
    return buffer;
}

char *probe_read_alignment(int j, int *psize) { 
    return psg.data[j].read_alignment(psize);
}

GB_ERROR probe_input_data::init(GBDATA *gb_species) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_ali  = GB_entry(gb_species, psg.alignment_name);
    GBDATA   *gb_data = gb_ali ? GB_entry(gb_ali, "data") : NULL;

    if (!gb_data) {
        error = GBS_global_string("Species '%s' has no data in '%s'", GBT_read_name(gb_species), psg.alignment_name);
    }
    else {
        int   hsize;
        char *sdata = probe_read_string_append_point(gb_data, &hsize);

        if (!sdata) {
            error = GBS_global_string("Could not read data in '%s' for species '%s'\n(Reason: %s)",
                                      psg.alignment_name, GBT_read_name(gb_species), GB_await_error());
        }
        else {
            name = strdup(GBT_read_name(gb_species));

            fullname                = GBT_read_string(gb_species, "full_name");
            if (!fullname) fullname = strdup("");

            gbd   = gb_species;

            set_checksum(GB_checksum(sdata, hsize, 1, ".-"));
            int csize = probe_compress_sequence(sdata, hsize);

            set_data(GB_memdup(sdata, csize), csize);
            free(sdata);
        }
    }

    return error;
}

void probe_read_alignments() {
    // reads sequence data into psg.data

    GB_begin_transaction(psg.gb_main);

    // read ref SAI (e.g. ecoli)
    {
        char   *def_ref = GBT_get_default_ref(psg.gb_main);
        GBDATA *gb_ref  = GBT_find_SAI_rel_SAI_data(psg.gb_sai_data, def_ref);

        psg.ecoli = 0;
        if (gb_ref) {
            GBDATA *gb_data = GBT_read_sequence(gb_ref, psg.alignment_name);
            if (gb_data) {
                psg.ecoli = GB_read_string(gb_data);
            }
        }
        free(def_ref);
    }

    int icount = GB_number_of_subentries(psg.gb_species_data);
    
    psg.data       = new probe_input_data[icount];
    psg.data_count = 0;

    int data_missing = 0;

    printf("Database contains %i species\n", icount);
    {
        arb_progress progress("Preparing sequence data", icount);
        int count = 0;

        for (GBDATA *gb_species = GBT_first_species_rel_species_data(psg.gb_species_data);
             gb_species;
             gb_species = GBT_next_species(gb_species))
        {
            probe_input_data& pid = psg.data[count];

            GB_ERROR error = pid.init(gb_species);
            if (error) {
                fputs(error, stderr);
                fputc('\n', stderr);
                data_missing++;
            }
            else {
                count++;
            }
            progress.inc();
        }

        pt_assert((count+data_missing) == icount);

        psg.data_count = count;
        GB_commit_transaction(psg.gb_main);
    }

    

    if (data_missing) {
        printf("\n%i species were ignored because of missing data.\n", data_missing);
    }
    else {
        printf("\nAll species contain data in alignment '%s'.\n", psg.alignment_name);
    }
    fflush_all();
}

void PT_build_species_hash() {
    long i;
    psg.namehash = GBS_create_hash(psg.data_count, GB_MIND_CASE);
    for (i=0; i<psg.data_count; i++) {
        GBS_write_hash(psg.namehash, psg.data[i].get_name(), i+1);
    }
    unsigned int    max_size;
    max_size = 0;
    for (i = 0; i < psg.data_count; i++) {  // get max sequence len
        max_size = std::max(max_size, (unsigned)(psg.data[i].get_size()));
        psg.char_count += psg.data[i].get_size();
    }
    psg.max_size = max_size;

    if (psg.ecoli) {
        BI_ecoli_ref *ref = new BI_ecoli_ref;
        ref->init(psg.ecoli, strlen(psg.ecoli));
        psg.bi_ecoli = ref;
    }
}


long PT_abs_2_rel(long pos) {
    if (!psg.ecoli) return pos;
    return psg.bi_ecoli->abs_2_rel(pos);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

inline int *intcopy(int i) { int *ip = new int; *ip = i; return ip; }

#define CACHED(p,t) that(p.is_cached()).is_equal_to(t)
#define CACHED_p123(t1,t2,t3) all().of(CACHED(p1, t1), CACHED(p2, t2), CACHED(p3, t3))

void TEST_CachedPtr() {
    using namespace cache;
    {
        typedef SmartPtr<int> IntPtr;

        CacheHandle<IntPtr> p1;
        CacheHandle<IntPtr> p2;
        CacheHandle<IntPtr> p3;

        {
            Cache<IntPtr> cache(2);
            TEST_EXPECT_ZERO(cache.entries()); // nothing cached yet

            p1.assign(intcopy(1), cache);
            TEST_EXPECT_EQUAL(*p1.access(cache), 1);
            TEST_EXPECT_EQUAL(cache.entries(), 1);
            TEST_EXPECTATION(CACHED_p123(true, false, false));

            p2.assign(intcopy(2), cache);
            TEST_EXPECT_EQUAL(*p2.access(cache), 2);
            TEST_EXPECT_EQUAL(cache.entries(), 2);
            TEST_EXPECTATION(CACHED_p123(true, true, false));

            p3.assign(intcopy(3), cache);
            TEST_EXPECT_EQUAL(*p3.access(cache), 3);
            TEST_EXPECT_EQUAL(cache.entries(), 2);
            TEST_EXPECTATION(CACHED_p123(false, true, true)); // p1 has been invalidated by caching p3

            p3.assign(intcopy(33), cache); // test re-assignment
            TEST_EXPECT_EQUAL(*p3.access(cache), 33);
            TEST_EXPECT_EQUAL(cache.entries(), 2);
            TEST_EXPECTATION(CACHED_p123(false, true, true)); // p2 still cached

            TEST_EXPECT_EQUAL(*p2.access(cache), 2);          // should make p2 the LRU cache entry
            TEST_EXPECT_EQUAL(cache.entries(), 2);
            TEST_EXPECTATION(CACHED_p123(false, true, true)); // p2 still cached

            IntPtr s4;
            {
                CacheHandle<IntPtr> p4;
                p4.assign(intcopy(4), cache);
                TEST_EXPECT_EQUAL(*p4.access(cache), 4);
                TEST_EXPECT_EQUAL(cache.entries(), 2);

                s4 = p4.access(cache); // keep data of p4 in s4
                TEST_EXPECT_EQUAL(s4.references(), 2); // ref'd by s4 and p4

                p4.release(cache); // need to release p4 before destruction (otherwise assertion fails)
            }

            TEST_EXPECT_EQUAL(*s4, 4);             // check kept value of deleted CacheHandle
            TEST_EXPECT_EQUAL(s4.references(), 1); // only ref'd by s4

            TEST_EXPECT_EQUAL(cache.entries(), 1);             // contains only p2 (p4 has been released)
            TEST_EXPECTATION(CACHED_p123(false, true, false)); // p3 has been invalidated by caching p4
        }

        TEST_EXPECTATION(CACHED_p123(false, false, false)); // Cache was destroyed = > all CacheHandle will be invalid
        // no need to release p1..p3 (due cache was destroyed)
    }

    // test cache of SmartCharPtr
    {
        Cache<SmartCharPtr> cache(3);

        {
            const int P = 4;
            CacheHandle<SmartCharPtr> p[P];
            const char *word[] = { "apple", "orange", "pie", "juice" };

            for (int i = 0; i<P; ++i) p[i].assign(strdup(word[i]), cache);
            TEST_REJECT(p[0].is_cached());
            for (int i = 1; i<P; ++i) TEST_EXPECT_EQUAL(&*p[i].access(cache), word[i]);

            TEST_REJECT(p[0].is_cached());
            TEST_EXPECT(p[1].is_cached()); // oldest entry

            cache.resize(cache.size()-1);
            TEST_REJECT(p[1].is_cached()); // invalidated by resize

            for (int i = P-1; i >= 0; --i) p[i].assign(strdup(word[P-1-i]), cache);

            for (int i = 0; i<2; ++i) TEST_EXPECT_EQUAL(&*p[i].access(cache), word[P-1-i]);
            for (int i = 2; i<P; ++i) TEST_REJECT(p[i].is_cached());

            cache.flush();
        }
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
