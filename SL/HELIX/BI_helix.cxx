#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>

#ifdef _USE_AW_WINDOW
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#endif

#include "BI_helix.hxx"

#define LEFT_HELIX "{[<("
#define RIGHT_HELIX "}]>)"
#define LEFT_NONS "#*abcdefghij"
#define RIGHT_NONS "#*ABCDEFGHIJ"

char BI_helix::error[256];

struct helix_stack {
    struct helix_stack *next;
    long    pos;
    BI_PAIR_TYPE type;
    char    c;
};

void BI_helix::_init(void)
{
    int i;
    for (i=0;i<HELIX_MAX; i++) pairs[i] = 0;
    for (i=0;i<HELIX_MAX; i++) char_bind[i] = 0;

    entries = 0;
    size = 0;

    pairs[HELIX_NONE]=strdup("");
    char_bind[HELIX_NONE] = strdup(" ");

    pairs[HELIX_STRONG_PAIR]=strdup("CG");
    char_bind[HELIX_STRONG_PAIR] = strdup("-");

    pairs[HELIX_PAIR]=strdup("AU GU");
    char_bind[HELIX_PAIR] = strdup("-");

    pairs[HELIX_WEAK_PAIR]=strdup("GA GT");
    char_bind[HELIX_WEAK_PAIR] = strdup(".");

    pairs[HELIX_NO_PAIR]=strdup("AA AC CC CT CU TU");
    char_bind[HELIX_NO_PAIR] = strdup("#");

    pairs[HELIX_USER0]=strdup(".A .C .G .T .U");
    char_bind[HELIX_USER0] = strdup("?");

    pairs[HELIX_USER1]=strdup("-A -C -G -T -U");
    char_bind[HELIX_USER1] = strdup("#");

    pairs[HELIX_USER2]=strdup(".. --");
    char_bind[HELIX_USER2] = strdup(" ");

    pairs[HELIX_USER3]=strdup(".-");
    char_bind[HELIX_USER3] = strdup(" ");

    pairs[HELIX_DEFAULT]=strdup("");
    char_bind[HELIX_DEFAULT] = strdup("?");

    for (i=HELIX_NON_STANDART0;i<=HELIX_NON_STANDART9;i++){
        pairs[i] = strdup("");
        char_bind[i] = strdup("");
    }

    pairs[HELIX_NO_MATCH]=strdup("");
    char_bind[HELIX_NO_MATCH] = strdup("|");

    // deleteable = 1;
}

BI_helix::BI_helix(void) {
    _init();
}

BI_helix::~BI_helix(void){
    // if (!deleteable){
        // GB_warning("Internal program error: You cannot delete BI_helix !!");
    // }
    unsigned i;
    for (i=0;i<HELIX_MAX; i++)  free(pairs[i]);
    for (i=0;i<HELIX_MAX; i++)  free(char_bind[i]);

    if (entries) {
        for (i = 0; i<size; ++i) {
            if (entries[i].helix_nr && entries[i].helix_nr[0] != '-') {
#if defined(DEBUG)
                unsigned pair_pos   = entries[i].pair_pos;
                bi_assert(entries[pair_pos].helix_nr != 0);
                bi_assert(entries[pair_pos].helix_nr[0] == '-');
                bi_assert(entries[pair_pos].helix_nr == (entries[i].helix_nr-1));
#endif // DEBUG
                entries[i].helix_nr = 0; // those NOT starting with '-' are only pointers into pairing helix_nr
            }
        }

        char *to_free = 0;      // each position in one helix points to the same string (free it only once!)

        for (i = 0; i<size; ++i) {
            if (entries[i].helix_nr) {
#if defined(DEBUG)
                bi_assert(entries[i].helix_nr != 0);
                bi_assert(entries[i].helix_nr[0] == '-');
                unsigned pair_pos = entries[i].pair_pos;
                bi_assert(entries[pair_pos].helix_nr == 0);
#endif // DEBUG
                if (to_free != entries[i].helix_nr) {
                    free(to_free);
                    to_free = entries[i].helix_nr;
                }
            }
        }

        free(to_free);
        free(entries);
    }
}

