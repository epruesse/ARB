/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef GEN_LOCAL_HXX
#define GEN_LOCAL_HXX

#ifndef GEN_HXX
#include "GEN.hxx"
#endif

#ifndef NDEBUG
# define gen_assert(bed) do { if (!(bed)) *(int *)0 = 0; } while (0)
# ifndef DEBUG
#  error DEBUG is NOT defined - but it has to!
# endif
#else
# ifdef DEBUG
#  error DEBUG is defined - but it should not!
# endif
# define gen_assert(bed)
#endif /* NDEBUG */

// to create new genes:
#define AWAR_GENE_DEST "tmp/gene/dest"
#define AWAR_GENE_POS1 "tmp/gene/pos1"
#define AWAR_GENE_POS2 "tmp/gene/pos2"

#define AWAR_GENMAP_DISPLAY_TYPE "genemap/display/type"

// display book-style:
#define AWAR_GENMAP_BOOK_WIDTH_FACTOR   "genemap/display/book/width_factor"
#define AWAR_GENMAP_BOOK_BASES_PER_LINE "genemap/display/book/base_per_line"
#define AWAR_GENMAP_BOOK_LINE_HEIGHT    "genemap/display/book/line_height"
#define AWAR_GENMAP_BOOK_LINE_SPACE     "genemap/display/book/line_space"

// display vertical-style:
#define AWAR_GENMAP_VERTICAL_FACTOR_X "genemap/display/vertical/factor_x"
#define AWAR_GENMAP_VERTICAL_FACTOR_Y "genemap/displax/vertical/factor_y"

// display radial-style:
#define AWAR_GENMAP_RADIAL_INSIDE  "genemap/displax/radial/inside"
#define AWAR_GENMAP_RADIAL_OUTSIDE "genemap/displax/radial/outside"

#else
#error GEN_local.hxx included twice
#endif // GEN_LOCAL_HXX
