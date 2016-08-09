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
#include "PT_compress.h"

#include <arbdbt.h>
#include <BI_basepos.hxx>
#include <arb_progress.h>
#include <arb_file.h>

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
            if (!error) psg.gb_main = gb_main;
            error = GB_end_transaction(gb_main, error);
        }
    }
    return error;
}

uchar PT_compressed::translate[256]; 
bool  PT_compressed::translation_initialized = false;

#if defined(COUNT_COMPRESSES_BASES)
BaseCounter PT_compressed::base_counter;
#endif

size_t probe_compress_sequence(char *seq, size_t seqsize) {
    // translates a readable sequence into PT_base
    // (see also: probe_2_readable)

    PT_compressed compressed(seqsize);

    compressed.createFrom(reinterpret_cast<unsigned char*>(seq), seqsize);
    pt_assert(compressed.get_size() <= (seqsize+1));

    memcpy(seq, compressed.get_seq(), compressed.get_size());
    return compressed.get_size();
}

char *readable_probe(const char *compressed_probe, size_t len, char T_or_U) {
    static SmartMallocPtr(uchar) smart_tab;
    uchar *tab = NULL;

    if (smart_tab.isNull()) {
        tab = (uchar *) ARB_alloc(256);
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
    
    char *result = (char*)ARB_alloc(len+1);
    for (size_t i = 0; i<len; ++i) {
        result[i] = tab[safeCharIndex(compressed_probe[i])];
    }
    result[len] = 0;
    return result;
}

inline GBDATA *expect_entry(GBDATA *gb_species, const char *entry_name) {
    GBDATA *gb_entry = GB_entry(gb_species, entry_name);
    if (!gb_entry) {
        GB_export_errorf("Expected entry '%s' is missing for species '%s'",
                         entry_name, GBT_read_name(gb_species));
    }
    return gb_entry;
}

cache::Cache<SmartCharPtr>                  probe_input_data::seq_cache(1);     // resized later
cache::Cache<probe_input_data::SmartIntPtr> probe_input_data::rel2abs_cache(1); // resized later

GB_ERROR probe_input_data::init(GBDATA *gb_species_) {
    GBDATA *gb_cs      = expect_entry(gb_species_, "cs");
    GBDATA *gb_compr   = expect_entry(gb_species_, "compr");
    GBDATA *gb_baseoff = expect_entry(gb_species_, "baseoff");

    GB_ERROR error = NULL;
    if (!gb_cs || !gb_compr || !gb_baseoff) error = GB_await_error();
    else {
        gb_species = gb_species_;
        size       = GB_read_count(gb_compr);
    }

    return error;
}

inline GB_ERROR PT_prepare_species_sequence(GBDATA *gb_species, const char *alignment_name, bool& data_missing, PT_compressed& compressed) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_ali  = GB_entry(gb_species, alignment_name);
    GBDATA   *gb_data = gb_ali ? GB_entry(gb_ali, "data") : NULL;

    data_missing = false;

    if (!gb_data) {
        data_missing = true;
    }
    else {
        const char *seq = GB_read_char_pntr(gb_data);
        if (!seq) {
            error = GBS_global_string("Could not read data in '%s' for species '%s'\n(Reason: %s)",
                                      alignment_name, GBT_read_name(gb_species), GB_await_error());
        }
        else {
            size_t seqlen = GB_read_string_count(gb_data);
            if (seqlen>compressed.get_allowed_size()) {
                error = GBS_global_string("Sequence too long in '%s' of '%s'\n(Hint: format alignment to fix this problem)",
                                          alignment_name, GBT_read_name(gb_species));
            }

            if (!error) {
                compressed.createFrom(seq, seqlen);
                {
                    uint32_t  checksum = GB_checksum(seq, seqlen, 1, ".-");
                    GBDATA   *gb_cs    = GB_create(gb_species, "cs", GB_INT);
                    error              = gb_cs
                        ? GB_write_int(gb_cs, int32_t(checksum))
                        : GB_await_error();
                }
            }

            if (!error) {
                GBDATA *gb_compr = GB_create(gb_species, "compr", GB_BYTES);
                error            = gb_compr
                    ? GB_write_bytes(gb_compr, compressed.get_seq(), compressed.get_size())
                    : GB_await_error();
            }

            if (!error) {
                GBDATA *gb_baseoff = GB_create(gb_species, "baseoff", GB_INTS);
                error = gb_baseoff
                    ? GB_write_ints(gb_baseoff, compressed.get_offsets(), compressed.get_size())
                    : GB_await_error();
            }

            if (!error) error = GB_delete(gb_ali); // delete original seq data
        }
    }

    return error;
}

