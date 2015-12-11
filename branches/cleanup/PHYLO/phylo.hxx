// =============================================================== //
//                                                                 //
//   File      : phylo.hxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PHYLO_HXX
#define PHYLO_HXX

#ifndef PH_FILTER_HXX
#include "PH_filter.hxx"
#endif
#ifndef AP_MATRIX_HXX
#include <AP_matrix.hxx>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define ph_assert(cond) arb_assert(cond)

#define AWAR_PHYLO_ALIGNMENT     "tmp/phyl/alignment"
#define AWAR_PHYLO_FILTER_FILTER "phyl/filter/filter"

#define AWAR_PHYLO_FILTER_STARTCOL "phyl/filter/startcol"
#define AWAR_PHYLO_FILTER_STOPCOL  "phyl/filter/stopcol"
#define AWAR_PHYLO_FILTER_MINHOM   "phyl/filter/minhom"
#define AWAR_PHYLO_FILTER_MAXHOM   "phyl/filter/maxhom"
#define AWAR_PHYLO_FILTER_POINT    "phyl/filter/point"
#define AWAR_PHYLO_FILTER_MINUS    "phyl/filter/minus"
#define AWAR_PHYLO_FILTER_REST     "phyl/filter/rest"
#define AWAR_PHYLO_FILTER_LOWER    "phyl/filter/lower"

#define AWAR_PHYLO_MARKERLINENAME "tmp/phylo/markerlinename"

enum {
    DONT_COUNT           = 0,
    SKIP_COLUMN_IF_MAX   = 1,
    SKIP_COLUMN_IF_OCCUR = 2,
    COUNT_DONT_USE_MAX   = 3,
    TREAT_AS_UPPERCASE   = 4,
    TREAT_AS_REGULAR     = 5,

    FILTER_MODES, // has to be last!
};


#define PH_DB_CACHE_SIZE    2000000

#define AP_F_LOADED    ((AW_active)1)
#define AP_F_NLOADED   ((AW_active)2)
#define AP_F_SEQUENCES ((AW_active)4)
#define AP_F_MATRIX    ((AW_active)8)
#define AP_F_TREE      ((AW_active)16)
#define AP_F_ALL       ((AW_active)-1)

#define PH_CORRECTION_BANDELT_STRING "bandelt"

#define NIL 0

// matrix definitions
#define PH_TRANSFORMATION_JUKES_CANTOR_STRING        "J+C"
#define PH_TRANSFORMATION_KIMURA_STRING              "KIMURA"
#define PH_TRANSFORMATION_TAJIMA_NEI_STRING          "T+N"
#define PH_TRANSFORMATION_TAJIMA_NEI_PAIRWISE_STRING "T+N-P"
#define PH_TRANSFORMATION_BANDELT_STRING             "B"
#define PH_TRANSFORMATION_BANDELT_JC_STRING          "B+J+C"
#define PH_TRANSFORMATION_BANDELT2_STRING            "B2"
#define PH_TRANSFORMATION_BANDELT2_JC_STRING         "B2+J+C"

enum PH_CORRECTION {
    PH_CORRECTION_NONE,
    PH_CORRECTION_BANDELT
};


enum {
    PH_GC_0,
    PH_GC_1,
    PH_GC_0_DRAG
};

// make awars :
void PH_create_filter_variables(AW_root *aw_root, AW_default aw_def);

AW_window *PH_create_filter_window(AW_root *aw_root);



enum display_type { NONE, species_dpy, filter_dpy, matrix_dpy, tree_dpy };


typedef double AP_FLOAT;

enum PH_TRANSFORMATION {
    PH_TRANSFORMATION_NONE,
    PH_TRANSFORMATION_JUKES_CANTOR,
    PH_TRANSFORMATION_KIMURA,
    PH_TRANSFORMATION_TAJIMA_NEI,
    PH_TRANSFORMATION_TAJIMA_NEI_PAIRWISE,
    PH_TRANSFORMATION_BANDELT,
    PH_TRANSFORMATION_BANDELT_JC,
    PH_TRANSFORMATION_BANDELT2,
    PH_TRANSFORMATION_BANDELT2_JC
};

// ---------------------------
//      class definitions

class PH_root : virtual Noncopyable {
    char        *use;
    GBDATA      *gb_main;

    static PH_root *SINGLETON;

public:

    PH_root()
        : use(NULL),
          gb_main(NULL)
    {
        ph_assert(!SINGLETON);
        SINGLETON = this;
    }
    ~PH_root() {
        ph_assert(this == SINGLETON);
        SINGLETON = NULL;
        free(use);
    }

    GB_ERROR open(const char *db_server);
    GBDATA *get_gb_main() const { ph_assert(gb_main); return gb_main; }
    const char *get_aliname() const { return use; }
};


class PHDATA : virtual Noncopyable {
    // connection to database
    // pointers to all elements and important values of the database

    struct PHENTRY : virtual Noncopyable {
        unsigned int  key;
        char         *name;
        char         *full_name;
        GBDATA       *gb_species_data_ptr;
        PHENTRY      *next;
        PHENTRY      *prev;
        bool          selected;

        PHENTRY()
            : key(0),
              name(NULL),
              full_name(NULL),
              gb_species_data_ptr(NULL),
              next(NULL),
              prev(NULL),
              selected(false)
        {}

        ~PHENTRY() {
            ph_assert(0); // @@@ why not called?
        }
    };

    unsigned int last_key_number;
    long         seq_len;

    AW_root *aw_root; // only link
    PH_root *ph_root; // only link

    PHENTRY *entries;

public:
    GBDATA *get_gb_main() { return ph_root->get_gb_main(); }

    char *use;               // @@@ elim (PH_root has same field)

    PHENTRY      **hash_elements;
    unsigned int   nentries;                        // total number of entries

    static PHDATA *ROOT;                            // 'global' pointer

    AP_smatrix *matrix;                             // calculated matrix
    float      *markerline;

    PHDATA(AW_root *aw_root_, PH_root *ph_root_)
        : last_key_number(0),
          seq_len(0),
          aw_root(aw_root_),
          ph_root(ph_root_),
          entries(NULL),
          use(NULL),
          hash_elements(NULL),
          nentries(0),
          matrix(NULL),
          markerline(NULL)
    {}
    ~PHDATA() {
        unload();
        delete matrix;
    }

    char *load(char*& use); // open database and get pointers to it
    char *unload();

    long get_seq_len() { return seq_len; }
};

#else
#error phylo.hxx included twice
#endif // PHYLO_HXX
