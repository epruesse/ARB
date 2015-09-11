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

class GBDATA;
class AW_root;
class AW_window;

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
    GBDATA *gb_main;
    int     ref_len;
    char   *reference;
    char   *init_species_name;
    char    nodiff;

public:
    ED4_reference(GBDATA *gb_main);
    ~ED4_reference();

    void clear();
    void define(const char *species_name, const char *alignment_name);
    void define(const char *name, const char *sequence_data, int len);

    bool is_set() const { return reference; }

    void expand_to_length(int len);             // make sure that reference is at least len long

    void set_nodiff_indicator(char ind) { nodiff = ind; }

    int convert(char c, int pos) const                          { return (c=='-' || c!=reference[pos]) ? c : nodiff; }
    int reference_species_is(const char *species_name) const    { return init_species_name ? strcmp(species_name, init_species_name)==0 : 0; } // @@@ test using 'ED4_terminal*'
};

AW_window *ED4_create_seq_colors_window(AW_root *awr, ED4_seq_colors *sc);
AW_window *ED4_create_viewDifferences_window(AW_root *awr);


#else
#error ed4_seq_colors.hxx included twice
#endif // AWT_SEQ_COLORS_HXX
