#ifndef awt_seq_colors_hxx_included
#define awt_seq_colors_hxx_included

#define AWAR_SEQ_PATH  "awt/seq_colors/"
#define AWAR_SEQ_NAME_STRINGS_TEMPLATE    AWAR_SEQ_PATH  "strings/elem_%i"
#define AWAR_SEQ_NAME_TEMPLATE                  AWAR_SEQ_PATH  "set_%i/elem_%i"
#define AWAR_SEQ_NAME_SELECTOR_NA            AWAR_SEQ_PATH   "na/select"
#define AWAR_SEQ_NAME_SELECTOR_AA             AWAR_SEQ_PATH   "aa/select"

#define AWT_SEQ_COLORS_MAX_SET   5
#define AWT_SEQ_COLORS_MAX_ELEMS 28 // has to be a even number!

class AWT_seq_colors {
    int base_gc;
    AW_CL cd1;
    AW_CL cd2;
    AW_CB callback;
    int cbexists;
public:
    AW_window *aww;
    void run_cb();
    void reload();

    GBDATA *gb_def;
// real public
    char char_2_gc[256];	// translate to gc
    char char_2_char[256];	// translate to char
    char char_2_gc_aa[256];	// translate to gc  - for aminoacid sequence 
    char char_2_char_aa[256];	// translate to char - for aminoacid sequence 
    AWT_seq_colors(GBDATA *gb_default, int base_gc,AW_CB cb,AW_CL cd1,AW_CL cd2);
};

class AWT_reference{
    GBDATA *gb_main;
    int	ref_len;
    char *reference;
    char *init_species_name;
public:
    AWT_reference(GBDATA *gb_main);
    ~AWT_reference();

    void init();
    void init(const char *species_name, const char *alignment_name);
    void init(const char *name, const char *sequence_data, int len);

    void expand_to_length(int len);		// make sure that reference is at least len long

    int convert(char c,int pos) const 				{ return (c=='-' || c!=reference[pos]) ? c : '.'; }
    int reference_species_is(const char *species_name) const 	{ return init_species_name ? strcmp(species_name, init_species_name)==0 : 0; }
};

AW_window *create_seq_colors_window(AW_root *awr, AWT_seq_colors *asc);


#endif