extern "C" long BI_helix_check_error(const char *key, long val)
{
    struct helix_stack *stack = (struct helix_stack *)val;
    if (BI_helix::error[0]) return val;
    if (stack) {
        sprintf(BI_helix::error,"Too many '%c' in Helix '%s' pos %li",
                stack->c, key, stack->pos);
    }
    return val;
}


extern "C" long BI_helix_free_hash(const char *key, long val)
{
    key = key;
    struct helix_stack *stack = (struct helix_stack *)val;
    struct helix_stack *next;
    for ( ; stack; stack = next) {
        next = stack->next;
        delete stack;
    }
    return 0;
}

const char *BI_helix::init(char *helix_nr, char *helix, size_t sizei)
    /* helix_nr string of helix identifiers
   helix    helix
   size     alignment len
*/
{
    GB_HASH *hash = GBS_create_hash(256,1);
    size_t pos;
    char c;
    char ident[256];
    char *sident;
    struct helix_stack *laststack = 0,*stack;

    size = sizei;

    {
        size_t len = strlen(helix);
        if (len > size) len = size;
        char *h = (char *)malloc(size+1);
        h[size] = 0;
        if (len<size) memset(h+len,'.', size-len);
        memcpy(h,helix,len);
        helix = h;
    }

    if (helix_nr) {
        size_t len = strlen(helix_nr);
        if (len > size) len = (int)size;
        char *h = (char *)malloc((int)size+1);
        h[size] = 0;
        if (len<size) memset(h+len,'.',(int)(size-len));
        memcpy(h,helix_nr,len);
        helix_nr = h;
    }

    error[0] = 0;

    strcpy(ident,"0");
    entries = (struct BI_helix_entry *)GB_calloc(sizeof(struct BI_helix_entry),(size_t)size);
    sident  = 0;
    for (pos = 0; pos < size; pos ++ ) {
        if (helix_nr) {
            if ( isdigit(helix_nr[pos])) {
                int j;
                for (j=0;j<3 && pos+j<size;j++) {
                    c = helix_nr[pos+j];
                    helix_nr[pos+j] = ' ';
                    if ( isalnum(c) ) {
                        ident[j] = c;
                        ident[j+1] = 0;
                        if ( isdigit(c) ) continue;
                    }
                    break;
                }
            }
        }
        c = helix[pos];
        if (strchr(LEFT_HELIX,c) || strchr(LEFT_NONS,c)  ){ // push
            laststack = (struct helix_stack *)GBS_read_hash(hash,ident);
            stack = new helix_stack;
            stack->next = laststack;
            stack->pos = pos;
            stack->c = c;
            GBS_write_hash(hash,ident,(long)stack);
        }
        else if (strchr(RIGHT_HELIX,c) || strchr(RIGHT_NONS,c) ){   // pop
            stack = (struct helix_stack *)GBS_read_hash(hash,ident);
            if (!stack) {
                sprintf(error,"Too many '%c' in Helix '%s' pos %i",c,ident,pos);
                goto helix_end;
            }
            if (strchr(RIGHT_HELIX,c)) {
                entries[pos].pair_type = HELIX_PAIR;
                entries[stack->pos].pair_type = HELIX_PAIR;
            }else{
                c = tolower(c);
                if (stack->c != c) {
                    sprintf(error,"Character '%c' pos %li don't match character '%c' pos %i in Helix '%s'",
                            stack->c,stack->pos,c,pos,ident);
                    goto helix_end;
                }
                if (isalpha(c)) {
                    entries[pos].pair_type = (BI_PAIR_TYPE)(HELIX_NON_STANDART0+c-'a');
                    entries[stack->pos].pair_type = (BI_PAIR_TYPE)(HELIX_NON_STANDART0+c-'a');
                }else{
                    entries[pos].pair_type = HELIX_NO_PAIR;
                    entries[stack->pos].pair_type = HELIX_NO_PAIR;
                }
            }
            entries[pos].pair_pos = stack->pos;
            entries[stack->pos].pair_pos = pos;
            GBS_write_hash(hash,ident,(long)stack->next);

            if (sident == 0 || strcmp(sident+1,ident) != 0) {
                sident = (char*)malloc(strlen(ident)+2);
                sprintf(sident,"-%s",ident);
            }
            entries[pos].helix_nr        = sident+1;
            entries[stack->pos].helix_nr = sident;
            bi_assert((long)pos != stack->pos);

            delete stack;
        }
    }

    GBS_hash_do_loop(hash,BI_helix_check_error);

 helix_end:;
    GBS_hash_do_loop(hash,BI_helix_free_hash);
    GBS_free_hash(hash);
    free(helix_nr);
    free(helix);
    if (error[0]) return error;
    return 0;
}


