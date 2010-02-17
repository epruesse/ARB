// =============================================================== //
//                                                                 //
//   File      : adhash.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <climits>
#include <cfloat>
#include <cctype>

#include "gb_main.h"
#include "gb_data.h"
#include "gb_tune.h"
#include "gb_hashindex.h"

struct gbs_hash_entry {
    char *key;
    long val;
    struct gbs_hash_entry *next;
};
struct GB_HASH {
    size_t  size;
    size_t  nelem;
    GB_CASE case_sens;

    struct gbs_hash_entry **entries; // the hash table (has 'size' entries)

    void (*freefun)(long val); // function to free hash values (see GBS_create_dynaval_hash)

};

struct numhash_entry {
    long           key;
    long           val;
    numhash_entry *next;
};

struct GB_NUMHASH {
    long            size;
    numhash_entry **entries;
};

// prime numbers

#define KNOWN_PRIMES 279
static long sorted_primes[KNOWN_PRIMES] = {
    3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 47, 53, 59, 67, 71, 79, 89, 97, 103, 109, 127, 137, 149, 157, 167, 179, 191, 211,
    223, 239, 257, 271, 293, 311, 331, 349, 373, 397, 419, 443, 467, 499, 541, 571, 607, 641, 677, 719, 757, 797, 839, 887, 937,
    991, 1049, 1109, 1171, 1237, 1303, 1373, 1447, 1531, 1613, 1699, 1789, 1889, 1993, 2099, 2213, 2333, 2459, 2591, 2729, 2879,
    3037, 3203, 3373, 3557, 3761, 3967, 4177, 4397, 4637, 4889, 5147, 5419, 5711, 6029, 6353, 6689, 7043, 7417, 7817, 8231, 8669,
    9127, 9613, 10133, 10667, 11239, 11831, 12457, 13121, 13829, 14557, 15329, 16139, 16993, 17891, 18839, 19841, 20887, 21991, 23159,
    24379, 25667, 27031, 28463, 29983, 31567, 33247, 35023, 36871, 38821, 40867, 43019, 45289, 47681, 50207, 52859, 55661, 58601,
    61687, 64937, 68371, 71971, 75767, 79757, 83969, 88397, 93053, 97961, 103123, 108553, 114269, 120293, 126631, 133303, 140321,
    147709, 155501, 163697, 172313, 181387, 190979, 201031, 211619, 222773, 234499, 246889, 259907, 273601, 288007, 303187, 319147,
    335953, 353641, 372263, 391861, 412487, 434201, 457057, 481123, 506449, 533111, 561173, 590713, 621821, 654553, 689021, 725293,
    763471, 803659, 845969, 890501, 937373, 986717, 1038671, 1093357, 1150909, 1211489, 1275269, 1342403, 1413077, 1487459, 1565747,
    1648181, 1734937, 1826257, 1922383, 2023577, 2130101, 2242213, 2360243, 2484473, 2615243, 2752889, 2897789, 3050321, 3210871,
    3379877, 3557773, 3745051, 3942209, 4149703, 4368113, 4598063, 4840103, 5094853, 5363011, 5645279, 5942399, 6255157, 6584377,
    6930929, 7295719, 7679713, 8083919, 8509433, 8957309, 9428759, 9925021, 10447391, 10997279, 11576087, 12185359, 12826699, 13501819,
    14212447, 14960471, 15747869, 16576727, 17449207, 18367597, 19334317, 20351927, 21423107, 22550639, 23737523, 24986867, 26301967,
    27686291, 29143493, 30677363, 32291971, 33991597, 35780639, 37663841, 39646153, 41732809, 43929307, 46241389, 48675167, 51237019,
    53933713, 56772371, 59760391, 62905681, 66216511, 69701591, 73370107, 77231711, 81296543, 85575313, 90079313, 94820347, 99810899
};


// define CALC_PRIMES only to expand the above table
#if defined(DEBUG)
// #define CALC_PRIMES
#endif // DEBUG

#ifdef CALC_PRIMES

