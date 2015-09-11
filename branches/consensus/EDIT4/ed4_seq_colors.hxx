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

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

#define AWAR_SEQ_PATH                  "awt/seq_colors/"
#define AWAR_SEQ_NAME_STRINGS_TEMPLATE AWAR_SEQ_PATH  "strings/elem_%i"
#define AWAR_SEQ_NAME_TEMPLATE         AWAR_SEQ_PATH  "set_%i/elem_%i"
#define AWAR_SEQ_NAME_SELECTOR_NA      AWAR_SEQ_PATH   "na/select"
#define AWAR_SEQ_NAME_SELECTOR_AA      AWAR_SEQ_PATH   "aa/select"

class AWT_seq_colors {
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

    AWT_seq_colors(int baseGC, void (*changed_cb)());
};

class AWT_reference : virtual Noncopyable {
    GBDATA *gb_main;
    int     ref_len;
    char   *reference;
    char   *init_species_name;

public:
    AWT_reference(GBDATA *gb_main);
    ~AWT_reference();

    void clear();
    void define(const char *species_name, const char *alignment_name);
    void define(const char *name, const char *sequence_data, int len);

    void expand_to_length(int len);             // make sure that reference is at least len long

    int convert(char c, int pos) const                          { return (c=='-' || c!=reference[pos]) ? c : '.'; } // @@@ customize
    int reference_species_is(const char *species_name) const    { return init_species_name ? strcmp(species_name, init_species_name)==0 : 0; }
};

AW_window *create_seq_colors_window(AW_root *awr, AWT_seq_colors *asc);


#else
#error awt_seq_colors.hxx included twice
#endif // AWT_SEQ_COLORS_HXX