const char *BI_helix::init(GBDATA *gb_helix_nr,GBDATA *gb_helix,size_t sizei)
{
    if (gb_helix) GB_push_transaction(gb_helix);
    char *helix_nr = 0;
    char *helix = 0;
    const char *err = 0;
    if (!gb_helix) err =  "Cannot find the helix";
    else if (gb_helix_nr) helix_nr = GB_read_string(gb_helix_nr);
    if (!err) {
        helix = GB_read_string(gb_helix);
        err = init(helix_nr,helix,sizei);
    }
    free(helix_nr);
    free(helix);
    if (gb_helix) GB_pop_transaction(gb_helix);
    return err;
}

const char *BI_helix::init(GBDATA *gb_main, const char *alignment_name, const char *helix_nr_name, const char *helix_name)
{
    const char *err = 0;
    GB_push_transaction(gb_main);
    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    long size2 = GBT_get_alignment_len(gb_main,alignment_name);
    if (size2<=0) err = (char *)GB_get_error();
    if (!err) {
        GBDATA *gb_helix_nr_con = GBT_find_SAI_rel_exdata(gb_extended_data, helix_nr_name);
        GBDATA *gb_helix_con = GBT_find_SAI_rel_exdata(gb_extended_data, helix_name);
        GBDATA *gb_helix = 0;
        GBDATA *gb_helix_nr = 0;

        if (gb_helix_nr_con)    gb_helix_nr = GBT_read_sequence(gb_helix_nr_con,alignment_name);
        if (gb_helix_con)       gb_helix = GBT_read_sequence(gb_helix_con,alignment_name);

        err = init(gb_helix_nr, gb_helix, size2);
    }
    GB_pop_transaction(gb_main);
    return err;
}

const char *BI_helix::init(GBDATA *gb_main)
{
    const char *err = 0;
    GB_push_transaction(gb_main);
    char *helix = GBT_get_default_helix(gb_main);
    char *helix_nr = GBT_get_default_helix_nr(gb_main);
    char *use = GBT_get_default_alignment(gb_main);
    err =   init(gb_main,use,helix_nr,helix);
    GB_pop_transaction(gb_main);
    free(helix);
    free(helix_nr);
    free(use);

    return err;

}

extern "C" {
    char *GB_give_buffer(long size);
}

int BI_helix::_check_pair(char left, char right, BI_PAIR_TYPE pair_type)
{
    int i;
    int len = strlen(pairs[pair_type])-1;
    char *pai = pairs[pair_type];
    for (i=0; i<len;i+=3){
        if ( pai[i] == left && pai[i+1] == right) return 1;
        if ( pai[i] == right && pai[i+1] == left) return 1;
    }
    return 0;
}

