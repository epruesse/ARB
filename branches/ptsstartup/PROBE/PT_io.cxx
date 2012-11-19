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

inline GBDATA *expect_entry(GBDATA *gb_species, const char *entry_name) {
    GBDATA *gb_entry = GB_entry(gb_species, entry_name);
    if (!gb_entry) {
        GB_export_errorf("Expected entry '%s' is missing for species '%s'",
                         entry_name, GBT_read_name(gb_species));
    }
    return gb_entry;
}

GB_ERROR probe_input_data::init(GBDATA *gb_species_, bool& no_data) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_ali  = GB_entry(gb_species_, psg.alignment_name);
    GBDATA   *gb_data = gb_ali ? GB_entry(gb_ali, "data") : NULL;

    no_data = false;
    if (!gb_data) {
        // @@@ when these are deleted during cleanup, this branch gets impossible
        error   = GBS_global_string("Species '%s' has no data in '%s'", GBT_read_name(gb_species_), psg.alignment_name);
        no_data = true;
    }
    else {
        GBDATA *gb_cs      = expect_entry(gb_species_, "cs");
        GBDATA *gb_compr   = expect_entry(gb_species_, "compr");
        GBDATA *gb_baseoff = expect_entry(gb_species_, "baseoff");

        if (!gb_cs || !gb_compr || !gb_baseoff) error = GB_await_error();
        else {
            gb_species = gb_species_;

            int      csize = GB_read_count(gb_compr);
            GB_CSTR  compr = GB_read_bytes_pntr(gb_compr);

            set_data(GB_memdup(compr, csize), csize);
        }
    }

    return error;
}

inline GB_ERROR PT_prepare_species_sequence(GBDATA *gb_species, const char *alignment_name, bool& data_missing, PT_compressed& compressed) {
    GB_ERROR  error   = NULL;
    GBDATA   *gb_ali  = GB_entry(gb_species, alignment_name);
    GBDATA   *gb_data = gb_ali ? GB_entry(gb_ali, "data") : NULL;

    data_missing = false;

    if (!gb_data) {
        error = GBS_global_string("Species '%s' has no data in '%s'", GBT_read_name(gb_species), alignment_name);
        data_missing = true;

        // @@@ delete all species w/o data in wanted alignment
    }
    else {
        const char *seq = GB_read_char_pntr(gb_data);

        if (!seq) {
            error = GBS_global_string("Could not read data in '%s' for species '%s'\n(Reason: %s)",
                                      psg.alignment_name, GBT_read_name(gb_species), GB_await_error());
        }
        else {
            size_t seqlen = GB_read_string_count(gb_data);
            compressed.createFrom(seq, seqlen);

            {
                uint32_t  checksum = GB_checksum(seq, seqlen, 1, ".-");
                GBDATA   *gb_cs    = GB_create(gb_species, "cs", GB_INT);
                error              = gb_cs
                    ? GB_write_int(gb_cs, int32_t(checksum))
                    : GB_await_error();
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
            arb_progress progress("Preparing sequence data", icount);
            for (GBDATA *gb_species = GBT_first_species_rel_species_data(psg.gb_species_data);
                 gb_species;
                 gb_species = GBT_next_species(gb_species))
            {
                bool no_data;
                // @@@ directly pass-down missing-data-counter and do not report error in that case
                error = PT_prepare_species_sequence(gb_species, ali_name, no_data, compressBuffer);
                if (no_data) {
                    data_missing++;
                    error = NULL;
                }
                progress.inc();
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

void probe_read_prebuild_alignments() {
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

            bool     no_data;
            GB_ERROR error = pid.init(gb_species, no_data);

            if (error) {
                fputs(error, stderr);
                fputc('\n', stderr);
                if (no_data) {
                    data_missing++;
                    error = 0;
                }
            }
            else {
                pt_assert(!no_data);
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

