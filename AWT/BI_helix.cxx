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
    long	pos;
    BI_PAIR_TYPE type;
    char	c;
};

struct {
    const char *awar;
    BI_PAIR_TYPE pair_type;
} helix_awars[] = {
    { "Strong_Pair", HELIX_STRONG_PAIR },
    { "Normal_Pair", HELIX_PAIR },
    { "Weak_Pair", HELIX_WEAK_PAIR },
    { "No_Pair", HELIX_NO_PAIR },
    { "User_Pair", HELIX_USER0 },
    { "User_Pair2", HELIX_USER1 },
    { "User_Pair3", HELIX_USER2 },
    { "User_Pair4", HELIX_USER3 },
    { "Default", HELIX_DEFAULT },
    { "Non_Standart_aA", HELIX_NON_STANDART0 },
    { "Non_Standart1", HELIX_NON_STANDART1 },
    { "Non_Standart2", HELIX_NON_STANDART2 },
    { "Non_Standart3", HELIX_NON_STANDART3 },
    { "Non_Standart4", HELIX_NON_STANDART4 },
    { "Non_Standart5", HELIX_NON_STANDART5 },
    { "Non_Standart6", HELIX_NON_STANDART6 },
    { "Non_Standart7", HELIX_NON_STANDART7 },
    { "Non_Standart8", HELIX_NON_STANDART8 },
    { "Non_Standart9", HELIX_NON_STANDART9 },
    { "Not_Non_Standart", HELIX_NO_MATCH },
    { 0, HELIX_NONE },
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

    deleteable = 1;
}

BI_helix::BI_helix(void)
{ _init(); }

#ifdef _USE_AW_WINDOW
BI_helix::BI_helix(AW_root * aw_root)
{
    int i;
    char awar[256];

    _init();
    int j;
    for (j=0; helix_awars[j].awar; j++){
        i = helix_awars[j].pair_type;
        sprintf(awar,HELIX_AWAR_PAIR_TEMPLATE, helix_awars[j].awar);
        aw_root->awar_string( awar,pairs[i])->add_target_var(&pairs[i]);
        sprintf(awar,HELIX_AWAR_SYMBOL_TEMPLATE, helix_awars[j].awar);
        aw_root->awar_string( awar, char_bind[i])->add_target_var(&char_bind[i]);
    }
    deleteable = 0;
}
#endif

