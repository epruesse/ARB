// =========================================================== //
//                                                             //
//   File      : ed4_seq_colors.hxx                            //
//   Purpose   : Sequence foreground coloring.                 //
//               Viewing differences only.                     //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_SEQ_COLORS_HXX
#define AWT_SEQ_COLORS_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef _GLIBCXX_CSTRING
#include <cstring>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef _GLIBCXX_CCTYPE
#include <cctype>
#endif

#define e4_assert(bed) arb_assert(bed)

class GBDATA;
class AW_root;
class AW_window;
class ED4_sequence_terminal;
class ED4_species_manager;

class ED4_seq_colors {
    int base_gc;
    void (*cb)();

public:
    void run_cb() const { if (cb) cb(); }
    void reload();

    // real public
    char char_2_gc[256];         // translate to gc
    char char_2_char[256];       // translate to char
    char char_2_gc_aa[256];      // translate to gc  - for aminoacid sequence
    char char_2_char_aa[256];    // translate to char - for aminoacid sequence

    ED4_seq_colors(int baseGC, void (*changed_cb)());
};

class ED4_reference : virtual Noncopyable {
    // general:
    GBDATA *gb_main;
    char    nodiff;
    bool    mindcase;

    // current reference:
    int   ref_len;
    char *reference;

    const ED4_sequence_terminal *ref_term;

    void update_data();

public:
    ED4_reference(GBDATA *gb_main);
    ~ED4_reference();

    void set_nodiff_indicator(char ind) { nodiff = ind; }
    void set_case_sensitive(bool mindcase_) { mindcase = mindcase_; }

    void clear();
    void define(const ED4_sequence_terminal *rterm);

    bool is_set() const { return reference; }
    void expand_to_length(int len); // make sure that reference is at least len long

    int convert(char c, int pos) const {
        if (c == '-') return c; // @@@ change behavior?
        if (c != reference[pos]) {
            if (mindcase) return c;
            if (tolower(c) != tolower(reference[pos])) return c;
        }
        return nodiff;
    }
    bool reference_species_is(const ED4_sequence_terminal *term) const {
        e4_assert(is_set()); // otherwise check makes no sense
        return term == ref_term;
    }
    bool only_show_diff_for(const ED4_sequence_terminal *term) const {
        return is_set() && !reference_species_is(term);
    }

    void data_changed_cb(ED4_species_manager *calledFrom);
};

AW_window *ED4_create_seq_colors_window(AW_root *awr, ED4_seq_colors *sc);
AW_window *ED4_create_viewDifferences_window(AW_root *awr);
void ED4_toggle_viewDifferences(AW_root *awr);
void ED4_viewDifferences_setNewReference();
void ED4_viewDifferences_disable();


#else
#error ed4_seq_colors.hxx included twice
#endif // AWT_SEQ_COLORS_HXX
