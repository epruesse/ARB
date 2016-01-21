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
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
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
#define AWAR_PHYLO_FILTER_DOT      "phyl/filter/point"
#define AWAR_PHYLO_FILTER_MINUS    "phyl/filter/minus"
#define AWAR_PHYLO_FILTER_AMBIG    "phyl/filter/rest"
#define AWAR_PHYLO_FILTER_LOWER    "phyl/filter/lower"
#define AWAR_PHYLO_FILTER_AUTOCALC "phyl/filter/autocalc"

#define AWAR_PHYLO_MARKERLINENAME "tmp/phylo/markerlinename"

enum FilterMode {
    DONT_COUNT,
    SKIP_COLUMN_IF_MAX,
    SKIP_COLUMN_IF_OCCUR,
    COUNT_DONT_USE_MAX,
    TREAT_AS_UPPERCASE,
    TREAT_AS_REGULAR,

    // Note: enum values are saved in properties (do not modify w/o need!)
    // update on changes: PH_main.cxx@filter_text
};
const int FILTER_MODES = TREAT_AS_REGULAR+1;


#define PH_DB_CACHE_SIZE    2000000

enum {
    PH_GC_0,
    PH_GC_1,
    PH_GC_0_DRAG
};

void       PH_create_filter_variables(AW_root *aw_root, AW_default default_file, GBDATA *gb_main);
AW_window *PH_create_filter_window(AW_root *aw_root);

enum display_type {
    DISP_NONE,    // initial
    DISP_SPECIES, // after startup, filter not calculated yet
    DISP_FILTER,  // after filter has been calculated
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

        PHENTRY()
            : key(0),
              name(NULL),
              full_name(NULL),
              gb_species_data_ptr(NULL),
              next(NULL),
              prev(NULL)
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

    char *unload();

public:
    GBDATA *get_gb_main() { return ph_root->get_gb_main(); }

    char *use;               // @@@ elim (PH_root has same field)

    PHENTRY      **hash_elements;
    unsigned int   nentries;                        // total number of entries

    static PHDATA *ROOT;                            // 'global' pointer

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
          markerline(NULL)
    {}
    ~PHDATA() {
        unload();
    }

    char *load(char*& use); // open database and get pointers to it

    long get_seq_len() { return seq_len; }
};

#else
#error phylo.hxx included twice
#endif // PHYLO_HXX
