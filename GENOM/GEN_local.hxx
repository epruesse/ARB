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


#define AWAR_GENE_DEST             "tmp/gene/dest"
#define AWAR_GENE_POS1             "tmp/gene/pos1"
#define AWAR_GENE_POS2             "tmp/gene/pos2"
#define AWAR_GENMAP_BASES_PER_LINE "genemap/display/base_per_line"
#define AWAR_GENMAP_LINE_HEIGHT    "genemap/display/line_height"
#define AWAR_GENMAP_LINE_SPACE     "genemap/display/line_space"


#else
#error GEN_local.hxx included twice
#endif // GEN_LOCAL_HXX