#define CALC_PRIMES_UP_TO 100000000L
#define PRIME_UNDENSITY   20L   // the higher, the less primes are stored

#warning "please don't define CALC_PRIMES permanently"

static unsigned char bit_val[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

static int bit_value(const unsigned char *eratosthenes, long num) {
    // 'num' is odd and lowest 'num' is 3
    long bit_num  = ((num-1) >> 1)-1; // 3->0 5->1 7->2 etc.
    long byte_num = bit_num >> 3; // div 8
    char byte     = eratosthenes[byte_num];

    gb_assert(bit_num >= 0);
    gb_assert((num&1) == 1);    // has to odd

    bit_num = bit_num &  7;

    return (byte & bit_val[bit_num]) ? 1 : 0;
}
static void set_bit_value(unsigned char *eratosthenes, long num, int val) {
    // 'num' is odd and lowest 'num' is 3; val is 0 or 1
    long bit_num  = ((num-1) >> 1)-1; // 3->0 5->1 7->2 etc.
    long byte_num = bit_num >> 3; // div 8
    char byte     = eratosthenes[byte_num];

    gb_assert(bit_num >= 0);
    gb_assert((num&1) == 1);    // has to odd

    bit_num = bit_num &  7;

    if (val) {
        byte |= bit_val[bit_num];
    }
    else {
        byte &= (0xff - bit_val[bit_num]);
    }
    eratosthenes[byte_num] = byte;
}

static void calculate_primes_upto() {
    {
        long           bits_needed  = CALC_PRIMES_UP_TO/2+1; // only need bits for odd numbers
        long           bytes_needed = (bits_needed/8)+1;
        unsigned char *eratosthenes = GB_calloc(bytes_needed, 1); // bit = 1 means "is not a prime"
        long           prime_count  = 0;
        long           num;

        printf("eratosthenes' size = %li\n", bytes_needed);

        if (!eratosthenes) {
            GB_internal_error("out of memory");
            return;
        }

        for (num = 3; num <= CALC_PRIMES_UP_TO; num += 2) {
            if (bit_value(eratosthenes, num) == 0) { // is a prime number
                long num2;
                prime_count++;
                for (num2 = num*2; num2 <= CALC_PRIMES_UP_TO; num2 += num) { // with all multiples
                    if ((num2&1) == 1) { // skip even numbers
                        set_bit_value(eratosthenes, num2, 1);
                    }
                }
            }
            // otherwise it is no prime and all multiples are already set to 1
        }

        // thin out prime numbers (we don't need all of them)
        {
            long prime_count2 = 0;
            long last_prime   = -1000;
            int  index;
            int  printed      = 0;

            for (num = 3; num <= CALC_PRIMES_UP_TO; num += 2) {
                if (bit_value(eratosthenes, num) == 0) { // is a prime number
                    long diff = num-last_prime;
                    if ((diff*PRIME_UNDENSITY)<num) {
                        set_bit_value(eratosthenes, num, 1); // delete unneeded prime
                    }
                    else {
                        prime_count2++; // count needed primes
                        last_prime = num;
                    }
                }
            }

            printf("\nUsing %li prime numbers up to %li:\n\n", prime_count2, CALC_PRIMES_UP_TO);
            printf("#define KNOWN_PRIMES %li\n", prime_count2);
            printf("static long sorted_primes[KNOWN_PRIMES] = {\n    ");
            printed = 4;

            index = 0;
            for (num = 3; num <= CALC_PRIMES_UP_TO; num += 2) {
                if (bit_value(eratosthenes, num) == 0) { // is a prime number
                    if (printed>128) {
                        printf("\n    ");
                        printed = 4;
                    }
                    if (num>INT_MAX) {
                        printed += printf("%liL, ", num);
                    }
                    else {
                        printed += printf("%li, ", num);
                    }
                }
            }
            printf("\n};\n\n");
        }

        free(eratosthenes);
    }
    fflush(stdout);
    exit(1);
}

#endif // CALC_PRIMES

long gbs_get_a_prime(long above_or_equal_this) {
    // return a prime number above_or_equal_this
    // NOTE: it is not necessarily the next prime number, because we don't calculate all prime numbers!

#if defined(CALC_PRIMES)
    calculate_primes_upto(above_or_equal_this);
#endif // CALC_PRIMES

    if (sorted_primes[KNOWN_PRIMES-1] >= above_or_equal_this) {
        int l = 0, h = KNOWN_PRIMES-1;

        while (l < h) {
            int m = (l+h)/2;
#if defined(DEBUG) && 0
            printf("l=%-3i m=%-3i h=%-3i above_or_equal_this=%li sorted_primes[%i]=%li sorted_primes[%i]=%li sorted_primes[%i]=%li\n",
                   l, m, h, above_or_equal_this, l, sorted_primes[l], m, sorted_primes[m], h, sorted_primes[h]);
#endif // DEBUG
            gb_assert(l <= m);
            gb_assert(m <= h);
            if (sorted_primes[m] > above_or_equal_this) {
                h = m-1;
            }
            else {
                if (sorted_primes[m] < above_or_equal_this) {
                    l = m+1;
                }
                else {
                    h = l = m;
                }
            }
        }

        if (sorted_primes[l] < above_or_equal_this) {
            l++;                // take next
            gb_assert(l<KNOWN_PRIMES);
        }

        gb_assert(sorted_primes[l] >= above_or_equal_this);
        gb_assert(l == 0 || sorted_primes[l-1] < above_or_equal_this);

        return sorted_primes[l];
    }

    fprintf(stderr, "Warning: gbs_get_a_prime failed for value %li (performance bleed)\n", above_or_equal_this);
    gb_assert(0); // add more primes to sorted_primes[]

    return above_or_equal_this;
}

// -----------------------------------------------
//      Some Hash Procedures for [string,long]

GB_HASH *GBS_create_hash(long user_size, GB_CASE case_sens) {
    /* Create a hash of size size, this hash is using linked list to avoid collisions,
     *  ignore_case == 0 -> 'a != A'
     *  ignore_case != 0 -> 'a == A'
     */

    GB_HASH *hs;
    long     size = gbs_get_a_prime(user_size);     // use next prime number for hash size

    hs            = (GB_HASH*)GB_calloc(sizeof(*hs), 1);
    hs->size      = size;
    hs->nelem     = 0;
    hs->case_sens = case_sens;
    hs->entries   = (struct gbs_hash_entry **)GB_calloc(sizeof(struct gbs_hash_entry *), size);
    hs->freefun   = NULL;

    return hs;
}

GB_HASH *GBS_create_dynaval_hash(long user_size, GB_CASE case_sens, void (*freefun)(long)) {
    // like GBS_create_hash, but values stored in hash get freed using 'freefun'

    GB_HASH *hs = GBS_create_hash(user_size, case_sens);
    hs->freefun = freefun;
    return hs;
}

void GBS_dynaval_free(long val) {
    free((char*)val);
}

#if defined(DEBUG)
static void dump_access(const char *title, GB_HASH *hs, double mean_access) {
    fprintf(stderr,
            "%s: size=%zu elements=%zu mean_access=%.2f hash-speed=%.1f%%\n",
            title, hs->size, hs->nelem, mean_access, 100.0/mean_access);
}
#endif // DEBUG

void GBS_optimize_hash(GB_HASH *hs) {
    if (hs->nelem > hs->size) {                     // hash is overfilled (even full is bad)
        size_t new_size = gbs_get_a_prime(hs->nelem*3);

#if defined(DEBUG)
        dump_access("Optimizing filled hash", hs, GBS_hash_mean_access_costs(hs));
#endif // DEBUG

        if (new_size>hs->size) { // avoid overflow
            gbs_hash_entry **new_entries = (gbs_hash_entry**)GB_calloc(sizeof(*new_entries), new_size);
            size_t           pos;

            for (pos = 0; pos<hs->size; ++pos) {
                struct gbs_hash_entry *e;
                struct gbs_hash_entry *next;

                for (e = hs->entries[pos]; e; e = next) {
                    long new_idx;
                    next = e->next;

                    GB_CALC_HASH_INDEX(e->key, new_idx, new_size, hs->case_sens);

                    e->next              = new_entries[new_idx];
                    new_entries[new_idx] = e;
                }
            }

            free(hs->entries);

            hs->size    = new_size;
            hs->entries = new_entries;
        }
#if defined(DEBUG)
        dump_access("Optimized hash        ", hs, GBS_hash_mean_access_costs(hs));
#endif // DEBUG

    }
}

static long gbs_hash_to_strstruct(const char *key, long val, void *cd_out) {
    const char           *p;
    int                   c;
    struct GBS_strstruct *out = (struct GBS_strstruct*)cd_out;

    for (p = key; (c=*p);  p++) {
        GBS_chrcat(out, c);
        if (c==':') GBS_chrcat(out, c);
    }
    GBS_chrcat(out, ':');
    GBS_intcat(out, val);
    GBS_chrcat(out, ' ');

    return val;
}

char *GBS_hashtab_2_string(GB_HASH *hash) {
    struct GBS_strstruct *out = GBS_stropen(1024);
    GBS_hash_do_loop(hash, gbs_hash_to_strstruct, out);
    return GBS_strclose(out);
}


void GBS_string_2_hashtab(GB_HASH *hash, char *data) { // modifies data
    char *p, *d, *dp;
    int   c;
    char *nextp;
    char *str;
    int   strlen;
    long  val;

    for (p = data; p;   p = nextp) {
        strlen = 0;
        for (dp = p; (c = *dp); dp++) {
            if (c==':') {
                if (dp[1] == ':') dp++;
                else break;
            }
            strlen++;
        }
        if (*dp) {
            nextp = strchr(dp, ' ');
            if (nextp) nextp++;
        }
        else break;

        str = (char *)GB_calloc(sizeof(char), strlen+1);
        for (dp = p, d = str; (c = *dp);  dp++) {
            if (c==':') {
                if (dp[1] == ':') {
                    *(d++) = c;
                    dp++;
                }
                else break;
            }
            else {
                *(d++) = c;
            }
        }
        val = atoi(dp+1);
        GBS_write_hash_no_strdup(hash, str, val);
    }
}

static struct gbs_hash_entry *find_hash_entry(const GB_HASH *hs, const char *key, size_t *index) {
    struct gbs_hash_entry *e;
    if (hs->case_sens == GB_IGNORE_CASE) {
        GB_CALC_HASH_INDEX_CASE_IGNORED(key, *index, hs->size);
        for (e=hs->entries[*index]; e; e=e->next) {
            if (!strcasecmp(e->key, key)) return e;
        }
    }
    else {
        GB_CALC_HASH_INDEX_CASE_SENSITIVE(key, *index, hs->size);
        for (e=hs->entries[*index]; e; e=e->next) {
            if (!strcmp(e->key, key)) return e;
        }
    }
    return 0;
}

long GBS_read_hash(const GB_HASH *hs, const char *key) {
    size_t                 i;
    struct gbs_hash_entry *e = find_hash_entry(hs, key, &i);

    return e ? e->val : 0;
}

static void delete_from_list(GB_HASH *hs, size_t i, struct gbs_hash_entry *e) {
    // delete the hash entry 'e' from list at index 'i'
    hs->nelem--;
    if (hs->entries[i] == e) {
        hs->entries[i] = e->next;
    }
    else {
        struct gbs_hash_entry *ee;
        for (ee = hs->entries[i]; ee->next != e; ee = ee->next) ;
        if (ee->next == e) {
            ee->next = e->next;
        }
        else {
            GB_internal_error("Database may be corrupt, hash tables error");
        }
    }
    free(e->key);
    if (hs->freefun) hs->freefun(e->val);
    gbm_free_mem((char *)e, sizeof(struct gbs_hash_entry), GBM_HASH_INDEX);
}

static long write_hash(GB_HASH *hs, char *key, bool copyKey, long val) {
    /* returns the old value (or 0 if key had no entry)
     * if 'copyKey' == false, 'key' will be freed (now or later) and may be invalid!
     * if 'copyKey' == true, 'key' will not be touched in any way!
     */

    size_t          i;
    struct gbs_hash_entry *e       = find_hash_entry(hs, key, &i);
    long                   oldval  = 0;

    if (e) {
        oldval = e->val;

        if (!val) delete_from_list(hs, i, e); // (val == 0 is not stored, cause 0 is the default value)
        else      e->val = val;

        if (!copyKey) free(key); // already had an entry -> delete unused mem
    }
    else if (val != 0) {        // don't store 0
        // create new hash entry
        e       = (struct gbs_hash_entry *)gbm_get_mem(sizeof(struct gbs_hash_entry), GBM_HASH_INDEX);
        e->next = hs->entries[i];
        e->key  = copyKey ? strdup(key) : key;
        e->val  = val;

        hs->entries[i] = e;
        hs->nelem++;
    }
    else {
        if (!copyKey) free(key); // don't need an entry -> delete unused mem
    }
    return oldval;
}

long GBS_write_hash(GB_HASH *hs, const char *key, long val) {
    // returns the old value (or 0 if key had no entry)
    return write_hash(hs, (char*)key, true, val);
}

long GBS_write_hash_no_strdup(GB_HASH *hs, char *key, long val) {
    /* same as GBS_write_hash, but does no strdup. 'key' is freed later in GBS_free_hash,
     * so the user has to 'malloc' the string and give control to the hash.
     * Note: after calling this function 'key' may be invalid!
     */
    return write_hash(hs, key, false, val);
}

long GBS_incr_hash(GB_HASH *hs, const char *key) {
    // returns new value
    size_t                 i;
    struct gbs_hash_entry *e = find_hash_entry(hs, key, &i);
    long                   result;

    if (e) {
        result = ++e->val;
        if (!result) delete_from_list(hs, i, e);
    }
    else {
        e       = (struct gbs_hash_entry *)gbm_get_mem(sizeof(struct gbs_hash_entry), GBM_HASH_INDEX);
        e->next = hs->entries[i];
        e->key  = strdup(key);
        e->val  = result = 1;

        hs->entries[i] = e;
        hs->nelem++;
    }
    return result;
}

#if defined(DEVEL_RALF)
// #define DUMP_HASH_ENTRIES
#endif // DEVEL_RALF

#if defined(DEBUG)
double GBS_hash_mean_access_costs(GB_HASH *hs) {
    /* returns the mean access costs of the hash [1.0 .. inf[
     * 1.0 is optimal
     * 2.0 means: hash speed is 50% (1/2.0)
    */
    double mean_access = 1.0;

    if (hs->nelem) {
        int    strcmps_needed = 0;
        size_t pos;

        for (pos = 0; pos<hs->size; pos++) {
            int                    strcmps = 1;
            struct gbs_hash_entry *e;

            for (e = hs->entries[pos]; e; e = e->next) {
                strcmps_needed += strcmps++;
            }
        }

        mean_access = (double)strcmps_needed/hs->nelem;
    }
    return mean_access;
}
#endif // DEBUG

void GBS_free_hash_entries(GB_HASH *hs)
{
    long i;
    long e2;
    struct gbs_hash_entry *e, *ee;

    e2 = hs->size;

#if defined(DUMP_HASH_ENTRIES)
    for (i = 0; i < e2; i++) {
        printf("hash[%li] =", i);
        for (e = hs->entries[i]; e; e = e->next) {
            printf(" '%s'", e->key);
        }
        printf("\n");
    }
#endif // DUMP_HASH_ENTRIES

#if defined(DEBUG)
    if (e2 >= 30) { // ignore small hashes
        double mean_access = GBS_hash_mean_access_costs(hs);
        if (mean_access > 1.5) { // every 2nd access is a collision - increase hash size?
            dump_access("hash-size-warning", hs, mean_access);
#if defined(DEVEL_RALF)
            gb_assert(mean_access<2.0); // hash with 50% speed or less
#endif // DEVEL_RALF
        }
    }
#endif // DEBUG

    for (i = 0; i < e2; i++) {
        for (e = hs->entries[i]; e; e = ee) {
            free(e->key);
            if (hs->freefun) hs->freefun(e->val);
            ee              = e->next;
            gbm_free_mem((char *)e, sizeof(struct gbs_hash_entry), GBM_HASH_INDEX);
        }
        hs->entries[i] = 0;
    }
}

void GBS_free_hash(GB_HASH *hs)
{
    if (!hs) return;
    GBS_free_hash_entries(hs);
    free(hs->entries);
    free(hs);
}

// determine hash quality

struct gbs_hash_statistic_summary {
    long   count;               // how many stats
    long   min_size, max_size, sum_size;
    long   min_nelem, max_nelem, sum_nelem;
    long   min_collisions, max_collisions, sum_collisions;
    double min_fill_ratio, max_fill_ratio, sum_fill_ratio;
    double min_hash_quality, max_hash_quality, sum_hash_quality;
};

static GB_HASH *stat_hash = 0;

static void init_hash_statistic_summary(gbs_hash_statistic_summary *stat) {
    stat->count          = 0;
    stat->min_size       = stat->min_nelem = stat->min_collisions = LONG_MAX;
    stat->max_size       = stat->max_nelem = stat->max_collisions = LONG_MIN;
    stat->min_fill_ratio = stat->min_hash_quality = DBL_MAX;
    stat->max_fill_ratio = stat->max_hash_quality = DBL_MIN;

    stat->sum_size       = stat->sum_nelem = stat->sum_collisions = 0;
    stat->sum_fill_ratio = stat->sum_hash_quality = 0.0;
}

static gbs_hash_statistic_summary *get_stat_summary(const char *id) {
    long found;
    if (!stat_hash) stat_hash = GBS_create_hash(10, GB_MIND_CASE);
    found                     = GBS_read_hash(stat_hash, id);
    if (!found) {
        gbs_hash_statistic_summary *stat = (gbs_hash_statistic_summary*)GB_calloc(1, sizeof(*stat));
        init_hash_statistic_summary(stat);
        found                            = (long)stat;
        GBS_write_hash(stat_hash, id, found);
    }

    return (gbs_hash_statistic_summary*)found;
}

static void addto_hash_statistic_summary(gbs_hash_statistic_summary *stat, long size, long nelem, long collisions, double fill_ratio, double hash_quality) {
    stat->count++;

    if (stat->min_size > size) stat->min_size = size;
    if (stat->max_size < size) stat->max_size = size;

    if (stat->min_nelem > nelem) stat->min_nelem = nelem;
    if (stat->max_nelem < nelem) stat->max_nelem = nelem;

    if (stat->min_collisions > collisions) stat->min_collisions = collisions;
    if (stat->max_collisions < collisions) stat->max_collisions = collisions;

    if (stat->min_fill_ratio > fill_ratio) stat->min_fill_ratio = fill_ratio;
    if (stat->max_fill_ratio < fill_ratio) stat->max_fill_ratio = fill_ratio;

    if (stat->min_hash_quality > hash_quality) stat->min_hash_quality = hash_quality;
    if (stat->max_hash_quality < hash_quality) stat->max_hash_quality = hash_quality;

    stat->sum_size         += size;
    stat->sum_nelem        += nelem;
    stat->sum_collisions       += collisions;
    stat->sum_fill_ratio   += fill_ratio;
    stat->sum_hash_quality += hash_quality;
}

void GBS_clear_hash_statistic_summary(const char *id) {
    init_hash_statistic_summary(get_stat_summary(id));
}

void GBS_print_hash_statistic_summary(const char *id) {
    gbs_hash_statistic_summary *stat  = get_stat_summary(id);
    long                        count = stat->count;
    printf("Statistic summary for %li hashes of type '%s':\n", count, id);
    printf("- size:          min = %6li ; max = %6li ; mean = %6.1f\n", stat->min_size, stat->max_size, (double)stat->sum_size/count);
    printf("- nelem:         min = %6li ; max = %6li ; mean = %6.1f\n", stat->min_nelem, stat->max_nelem, (double)stat->sum_nelem/count);
    printf("- fill_ratio:    min = %5.1f%% ; max = %5.1f%% ; mean = %5.1f%%\n", stat->min_fill_ratio*100.0, stat->max_fill_ratio*100.0, (double)stat->sum_fill_ratio/count*100.0);
    printf("- collisions:    min = %6li ; max = %6li ; mean = %6.1f\n", stat->min_collisions, stat->max_collisions, (double)stat->sum_collisions/count);
    printf("- hash_quality:  min = %5.1f%% ; max = %5.1f%% ; mean = %5.1f%%\n", stat->min_hash_quality*100.0, stat->max_hash_quality*100.0, (double)stat->sum_hash_quality/count*100.0);
}

void GBS_calc_hash_statistic(GB_HASH *hs, const char *id, int print) {
    size_t i;
    long   queues     = 0;
    long   collisions;
    double fill_ratio = (double)hs->nelem/hs->size;
    double hash_quality;

    for (i = 0; i < hs->size; i++) {
        if (hs->entries[i]) queues++;
    }
    collisions = hs->nelem - queues;

    hash_quality = (double)queues/hs->nelem; // no collisions means 100% quality

    if (print != 0) {
        printf("Statistic for hash '%s':\n", id);
        printf("- size       = %zu\n", hs->size);
        printf("- elements   = %zu (fill ratio = %4.1f%%)\n", hs->nelem, fill_ratio*100.0);
        printf("- collisions = %li (hash quality = %4.1f%%)\n", collisions, hash_quality*100.0);
    }

    addto_hash_statistic_summary(get_stat_summary(id), hs->size, hs->nelem, collisions, fill_ratio, hash_quality);
}

void GBS_hash_do_loop(GB_HASH *hs, gb_hash_loop_type func, void *client_data)
{
    long i, e2;
    struct gbs_hash_entry *e, *next;
    e2 = hs->size;
    for (i=0; i<e2; i++) {
        for (e = hs->entries[i]; e; e = next) {
            next = e->next;
            if (e->val) {
                e->val = func(e->key, e->val, client_data);
                if (!e->val) delete_from_list(hs, i, e);
            }
        }
    }
}

long GBS_hash_count_elems(GB_HASH *hs) {
    long e2    = hs->size;
    long count = 0;
    long i;
    struct gbs_hash_entry *e;

    for (i = 0; i<e2; ++i) {
        for (e=hs->entries[i]; e; e=e->next) {
            if (e->val) {
                ++count;
            }
        }
    }

    return count;
}

long GBS_hash_count_value(GB_HASH *hs, long val) {
    long e2    = hs->size;
    long count = 0;
    long i;
    struct gbs_hash_entry *e;

    gb_assert(val != 0); // counting zero values makes no sense (cause these are not stored in the hash)

    for (i = 0; i<e2; ++i) {
        for (e=hs->entries[i]; e; e=e->next) {
            if (e->val == val) {
                ++count;
            }
        }
    }

    return count;
}

const char *GBS_hash_next_element_that(GB_HASH *hs, const char *last_key, bool (*condition)(const char *key, long val, void *cd), void *cd) {
    /* Returns the key of the next element after 'last_key' matching 'condition' (i.e. where condition returns true).
     * If 'last_key' is NULL, the first matching element is returned.
     * Returns NULL if no (more) elements match the 'condition'.
     */

    size_t                 size = hs->size;
    size_t                 i    = 0;
    struct gbs_hash_entry *e    = 0;

    if (last_key) {
        e = find_hash_entry(hs, last_key, &i);
        if (!e) return NULL;

        e = e->next;       // use next entry after 'last_key'
        if (!e) i++;
    }

    for (; i<size && !e; ++i) e = hs->entries[i]; // search first/next entry

    while (e) {
        if ((*condition)(e->key, e->val, cd)) break;
        e = e->next;
        if (!e) {
            for (i++; i<size && !e; ++i) e = hs->entries[i];
        }
    }

    return e ? e->key : NULL;
}

static int wrap_hashCompare4gb_sort(const void *v0, const void *v1, void *sorter) {
    const struct gbs_hash_entry *e0 = (const struct gbs_hash_entry*)v0;
    const struct gbs_hash_entry *e1 = (const struct gbs_hash_entry*)v1;

    return ((gbs_hash_compare_function)sorter)(e0->key, e0->val, e1->key, e1->val);
}

void GBS_hash_do_sorted_loop(GB_HASH *hs, gb_hash_loop_type func, gbs_hash_compare_function sorter, void *client_data) {
    long   i, j, e2;
    struct gbs_hash_entry *e, **mtab;
    e2 = hs->size;
    mtab = (struct gbs_hash_entry **)GB_calloc(sizeof(void *), hs->nelem);
    for (j = 0, i = 0; i < e2; i++) {
        for (e = hs->entries[i]; e; e = e->next) {
            if (e->val) {
                mtab[j++] = e;
            }
        }
    }
    GB_sort((void **) mtab, 0, j, wrap_hashCompare4gb_sort, (void*)sorter);
    for (i = 0; i < j; i++) {
        long new_val = func(mtab[i]->key, mtab[i]->val, client_data);
        if (new_val != mtab[i]->val) GBS_write_hash(hs, mtab[i]->key, new_val);
    }
    free(mtab);
}

int GBS_HCF_sortedByKey(const char *k0, long v0, const char *k1, long v1) {
    // GBUSE(v0);
    // GBUSE(v1);
    return strcmp(k0, k1);
}

// ---------------------------------------------
//      Some Hash Procedures for [long,long]

inline long gbs_numhash_index(long key, long size) {
    long x;
    x = (key * (long long)97)%size;     // make one multiplier a (long long) to avoid
    if (x<0) x += size;                 // int overflow and abort if compiled with -ftrapv
    return x;
}


GB_NUMHASH *GBS_create_numhash(long user_size) {
    long        size = gbs_get_a_prime(user_size);  // use next prime number for hash size
    GB_NUMHASH *hs   = (GB_NUMHASH *)GB_calloc(sizeof(*hs), 1);

    hs->size    = size;
    hs->entries = (numhash_entry **)GB_calloc(sizeof(*(hs->entries)), (size_t)size);

    return hs;
}


long GBS_read_numhash(GB_NUMHASH *hs, long key) {
    numhash_entry *e;
    long           i = gbs_numhash_index(key, hs->size);

    for (e = hs->entries[i]; e; e = e->next) {
        if (e->key==key) return e->val;
    }
    return 0;
}

long GBS_write_numhash(GB_NUMHASH *hs, long key, long val) {
    numhash_entry *e;
    long           i2;
    long           i = gbs_numhash_index(key, hs->size);

    if (!val) {
        numhash_entry *oe;
        oe = 0;
        for (e = hs->entries[i]; e; e = e->next) {
            if (e->key == key) {
                if (oe) {
                    oe->next = e->next;
                }
                else {
                    hs->entries[i] = e->next;
                }
                gbm_free_mem((char *) e, sizeof(*e), GBM_HASH_INDEX);
                return 0;
            }
            oe = e;
        }
        printf("free %lx not found\n", (long)e);
        return 0;
    }
    for (e=hs->entries[i]; e; e=e->next)
    {
        if (e->key==key) {
            i2 = e->val;
            e->val = val;
            return i2;
        }
    }
    e = (numhash_entry *)gbm_get_mem(sizeof(*e), GBM_HASH_INDEX);
    e->next = hs->entries[i];
    e->key = key;
    e->val = val;
    hs->entries[i] = e;
    return 0;
}

void GBS_free_numhash(GB_NUMHASH *hs) {
    long e2 = hs->size;

    for (long i=0; i<e2; i++) {
        numhash_entry *ee;
        for (numhash_entry *e = hs->entries[i]; e; e=ee) {
            ee = e->next;
            gbm_free_mem((char *)e, sizeof(*e), GBM_HASH_INDEX);
        }
    }

    free(hs->entries);
    free(hs);
}

