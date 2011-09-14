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

#include <sys/stat.h>

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

void PT_base_2_string(char *id_string, long len) {
    //! get a string with readable bases from a string with PT_?
    char    c;
    char    *src,
        *dest;
    if (!len) len = strlen(id_string);
    dest = src = id_string;

    while ((len--)>0) {
        c=*(src++);
        switch (c) {
            case PT_A: *(dest++)  = 'A'; break;
            case PT_C: *(dest++)  = 'C'; break;
            case PT_G: *(dest++)  = 'G'; break;
            case PT_T: *(dest++)  = 'U'; break;
            case PT_N: *(dest++)  = 'N'; break;
            case 0: *(dest++)     = '0'; break;
            default: *(dest++)    = c; break;
        }

    }
    *dest = '\0';
}

ARB_ERROR probe_read_data_base(const char *name) { // goes to header: __ATTR__USERESULT
    ARB_ERROR error;
    GB_set_verbose();

    psg.gb_shell = new GB_shell;

    GBDATA *gb_main     = GB_open(name, "r");
    if (!gb_main) error = GB_await_error();
    else {
        error = GB_begin_transaction(gb_main);
        if (!error) {
            GBDATA *gb_species_data = GB_entry(gb_main, "species_data");
            if (!gb_species_data) {
                error = GBS_global_string("Database %s is empty (no species_data)", name);
            }
            else {
                psg.gb_main         = gb_main;
                psg.gb_species_data = gb_species_data;
                psg.gb_sai_data     = GBT_get_SAI_data(gb_main);
            }
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

char *probe_read_alignment(int j, int *psize) {
    char           *buffer     = 0;
    GBDATA         *gb_species = psg.data[j].gbd;
    GB_transaction  ta(gb_species);
    GBDATA         *gb_ali     = GB_entry(gb_species, psg.alignment_name);

    if (gb_ali) {
        GBDATA *gb_data = GB_entry(gb_ali, "data");
        if (gb_data) buffer = probe_read_string_append_point(gb_data, psize);
    }
    return buffer;
}

/** Cache:
 * Instead of extracting the pure sequence data and checksum from the arb
 * database each time the pt server loads, cache this data in a separate
 * file to regain near instant pt server startup time.
 *
 * The format of the file is as follows:
 * uint32: magic
 * uint32: version
 * uint32: number of sequences c
 * uint32[c]: lengths of sequences
 * uint32[c]: checksums
 * char[]: sequences
 */
static const uint cache_magic = 0x97CAC11E;
static const uint cache_version = 1;

static char* cache_mkfilename(const char *arbfile) {
    const char*  const suffix = ".ptc";
    char *cname = (char*)calloc(sizeof(char),strlen(arbfile)+strlen(suffix)+1);
    sprintf(cname, "%s%s", arbfile, suffix);
    return cname;
}

static int cache_load(const char* filename, const uint count) {
    char *cname = cache_mkfilename(filename);

    // get stats for arb-file
    struct stat s_arb, s_ptc;
    if (stat(filename,&s_arb)) {
        perror("Unable to stat arb file! Utterly weird, I just opened it...");
        exit(1);
    }

    // get stats for cache-file
    if (stat(cname, &s_ptc)) {
        printf("Couldn't find cache file.\n");
        return 1;
    }

    // check that cache file is newer than arb file
    if (s_arb.st_mtime > s_ptc.st_mtime) {
        printf("Cachefile older than ARB file.\n");
        return 1;
    }

    printf("Loading data from cache:\n");

    FILE *fp = fopen(cname, "r");
    if (!fp) {
        perror("Unable to open cache file.");
        return 1;
    }
    free(cname);

    uint rval;
    fread(&rval, sizeof(int), 1, fp);
    if (rval != cache_magic) {
        printf("Cache file not a cache file!?!\n");
        return 1;
    }

    fread(&rval, sizeof(int), 1, fp);
    if (rval != cache_version) {
        printf("Cache file has wrong version");
        return 1;
    }

    fread(&rval, sizeof(int), 1, fp);
    if (rval != count) {
        printf("Unable to load cache: wrong number of sequences!\n");
        return 1;
    }

    int *sizes = (int*)calloc(sizeof(int), count);
    if (fread(sizes, sizeof(int), count, fp) != count) {
        printf("Cache file too short!");
        return 1;
    }

    uint *checksums = (uint*)calloc(sizeof(uint), count);
    if (fread(checksums, sizeof(uint), count, fp) != count) {
        printf("Cache file too short!");
        return 1;
    }

    ulong datasize = 0;
    for (uint i=0; i<count; i++) {
        psg.data[i].size = sizes[i];
        datasize += sizes[i];
        psg.data[i].checksum = checksums[i];
    }

    free(sizes);
    free(checksums);

    char *data = (char*)calloc(sizeof(char), datasize);
    if (fread(data, sizeof(char), datasize, fp) != datasize) {
        printf("Cache file too short!");
        return 1;
    }

    for (uint i=0; i<count; i++) {
        psg.data[i].data = data;
        data += psg.data[i].size;
    }

    printf("done\n");

    return 0;
}

void cache_save(const char *filename, int count) {
    char *cname = cache_mkfilename(filename);
    FILE *fp = fopen(cname, "w+");
    if (!fp) {
        perror("Unable to open cache file! Not saving cache.");
        return;
    }
    free(cname);

    printf("PT-cache: writing cache file...\n");

    fwrite(&cache_magic, sizeof(uint), 1, fp);
    fwrite(&cache_version, sizeof(uint), 1, fp);
    fwrite(&count, sizeof(count), 1, fp);


    int *sizes = (int*)calloc(sizeof(int), count);
    uint *checksums = (uint*)calloc(sizeof(uint), count);
    for (int i=0; i<count; i++) {
        sizes[i]=psg.data[i].size;
        checksums[i]=psg.data[i].checksum;
    }
    fwrite(sizes, sizeof(int), count, fp);
    fwrite(checksums, sizeof(uint), count, fp);
    free(sizes);
    free(checksums);

    for (int i=0; i<count; i++) {
        fwrite(psg.data[i].data, sizeof(char), psg.data[i].size, fp);
    }

    fclose(fp);

    printf("done\n");
}

void probe_read_alignments(const char* filename) {
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
    
    psg.data       = (probe_input_data *)calloc(sizeof(probe_input_data), icount);
    psg.data_count = 0;

    int data_missing = 0;

    printf("Database contains %i species\n", icount);
    {
        arb_progress progress("Preparing sequence data", icount);
        int count = 0;
        int recache = cache_load(filename, icount);

        for (GBDATA *gb_species = GBT_first_species_rel_species_data(psg.gb_species_data);
             gb_species;
             gb_species = GBT_next_species(gb_species))
        {
            probe_input_data& pid = psg.data[count];

            pid.name     = strdup(GBT_read_name(gb_species));
            pid.fullname = GBT_read_string(gb_species, "full_name");

            if (!pid.fullname) pid.fullname = strdup("");

            pid.is_group = 1;
            pid.gbd      = gb_species;

            GBDATA *gb_ali  = GB_entry(gb_species, psg.alignment_name);
            GBDATA *gb_data = gb_ali ? GB_entry(gb_ali, "data") : NULL;
            if (!gb_data) {
                fprintf(stderr, "Species '%s' has no data in '%s'\n", pid.name, psg.alignment_name);
                data_missing++;
            }
            else if (recache) {
                int   hsize;
                char *data = probe_read_string_append_point(gb_data, &hsize);

                if (!data) {
                    GB_ERROR error = GB_await_error();
                    fprintf(stderr, "Could not read data in '%s' for species '%s'\n(Reason: %s)\n",
                            psg.alignment_name, pid.name, error);
                    data_missing++;
                }
                else {
                    if (recache) {
                    pid.checksum = GB_checksum(data, hsize, 1, ".-");
                    int   size = probe_compress_sequence(data, hsize);

                    pid.data = GB_memdup(data, size);
                    pid.size = size;

                    free(data);
                    count++;
                }
                }
                    
            } else {
                count++;
            }
            progress.inc();
        }

        pt_assert((count+data_missing) == icount);

        psg.data_count = count;
        GB_commit_transaction(psg.gb_main);
        if (recache && data_missing == 0)
            cache_save(filename, count);
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
        GBS_write_hash(psg.namehash, psg.data[i].name, i+1);
    }
    unsigned int    max_size;
    max_size = 0;
    for (i = 0; i < psg.data_count; i++) {  // get max sequence len
        max_size = std::max(max_size, (unsigned)(psg.data[i].size));
        psg.char_count += psg.data[i].size;
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

