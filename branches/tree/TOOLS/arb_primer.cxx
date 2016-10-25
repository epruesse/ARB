// =============================================================== //
//                                                                 //
//   File      : arb_primer.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <arb_strarray.h>

#define ADD_LEN        10
#define PRM_BUFFERSIZE 256


struct arb_prm_struct : virtual Noncopyable {
    ConstStrArray   alignment_names;
    int             al_len;
    int             max_name;
    GBDATA         *gb_main;
    char            buffer[PRM_BUFFERSIZE];
    const char     *source;
    int             prmanz;
    int             prmlen;
    int             prmsmin;
    char          **data;
    int             sp_count;
    int             key_cnt;
    int             one_key_cnt;
    int             reduce;
    FILE           *out;
    char           *outname;

    arb_prm_struct()
        : al_len(0),
          max_name(0),
          gb_main(NULL),
          source(NULL),
          prmanz(0),
          prmlen(0),
          prmsmin(0),
          data(NULL),
          sp_count(0),
          key_cnt(0),
          one_key_cnt(0),
          reduce(0),
          out(NULL),
          outname(NULL)
    {
        memset(buffer, 0, sizeof(buffer));
    }
    ~arb_prm_struct() {
        if (data) {
            for (int i = 0; i<sp_count; ++i) free(data[i]);
            free(data);
        }
        free(outname);
    }

};
static arb_prm_struct aprm;

inline int getNumFromStdin() {
    // returns entered number (or 0)
    int i = 0;
    if (fgets(aprm.buffer, PRM_BUFFERSIZE, stdin)) {
        i = atoi(aprm.buffer);
    }
    return i;
}

static GB_ERROR arb_prm_menu() {
    printf(" Please select an Alignment:\n");
    int i;
    for (i=1; aprm.alignment_names[i-1]; ++i) {
        printf("%i: %s\n", i, aprm.alignment_names[i-1]);
    }
    aprm.max_name = i;

    GB_ERROR error = NULL;
    i = getNumFromStdin();
    if ((i<1) || (i>=aprm.max_name)) {
        error = GBS_global_string("selection %i out of range", i);
    }
    else {
        aprm.source = aprm.alignment_names[i-1];

        printf("This module will search for primers for all positions.\n"
               "   The best result is one primer for all (marked) taxa , the worst case\n"
               "   are n primers for n taxa.\n"
               "   Please specify the maximum number of primers:\n"
            );
        aprm.prmanz = getNumFromStdin();

        printf("Select minimum length of a primer, the maximum will be (minimum + %i)\n", ADD_LEN);
        i = getNumFromStdin();
        if ((i<4) || (i>30)) {
            error = GBS_global_string("selection %i out of range", i);
        }
        else {
            aprm.prmlen = i;

            printf("There may be short sequences or/and deletes in full sequences\n"
                   "   So a primer normally does not match all sequences\n"
                   "   Specify minimum percentage of species (0-100 %%):\n");
            i = getNumFromStdin();
            if ((i<1) || (i>100)) {
                error = GBS_global_string("selection %i out of range", i);
            }
            else {
                aprm.prmsmin = i;

                printf("Write output to file (enter \"\" to write to screen)\n");
                if (fgets(aprm.buffer, PRM_BUFFERSIZE, stdin)) {
                    char *lf      = strchr(aprm.buffer, '\n');
                    if (lf) lf[0] = 0; // remove linefeed from filename
                    aprm.outname  = ARB_strdup(aprm.buffer);
                }
                else {
                    aprm.outname  = ARB_strdup("");
                }
            }
        }
    }
    return error;
}

