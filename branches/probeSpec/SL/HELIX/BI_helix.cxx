// =============================================================== //
//                                                                 //
//   File      : BI_helix.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "BI_helix.hxx"

#include <arbdbt.h>

#include <cctype>

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

void BI_helix::_init()
{
    int i;
    for (i=0; i<HELIX_MAX; i++) pairs[i] = 0;
    for (i=0; i<HELIX_MAX; i++) char_bind[i] = 0;

    entries = 0;
    Size = 0;

    pairs[HELIX_NONE]=strdup("");
    char_bind[HELIX_NONE] = strdup(" ");

    pairs[HELIX_STRONG_PAIR]=strdup("CG AT AU");
    char_bind[HELIX_STRONG_PAIR] = strdup("~");

    pairs[HELIX_PAIR]=strdup("GT GU");
    char_bind[HELIX_PAIR] = strdup("-");

    pairs[HELIX_WEAK_PAIR]=strdup("GA");
    char_bind[HELIX_WEAK_PAIR] = strdup("=");

    pairs[HELIX_NO_PAIR]=strdup("AA AC CC CT CU GG TU");
    char_bind[HELIX_NO_PAIR] = strdup("#");

    pairs[HELIX_USER0]=strdup(".A .C .G .T .U");
    char_bind[HELIX_USER0] = strdup("*");

    pairs[HELIX_USER1]=strdup("-A -C -G -T -U");
    char_bind[HELIX_USER1] = strdup("#");

    pairs[HELIX_USER2]=strdup("UU TT");
    char_bind[HELIX_USER2] = strdup("+");

    pairs[HELIX_USER3]=strdup("");
    char_bind[HELIX_USER3] = strdup("");

    pairs[HELIX_DEFAULT]=strdup("");
    char_bind[HELIX_DEFAULT] = strdup("");

    for (i=HELIX_NON_STANDARD0; i<=HELIX_NON_STANDARD9; i++) {
        pairs[i] = strdup("");
        char_bind[i] = strdup("");
    }

    pairs[HELIX_NO_MATCH]=strdup("");
    char_bind[HELIX_NO_MATCH] = strdup("|");
}

BI_helix::BI_helix() {
    _init();
}

BI_helix::~BI_helix() {
    unsigned i;
    for (i=0; i<HELIX_MAX; i++) free(pairs[i]);
    for (i=0; i<HELIX_MAX; i++) free(char_bind[i]);

    if (entries) {
        for (i = 0; i<Size; ++i) {
            if (entries[i].allocated) {
                free(entries[i].helix_nr);
            }
        }
        free(entries);
    }
}

static long BI_helix_check_error(const char *key, long val, void *) {
    struct helix_stack *stack = (struct helix_stack *)val;
    if (!BI_helix::get_error() && stack) { // don't overwrite existing error
        BI_helix::set_error(GBS_global_string("Too many '%c' in Helix '%s' pos %li", stack->c, key, stack->pos));
    }
    return val;
}


static long BI_helix_free_hash(const char *, long val, void *) {
    struct helix_stack *stack = (struct helix_stack *)val;
    struct helix_stack *next;
    for (; stack; stack = next) {
        next = stack->next;
        delete stack;
    }
    return 0;
}

