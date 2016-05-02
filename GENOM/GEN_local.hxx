// =============================================================== //
//                                                                 //
//   File      : GEN_local.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2001           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GEN_LOCAL_HXX
#define GEN_LOCAL_HXX

#ifndef GEN_HXX
#include "GEN.hxx"
#endif

#define gen_assert(bed) arb_assert(bed)

// contains the path to the gene:  "organism_name;gene_name"
// writing this awar has no effect
#define AWAR_COMBINED_GENE_NAME "tmp/gene/combined_name"

// to extract genes to pseudo-species:
#define AWAR_GENE_EXTRACT_ALI "tmp/gene/extract/ali"

// --------------------------------------------------------------------------------
// view-local awars for organism and gene

#define AWAR_LOCAL_ORGANISM_NAME(window_nr) GEN_window_local_awar_name("tmp/genemap/organism", window_nr)
#define AWAR_LOCAL_GENE_NAME(window_nr)     GEN_window_local_awar_name("tmp/genemap/gene", window_nr)

#define AWAR_LOCAL_ORGANISM_LOCK(window_nr) GEN_window_local_awar_name("tmp/genemap/organism_lock", window_nr)
#define AWAR_LOCAL_GENE_LOCK(window_nr)     GEN_window_local_awar_name("tmp/genemap/gene_lock", window_nr)

// --------------------------------------------------------------------------------
// display styles:
#define AWAR_GENMAP_DISPLAY_TYPE(window_nr) GEN_window_local_awar_name("genemap/display/type", window_nr)

// all display styles:
#define AWAR_GENMAP_ARROW_SIZE   "genemap/display/arrow_size"
#define AWAR_GENMAP_SHOW_HIDDEN  "genemap/display/show_hidden"
#define AWAR_GENMAP_SHOW_ALL_NDS "genemap/display/show_all_nds"

// display book-style:
#define AWAR_GENMAP_BOOK_WIDTH_FACTOR   "genemap/display/book/width_factor"
#define AWAR_GENMAP_BOOK_BASES_PER_LINE "genemap/display/book/base_per_line"
#define AWAR_GENMAP_BOOK_LINE_HEIGHT    "genemap/display/book/line_height"
#define AWAR_GENMAP_BOOK_LINE_SPACE     "genemap/display/book/line_space"

// display vertical-style:
#define AWAR_GENMAP_VERTICAL_FACTOR_X "genemap/display/vertical/factor_x"
#define AWAR_GENMAP_VERTICAL_FACTOR_Y "genemap/display/vertical/factor_y"

// display radial-style:
#define AWAR_GENMAP_RADIAL_INSIDE  "genemap/display/radial/inside"
#define AWAR_GENMAP_RADIAL_OUTSIDE "genemap/display/radial/outside"

// other options:
#define AWAR_GENMAP_AUTO_JUMP       "genemap/options/autojump"

// --------------------------------------------------------------------------------

const char *GEN_window_local_awar_name(const char *awar_name, int window_nr);

struct GEN_create_map_param {
    GBDATA *gb_main;
    int     window_nr;
    GEN_create_map_param(GBDATA *gb_main_, int window_nr_) : gb_main(gb_main_) , window_nr(window_nr_) { }
};

GB_ERROR GEN_mark_organism_or_corresponding_organism(GBDATA *gb_species, int *client_data);

// --------------------------------------------------------------------------------

#else
#error GEN_local.hxx included twice
#endif // GEN_LOCAL_HXX
