/********************************************************************************************
            Some Hash/Cash Procedures
********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
/* #include <malloc.h> */
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h> 

#include "adlocal.h"
/*#include "arbdb.h"*/

        /* memory management */

struct gbs_hash_entry {
    char *key;
    long val;
    struct gbs_hash_entry *next;
};
typedef struct gbs_hash_struct {
    long size;
    long nelem;
    int upper_case;

    int loop_pos;
    struct gbs_hash_entry *loop_entry;
    struct gbs_hash_entry **entries;
} gbs_hash;

struct gbs_hashi_entry {
    long key;
    long val;
    struct gbs_hashi_entry *next;
};

struct gbs_hashi_struct {
    long size;
    struct gbs_hashi_entry **entries;
};

/* prime numbers */

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


/* define CALC_PRIMES only to expand the above table */
#if defined(DEBUG)
/* #define CALC_PRIMES */
#endif /* DEBUG */

#ifdef CALC_PRIMES

#define CALC_PRIMES_UP_TO 100000000L
#define PRIME_UNDENSITY   20L   /* the higher, the less primes are stored */

#warning "please don't define CALC_PRIMES permanently"

static unsigned char bit_val[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

static int bit_value(const unsigned char *erastothenes, long num) { // 'num' is odd and lowest 'num' is 3
    long bit_num  = ((num-1) >> 1)-1; // 3->0 5->1 7->2 etc.
    long byte_num = bit_num >> 3; // div 8
    char byte     = erastothenes[byte_num];

    gb_assert(bit_num >= 0);
    gb_assert((num&1) == 1);    // has to odd

    bit_num = bit_num &  7;

    return (byte & bit_val[bit_num]) ? 1 : 0;
}
static void set_bit_value(unsigned char *erastothenes, long num, int val) { // 'num' is odd and lowest 'num' is 3; val is 0 or 1
    long bit_num  = ((num-1) >> 1)-1; // 3->0 5->1 7->2 etc.
    long byte_num = bit_num >> 3; // div 8
    char byte     = erastothenes[byte_num];

    gb_assert(bit_num >= 0);
    gb_assert((num&1) == 1);    // has to odd

    bit_num = bit_num &  7;

    if (val) {
        byte |= bit_val[bit_num];
    }
    else {
        byte &= (0xff - bit_val[bit_num]);
    }
    erastothenes[byte_num] = byte;
}

static void calculate_primes_upto() {
    {
        long           bits_needed  = CALC_PRIMES_UP_TO/2+1; // only need bits for odd numbers
        long           bytes_needed = (bits_needed/8)+1;
        unsigned char *erastothenes = GB_calloc(bytes_needed, 1); // bit = 1 means "is not a prime"
        long           prime_count  = 0;
        long           num;

        printf("erastothenes' size = %li\n", bytes_needed);

        if (!erastothenes) {
            GB_internal_error("out of memory");
            return;
        }

        for (num = 3; num <= CALC_PRIMES_UP_TO; num += 2) {
            if (bit_value(erastothenes, num) == 0) { // is a prime number
                long num2;
                prime_count++;
                for (num2 = num*2; num2 <= CALC_PRIMES_UP_TO; num2 += num) { // with all multiples
                    if ((num2&1) == 1) { // skip even numbers
                        set_bit_value(erastothenes, num2, 1);
                    }
                }
            }
            // otherwise it is no prime and all multiples are already set to 1
        }

        /* thin out prime numbers (we don't need all of them) */
        {
            long prime_count2 = 0;
            long last_prime   = -1000;
            int  index;
            int  printed      = 0;

            for (num = 3; num <= CALC_PRIMES_UP_TO; num += 2) {
                if (bit_value(erastothenes, num) == 0) { // is a prime number
                    long diff = num-last_prime;
                    if ((diff*PRIME_UNDENSITY)<num) {
                        set_bit_value(erastothenes, num, 1); // delete unneeded prime
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
                if (bit_value(erastothenes, num) == 0) { // is a prime number
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

        free(erastothenes);
    }
    fflush(stdout);
    exit(1);
}

#endif /* CALC_PRIMES */

long GBS_get_a_prime(long above_or_equal_this) {
    // return a prime number above_or_equal_this
    // NOTE: it is not necessarily the next prime number, because we don't calculate all prime numbers!
    
#if defined(CALC_PRIMES)
    calculate_primes_upto(above_or_equal_this);
#endif /* CALC_PRIMES */

    if (sorted_primes[KNOWN_PRIMES-1] >= above_or_equal_this) {
        int l = 0, h = KNOWN_PRIMES-1;
        
        while (l < h) {
            int m = (l+h)/2;
#if defined(DEBUG) && 0
            printf("l=%-3i m=%-3i h=%-3i above_or_equal_this=%li sorted_primes[%i]=%li sorted_primes[%i]=%li sorted_primes[%i]=%li\n",
                   l, m, h, above_or_equal_this, l, sorted_primes[l], m, sorted_primes[m], h, sorted_primes[h]);
#endif /* DEBUG */
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

    fprintf(stderr, "Warning: GBS_get_a_prime failed for value %li (performance bleed)\n", above_or_equal_this);
    gb_assert(0); // add more primes to sorted_primes[]

    return above_or_equal_this;
}

/********************************************************************************************
                    Some Hash Procedures for [string,long]
********************************************************************************************/

#define GB_CALC_HASH_INDEX(string,index,size) do {                          \
    register const char *local_ptr = (string);                              \
    register int local_i;                                                   \
    (index) = 0xffffffffL;                                                  \
    while ((local_i=(*(local_ptr++)))) {                                    \
        (index) = crctab[((int)(index)^local_i) & 0xff] ^ ((index) >> 8);   \
    }                                                                       \
    (index) = (index) % (size);                                             \
} while(0)

#define GB_CALC_HASH_INDEX_TO_UPPER(string,index,size) {                        \
        register const char *_ptr = string;                                     \
        register int _i;                                                        \
        index = 0xffffffffL; while ( (_i = *(_ptr++))){                         \
            index = crctab[((int) index ^ toupper(_i)) & 0xff] ^ (index >> 8);  \
        }                                                                       \
        index = index % size;                                                   \
    }


GB_HASH *GBS_create_hash(long user_size,int ignore_case) {
    /* Create a hash of size size, this hash is using linked list to avoid collisions,
     *  ignore_case == 0 -> 'a != A'
     *  ignore_case != 0 -> 'a == A'
     */
    struct gbs_hash_struct *hs;
    long                    size    = GBS_get_a_prime(user_size); // use next prime number for hash size

    hs             = (struct gbs_hash_struct *)GB_calloc(sizeof(struct gbs_hash_struct),1);
    hs->size       = size;
    hs->nelem      = 0;
    hs->upper_case = ignore_case;
    hs->entries    = (struct gbs_hash_entry **)GB_calloc(sizeof(struct gbs_hash_entry *),(size_t)size);
    return hs;
}

static void *gbs_save_hash_strstruct = 0;

long gbs_hash_to_strstruct(const char *key, long val){
    const char *p;
    int c;
    for ( p = key; (c=*p) ; p++) {
        GBS_chrcat(gbs_save_hash_strstruct,c);
        if (c==':') GBS_chrcat(gbs_save_hash_strstruct,c);
    }
    GBS_chrcat(gbs_save_hash_strstruct,':');
    GBS_intcat(gbs_save_hash_strstruct,val);
    GBS_chrcat(gbs_save_hash_strstruct,' ');
    return val;
}

char *GBS_hashtab_2_string(GB_HASH *hash) {
    gbs_save_hash_strstruct = GBS_stropen(1024);
    GBS_hash_do_loop(hash, gbs_hash_to_strstruct);
    return GBS_strclose(gbs_save_hash_strstruct);
}


char *GBS_string_2_hashtab(GB_HASH *hash, char *data){  /* destroys data */
    char *p,*d,*dp;
    int c;
    char *nextp;
    char *error = 0;
    char *str;
    int strlen;
    long val;

    for ( p = data; p ; p = nextp ){
        strlen = 0;
        for (dp = p; (c = *dp); dp++){
            if (c==':') {
                if (dp[1] == ':') dp++;
                else break;
            }
            strlen++;
        }
        if (*dp) {
            nextp = strchr(dp,' ');
            if (nextp) nextp++;
        }
        else break;

        str = (char *)GB_calloc(sizeof(char),strlen+1);
        for (dp = p, d = str; (c = *dp) ; dp++){
            if (c==':'){
                if (dp[1] == ':') {
                    *(d++) = c;
                    dp++;
                }else break;
            }else{
                *(d++) = c;
            }
        }
        val = atoi(dp+1);
        GBS_write_hash_no_strdup(hash,str,val);
    }

    return error;
}

long GBS_read_hash(GB_HASH *hs,const char *key)
{
    struct gbs_hash_entry *e;
    unsigned long i;
    if (hs->upper_case){
        GB_CALC_HASH_INDEX_TO_UPPER(key,i,hs->size);
        for(e=hs->entries[i];e;e=e->next){
            if (!strcasecmp(e->key,key)) return e->val;
        }
        return 0;
    }else{
        GB_CALC_HASH_INDEX(key,i,hs->size);
        for(e=hs->entries[i];e;e=e->next){
            if (!strcmp(e->key,key)) return e->val;
        }
        return 0;
    }
}


long GBS_write_hash(GB_HASH *hs,const char *key,long val)
{
    struct gbs_hash_entry *e;
    long i2;
    unsigned long i;
    if (hs->upper_case){
        GB_CALC_HASH_INDEX_TO_UPPER(key,i,hs->size);
        for(e=hs->entries[i];e;e=e->next){
            if (!strcasecmp(e->key,key)) break;
        }
    }else{
        GB_CALC_HASH_INDEX(key,i,hs->size);
        for(e=hs->entries[i];e;e=e->next){
            if (!strcmp(e->key,key)) break;
        }
    }
    if (e){
        i2 = e->val;
        if (!val) {
            hs->nelem--;
            if (hs->entries[i] == e) {
                hs->entries[i] = e->next;
            }else{
                struct gbs_hash_entry *ee;
                for (ee = hs->entries[i]; ee->next != e; ee= ee->next);
                if (ee->next ==e) {
                    ee->next = e->next;
                } else{
                    GB_internal_error("Database may be corrupt, hash tables error");
                }
            }
            free( e->key );
            gbm_free_mem((char *)e,sizeof(struct gbs_hash_entry),GBM_HASH_INDEX);
        }else{
            e->val = val;
        }
        return i2;
    }

    if (val == 0) return 0;

    e = (struct gbs_hash_entry *)gbm_get_mem(sizeof(struct gbs_hash_entry),GBM_HASH_INDEX);
    e->next = hs->entries[i];
    e->key = GB_STRDUP(key);
    e->val = val;
    hs->entries[i] = e;
    hs->nelem++;
    return 0;
}

long GBS_write_hash_no_strdup(GB_HASH *hs,char *key,long val)
     /* does no GB_STRDUP, but the string is freed later in GBS_free_hash,
    so the user has to 'malloc' the string and give control to the hash functions !!!! */
{
    struct gbs_hash_entry *e;
    long i2;
    unsigned long i;
    if (hs->upper_case){
        GB_CALC_HASH_INDEX_TO_UPPER(key,i,hs->size);
        for(e=hs->entries[i];e;e=e->next){
            if (!strcasecmp(e->key,key)) break;
        }
    }else{
        GB_CALC_HASH_INDEX(key,i,hs->size);
        for(e=hs->entries[i];e;e=e->next){
            if (!strcmp(e->key,key)) break;
        }
    }
    if (e){

        i2 = e->val;
        e->val = val;
        return i2;
    }

    e = (struct gbs_hash_entry *)gbm_get_mem(sizeof(struct gbs_hash_entry),GBM_HASH_INDEX);
    e->next = hs->entries[i];
    e->key = key;
    e->val = val;
    hs->entries[i] = e;
    hs->nelem++;
    return 0;
}

long GBS_incr_hash(GB_HASH *hs,const char *key)
{
    struct gbs_hash_entry *e;
    unsigned long i;
    if (hs->upper_case){
        GB_CALC_HASH_INDEX_TO_UPPER(key,i,hs->size);
        for(e=hs->entries[i];e;e=e->next){
            if (!strcasecmp(e->key,key)) break;
        }
    }else{
        GB_CALC_HASH_INDEX(key,i,hs->size);
        for(e=hs->entries[i];e;e=e->next){
            if (!strcmp(e->key,key)) break;
        }
    }
    if (e){

        e->val++;
        return e->val;
    }
    e = (struct gbs_hash_entry *)gbm_get_mem(sizeof(struct gbs_hash_entry),GBM_HASH_INDEX);
    e->next = hs->entries[i];
    e->key = (char *)GB_STRDUP(key);
    e->val = 1;
    hs->entries[i] = e;
    hs->nelem++;
    return 1;
}

void GBS_free_hash_entries(GB_HASH *hs)
{
    register long          i;
    register long          e2;
    struct gbs_hash_entry *e, *ee;
#if defined(DEBUG)
    int                    queues = 0;
#endif /* DEBUG */

    e2 = hs->size;
    for (i = 0; i < e2; i++) {
#if defined(DEBUG)
        if (hs->entries[i]) queues++;
#endif /* DEBUG */
        for (e = hs->entries[i]; e; e = ee) {
            free(e->key);
            ee = e->next;
            gbm_free_mem((char *)e,sizeof(struct gbs_hash_entry),GBM_HASH_INDEX);
        }
        hs->entries[i] = 0;
    }
#if defined(DEBUG)
    {
        long   collisions     = hs->nelem-queues;
        double collision_rate = (double)collisions/hs->nelem;

        if (collision_rate > 0.3) { // more than 30% collisions - increase hash ?
            printf("hash-size-warning: size=%li elements=%li collisions=%li collision_rate=%5.1f%%\n",
                   hs->size, hs->nelem, collisions, collision_rate*100.0);
        }
    }
#endif /* DEBUG */
}

void GBS_free_hash(GB_HASH *hs)
{
    if (!hs) return;
    GBS_free_hash_entries(hs);
    free((char *)hs->entries);
    free((char *)hs);
}

/* determine hash quality */

typedef struct {
    long   count;               // how many stats
    long   min_size, max_size, sum_size;
    long   min_nelem, max_nelem, sum_nelem;
    long   min_collisions, max_collisions, sum_collisions;
    double min_fill_ratio, max_fill_ratio, sum_fill_ratio;
    double min_hash_quality, max_hash_quality, sum_hash_quality;
} gbs_hash_statistic_summary;

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
    if (!stat_hash) stat_hash = GBS_create_hash(10, 0);
    found                     = GBS_read_hash(stat_hash, id);
    if (!found) {
        gbs_hash_statistic_summary *stat = GB_calloc(1, sizeof(*stat));
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
    long   i;
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
        printf("- size       = %li\n", hs->size);
        printf("- elements   = %li (fill ratio = %4.1f%%)\n", hs->nelem, fill_ratio*100.0);
        printf("- collisions = %li (hash quality = %4.1f%%)\n", collisions, hash_quality*100.0);
    }

    addto_hash_statistic_summary(get_stat_summary(id), hs->size, hs->nelem, collisions, fill_ratio, hash_quality);
}

void GBS_free_hash_entries_free_pointer(GB_HASH *hs)
{
    register long    i;
    register long    e2;
    struct gbs_hash_entry *e, *ee;
    e2 = hs->size;
    for (i = 0; i < e2; i++) {
        for (e = hs->entries[i]; e; e = ee) {
            free(e->key);
            if (e->val) free((char *)e->val);
            ee = e->next;
            gbm_free_mem((char *)e,sizeof(struct gbs_hash_entry),GBM_HASH_INDEX);
        }
        hs->entries[i] = 0;
    }
}
void GBS_free_hash_free_pointer(GB_HASH *hs)
{
    GBS_free_hash_entries_free_pointer(hs);
    free((char *)hs->entries);
    free((char *)hs);
}

void GBS_hash_do_loop(GB_HASH *hs, gb_hash_loop_type func)
{
    register long i,e2;
    struct gbs_hash_entry *e;
    e2 = hs->size;
    for (i=0;i<e2;i++) {
        for (e=hs->entries[i];e;e=e->next) {
            if (e->val) {
                e->val = func(e->key,e->val);
            }
        }
    }
}

void GBS_hash_do_loop2(GB_HASH *hs, gb_hash_loop_type2 func, void *parameter)
{
    register long i,e2;
    struct gbs_hash_entry *e;
    e2 = hs->size;
    for (i=0;i<e2;i++) {
        for (e=hs->entries[i];e;e=e->next) {
            if (e->val) {
                e->val = func(e->key,e->val, parameter);
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


void GBS_hash_next_element(GB_HASH *hs,const  char **key, long *val){
    struct gbs_hash_entry *e = hs->loop_entry;
    register long i,e2;
    if (!e){
        if (key) *key = 0;
        *val = 0;
        return;
    }
    if (key) *key = e->key;
    *val = e->val;
    e2 = hs->size;

    if (e->next){
        hs->loop_entry = e->next;
    }else{
        for (i=hs->loop_pos+1;i<e2;i++) {
            e=hs->entries[i];
            if (e){
                hs->loop_pos = i;
                hs->loop_entry = e;
/*                 GBS_hash_next_element(hs,key,val); */
                return;
            }
        }
    }
    hs->loop_entry = 0;
}

void GBS_hash_first_element(GB_HASH *hs,const char **key, long *val){
    struct gbs_hash_entry *e;
    register long i,e2;
    e2 = hs->size;
    for (i=0;i<e2;i++) {
        e=hs->entries[i];
        if (e){
            hs->loop_pos = i;
            hs->loop_entry = e;
            GBS_hash_next_element(hs,key,val);

            return;
        }
    }
    if (key) *key = 0;
    *val = 0;
    return;
}

gbs_hash_sort_func_type gbh_sort_func;

#ifdef __cplusplus
extern "C" {
#endif

    long g_bs_compare_two_items(void *v0, void *v1, char *unused) {
        struct gbs_hash_entry *e0 = (struct gbs_hash_entry*)v0;
        struct gbs_hash_entry *e1 = (struct gbs_hash_entry*)v1;
        GBUSE(unused);

        return gbh_sort_func(e0->key, e0->val, e1->key, e1->val);
    }

#ifdef __cplusplus
}
#endif

void GBS_hash_do_sorted_loop(GB_HASH *hs, gb_hash_loop_type func, gbs_hash_sort_func_type sorter) {
    register long   i, j, e2;
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
    gbh_sort_func = sorter;
    GB_mergesort((void **) mtab, 0, j, g_bs_compare_two_items,0);
    for (i = 0; i < j; i++) {
        func(mtab[i]->key, mtab[i]->val);
    }
    free((char *)mtab);
}

/********************************************************************************************
                    Some Hash Procedures for [long,long]
********************************************************************************************/

long gbs_hashi_index(long key, long size)
{
    register long x;
    x = (key*97)%size;
    if (x<0) x+= size;
    return x;
}


long GBS_create_hashi(long size)
{
    struct gbs_hashi_struct *hs;
    hs = (struct gbs_hashi_struct *)GB_calloc(sizeof(struct gbs_hashi_struct),1);
    hs->size = size;
    hs->entries = (struct gbs_hashi_entry **)GB_calloc(sizeof(struct gbs_hashi_entry *),(size_t)size);
    return (long)hs;
}


long GBS_read_hashi(long hashi,long key)
{
    struct gbs_hashi_struct *hs = (struct gbs_hashi_struct *)hashi;
    struct gbs_hashi_entry *e;
    long i;
    i = gbs_hashi_index(key,hs->size);
    for(e=hs->entries[i];e;e=e->next)
    {
        if (e->key==key) return e->val;
    }
    return 0;
}

long GBS_write_hashi(long hashi,long key,long val)
{
    struct gbs_hashi_struct *hs = (struct gbs_hashi_struct *)hashi;
    struct gbs_hashi_entry *e;
    long i2;
    long i;
    i = gbs_hashi_index(key,hs->size);
    if (!val) {
        struct gbs_hashi_entry *oe;
        oe = 0;
        for (e = hs->entries[i]; e; e = e->next) {
            if (e->key == key) {
                if (oe) {
                    oe->next = e->next;
                } else {
                    hs->entries[i] = e->next;
                }
                gbm_free_mem((char *) e, sizeof(struct gbs_hashi_entry),GBM_HASH_INDEX);
                return 0;
            }
            oe = e;
        }
        printf("free %lx not found\n",(long)e);
        return 0;
    }
    for(e=hs->entries[i];e;e=e->next)
    {
        if (e->key==key) {
            i2 = e->val;
            e->val = val;
            return i2;
        }
    }
    e = (struct gbs_hashi_entry *)gbm_get_mem(sizeof(struct gbs_hashi_entry),GBM_HASH_INDEX);
    e->next = hs->entries[i];
    e->key = key;
    e->val = val;
    hs->entries[i] = e;
    return 0;
}

long GBS_free_hashi(long hash)
{
    struct gbs_hashi_struct *hs = (struct gbs_hashi_struct *)hash;
    register long i;
    register long e2;
    struct gbs_hashi_entry *e,*ee;
    e2 = hs->size;
    for (i=0;i<e2;i++) {
        for (e=hs->entries[i];e;e=ee) {
            ee = e->next;
            gbm_free_mem((char *)e,sizeof(struct gbs_hashi_entry),GBM_HASH_INDEX);
        }
    }
    free ((char *)hs->entries);
    free ((char *)hs);
    return 0;
}



/********************************************************************************************
            Cache Cache Cache
********************************************************************************************/

void gb_init_cache(GB_MAIN_TYPE *Main){
    int i;
    if (Main->cache.entries) return;
    Main->cache.entries = (struct gb_cache_entry_struct *)GB_calloc(sizeof(struct gb_cache_entry_struct),
                                                                    GB_MAX_CACHED_ENTRIES);
    Main->cache.max_data_size = GB_TOTAL_CACHE_SIZE;
    Main->cache.max_entries = GB_MAX_CACHED_ENTRIES;
    for (i=0;i<GB_MAX_CACHED_ENTRIES-1;i++) {
        Main->cache.entries[i].next = i+1;
    }
    Main->cache.firstfree_entry = 1;
}

char *gb_read_cache(GBDATA *gbd) {
    GB_MAIN_TYPE *Main;
    register struct gb_cache_struct *cs;
    register long i;
    register long n,p;
    if (!(i=gbd->cache_index)) return 0;
    Main = GB_MAIN(gbd);
    cs = &Main->cache;
    n = cs->entries[i].next; p = cs->entries[i].prev;
    /* remove entry from list */
    if (i == cs->newest_entry) cs->newest_entry = n;
    if (i == cs->oldest_entry) cs->oldest_entry = p;
    cs->entries[n].prev = p;
    cs->entries[p].next = n;
    /* check validity */
    if (GB_GET_EXT_UPDATE_DATE(gbd) > cs->entries[i].clock) {
        free( cs->entries[i].data) ;
        cs->entries[i].data = 0;
        cs->sum_data_size -= cs->entries[i].sizeof_data;

        gbd->cache_index = 0;

        /* insert deleted entry in free list */
        cs->entries[i].next = cs->firstfree_entry;
        cs->firstfree_entry = i;
        return 0;
    }

    /* insert entry on top of list */
    cs->entries[i].next = cs->newest_entry;
    cs->entries[cs->newest_entry].prev = i;
    cs->newest_entry = i;
    cs->entries[i].prev = 0;
    if (!cs->oldest_entry) cs->oldest_entry = i;

    return cs->entries[i].data;
}

void *gb_free_cache(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    register struct gb_cache_struct *cs;
    register long i;
    register long n,p;
    if (!(i=gbd->cache_index)) return 0;
    cs = &Main->cache;
    n = cs->entries[i].next; p = cs->entries[i].prev;
    /* remove entry from list */
    if (i == cs->newest_entry) cs->newest_entry = n;
    if (i == cs->oldest_entry) cs->oldest_entry = p;
    cs->entries[n].prev = p;
    cs->entries[p].next = n;

    /* free cache */
    free( cs->entries[i].data) ;
    cs->entries[i].data = 0;
    cs->sum_data_size -= cs->entries[i].sizeof_data;

    gbd->cache_index = 0;

    /* insert deleted entry in free list */
    cs->entries[i].next = cs->firstfree_entry;
    cs->firstfree_entry = i;
    return 0;
}

char *delete_old_cache_entries(struct gb_cache_struct *cs, long needed_size, long max_data_size)
     /* call with max_data_size==0 to flush cache */
{
    register long n,p;
    register long i;
    char *data = 0;

    while ( ( (!cs->firstfree_entry) || ( needed_size + cs->sum_data_size >= max_data_size))
            && cs->oldest_entry) {
        i = cs->oldest_entry;
        n = cs->entries[i].next; p = cs->entries[i].prev;
        /* remove entry from list */
        if (i == cs->newest_entry) cs->newest_entry = n;
        if (i == cs->oldest_entry) cs->oldest_entry = p;
        cs->entries[n].prev = p;
        cs->entries[p].next = n;

        /* insert deleted entry in free list */
        cs->entries[i].gbd->cache_index = 0;
        cs->entries[i].next = cs->firstfree_entry;
        cs->firstfree_entry = i;
        /* delete all unused memorys */
        if (data || ( needed_size != cs->entries[i].sizeof_data)  ) {
            free(cs->entries[i].data);
        }else{
            data = cs->entries[i].data;
        }
        cs->sum_data_size -= cs->entries[i].sizeof_data;
        cs->entries[i].data = 0;
    }
    return data;
}

char *gb_flush_cache(GBDATA *gbd)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    register struct gb_cache_struct *cs = &Main->cache;

    delete_old_cache_entries(cs, 0, 0);
    return 0;
}

char *gb_alloc_cache_index(GBDATA *gbd,long size) {
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    register struct gb_cache_struct *cs = &Main->cache;
    register long i;
    char *data = 0;

    data = delete_old_cache_entries(cs, size, cs->max_data_size); /* delete enough old memory */

    i = cs->firstfree_entry;
    if (!i) {
        GB_internal_error("internal cache error");
        return 0;
    }

    /* get free element */
    cs->firstfree_entry = cs->entries[i].next;
    /* insert it on top of used list */
    cs->entries[i].next = cs->newest_entry;
    cs->entries[cs->newest_entry].prev = i;
    cs->newest_entry = i;
    cs->entries[i].prev = 0;
    if (!cs->oldest_entry) cs->oldest_entry = i;

    /* create data */
    cs->sum_data_size += size;
    if (!data) data = (char *) malloc((int)size);
    cs->entries[i].sizeof_data = (int)size;
    cs->entries[i].data = data;
    cs->entries[i].gbd = gbd;
    gbd->cache_index = (short)i;

    return data;
}

char *GB_set_cache_size(GBDATA *gbd, long size){
    GB_MAIN(gbd)->cache.max_data_size = size;
    return 0;
}