GB_ERROR PT_prepare_data(GBDATA *gb_main) {
    GB_ERROR  error           = GB_begin_transaction(gb_main);
    GBDATA   *gb_species_data = GBT_get_species_data(gb_main);

    if (!gb_species_data) {
        error = GB_await_error();
    }
    else {
        int icount = GB_number_of_subentries(gb_species_data);
        int data_missing = 0;

        char *ali_name = GBT_get_default_alignment(gb_main);
        long  ali_len  = GBT_get_alignment_len(gb_main, ali_name);

        PT_compressed compressBuffer(ali_len);

        printf("Database contains %i species\n", icount);
        {
            {
                arb_progress progress("Preparing sequence data", icount);
                for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
                     gb_species && !error;
                    )
                {
                    GBDATA *gb_next = GBT_next_species(gb_species);
                    bool    no_data;

                    error = PT_prepare_species_sequence(gb_species, ali_name, no_data, compressBuffer);
                    if (no_data) {
                        pt_assert(!error);
                        data_missing++;
                        error = GB_delete(gb_species);
                    }
                    progress.inc();
                    gb_species = gb_next;
                }
                if (error) progress.done();
            }

            if (!error) {
                char   *master_data_name  = GBS_global_string_copy("%s/@master_data", GB_SYSTEM_FOLDER);
                GBDATA *gb_master_data    = GB_search(gb_main, master_data_name, GB_FIND);
                if (gb_master_data) error = GB_delete(gb_master_data);
                free(master_data_name);
            }
        }
        if (data_missing) {
            printf("\n%i species were ignored because of missing data.\n", data_missing);
        }
        else {
            printf("\nAll species contain data in alignment '%s'.\n", ali_name);
        }
        fflush_all();
        free(ali_name);
    }

    error = GB_end_transaction(gb_main, error);
    return error;
}

GB_ERROR PT_init_input_data() {
    // reads sequence data into psg.data

    GB_begin_transaction(psg.gb_main);

    // read ref SAI (e.g. ecoli)
    {
        char   *def_ref     = GBT_get_default_ref(psg.gb_main);
        GBDATA *gb_sai_data = GBT_get_SAI_data(psg.gb_main);
        GBDATA *gb_ref      = GBT_find_SAI_rel_SAI_data(gb_sai_data, def_ref);

        psg.ecoli = 0;
        if (gb_ref) {
            GBDATA *gb_data = GBT_find_sequence(gb_ref, psg.alignment_name);
            if (gb_data) {
                psg.ecoli = GB_read_string(gb_data);
            }
        }
        free(def_ref);
    }

    GBDATA *gb_species_data = GBT_get_species_data(psg.gb_main);
    int     icount          = GB_number_of_subentries(gb_species_data);

    psg.data = new probe_input_data[icount];
    psg.data_count = 0;

    printf("Database contains %i species\n", icount);

    GB_ERROR error = NULL;
    {
        arb_progress progress("Checking data", icount);
        int count = 0;

        for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
             gb_species;
             gb_species = GBT_next_species(gb_species))
        {
            probe_input_data& pid = psg.data[count];

            error = pid.init(gb_species);
            if (error) break;
            count++;
            progress.inc();
        }

        psg.data_count = count;
        GB_commit_transaction(psg.gb_main);

        if (error) progress.done();
    }

    fflush_all();
    return error;
}

void PT_build_species_hash() {
    long i;
    psg.namehash = GBS_create_hash(psg.data_count, GB_MIND_CASE);
    for (i=0; i<psg.data_count; i++) {
        GBS_write_hash(psg.namehash, psg.data[i].get_shortname(), i+1);
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


long PT_abs_2_ecoli_rel(long pos) {
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

            for (int i = 0; i<P; ++i) p[i].assign(ARB_strdup(word[i]), cache);
            TEST_REJECT(p[0].is_cached());
            for (int i = 1; i<P; ++i) TEST_EXPECT_EQUAL(&*p[i].access(cache), word[i]);

            TEST_REJECT(p[0].is_cached());
            TEST_EXPECT(p[1].is_cached()); // oldest entry

            cache.resize(cache.size()-1);
            TEST_REJECT(p[1].is_cached()); // invalidated by resize

            for (int i = P-1; i >= 0; --i) p[i].assign(ARB_strdup(word[P-1-i]), cache);

            for (int i = 0; i<2; ++i) TEST_EXPECT_EQUAL(&*p[i].access(cache), word[P-1-i]);
            for (int i = 2; i<P; ++i) TEST_REJECT(p[i].is_cached());

            cache.flush();
        }
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
