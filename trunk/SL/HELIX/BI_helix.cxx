#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>

#include "BI_helix.hxx"

#define LEFT_HELIX "{[<("
#define RIGHT_HELIX "}]>)"
#define LEFT_NONS "#*abcdefghij"
#define RIGHT_NONS "#*ABCDEFGHIJ"

char *BI_helix::helix_error = 0;

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
    Size = 0;

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

    for (i=HELIX_NON_STANDARD0;i<=HELIX_NON_STANDARD9;i++){
        pairs[i] = strdup("");
        char_bind[i] = strdup("");
    }

    pairs[HELIX_NO_MATCH]=strdup("");
    char_bind[HELIX_NO_MATCH] = strdup("|");
}

BI_helix::BI_helix(void) {
    _init();
}

BI_helix::~BI_helix(void){
    unsigned i;
    for (i=0;i<HELIX_MAX; i++)  free(pairs[i]);
    for (i=0;i<HELIX_MAX; i++)  free(char_bind[i]);

    if (entries) {
        for (i = 0; i<Size; ++i) {
            if (entries[i].allocated) {
                free(entries[i].helix_nr);
            }
        }
        free(entries);
    }
}

extern "C" long BI_helix_check_error(const char *key, long val)
{
    struct helix_stack *stack = (struct helix_stack *)val;
    if (!BI_helix::get_error() && stack) { // don't overwrite existing error
        BI_helix::set_error(GBS_global_string("Too many '%c' in Helix '%s' pos %li", stack->c, key, stack->pos));
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

const char *BI_helix::initFromData(const char *helix_nr, const char *helix, size_t sizei)
/* helix_nr string of helix identifiers
   helix    helix
   size     alignment len
*/
{
    clear_error();
    
    GB_HASH *hash = GBS_create_hash(256,1);
    size_t pos;
    char c;
    char ident[256];
    char *sident;
    struct helix_stack *laststack = 0,*stack;

    Size = sizei;

    {
        size_t len = strlen(helix);
        if (len > Size) len = Size;
        char *h = (char *)malloc(Size+1);
        h[Size] = 0;
        if (len<Size) memset(h+len,'.', Size-len);
        memcpy(h,helix,len);
        helix = h;
    }

    if (helix_nr) {
        size_t len = strlen(helix_nr);
        if (len > Size) len = (int)Size;
        char *h = (char *)malloc((int)Size+1);
        h[Size] = 0;
        if (len<Size) memset(h+len,'.',(int)(Size-len));
        memcpy(h,helix_nr,len);
        helix_nr = h;
    }

    strcpy(ident,"0");
    long pos_scanned_till = -1;

    entries = (struct BI_helix_entry *)GB_calloc(sizeof(struct BI_helix_entry),(size_t)Size);
    sident  = 0;
    
    for (pos = 0; pos < Size; pos ++ ) {
        if (helix_nr) {
            if (long(pos)>pos_scanned_till && isalnum(helix_nr[pos])) {
                for (int j=0; (pos+j)<Size; j++) {
                    char hn = helix_nr[pos+j];
                    if (isalnum(hn)) {
                        ident[j] = hn;
                    }
                    else {
                        ident[j]         = 0;
                        pos_scanned_till = pos+j;
                        break;
                    }
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
                bi_assert(!helix_error); // already have an error
                helix_error = GBS_global_string_copy("Too many '%c' in Helix '%s' pos %i", c, ident, pos);
                goto helix_end;
            }
            if (strchr(RIGHT_HELIX,c)) {
                entries[pos].pair_type = HELIX_PAIR;
                entries[stack->pos].pair_type = HELIX_PAIR;
            }else{
                c = tolower(c);
                if (stack->c != c) {
                    bi_assert(!helix_error); // already have an error
                    helix_error = GBS_global_string_copy("Character '%c' pos %li doesn't match character '%c' pos %i in Helix '%s'",
                                                         stack->c, stack->pos, c, pos, ident);
                    goto helix_end;
                }
                if (isalpha(c)) {
                    entries[pos].pair_type = (BI_PAIR_TYPE)(HELIX_NON_STANDARD0+c-'a');
                    entries[stack->pos].pair_type = (BI_PAIR_TYPE)(HELIX_NON_STANDARD0+c-'a');
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
                
                entries[stack->pos].allocated = true;
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

    return get_error();
}


const char *BI_helix::init(GBDATA *gb_helix_nr, GBDATA *gb_helix, size_t sizei) {
    clear_error();
    
    if (!gb_helix) set_error("Can't find SAI:HELIX");
    else if (!gb_helix_nr) set_error("Can't find SAI:HELIX_NR");
    else {
        GB_transaction ta(gb_helix);
        initFromData(GB_read_char_pntr(gb_helix_nr), GB_read_char_pntr(gb_helix), sizei);
    }
    
    return get_error();
}

const char *BI_helix::init(GBDATA *gb_main, const char *alignment_name, const char *helix_nr_name, const char *helix_name)
{
    GB_transaction ta(gb_main);
    clear_error();

    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    long    size2            = GBT_get_alignment_len(gb_main,alignment_name);

    if (size2<=0) {
        set_error(GB_get_error());
    }
    else {
        GBDATA *gb_helix_nr_con = GBT_find_SAI_rel_exdata(gb_extended_data, helix_nr_name);
        GBDATA *gb_helix_con = GBT_find_SAI_rel_exdata(gb_extended_data, helix_name);
        GBDATA *gb_helix = 0;
        GBDATA *gb_helix_nr = 0;

        if (gb_helix_nr_con)    gb_helix_nr = GBT_read_sequence(gb_helix_nr_con,alignment_name);
        if (gb_helix_con)       gb_helix = GBT_read_sequence(gb_helix_con,alignment_name);

        init(gb_helix_nr, gb_helix, size2);
    }

    return get_error();
}

const char *BI_helix::init(GBDATA *gb_main)
{
    GB_transaction ta(gb_main);
    clear_error();
    
    char *helix    = GBT_get_default_helix(gb_main);
    char *helix_nr = GBT_get_default_helix_nr(gb_main);
    char *use      = GBT_get_default_alignment(gb_main);

    init(gb_main,use,helix_nr,helix);
    
    free(helix);
    free(helix_nr);
    free(use);

    return get_error();
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

long BI_helix::first_pair_position() const {
    return entries[0].pair_type == HELIX_NONE ? next_pair_position(0) : 0;
}

long BI_helix::next_pair_position(size_t pos) const {
    if (entries[pos].next_pair_pos == 0) {
        size_t p;
        long   next_pos = -1;
        for (p = pos+1; p<Size && next_pos == -1; ++p) {
            if (entries[p].pair_type != HELIX_NONE) {
                next_pos = p;
            }
            else if (entries[p].next_pair_pos != 0) {
                next_pos = entries[p].next_pair_pos;
            }
        }

        size_t q = p<Size ? p-2: Size-1;

        for (p = pos; p <= q; ++p) {
            bi_assert(entries[p].next_pair_pos == 0);
            entries[p].next_pair_pos = next_pos;
        }
    }
    return entries[pos].next_pair_pos;
}

long BI_helix::first_position(const char *helix_Nr) const {
    long pos;
    for (pos = first_pair_position(); pos != -1; pos = next_pair_position(pos)) {
        if (strcmp(helix_Nr, entries[pos].helix_nr) == 0) break;
    }
    return pos;
}

long BI_helix::last_position(const char *helix_Nr) const {
    long pos = first_position(helix_Nr);
    if (pos != -1) {
        long next_pos = next_pair_position(pos);
        while (next_pos != -1 && strcmp(helix_Nr, entries[next_pos].helix_nr) == 0) {
            pos      = next_pos;
            next_pos = next_pair_position(next_pos);
        }
    }
    return pos;
}




/***************************************************************************************
*******         Reference to abs pos                    ********
****************************************************************************************/
void BI_ecoli_ref::bi_exit(void){
    delete [] abs2rel;
    delete [] rel2abs;
    abs2rel = 0;
    rel2abs = 0;
}

BI_ecoli_ref::BI_ecoli_ref(void) {
    memset((char *)this,0,sizeof(BI_ecoli_ref));
}

BI_ecoli_ref::~BI_ecoli_ref(void){
    bi_exit();
}

inline bool isGap(char c) { return c == '-' || c == '.'; }

const char *BI_ecoli_ref::init(const char *seq, size_t size) {
    bi_exit();

    abs2rel = new size_t[size];
    rel2abs = new size_t[size];
    memset((char *)rel2abs,0,(size_t)(sizeof(*rel2abs)*size));

    relLen = 0;
    absLen = size;
    size_t i;
    size_t sl = strlen(seq);
    for (i=0; i<size; i++) {
        abs2rel[i]      = relLen;
        rel2abs[relLen] = i;
        if (i<sl && !isGap(seq[i])) ++relLen;
    }
    return 0;
}

const char *BI_ecoli_ref::init(GBDATA *gb_main,char *alignment_name, char *ref_name) {
    GB_transaction ta(gb_main);

    GB_ERROR err  = 0;
    long     size = GBT_get_alignment_len(gb_main,alignment_name);
    
    if (size<=0) {
        err = GB_get_error();
        bi_assert(err);
    }
    else {
        GBDATA *gb_ref_con   = GBT_find_SAI(gb_main, ref_name);
        if (!gb_ref_con) err = GBS_global_string("I cannot find the SAI '%s'",ref_name);
        else {
            GBDATA *gb_ref   = GBT_read_sequence(gb_ref_con,alignment_name);
            if (!gb_ref) err = GBS_global_string("Your SAI '%s' has no sequence '%s/data'", ref_name, alignment_name);
            else {
                err = init(GB_read_char_pntr(gb_ref), size);
            }
        }
    }
    return err;
}

const char *BI_ecoli_ref::init(GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    char     *ref = GBT_get_default_ref(gb_main);
    char     *use = GBT_get_default_alignment(gb_main);
    GB_ERROR  err = init(gb_main,use,ref);

    free(ref);
    free(use);

    return err;
}