const char *BI_helix::initFromData(const char *helix_nr_in, const char *helix_in, size_t sizei)
/* helix_nr string of helix identifiers
   helix    helix
   size     alignment len
*/
{
    clear_error();

    GB_HASH *hash = GBS_create_hash(256, GB_IGNORE_CASE);
    size_t   pos;
    char     c;
    char     ident[256];
    char    *sident;
    
    struct helix_stack *laststack = 0, *stack;

    Size = sizei;

    char *helix = 0;
    {
        size_t len          = strlen(helix_in);
        if (len > Size) len = Size;

        char *h = (char *)malloc(Size+1);
        h[Size] = 0;

        if (len<Size) memset(h+len, '.', Size-len);
        memcpy(h, helix_in, len);
        helix = h;
    }

    char *helix_nr = 0;
    if (helix_nr_in) {
        size_t len          = strlen(helix_nr_in);
        if (len > Size) len = (int)Size;

        char *h = (char *)malloc((int)Size+1);
        h[Size] = 0;

        if (len<Size) memset(h+len, '.', (int)(Size-len));
        memcpy(h, helix_nr_in, len);
        helix_nr = h;
    }

    strcpy(ident, "0");
    long pos_scanned_till = -1;

    entries = (BI_helix_entry *)GB_calloc(sizeof(BI_helix_entry), (size_t)Size);
    sident  = 0;

    for (pos = 0; pos < Size; pos ++) {
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
        if (strchr(LEFT_HELIX, c) || strchr(LEFT_NONS, c)) { // push
            laststack = (struct helix_stack *)GBS_read_hash(hash, ident);
            stack = new helix_stack;
            stack->next = laststack;
            stack->pos = pos;
            stack->c = c;
            GBS_write_hash(hash, ident, (long)stack);
        }
        else if (strchr(RIGHT_HELIX, c) || strchr(RIGHT_NONS, c)) { // pop
            stack = (struct helix_stack *)GBS_read_hash(hash, ident);
            if (!stack) {
                bi_assert(!helix_error); // already have an error
                helix_error = GBS_global_string_copy("Too many '%c' in Helix '%s' pos %zu", c, ident, pos);
                goto helix_end;
            }
            if (strchr(RIGHT_HELIX, c)) {
                entries[pos].pair_type = HELIX_PAIR;
                entries[stack->pos].pair_type = HELIX_PAIR;
            }
            else {
                c = tolower(c);
                if (stack->c != c) {
                    bi_assert(!helix_error); // already have an error
                    helix_error = GBS_global_string_copy("Character '%c' pos %li doesn't match character '%c' pos %zu in Helix '%s'",
                                                         stack->c, stack->pos, c, pos, ident);
                    goto helix_end;
                }
                if (isalpha(c)) {
                    entries[pos].pair_type = (BI_PAIR_TYPE)(HELIX_NON_STANDARD0+c-'a');
                    entries[stack->pos].pair_type = (BI_PAIR_TYPE)(HELIX_NON_STANDARD0+c-'a');
                }
                else {
                    entries[pos].pair_type = HELIX_NO_PAIR;
                    entries[stack->pos].pair_type = HELIX_NO_PAIR;
                }
            }
            entries[pos].pair_pos = stack->pos;
            entries[stack->pos].pair_pos = pos;
            GBS_write_hash(hash, ident, (long)stack->next);

            if (sident == 0 || strcmp(sident+1, ident) != 0) {
                sident = (char*)malloc(strlen(ident)+2);
                sprintf(sident, "-%s", ident);

                entries[stack->pos].allocated = true;
            }
            entries[pos].helix_nr        = sident+1;
            entries[stack->pos].helix_nr = sident;
            bi_assert((long)pos != stack->pos);

            delete stack;
        }
    }

    GBS_hash_do_loop(hash, BI_helix_check_error, NULL);

 helix_end :
    GBS_hash_do_loop(hash, BI_helix_free_hash, NULL);
    GBS_free_hash(hash);

    free(helix_nr);
    free(helix);

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

    GBDATA *gb_sai_data = GBT_get_SAI_data(gb_main);
    long    size2       = GBT_get_alignment_len(gb_main, alignment_name);

    if (size2<=0) set_error(GB_await_error());
    else {
        GBDATA *gb_helix_nr_con = GBT_find_SAI_rel_SAI_data(gb_sai_data, helix_nr_name);
        GBDATA *gb_helix_con    = GBT_find_SAI_rel_SAI_data(gb_sai_data, helix_name);
        GBDATA *gb_helix        = 0;
        GBDATA *gb_helix_nr     = 0;

        if (gb_helix_nr_con)    gb_helix_nr = GBT_read_sequence(gb_helix_nr_con, alignment_name);
        if (gb_helix_con)       gb_helix = GBT_read_sequence(gb_helix_con, alignment_name);

        init(gb_helix_nr, gb_helix, size2);
    }

    return get_error();
}

const char *BI_helix::init(GBDATA *gb_main, const char *alignment_name)
{
    GB_transaction ta(gb_main);

    char *helix    = GBT_get_default_helix(gb_main);
    char *helix_nr = GBT_get_default_helix_nr(gb_main);

    const char *err = init(gb_main, alignment_name, helix_nr, helix);

    free(helix);
    free(helix_nr);

    return err;
}

const char *BI_helix::init(GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    char       *alignment_name = GBT_get_default_alignment(gb_main);
    const char *err            = init(gb_main, alignment_name);

    free(alignment_name);
    return err;
}

bool BI_helix::is_pairtype(char left, char right, BI_PAIR_TYPE pair_type) {
    int   len = strlen(pairs[pair_type])-1;
    char *pai = pairs[pair_type];

    for (int i=0; i<len; i+=3) {
        if ((pai[i] == left && pai[i+1] == right) ||
            (pai[i] == right && pai[i+1] == left)) return true;
    }
    return false;
}

int BI_helix::check_pair(char left, char right, BI_PAIR_TYPE pair_type) {
    // return value:
    // 2  = helix
    // 1  = weak helix
    // 0  = no helix
    // -1 = nothing

    left  = toupper(left);
    right = toupper(right);
    switch (pair_type) {
        case HELIX_PAIR:
            if (is_pairtype(left, right, HELIX_STRONG_PAIR) ||
                is_pairtype(left, right, HELIX_PAIR)) return 2;
            if (is_pairtype(left, right, HELIX_WEAK_PAIR)) return 1;
            return 0;

        case HELIX_NO_PAIR:
            if (is_pairtype(left, right, HELIX_STRONG_PAIR) ||
                is_pairtype(left, right, HELIX_PAIR)) return 0;
            return 1;

        default:
            return is_pairtype(left, right, pair_type) ? 1 : 0;
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

        size_t q = p<Size ? p-2 : Size-1;

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