int BI_helix::check_pair(char left, char right, BI_PAIR_TYPE pair_type)
    // returns 2 helix 1 weak helix 0 no helix  -1 nothing
{
    left = toupper(left);
    right = toupper(right);
    switch(pair_type) {
        case HELIX_PAIR:
            if (    _check_pair(left,right,HELIX_STRONG_PAIR) ||
                    _check_pair(left,right,HELIX_PAIR) ) return 2;
            if (    _check_pair(left,right,HELIX_WEAK_PAIR) ) return 1;
            return 0;
        case HELIX_NO_PAIR:
            if (    _check_pair(left,right,HELIX_STRONG_PAIR) ||
                    _check_pair(left,right,HELIX_PAIR) ) return 0;
            return 1;
        default:
            return _check_pair(left,right,pair_type);
    }
}


/***************************************************************************************
*******         Reference to abs pos                    ********
****************************************************************************************/
void BI_ecoli_ref::bi_exit(void){
    delete [] _abs_2_rel1;
    delete [] _abs_2_rel2;
    delete [] _rel_2_abs;
    _abs_2_rel1 = 0;
    _abs_2_rel2 = 0;
    _rel_2_abs = 0;
}

BI_ecoli_ref::BI_ecoli_ref(void)
{
    memset((char *)this,0,sizeof(BI_ecoli_ref));
}

BI_ecoli_ref::~BI_ecoli_ref(void){
    bi_exit();
}

const char *BI_ecoli_ref::init(char *seq,long size)
{
    bi_exit();
    
    _abs_2_rel1 = new long[size];
    _abs_2_rel2 = new long[size];
    _rel_2_abs  = new long[size];
    memset((char *)_rel_2_abs,0,(size_t)(sizeof(long)*size));

    _rel_len = 0;
    _abs_len = size;
    long rel_len2 = 0;
    long i;
    long sl = strlen(seq);
    for (i= 0;i<size;i++) {
        _abs_2_rel1[i] = _rel_len;
        _abs_2_rel2[i] = rel_len2;
        _rel_2_abs[_rel_len] = i;
        if (i>=sl || seq[i] == '.' || seq[i] == '-') {
            rel_len2++;
        }else{
            rel_len2 = 0;
            _rel_len ++;
        }
    }
    return 0;
}

const char *BI_ecoli_ref::init(GBDATA *gb_main,char *alignment_name, char *ref_name)
{
    const char *err = 0;
    GB_transaction dummy(gb_main);
    long size = GBT_get_alignment_len(gb_main,alignment_name);
    if (size<=0) err = (char*)GB_get_error();
    if (!err) {
        GBDATA *gb_ref_con =
            GBT_find_SAI(gb_main, ref_name);
        GBDATA *gb_ref = 0;
        if (gb_ref_con){
            gb_ref = GBT_read_sequence(gb_ref_con,alignment_name);
            if (gb_ref){
                err = init(GB_read_char_pntr(gb_ref), size);
            }else{
                err = (char *)GB_export_error("Your SAI '%s' has no sequence '%s/data'",
                                              ref_name,alignment_name);
            }
        }else{
            err = (char *)GB_export_error("I cannot find the SAI '%s'",ref_name);
        }
    }
    return err;
}

const char *BI_ecoli_ref::init(GBDATA *gb_main)
{
    const char *err = 0;
    GB_transaction dummy(gb_main);
    char *ref = GBT_get_default_ref(gb_main);
    char *use = GBT_get_default_alignment(gb_main);
    err =   init(gb_main,use,ref);
    free(ref);
    free(use);

    return err;

}


const char *BI_ecoli_ref::abs_2_rel(long in,long &out,long &add){
    if (!_abs_2_rel1) {
        out = in;
        add = 0;
        return "BI_ecoli_ref is not correctly initialized";
    }
    if (in >= _abs_len) in = _abs_len -1;
    if (in < 0 ) in  = 0;
    out = _abs_2_rel1[in];
    add = _abs_2_rel2[in];
    return 0;
}

const char *BI_ecoli_ref::rel_2_abs(long in,long add,long &out){
    if (!_abs_2_rel1) {
        out = in;
        return ("BI_ecoli_ref is not correctly initialized");
    }
    if (in >= _rel_len) in = _rel_len -1;
    if (in < 0 ) in  = 0;
    out = _rel_2_abs[in]+add;
    return 0;
}