static GB_ERROR arb_prm_read(int /* prmanz */) {
    GBDATA *gb_presets = GBT_get_presets(aprm.gb_main);
    {
        GBDATA *gb_source = GB_find_string(gb_presets, "alignment_name", aprm.source, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
        GBDATA *gb_len    = GB_brother(gb_source, "alignment_len");
        aprm.al_len       = GB_read_int(gb_len);
    }

    int sp_count = GBT_count_marked_species(aprm.gb_main);
    ARB_calloc(aprm.data, sp_count);

    sp_count = 0;
    for (GBDATA *gb_species = GBT_first_marked_species(aprm.gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        GBDATA *gb_source = GB_entry(gb_species, aprm.source);
        if (gb_source) {
            GBDATA *gb_source_data = GB_entry(gb_source, "data");
            if (gb_source_data) {
                const char *hdata = GB_read_char_pntr(gb_source_data);

                if (!hdata) {
                    GB_print_error();
                }
                else {
                    char *data             = ARB_calloc<char>(aprm.al_len+1);
                    aprm.data[sp_count ++] = data;
                    
                    if (sp_count % 50 == 0) printf("Reading taxa %i\n", sp_count);

                    int size = GB_read_string_count(gb_source_data);
                    int i;
                    for (i=0; i<size; i++) {
                        char c = hdata[i];
                        if ((c>='a') && (c<='z')) {
                            data[i] = c-'a'+'A';
                        }
                        else {
                            data[i] = c;
                        }
                    }
                    for (; i<aprm.al_len; i++) {
                        data[i] = '.';
                    }
                    data[i] = 0;
                }
            }
        }
    }
    printf("%i taxa read\n", sp_count);
    aprm.sp_count = sp_count;
    if (sp_count == 0) {
        return "No marked taxa found";
    }
    return NULL;
}

static long arb_count_keys(const char * /* key */, long val, void *)
{
    if (val >1) {
        aprm.key_cnt++;
    }
    else {
        aprm.one_key_cnt++;
    }
    return val;
}

static long arb_print_primer(const char *key, long val, void *)
{
    if (val <= 1) return val;
    int gc = 0;
    const char *p;
    for (p = key; *p; p++) {
        if (*p == 'G' || *p == 'C') gc++;
    }
    fprintf(aprm.out, "  %s matching %4li taxa     GC = %3i%%\n",
            key, val, 100*gc/(int)strlen(key));
    return val;
}

#define is_base(c) (((c>='a') && (c<='z')) || ((c>='A')&&(c<='Z')))

static int primer_print(char *dest, char * source, int size)
{
    char c;
    c = *(source++);
    if (!is_base(c)) return 1;
    while (size) {
        while (!is_base(c)) {
            c = *(source++);
            if (!c) return 1;
        }
        if (c == 'N' || c == 'n') return 1;
        *(dest++) = c;
        size--;
        if (!c) return 1;
        c = 0;
    }
    *dest = 0;
    return 0;
}


static long arb_reduce_primer_len(const char *key, long val, void *cl_hash) {
    GB_HASH* hash = (GB_HASH*)cl_hash;
    char     buffer[256];
    int      size = strlen(key)-aprm.reduce;

    strncpy(buffer, key, size);
    buffer[size] = 0;
    val += GBS_read_hash(hash, buffer);
    GBS_write_hash(hash, buffer, val);
    return val;
}

static void arb_prm_primer(int /* prmanz */)
{
    GB_HASH *mhash;
    int      sp;
    char    *buffer;
    int      pos;
    int      prmlen;
    int      pspecies;
    int      cutoff_cnt;
    int     *best_primer_cnt;
    int     *best_primer_new;
    int     *best_primer_swap;

    prmlen = aprm.prmlen + ADD_LEN + 1;

    ARB_calloc(buffer,          prmlen+1);
    ARB_calloc(best_primer_cnt, prmlen+1);
    ARB_calloc(best_primer_new, prmlen+1);

    for (pos = 0; pos < aprm.al_len; pos++) {
        prmlen = aprm.prmlen + ADD_LEN;
        mhash = GBS_create_hash(1024, GB_MIND_CASE);
        pspecies = 0;
        if (pos % 50 == 0) printf("Pos. %i (%i)\n", pos, aprm.al_len);
        cutoff_cnt = aprm.prmanz+1;
        for (sp = 0; sp < aprm.sp_count; sp++) {    // build initial hash table
            if (!primer_print(buffer, aprm.data[sp] + pos, prmlen)) {
                GBS_incr_hash(mhash, buffer);
                pspecies++;
            }
        }
        if (pspecies*100 >= aprm.prmsmin * aprm.sp_count) {     // reduce primer length
            for (; prmlen >= aprm.prmlen; prmlen-=aprm.reduce) {
                GB_HASH *hash = GBS_create_hash(aprm.prmanz, GB_MIND_CASE);

                aprm.key_cnt = 0;
                aprm.one_key_cnt = 0;
                GBS_hash_do_loop(mhash, arb_count_keys, NULL);
                if ((aprm.key_cnt + aprm.one_key_cnt < cutoff_cnt) &&
                    //  (aprm.key_cnt > aprm.one_key_cnt) &&
                    (aprm.key_cnt<best_primer_cnt[prmlen+1])) {
                    fprintf(aprm.out, "%3i primer found len %3i(of %4i taxa) for position %i\n", aprm.key_cnt, prmlen, pspecies, pos);
                    GBS_hash_do_loop(mhash, arb_print_primer, NULL);
                    fprintf(aprm.out, "\n\n");
                    cutoff_cnt = aprm.key_cnt;
                }
                best_primer_new[prmlen] = aprm.key_cnt;
                aprm.reduce = 1;
                while (aprm.key_cnt > aprm.prmanz*4) {
                    aprm.key_cnt/=4;
                    aprm.reduce++;
                }
                GBS_hash_do_loop(mhash, arb_reduce_primer_len, hash);
                GBS_free_hash(mhash);
                mhash = hash;
            }
        }
        else {
            for (; prmlen>0; prmlen--) best_primer_new[prmlen] = aprm.prmanz+1;
        }
        GBS_free_hash(mhash);
        best_primer_swap = best_primer_new;
        best_primer_new = best_primer_cnt;
        best_primer_cnt = best_primer_swap;
        mhash = 0;
    }

    free(best_primer_new);
    free(best_primer_cnt);
    free(buffer);
}

int ARB_main(int argc, char *argv[]) {
    const char *path  = NULL;

    while (argc >= 2) {
        if (strcmp(argv[1], "--help") == 0) {
            fprintf(stderr,
                    "Usage: arb_primer [dbname]\n"
                    "Searches sequencing primers\n");
            return EXIT_FAILURE;
        }
        path = argv[1];
        argv++; argc--;
    }

    if (!path) path = ":";

    GB_ERROR error = NULL;
    GB_shell shell;
    aprm.gb_main   = GB_open(path, "r");
    if (!aprm.gb_main) {
        error = GBS_global_string("Can't open db '%s' (Reason: %s)", path, GB_await_error());
    }
    else {
        GB_begin_transaction(aprm.gb_main);
        GBT_get_alignment_names(aprm.alignment_names, aprm.gb_main);
        GB_commit_transaction(aprm.gb_main);

        error = arb_prm_menu();

        if (!error) {
            GB_begin_transaction(aprm.gb_main);
            error = arb_prm_read(aprm.prmanz);
            if (!error) {
                GB_commit_transaction(aprm.gb_main);
                if (strlen(aprm.outname)) {
                    aprm.out = fopen(aprm.outname, "w");
                    if (!aprm.out) {
                        error = GB_IO_error("writing", aprm.outname);
                    }
                    else {
                        arb_prm_primer(aprm.prmanz);
                        fclose(aprm.out);
                    }
                }
                else {
                    aprm.out = stdout;
                    arb_prm_primer(aprm.prmanz);
                }
            }
            else {
                GB_abort_transaction(aprm.gb_main);
            }
        }
        GB_close(aprm.gb_main);
    }

    if (error) {
        fprintf(stderr, "Error in arb_primer: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