BI_helix::~BI_helix(void){
    if (!deleteable){
        GB_warning("Internal Programm Error: You cannot delete BI_helix !!");
    }
    int i;
    for (i=0;i<HELIX_MAX; i++)	delete pairs[i];
    for (i=0;i<HELIX_MAX; i++) 	delete char_bind[i];

    delete entries;
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
    /* helix_nr	string of helix identifiers
   helix	helix
   size		alignment len
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
    sident = strdup("-");
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
        if (strchr(LEFT_HELIX,c) || strchr(LEFT_NONS,c)  ){	// push
            laststack = (struct helix_stack *)GBS_read_hash(hash,ident);
            stack = new helix_stack;
            stack->next = laststack;
            stack->pos = pos;
            stack->c = c;
            GBS_write_hash(hash,ident,(long)stack);
        }
        else if (strchr(RIGHT_HELIX,c) || strchr(RIGHT_NONS,c) ){	// pop
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
                    sprintf(error,"Character '%c' pos %li don't mach character '%c' pos %i in Helix '%s'",
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

            if (strcmp(sident+1,ident)) {
                //free(sident);
                sident = (char*)malloc(strlen(ident)+2);
                sprintf(sident,"-%s",ident);
            }
            entries[pos].helix_nr = sident+1;
            entries[stack->pos].helix_nr = sident;

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
    err = 	init(gb_main,use,helix_nr,helix);
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
    // returns 2 helix 1 weak helix 0 no helix	-1 nothing
{
    left = toupper(left);
    right = toupper(right);
    switch(pair_type) {
        case HELIX_PAIR:
            if (	_check_pair(left,right,HELIX_STRONG_PAIR) ||
                    _check_pair(left,right,HELIX_PAIR) ) return 2;
            if (	_check_pair(left,right,HELIX_WEAK_PAIR) ) return 1;
            return 0;
        case HELIX_NO_PAIR:
            if (	_check_pair(left,right,HELIX_STRONG_PAIR) ||
                    _check_pair(left,right,HELIX_PAIR) ) return 0;
            return 1;
        default:
            return _check_pair(left,right,pair_type);
    }
}


#ifdef _USE_AW_WINDOW

char BI_helix::get_symbol(char left, char right, BI_PAIR_TYPE pair_type){
    left = toupper(left);
    right = toupper(right);
    int i;
    int erg;
    if (pair_type< HELIX_NON_STANDART0) {
        erg = *char_bind[HELIX_DEFAULT];
        for (i=HELIX_STRONG_PAIR; i< HELIX_NON_STANDART0; i++){
            if (_check_pair(left,right,(BI_PAIR_TYPE)i)){
                erg = *char_bind[i];
                break;
            }
        }
    }else{
        erg = *char_bind[HELIX_NO_MATCH];
        if (_check_pair(left,right,pair_type)) erg =  *char_bind[pair_type];
    }
    if (!erg) erg = ' ';
    return erg;
}

char *BI_helix::seq_2_helix(char *sequence,char undefsymbol){
    size_t size2 = strlen(sequence);
    bi_assert(size2<=size); // if this fails there is a sequence longer than the alignment
    char *helix = (char *)GB_calloc(sizeof(char),size+1);
    register unsigned long i,j;
    for (i=0; i<size2; i++) {
        if ( i<size && entries[i].pair_type) {
            j = entries[i].pair_pos;
            helix[i] = get_symbol(sequence[i],sequence[j],
                                  entries[i].pair_type);
        }else{
            helix[i] = undefsymbol;
        }
        if (helix[i] == ' ') helix[i] = undefsymbol;
    }
    return helix;
}

int BI_show_helix_on_device(AW_device *device, int gc, const char *opt_string, size_t opt_string_size, size_t start, size_t size,
                            AW_pos x,AW_pos y, AW_pos opt_ascent,AW_pos opt_descent,
                            AW_CL cduser, AW_CL cd1, AW_CL cd2)
{
    AWUSE(opt_ascent);AWUSE(opt_descent);
    BI_helix *THIS = (BI_helix *)cduser;
    char *buffer = GB_give_buffer(size+1);
    register unsigned long i,j,k;

    for (k=0; k<size; k++) {
        i = k+start;
        if ( i<THIS->size && THIS->entries[i].pair_type) {
            j = THIS->entries[i].pair_pos;
            char pairing_character = '.';
            if (j < opt_string_size){
                pairing_character = opt_string[j];
            }
            buffer[k] = THIS->get_symbol(opt_string[i],pairing_character,
                                         THIS->entries[i].pair_type);
        }else{
            buffer[k] = ' ';
        }
    }
    buffer[size] = 0;
    return device->text(gc,buffer,x,y,0.0,(AW_bitset)-1,cd1,cd2);
}

int BI_helix::show_helix( void *devicei, int gc1 , char *sequence,
                          AW_pos x, AW_pos y,
                          AW_bitset filter,
                          AW_CL cd1, AW_CL cd2){

    if (!entries) return 0;
    AW_device *device = (AW_device *)devicei;
    return device->text_overlay(gc1, sequence, 0, x , y, 0.0 , filter, (AW_CL)this, cd1, cd2,
                                1.0,1.0, BI_show_helix_on_device);
}



AW_window *create_helix_props_window(AW_root *awr, AW_cb_struct * /*owner*/awcbs){
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "HELIX_PROPS", "HELIX_PROPERTIES",400, 300 );

    aws->at           ( 10,10 );
    aws->auto_space(3,3);
    aws->callback     ( AW_POPDOWN );
    aws->create_button( "CLOSE", "CLOSE", "C" );
    aws->at_newline();

    aws->label_length( 18 );
    int i;
    int j;
    int ex= 0,ey = 0;
    char awar[256];
    for (j=0; helix_awars[j].awar; j++){

        aws->label_length( 25 );
        i = helix_awars[j].pair_type;

        if (i != HELIX_DEFAULT && i!= HELIX_NO_MATCH ) {
            sprintf(awar,HELIX_AWAR_PAIR_TEMPLATE, helix_awars[j].awar);
            aws->label(helix_awars[j].awar);
            aws->callback(awcbs);
            aws->create_input_field(awar,20);
        }else{
            aws->create_button(0,helix_awars[j].awar);
            aws->at_x(ex);
        }
        if (!j) aws->get_at_position(&ex,&ey);

        sprintf(awar,HELIX_AWAR_SYMBOL_TEMPLATE, helix_awars[j].awar);
        aws->callback(awcbs);
        aws->create_input_field(awar,3);
        aws->at_newline();
    }
    aws->window_fit();
    return (AW_window *)aws;
}
#endif

/***************************************************************************************
*******			Reference to abs pos					********
****************************************************************************************/
void BI_ecoli_ref::exit(void){
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
    exit();
}

const char *BI_ecoli_ref::init(char *seq,long size)
{
    exit();
    _abs_2_rel1 = new long[size];
    _abs_2_rel2 = new long[size];
    _rel_2_abs = new long[size];
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
    err = 	init(gb_main,use,ref);
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
