/********************************************************************************************
            Some Hash/Cash Procedures
********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
/* #include <malloc.h> */
#include <string.h>
#include <ctype.h>

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


GB_HASH *GBS_create_hash(long size,int upper_case){
    /* Create a hash of size size, this hash is using linked list to avoid collisions,
     *  if upper_case = 0 then 'a!=A'
     *  else if upper_case = 1 then 'a==A'
     *  */
    struct gbs_hash_struct *hs;
    hs = (struct gbs_hash_struct *)GB_calloc(sizeof(struct gbs_hash_struct),1);
    hs->size = size;
    hs->nelem = 0;
    hs->upper_case = upper_case;
    hs->entries = (struct gbs_hash_entry **)GB_calloc(sizeof(struct gbs_hash_entry *),(size_t)size);
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
    so the user have to 'malloc' the string and give control to the hash functions !!!! */
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
    register long    i;
    register long    e2;
    struct gbs_hash_entry *e, *ee;
    e2 = hs->size;
    for (i = 0; i < e2; i++) {
        for (e = hs->entries[i]; e; e = ee) {
            free(e->key);
            ee = e->next;
            gbm_free_mem((char *)e,sizeof(struct gbs_hash_entry),GBM_HASH_INDEX);
        }
        hs->entries[i] = 0;
    }
}

void GBS_free_hash(GB_HASH *hs)
{
    if (!hs) return;
    GBS_free_hash_entries(hs);
    free((char *)hs->entries);
    free((char *)hs);
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
