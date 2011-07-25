#ifndef BI_HELIX_HXX
#define BI_HELIX_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#ifndef bi_assert
#define bi_assert(bed) arb_assert(bed)
#endif

enum BI_PAIR_TYPE {
    HELIX_NONE,                                     // used in entries
    HELIX_STRONG_PAIR,
    HELIX_PAIR,                                     // used in entries
    HELIX_WEAK_PAIR,
    HELIX_NO_PAIR,                                  // used in entries
    HELIX_USER0,
    HELIX_USER1,
    HELIX_USER2,
    HELIX_USER3,
    HELIX_DEFAULT,
    HELIX_NON_STANDARD0,                            // used in entries
    HELIX_NON_STANDARD1,                            // used in entries
    HELIX_NON_STANDARD2,                            // used in entries
    HELIX_NON_STANDARD3,                            // used in entries
    HELIX_NON_STANDARD4,                            // used in entries
    HELIX_NON_STANDARD5,                            // used in entries
    HELIX_NON_STANDARD6,                            // used in entries
    HELIX_NON_STANDARD7,                            // used in entries
    HELIX_NON_STANDARD8,                            // used in entries
    HELIX_NON_STANDARD9,                            // used in entries
    HELIX_NO_MATCH,
    HELIX_MAX
};

struct BI_helix_entry {
    long          pair_pos;
    BI_PAIR_TYPE  pair_type;
    char         *helix_nr;

    long next_pair_pos;                             /* next position with pair_type != HELIX_NONE
                                                     * contains
                                                     *  0 if uninitialized,
                                                     * -1 behind last position */
    bool allocated;
};

class BI_helix : virtual Noncopyable {
    BI_helix_entry *entries;
    size_t          Size;

    void _init();

    static char *helix_error;                       // error occurring during init is stored here

    const char *init(GBDATA *gb_main, const char *alignment_name, const char *helix_nr_name, const char *helix_name);

protected:

    char *pairs[HELIX_MAX];
    char *char_bind[HELIX_MAX];

    bool is_pairtype(char left, char right, BI_PAIR_TYPE pair_type);

public:

    static char *get_error() { return helix_error; }
    static void clear_error() { freenull(helix_error); }
    static void set_error(const char *err) { freedup(helix_error, err); }

    BI_helix();
    ~BI_helix();

    const char *init(GBDATA *gb_main);
    const char *init(GBDATA *gb_main, const char *alignment_name);
    const char *init(GBDATA *gb_helix_nr, GBDATA *gb_helix, size_t size);
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





#endif
