// =============================================================== //
//                                                                 //
//   File      : adhash.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_data.h"
#include "gb_tune.h"
#include "gb_hashindex.h"

#include <arb_strbuf.h>
#include <arb_sort.h>

#include <climits>
#include <cfloat>
#include <cctype>


struct gbs_hash_entry {
    char           *key;
    long            val;
    gbs_hash_entry *next;
};
struct GB_HASH {
    size_t           size;                          // size of hashtable
    size_t           nelem;                         // number of elements inserted
    GB_CASE          case_sens;
    gbs_hash_entry **entries;                       // the hash table (has 'size' entries)

    void (*freefun)(long val); // function to free hash values (see GBS_create_dynaval_hash)

};

struct numhash_entry {
    long           key;
    long           val;
    numhash_entry *next;
};

struct GB_NUMHASH {
    long            size;                           // size of hashtable
    size_t          nelem;                          // number of elements inserted
    numhash_entry **entries;
};

// prime numbers

#define KNOWN_PRIMES 279
static size_t sorted_primes[KNOWN_PRIMES] = {
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

#define CALC_PRIMES_UP_TO 100000000U
#define PRIME_UNDENSITY   20U   // the higher, the less primes are stored

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
        size_t         bits_needed  = CALC_PRIMES_UP_TO/2+1; // only need bits for odd numbers
        size_t         bytes_needed = (bits_needed/8)+1;
        unsigned char *eratosthenes = (unsigned char *)ARB_calloc(bytes_needed, 1); // bit = 1 means "is not a prime"
        size_t         prime_count  = 0;
        size_t         num;

        printf("eratosthenes' size = %zu\n", bytes_needed);
        GBK_dump_backtrace(stderr, "calculate_primes_upto");

        for (num = 3; num <= CALC_PRIMES_UP_TO; num += 2) {
            if (bit_value(eratosthenes, num) == 0) { // is a prime number
                size_t num2;
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
            size_t prime_count2 = 0;
            size_t last_prime   = 1;
            size_t printed      = 0;

            for (num = 3; num <= CALC_PRIMES_UP_TO; num += 2) {
                if (bit_value(eratosthenes, num) == 0) { // is a prime number
                    size_t diff = num-last_prime;
                    if ((diff*PRIME_UNDENSITY)<num) {
                        set_bit_value(eratosthenes, num, 1); // delete unneeded prime
                    }
                    else {
                        prime_count2++; // count needed primes
                        last_prime = num;
                    }
                }
            }

            printf("\nUsing %zu prime numbers up to %zu:\n\n", prime_count2, CALC_PRIMES_UP_TO);
            printf("#define KNOWN_PRIMES %zu\n", prime_count2);
            printf("static size_t sorted_primes[KNOWN_PRIMES] = {\n    ");
            printed = 4;

            for (num = 3; num <= CALC_PRIMES_UP_TO; num += 2) {
                if (bit_value(eratosthenes, num) == 0) { // is a prime number
                    if (printed>128) {
                        printf("\n    ");
                        printed = 4;
                    }

                    if (num>INT_MAX) {
                        printed += printf("%zuU, ", num);
                    }
                    else {
                        printed += printf("%zu, ", num);
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

size_t gbs_get_a_prime(size_t above_or_equal_this) {
    // return a prime number above_or_equal_this
    // NOTE: it is not necessarily the next prime number, because we don't calculate all prime numbers!

#if defined(CALC_PRIMES)
    calculate_primes_upto();
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

    fprintf(stderr, "Warning: gbs_get_a_prime failed for value %zu (performance bleed)\n", above_or_equal_this);
    gb_assert(0); // add more primes to sorted_primes[]

    return above_or_equal_this; // NEED_NO_COV
}

// -----------------------------------------------
//      Some Hash Procedures for [string,long]

inline size_t hash_size(size_t estimated_elements) {
    size_t min_hash_size = 2*estimated_elements;    // -> fill rate ~ 50% -> collisions unlikely
    size_t next_prime    = gbs_get_a_prime(min_hash_size); // use next prime number

    return next_prime;
}


GB_HASH *GBS_create_hash(long estimated_elements, GB_CASE case_sens) {
    /*! Create a hash
     * @param estimated_elements estimated number of elements added to hash (if you add more elements, hash will still work, but get slow)
     * @param case_sens GB_IGNORE_CASE or GB_MIND_CASE
     * Uses linked lists to avoid collisions.
     */
    long     size = hash_size(estimated_elements);
    GB_HASH *hs   = (GB_HASH*)ARB_calloc(sizeof(*hs), 1);

    hs->size      = size;
    hs->nelem     = 0;
    hs->case_sens = case_sens;
    hs->entries   = (gbs_hash_entry **)ARB_calloc(sizeof(gbs_hash_entry *), size);
    hs->freefun   = NULL;

    return hs;
}

GB_HASH *GBS_create_dynaval_hash(long estimated_elements, GB_CASE case_sens, void (*freefun)(long)) {
    //! like GBS_create_hash, but values stored in hash get freed using 'freefun' when hash gets destroyed
    GB_HASH *hs = GBS_create_hash(estimated_elements, case_sens);
    hs->freefun = freefun;
    return hs;
}

void GBS_dynaval_free(long val) {
    free((void*)val);
}

#if defined(DEBUG)
inline void dump_access(const char *title, const GB_HASH *hs, double mean_access) {
    fprintf(stderr,
            "%s: size=%zu elements=%zu mean_access=%.2f hash-speed=%.1f%%\n",
            title, hs->size, hs->nelem, mean_access, 100.0/mean_access);
}

static double hash_mean_access_costs(const GB_HASH *hs) {
    /* returns the mean access costs of the hash [1.0 .. inf[
     * 1.0 is optimal
     * 2.0 means: hash speed is 50% (1/2.0)
    */
    double mean_access = 1.0;

    if (hs->nelem) {
        int    strcmps_needed = 0;
        size_t pos;

        for (pos = 0; pos<hs->size; pos++) {
            int             strcmps = 1;
            gbs_hash_entry *e;

            for (e = hs->entries[pos]; e; e = e->next) {
                strcmps_needed += strcmps++;
            }
        }

        mean_access = (double)strcmps_needed/hs->nelem;
    }
    return mean_access;
}
#endif // DEBUG


void GBS_optimize_hash(const GB_HASH *hs) {
    if (hs->nelem > hs->size) {                     // hash is overfilled (Note: even 50% fillrate is slow)
        size_t new_size = gbs_get_a_prime(hs->nelem*3);

#if defined(DEBUG)
        dump_access("Optimizing filled hash", hs, hash_mean_access_costs(hs));
#endif // DEBUG

        if (new_size>hs->size) { // avoid overflow
            gbs_hash_entry **new_entries = (gbs_hash_entry**)ARB_calloc(sizeof(*new_entries), new_size);
            size_t           pos;

            for (pos = 0; pos<hs->size; ++pos) {
                gbs_hash_entry *e;
                gbs_hash_entry *next;

                for (e = hs->entries[pos]; e; e = next) {
                    long new_idx;
                    next = e->next;

                    GB_CALC_HASH_INDEX(e->key, new_idx, new_size, hs->case_sens);

                    e->next              = new_entries[new_idx];
                    new_entries[new_idx] = e;
                }
            }

            free(hs->entries);

            {
                GB_HASH *hs_mutable = const_cast<GB_HASH*>(hs);
                hs_mutable->size    = new_size;
                hs_mutable->entries = new_entries;
            }
        }
#if defined(DEBUG)
        dump_access("Optimized hash        ", hs, hash_mean_access_costs(hs));
#endif // DEBUG

    }
}

static void gbs_hash_to_strstruct(const char *key, long val, void *cd_out) {
    const char    *p;
    int            c;
    GBS_strstruct *out = (GBS_strstruct*)cd_out;

    for (p = key; (c=*p);  p++) {
        GBS_chrcat(out, c);
        if (c==':') GBS_chrcat(out, c);
    }
    GBS_chrcat(out, ':');
    GBS_intcat(out, val);
    GBS_chrcat(out, ' ');
}

char *GBS_hashtab_2_string(const GB_HASH *hash) {
    GBS_strstruct *out = GBS_stropen(1024);
    GBS_hash_do_const_loop(hash, gbs_hash_to_strstruct, out);
    return GBS_strclose(out);
}


static gbs_hash_entry *find_hash_entry(const GB_HASH *hs, const char *key, size_t *index) {
    gbs_hash_entry *e;
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
    size_t          i;
    gbs_hash_entry *e = find_hash_entry(hs, key, &i);

    return e ? e->val : 0;
}

static void delete_from_list(GB_HASH *hs, size_t i, gbs_hash_entry *e) {
    // delete the hash entry 'e' from list at index 'i'
    hs->nelem--;
    if (hs->entries[i] == e) {
        hs->entries[i] = e->next;
    }
    else {
        gbs_hash_entry *ee;
        for (ee = hs->entries[i]; ee->next != e; ee = ee->next) ;
        if (ee->next == e) {
            ee->next = e->next;
        }
        else {
            GB_internal_error("Database may be corrupt, hash tables error"); // NEED_NO_COV
        }
    }
    free(e->key);
    if (hs->freefun) hs->freefun(e->val);
    gbm_free_mem(e, sizeof(gbs_hash_entry), GBM_HASH_INDEX);
}

static long write_hash(GB_HASH *hs, char *key, bool copyKey, long val) {
    /* returns the old value (or 0 if key had no entry)
     * if 'copyKey' == false, 'key' will be freed (now or later) and may be invalid!
     * if 'copyKey' == true, 'key' will not be touched in any way!
     */

    size_t          i;
    gbs_hash_entry *e      = find_hash_entry(hs, key, &i);
    long            oldval = 0;

    if (e) {
        oldval = e->val;

        if (!val) delete_from_list(hs, i, e); // (val == 0 is not stored, cause 0 is the default value)
        else      e->val = val;

        if (!copyKey) free(key); // already had an entry -> delete unused mem
    }
    else if (val != 0) {        // don't store 0
        // create new hash entry
        e       = (gbs_hash_entry *)gbm_get_mem(sizeof(gbs_hash_entry), GBM_HASH_INDEX);
        e->next = hs->entries[i];
        e->key  = copyKey ? ARB_strdup(key) : key;
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
    size_t          i;
    gbs_hash_entry *e = find_hash_entry(hs, key, &i);
    long            result;

    if (e) {
        result = ++e->val;
        if (!result) delete_from_list(hs, i, e);
    }
    else {
        e       = (gbs_hash_entry *)gbm_get_mem(sizeof(gbs_hash_entry), GBM_HASH_INDEX);
        e->next = hs->entries[i];
        e->key  = ARB_strdup(key);
        e->val  = result = 1;

        hs->entries[i] = e;
        hs->nelem++;
    }
    return result;
}

#if defined(DEVEL_RALF)
// #define DUMP_HASH_ENTRIES
#endif // DEVEL_RALF

static void GBS_erase_hash(GB_HASH *hs) {
    size_t hsize = hs->size;

#if defined(DUMP_HASH_ENTRIES)
    for (size_t i = 0; i < hsize; i++) {
        printf("hash[%zu] =", i);
        for (gbs_hash_entry *e = hs->entries[i]; e; e = e->next) {
            printf(" '%s'", e->key);
        }
        printf("\n");
    }
#endif // DUMP_HASH_ENTRIES

    // check hash size
    if (hsize >= 10) { // ignore small hashes
#if defined(DEBUG)
        double mean_access = hash_mean_access_costs(hs);
        if (mean_access > 1.5) { // every 2nd access is a collision - increase hash size?
            dump_access("hash-size-warning", hs, mean_access);
#if defined(DEVEL_RALF) && !defined(UNIT_TESTS)
            gb_assert(mean_access<2.0);             // hash with 50% speed or less
#endif // DEVEL_RALF
        }
#else
        if (hs->nelem >= (2*hsize)) {
            GB_warningf("Performance leak - very slow hash detected (elems=%zu, size=%zu)\n", hs->nelem, hs->size);
            GBK_dump_backtrace(stderr, "detected performance leak");
        }
#endif // DEBUG
    }

    for (size_t i = 0; i < hsize; i++) {
        for (gbs_hash_entry *e = hs->entries[i]; e; ) {
            free(e->key);
            if (hs->freefun) hs->freefun(e->val);

            gbs_hash_entry *next = e->next;
            gbm_free_mem(e, sizeof(gbs_hash_entry), GBM_HASH_INDEX);
            e = next;
        }
        hs->entries[i] = 0;
    }
    hs->nelem = 0;
}

void GBS_free_hash(GB_HASH *hs) {
    gb_assert(hs);
    GBS_erase_hash(hs);
    free(hs->entries);
    free(hs);
}

void GBS_hash_do_loop(GB_HASH *hs, gb_hash_loop_type func, void *client_data) {
    size_t hsize = hs->size;
    for (size_t i=0; i<hsize; i++) {
        for (gbs_hash_entry *e = hs->entries[i]; e; ) {
            gbs_hash_entry *next = e->next;
            if (e->val) {
                e->val = func(e->key, e->val, client_data);
                if (!e->val) delete_from_list(hs, i, e);
            }
            e = next;
        }
    }
}

void GBS_hash_do_const_loop(const GB_HASH *hs, gb_hash_const_loop_type func, void *client_data) {
    size_t hsize = hs->size;
    for (size_t i=0; i<hsize; i++) {
        for (gbs_hash_entry *e = hs->entries[i]; e; ) {
            gbs_hash_entry *next = e->next;
            if (e->val) func(e->key, e->val, client_data);
            e = next;
        }
    }
}

size_t GBS_hash_elements(const GB_HASH *hs) {
    return hs->nelem;
}

const char *GBS_hash_next_element_that(const GB_HASH *hs, const char *last_key, bool (*condition)(const char *key, long val, void *cd), void *cd) {
    /* Returns the key of the next element after 'last_key' matching 'condition' (i.e. where condition returns true).
     * If 'last_key' is NULL, the first matching element is returned.
     * Returns NULL if no (more) elements match the 'condition'.
     */

    size_t          size = hs->size;
    size_t          i    = 0;
    gbs_hash_entry *e    = 0;

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
    const gbs_hash_entry *e0 = (const gbs_hash_entry*)v0;
    const gbs_hash_entry *e1 = (const gbs_hash_entry*)v1;

    return ((gbs_hash_compare_function)sorter)(e0->key, e0->val, e1->key, e1->val);
}

void GBS_hash_do_sorted_loop(GB_HASH *hs, gb_hash_loop_type func, gbs_hash_compare_function sorter, void *client_data) {
    size_t           hsize = hs->size;
    gbs_hash_entry **mtab  = (gbs_hash_entry **)ARB_calloc(sizeof(void *), hs->nelem);
    
    size_t j = 0;
    for (size_t i = 0; i < hsize; i++) {
        for (gbs_hash_entry *e = hs->entries[i]; e; e = e->next) {
            if (e->val) {
                mtab[j++] = e;
            }
        }
    }

    GB_sort((void**)mtab, 0, j, wrap_hashCompare4gb_sort, (void*)sorter);
    
    for (size_t i = 0; i < j; i++) {
        long new_val = func(mtab[i]->key, mtab[i]->val, client_data);
        if (new_val != mtab[i]->val) GBS_write_hash(hs, mtab[i]->key, new_val);
    }
    
    free(mtab);
}

int GBS_HCF_sortedByKey(const char *k0, long /*v0*/, const char *k1, long /*v1*/) {
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


GB_NUMHASH *GBS_create_numhash(size_t estimated_elements) {
    size_t      size = hash_size(estimated_elements);
    GB_NUMHASH *hs   = (GB_NUMHASH *)ARB_calloc(sizeof(*hs), 1);

    hs->size    = size;
    hs->nelem   = 0;
    hs->entries = (numhash_entry **)ARB_calloc(sizeof(*(hs->entries)), (size_t)size);

    return hs;
}

long GBS_read_numhash(GB_NUMHASH *hs, long key) {
    size_t i = gbs_numhash_index(key, hs->size);
    for (numhash_entry *e = hs->entries[i]; e; e = e->next) {
        if (e->key==key) return e->val;
    }
    return 0;
}

long GBS_write_numhash(GB_NUMHASH *hs, long key, long val) {
    size_t i      = gbs_numhash_index(key, hs->size);
    long   oldval = 0;

    if (val == 0) { // erase
        numhash_entry **nextPtr = &(hs->entries[i]);

        for (numhash_entry *e = hs->entries[i]; e; e = e->next) {
            if (e->key == key) {
                *nextPtr = e->next;                  // unlink entry
                gbm_free_mem(e, sizeof(*e), GBM_HASH_INDEX);
                hs->nelem--;
                return 0;
            }
            nextPtr = &(e->next);
        }
    }
    else {
        for (numhash_entry *e=hs->entries[i]; e; e=e->next) {
            if (e->key==key) {
                oldval = e->val; gb_assert(oldval);
                e->val = val;
                break;
            }
        }

        if (!oldval) {
            numhash_entry *e = (numhash_entry *)gbm_get_mem(sizeof(*e), GBM_HASH_INDEX);

            e->next = hs->entries[i];
            e->key  = key;
            e->val  = val;

            hs->nelem++;
            hs->entries[i] = e;
        }
    }
    return oldval;
}

static void GBS_erase_numhash(GB_NUMHASH *hs) {
    size_t hsize = hs->size;

    for (size_t i=0; i<hsize; i++) {
        for (numhash_entry *e = hs->entries[i]; e; ) {
            numhash_entry *next = e->next;
            
            gbm_free_mem(e, sizeof(*e), GBM_HASH_INDEX);
            e = next;
        }
    }

    hs->nelem = 0;
}

void GBS_free_numhash(GB_NUMHASH *hs) {
    GBS_erase_numhash(hs);
    free(hs->entries);
    free(hs);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

// determine hash quality

struct gbs_hash_statistic_summary {
    long   count;               // how many stats
    long   min_size, max_size, sum_size;
    long   min_nelem, max_nelem, sum_nelem;
    long   min_collisions, max_collisions, sum_collisions;
    double min_fill_ratio, max_fill_ratio, sum_fill_ratio;
    double min_hash_quality, max_hash_quality, sum_hash_quality;

    void init() {
        count          = 0;
        min_size       = min_nelem = min_collisions = LONG_MAX;
        max_size       = max_nelem = max_collisions = LONG_MIN;
        min_fill_ratio = min_hash_quality = DBL_MAX;
        max_fill_ratio = max_hash_quality = DBL_MIN;

        sum_size       = sum_nelem = sum_collisions = 0;
        sum_fill_ratio = sum_hash_quality = 0.0;
    }
};

class hash_statistic_manager : virtual Noncopyable {
    GB_HASH *stat_hash;
public:
    hash_statistic_manager() : stat_hash(NULL) { }
    ~hash_statistic_manager() {
        if (stat_hash) GBS_free_hash(stat_hash);
    }

    gbs_hash_statistic_summary *get_stat_summary(const char *id) {
        if (!stat_hash) stat_hash = GBS_create_dynaval_hash(10, GB_MIND_CASE, GBS_dynaval_free);

        long found = GBS_read_hash(stat_hash, id);
        if (!found) {
            gbs_hash_statistic_summary *stat = (gbs_hash_statistic_summary*)ARB_calloc(1, sizeof(*stat));
            stat->init();
            found = (long)stat;
            GBS_write_hash(stat_hash, id, found);
        }

        return (gbs_hash_statistic_summary*)found;
    }
};

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
    stat->sum_collisions   += collisions;
    stat->sum_fill_ratio   += fill_ratio;
    stat->sum_hash_quality += hash_quality;
}

static hash_statistic_manager hash_stat_man;

static void test_clear_hash_statistic_summary(const char *id) {
    hash_stat_man.get_stat_summary(id)->init();
}

static void test_print_hash_statistic_summary(const char *id) {
    gbs_hash_statistic_summary *stat  = hash_stat_man.get_stat_summary(id);
    long                        count = stat->count;
    printf("Statistic summary for %li hashes of type '%s':\n", count, id);
    printf("- size:          min = %6li ; max = %6li ; mean = %6.1f\n", stat->min_size, stat->max_size, (double)stat->sum_size/count);
    printf("- nelem:         min = %6li ; max = %6li ; mean = %6.1f\n", stat->min_nelem, stat->max_nelem, (double)stat->sum_nelem/count);
    printf("- fill_ratio:    min = %5.1f%% ; max = %5.1f%% ; mean = %5.1f%%\n", stat->min_fill_ratio*100.0, stat->max_fill_ratio*100.0, (double)stat->sum_fill_ratio/count*100.0);
    printf("- collisions:    min = %6li ; max = %6li ; mean = %6.1f\n", stat->min_collisions, stat->max_collisions, (double)stat->sum_collisions/count);
    printf("- hash_quality:  min = %5.1f%% ; max = %5.1f%% ; mean = %5.1f%%\n", stat->min_hash_quality*100.0, stat->max_hash_quality*100.0, (double)stat->sum_hash_quality/count*100.0);
}

static void test_calc_hash_statistic(const GB_HASH *hs, const char *id, int print) {
    long   queues     = 0;
    long   collisions;
    double fill_ratio = (double)hs->nelem/hs->size;
    double hash_quality;

    for (size_t i = 0; i < hs->size; i++) {
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

    addto_hash_statistic_summary(hash_stat_man.get_stat_summary(id), hs->size, hs->nelem, collisions, fill_ratio, hash_quality);
}

static long test_numhash_count_elems(GB_NUMHASH *hs) {
    return hs->nelem;
}

static long insert_into_hash(const char *key, long val, void *cl_toHash) {
    GB_HASH *toHash = (GB_HASH*)cl_toHash;
    GBS_write_hash(toHash, key, val);
    return val;
}
static long erase_from_hash(const char *key, long val, void *cl_fromHash) {
    GB_HASH *fromHash = (GB_HASH*)cl_fromHash;
    long     val2     = GBS_read_hash(fromHash, key);

    if (val2 == val) {
        GBS_write_hash(fromHash, key, 0);
    }
    else {
        printf("value mismatch in hashes_are_equal(): key='%s' val: %li != %li\n", key, val2, val); // NEED_NO_COV
    }
    return val;
}

static bool hashes_are_equal(GB_HASH *h1, GB_HASH *h2) {
    size_t count1 = GBS_hash_elements(h1);
    size_t count2 = GBS_hash_elements(h2);

    bool equal = (count1 == count2);
    if (equal) {
        GB_HASH *copy = GBS_create_hash(count1, GB_MIND_CASE);
        
        GBS_hash_do_loop(h1, insert_into_hash, copy);
        GBS_hash_do_loop(h2, erase_from_hash, copy);

        equal = (GBS_hash_elements(copy) == 0);
        GBS_free_hash(copy);
    }
    return equal;
}

struct TestData : virtual Noncopyable {
    GB_HASH    *mind;
    GB_HASH    *ignore;
    GB_NUMHASH *num;

    TestData() {
        mind   = GBS_create_hash(100, GB_MIND_CASE);
        ignore = GBS_create_hash(100, GB_IGNORE_CASE);
        num    = GBS_create_numhash(100);
    }
    ~TestData() {
        GBS_free_numhash(num);
        GBS_free_hash(ignore);
        GBS_free_hash(mind);
    }

    void reset() {
        GBS_erase_hash(mind);
        GBS_erase_hash(ignore);
        GBS_erase_numhash(num);
    }

    GB_HASH *get_hash(bool case_sens) {
        return case_sens ? mind : ignore;
    }
};

static TestData TEST;

static size_t test_hash_count_value(GB_HASH *hs, long val) {
    size_t hsize    = hs->size;
    size_t count = 0;

    gb_assert(val != 0); // counting zero values makes no sense (cause these are not stored in the hash)

    for (size_t i = 0; i<hsize; ++i) {
        for (gbs_hash_entry *e=hs->entries[i]; e; e=e->next) {
            if (e->val == val) {
                ++count;
            }
        }
    }

    return count;
}

void TEST_GBS_write_hash() {
    TEST.reset();

    for (int case_sens = 0; case_sens <= 1; ++case_sens) {
        GB_HASH *hash = TEST.get_hash(case_sens);

        GBS_write_hash(hash, "foo", 1);
        TEST_EXPECT_EQUAL(GBS_hash_elements(hash), 1);
        TEST_EXPECT_EQUAL(GBS_read_hash(hash, "foo"), 1);

        GBS_write_hash(hash, "foo", 2);
        TEST_EXPECT_EQUAL(GBS_hash_elements(hash), 1);
        TEST_EXPECT_EQUAL(GBS_read_hash(hash, "foo"), 2);
        
        GBS_write_hash(hash, "foo", 0);
        TEST_EXPECT_ZERO(GBS_hash_elements(hash));
        TEST_EXPECT_ZERO(GBS_read_hash(hash, "foo"));

        GBS_write_hash(hash, "foo", 1);
        GBS_write_hash(hash, "FOO", 2);
        GBS_write_hash(hash, "BAR", 1);
        GBS_write_hash(hash, "bar", 2);

        if (case_sens) {
            TEST_EXPECT_EQUAL(GBS_hash_elements(hash), 4);

            TEST_EXPECT_EQUAL(GBS_read_hash(hash, "foo"), 1);
            TEST_EXPECT_EQUAL(GBS_read_hash(hash, "FOO"), 2);
            TEST_EXPECT_ZERO(GBS_read_hash(hash, "Foo"));
            
            TEST_EXPECT_EQUAL(test_hash_count_value(hash, 1), 2);
            TEST_EXPECT_EQUAL(test_hash_count_value(hash, 2), 2);
        }
        else {
            TEST_EXPECT_EQUAL(GBS_hash_elements(hash), 2);

            TEST_EXPECT_EQUAL(GBS_read_hash(hash, "foo"), 2);
            TEST_EXPECT_EQUAL(GBS_read_hash(hash, "FOO"), 2);
            TEST_EXPECT_EQUAL(GBS_read_hash(hash, "Foo"), 2);

            TEST_EXPECT_ZERO(test_hash_count_value(hash, 1));
            TEST_EXPECT_EQUAL(test_hash_count_value(hash, 2), 2);
        }

        if (case_sens) {
            TEST_EXPECT_ZERO(GBS_read_hash(hash, "foobar"));
            GBS_write_hash_no_strdup(hash, ARB_strdup("foobar"), 0);
            TEST_EXPECT_ZERO(GBS_read_hash(hash, "foobar"));
            GBS_write_hash_no_strdup(hash, ARB_strdup("foobar"), 3);
            TEST_EXPECT_EQUAL(GBS_read_hash(hash, "foobar"), 3);
            GBS_write_hash_no_strdup(hash, ARB_strdup("foobar"), 0);
            TEST_EXPECT_ZERO(GBS_read_hash(hash, "foobar"));
        }
    }
}

void TEST_GBS_incr_hash() {
    TEST.reset();

    for (int case_sens = 0; case_sens <= 1; ++case_sens) {
        GB_HASH *hash = TEST.get_hash(case_sens);

        GBS_incr_hash(hash, "foo");
        TEST_EXPECT_EQUAL(GBS_read_hash(hash, "foo"), 1);

        GBS_incr_hash(hash, "foo");
        TEST_EXPECT_EQUAL(GBS_read_hash(hash, "foo"), 2);

        GBS_incr_hash(hash, "FOO");
        TEST_EXPECT_EQUAL(GBS_read_hash(hash, "foo"), case_sens ? 2 : 3);
        TEST_EXPECT_EQUAL(GBS_read_hash(hash, "FOO"), case_sens ? 1 : 3);
    }
}

static void test_string_2_hashtab(GB_HASH *hash, char *data) {
    // modifies data
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

        str = (char *)ARB_calloc(sizeof(char), strlen+1);
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

void TEST_GBS_hashtab_2_string() {
    TEST.reset();

    for (int case_sens = 0; case_sens <= 1; ++case_sens) {
        GB_HASH *hash = TEST.get_hash(case_sens);

        GBS_write_hash(hash, "foo", 1);
        GBS_write_hash(hash, "bar", 2);
        GBS_write_hash(hash, "FOO", 3);
        GBS_write_hash(hash, "BAR", 4);
        GBS_write_hash(hash, "foo:bar", 3);
        GBS_write_hash(hash, "FOO:bar", 4);
    }
    for (int case_sens = 0; case_sens <= 1; ++case_sens) {
        GB_HASH *hash = TEST.get_hash(case_sens);
        
        char *as_string = GBS_hashtab_2_string(hash);
        TEST_REJECT_NULL(as_string);

        GB_HASH *hash2 = GBS_create_hash(1000, case_sens ? GB_MIND_CASE : GB_IGNORE_CASE);
        test_string_2_hashtab(hash2, as_string);
        TEST_EXPECT(hashes_are_equal(hash, hash2));
        TEST_EXPECT(hashes_are_equal(hash, hash2));

        free(as_string);
        GBS_free_hash(hash2);
    }

    {
        GB_HASH *hash      = TEST.get_hash(true);
        char    *as_string = GBS_hashtab_2_string(hash);

        GB_HASH *hash2 = GBS_create_hash(21, GB_MIND_CASE);
        GBS_hash_do_sorted_loop(hash, insert_into_hash, GBS_HCF_sortedByKey, hash2);
        
        GB_HASH *hash3 = GBS_create_hash(100, GB_MIND_CASE);
        GBS_hash_do_sorted_loop(hash, insert_into_hash, GBS_HCF_sortedByKey, hash3);

        char *as_string2 = GBS_hashtab_2_string(hash2); 
        char *as_string3 = GBS_hashtab_2_string(hash3); 

        TEST_EXPECT_EQUAL__BROKEN(as_string, as_string2, "FOO::bar:4 BAR:4 bar:2 foo:1 FOO:3 foo::bar:3 ");
        TEST_EXPECT_EQUAL        (as_string, as_string3);

        GBS_free_hash(hash3);
        GBS_free_hash(hash2);

        free(as_string3);
        free(as_string2);
        free(as_string);
    }
}

inline long key2val(long key, int pass) {
    long val;
    switch (pass) {
        case 1:
            val = key/3;
            break;
        case 2:
            val = key*17461;
            break;
        default :
            val = LONG_MIN;
            TEST_EXPECT(0); // NEED_NO_COV
            break;
    }
    return val;
}

void TEST_numhash() {
    GB_NUMHASH *numhash  = GBS_create_numhash(10);
    GB_NUMHASH *numhash2 = GBS_create_numhash(10);

    const long LOW  = -200;
    const long HIGH = 200;
    const long STEP = 17;

    long added = 0;
    for (int pass = 1; pass <= 2; ++pass) {
        added = 0;
        for (long key = LOW; key <= HIGH; key += STEP) {
            long val = key2val(key, pass);
            GBS_write_numhash(numhash, key, val);
            added++;
        }

        TEST_EXPECT_EQUAL(test_numhash_count_elems(numhash), added);

        for (long key = LOW; key <= HIGH; key += STEP) {
            TEST_EXPECT_EQUAL(key2val(key, pass), GBS_read_numhash(numhash, key));
        }
    }

    TEST_EXPECT_ZERO(GBS_read_numhash(numhash, -4711)); // not-existing entry

    // erase by overwrite:
    for (long key = LOW; key <= HIGH; key += STEP) {
        GBS_write_numhash(numhash2, key, GBS_read_numhash(numhash, key)); // copy numhash->numhash2
        GBS_write_numhash(numhash, key, (long)NULL);
    }
    TEST_EXPECT_EQUAL(test_numhash_count_elems(numhash2), added);
    TEST_EXPECT_ZERO(test_numhash_count_elems(numhash));

    GBS_free_numhash(numhash2);                     // free filled hash
    GBS_free_numhash(numhash);                      // free empty hash
}


static int freeCounter;
static void freeDynamicHashElem(long cl_ptr) {
    GBS_dynaval_free(cl_ptr);
    freeCounter++;
}

void TEST_GBS_dynaval_hash() {
    const int SIZE  = 10;
    const int ELEMS = 30;

    GB_HASH *dynahash = GBS_create_dynaval_hash(SIZE, GB_MIND_CASE, freeDynamicHashElem);

    for (int pass = 1; pass <= 2; ++pass) {
        freeCounter = 0;

        for (int i = 0; i<ELEMS; ++i) {
            char *val    = GBS_global_string_copy("value %i", i);
            char *oldval = (char*)GBS_write_hash(dynahash, GBS_global_string("key %i", i), (long)val);
            free(oldval);
        }

        TEST_EXPECT_ZERO(freeCounter); // overwriting values shall not automatically free them
    }

    freeCounter = 0;
    GBS_free_hash(dynahash);
    TEST_EXPECT_EQUAL(freeCounter, ELEMS);
}

void TEST_GBS_optimize_hash_and_stats() {
    const int SIZE = 10;
    const int FILL = 70;

    test_clear_hash_statistic_summary("test");
    for (int pass = 1; pass <= 3; ++pass) {
        GB_HASH *hash = GBS_create_hash(SIZE, GB_MIND_CASE);

        for (int i = 1; i <= FILL; ++i) {
            const char *key =  GBS_global_string("%i", i);
            GBS_write_hash(hash, key, i);
        }
        TEST_EXPECT(hash->nelem > hash->size); // ensure hash is overfilled!

        switch (pass) {
            case 1:                                 // nothing, only free overfilled hash below
                break;
            case 2:                                 // test overwrite overfilled hash
                for (int i = 1; i <= FILL; ++i) {
                    const char *key = GBS_global_string("%i", i);
                    
                    TEST_EXPECT_EQUAL(GBS_read_hash(hash, key), i);
                    GBS_write_hash(hash, key, 0);
                    TEST_EXPECT_ZERO(GBS_read_hash(hash, key));
                }
                break;
            case 3:                                 // test optimize
                GBS_optimize_hash(hash);
                TEST_EXPECT_LESS_EQUAL(hash->nelem, hash->size);
                break;
            default :
                TEST_EXPECT(0);                     // NEED_NO_COV
                break;
        }

        test_calc_hash_statistic(hash, "test", 1);
        GBS_free_hash(hash);
    }

    test_print_hash_statistic_summary("test");
}

static bool has_value(const char *, long val, void *cd) { return val == (long)cd; }
static bool has_value_greater(const char *, long val, void *cd) { return val > (long)cd; }

void TEST_GBS_hash_next_element_that() {
    TEST.reset();

    for (int case_sens = 0; case_sens <= 1; ++case_sens) {
        GB_HASH *hash = TEST.get_hash(case_sens);

        GBS_write_hash(hash, "foo", 0);
        GBS_write_hash(hash, "bar", 1);
        GBS_write_hash(hash, "foobar", 2);
        GBS_write_hash(hash, "barfoo", 3);

#define READ_REVERSE(value) GBS_hash_next_element_that(hash, NULL, has_value, (void*)value)
#define ASSERT_READ_REVERSE_RETURNS(value, expected) TEST_EXPECT_EQUAL((const char *)expected, READ_REVERSE(value));

        ASSERT_READ_REVERSE_RETURNS(0, NULL);
        ASSERT_READ_REVERSE_RETURNS(1, "bar");
        ASSERT_READ_REVERSE_RETURNS(2, "foobar");
        ASSERT_READ_REVERSE_RETURNS(3, "barfoo");
        ASSERT_READ_REVERSE_RETURNS(4, NULL);

        const char *key = NULL;
        long        sum = 0;

        for (int iter = 1; iter <= 3; ++iter) {
            key = GBS_hash_next_element_that(hash, key, has_value_greater, (void*)1);
            if (iter == 3) TEST_REJECT(key);
            else {
                TEST_REJECT_NULL(key);
                sum += GBS_read_hash(hash, key);
            }
        }
        TEST_EXPECT_EQUAL(sum, 5); // sum of all values > 1
    }
}

const size_t MAX_PRIME  = sorted_primes[KNOWN_PRIMES-1];

static size_t get_overflown_prime() { return gbs_get_a_prime(MAX_PRIME+1); }
#if defined(ASSERTION_USED)
static void detect_prime_overflow() { get_overflown_prime(); }
#endif // ASSERTION_USED

void TEST_hash_specials() {
    const size_t SOME_PRIME = 434201;
    TEST_EXPECT_EQUAL(gbs_get_a_prime(SOME_PRIME), SOME_PRIME);
    TEST_EXPECT_EQUAL(gbs_get_a_prime(MAX_PRIME), MAX_PRIME);

#if defined(ASSERTION_USED)
    TEST_EXPECT_CODE_ASSERTION_FAILS(detect_prime_overflow);
#else
    TEST_EXPECT_EQUAL(get_overflown_prime(), MAX_PRIME+1);
#endif // ASSERTION_USED
}

#endif // UNIT_TESTS



