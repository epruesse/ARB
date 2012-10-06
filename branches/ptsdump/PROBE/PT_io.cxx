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

int probe_compress_sequence(char *seq, int seqsize) {
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

        *dest++ = c;
        if (c == PT_QU) { // TODO: *seq = '.' ???
            offset += count_gaps_and_dots(seq + offset, seqsize - offset); // skip over gaps and dots
            // dest[-1] = PT_N; // @@@ uncomment this to handle '.' like 'N' (experimental!!!)
        }
    }

    if (dest[-1] != PT_QU) {
        *dest++ = PT_QU;
    }

#ifdef ARB_64
    pt_assert(!((dest - seq) & 0xffffffff00000000)); // must fit into 32 bit
#endif

    return dest - seq;
}

char *readable_probe(char *compressed_probe, size_t len, char T_or_U) {
    static SmartMallocPtr(uchar) smart_tab;
    uchar *tab = NULL;

    if (smart_tab.isNull()) {
        tab = (uchar *) malloc(256);
        memset(tab, '?', 256);

        tab[PT_A] = 'A';
        tab[PT_C] = 'C';
        tab[PT_G] = 'G';
        tab[PT_QU]      = 0;
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
        error = GBS_global_string("Species '%s' has no data in '%s'\n", GBT_read_name(gb_species), psg.alignment_name);
    }
    else {
        int   hsize;
        char *sdata = probe_read_string_append_point(gb_data, &hsize);

        if (!sdata) {
            error = GBS_global_string("Could not read data in '%s' for species '%s'\n(Reason: %s)\n",
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
    fflush(stdout);
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

