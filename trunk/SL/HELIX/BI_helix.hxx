#ifndef BI_HELIX_HXX
#define BI_HELIX_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define bi_assert(bed) arb_assert(bed)

#ifndef _CPP_CSTDLIB
#include <cstdlib>
#endif
#ifndef _CPP_CSTDIO
#include <cstdio>
#endif
#ifndef _CPP_CSTRING
#include <cstring>
#endif

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#define HELIX_MAX_NON_ST           10
#define HELIX_AWAR_PAIR_TEMPLATE   "Helix/pairs/%s"
#define HELIX_AWAR_SYMBOL_TEMPLATE "Helix/symbols/%s"
#define HELIX_AWAR_ENABLE          "Helix/enable"

typedef enum {
    HELIX_NONE,         // used in entries
    HELIX_STRONG_PAIR,
    HELIX_PAIR,         // used in entries
    HELIX_WEAK_PAIR,
    HELIX_NO_PAIR,          // used in entries
    HELIX_USER0,
    HELIX_USER1,
    HELIX_USER2,
    HELIX_USER3,
    HELIX_DEFAULT,
    HELIX_NON_STANDARD0,            // used in entries
    HELIX_NON_STANDARD1,            // used in entries
    HELIX_NON_STANDARD2,            // used in entries
    HELIX_NON_STANDARD3,            // used in entries
    HELIX_NON_STANDARD4,            // used in entries
    HELIX_NON_STANDARD5,            // used in entries
    HELIX_NON_STANDARD6,            // used in entries
    HELIX_NON_STANDARD7,            // used in entries
    HELIX_NON_STANDARD8,            // used in entries
    HELIX_NON_STANDARD9,            // used in entries
    HELIX_NO_MATCH,
    HELIX_MAX
} BI_PAIR_TYPE;

struct BI_helix_entry {
    long          pair_pos;
    BI_PAIR_TYPE  pair_type;
    char         *helix_nr;

    long next_pair_pos;         // next position with pair_type != HELIX_NONE
    // contains 0 if uninitialized, -1 behind last position
    bool allocated;
};

class BI_helix {
    BI_helix_entry *entries;
    size_t          Size;

    void _init(void);

    static char *helix_error;         // error occurring during init is stored here
    
    const char *init(GBDATA *gb_main, const char *alignment_name, const char *helix_nr_name, const char *helix_name);
    
protected:

    char *pairs[HELIX_MAX];
    char *char_bind[HELIX_MAX];
    
    int _check_pair(char left, char right, BI_PAIR_TYPE pair_type);

public:

    static char *get_error() { return helix_error; }
    static void clear_error() { free(helix_error); helix_error = 0; }
    static void set_error(const char *err) { free(helix_error); helix_error = err ? strdup(err) : 0; }

    BI_helix(void);
    ~BI_helix(void);

    const char *init(GBDATA *gb_main);
    const char *init(GBDATA *gb_main, const char *alignment_name);
    const char *init(GBDATA *gb_helix_nr,GBDATA *gb_helix,size_t size);
    const char *initFromData(const char *helix_nr, const char *helix, size_t size);

    int check_pair(char left, char right, BI_PAIR_TYPE pair_type); // return 1 if bases form a pair

    size_t size() const { return Size; }
    bool has_entries() const { return entries; }
    const BI_helix_entry& entry(size_t pos) const {
        bi_assert(pos<Size);
        bi_assert(entries);
        return entries[pos];
    }

    size_t opposite_position(size_t pos) const {
        const BI_helix_entry& Entry = entry(pos);
        bi_assert(Entry.pair_type != HELIX_NONE); // not a pair -> no opposite position
        return Entry.pair_pos;
    }
    BI_PAIR_TYPE pairtype(size_t pos) const { return pos<Size ? entry(pos).pair_type : HELIX_NONE; }
    const char *helixNr(size_t pos) const { return pairtype(pos) == HELIX_NONE ? 0 : entry(pos).helix_nr; }
    // Note: results of helixNr may be compared by == (instead of strcmp())

    long first_pair_position() const; // first pair position (or -1)
    long next_pair_position(size_t pos) const; // next pair position behind 'pos' (or -1)
    
    long first_position(const char *helixNr) const; // returns -1 for non-existing helixNr's
    long last_position(const char *helixNr) const; // returns -1 for non-existing helixNr's
};




class BI_ecoli_ref {
    size_t absLen;
    size_t relLen;

    size_t *abs2rel;
    size_t *rel2abs;
    
    void bi_exit();

public:
    BI_ecoli_ref();
    ~BI_ecoli_ref();

    const char *init(GBDATA *gb_main);
    const char *init(GBDATA *gb_main,char *alignment_name, char *ref_name);
    const char *init(const char *seq, size_t size);

    size_t abs_2_rel(size_t abs) const {
        bi_assert(abs2rel); // call init!
        if (abs >= absLen) abs = absLen-1;
        return abs2rel[abs];
    }

    size_t rel_2_abs(size_t rel) const {
        bi_assert(abs2rel); // call init!
        if (rel >= relLen) rel = relLen-1;
        return rel2abs[rel];
    }
    
    size_t base_count() const { return relLen; }
};


#endif
