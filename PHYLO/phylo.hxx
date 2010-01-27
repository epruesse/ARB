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

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif
#ifndef AP_MATRIX_HXX
#include <AP_matrix.hxx>
#endif
#ifndef PH_FILTER_HXX
#include "PH_filter.hxx"
#endif

#define ph_assert(cond) arb_assert(cond)

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

typedef enum {
    PH_CORRECTION_NONE,
    PH_CORRECTION_BANDELT
} PH_CORRECTION;


enum {
    PH_GC_0,
    PH_GC_1,
    PH_GC_0_DRAG
};


extern GBDATA *GLOBAL_gb_main;

// make awars :
void PH_create_matrix_variables(AW_root *aw_root, AW_default aw_def);
void PH_create_filter_variables(AW_root *aw_root, AW_default aw_def);

AW_window *PH_create_matrix_window(AW_root *aw_root);
AW_window *PH_create_filter_window(AW_root *aw_root);



enum display_type { NONE, species_dpy, filter_dpy, matrix_dpy, tree_dpy };


typedef double AP_FLOAT;

typedef enum {
    PH_TRANSFORMATION_NONE,
    PH_TRANSFORMATION_JUKES_CANTOR,
    PH_TRANSFORMATION_KIMURA,
    PH_TRANSFORMATION_TAJIMA_NEI,
    PH_TRANSFORMATION_TAJIMA_NEI_PAIRWISE,
    PH_TRANSFORMATION_BANDELT,
    PH_TRANSFORMATION_BANDELT_JC,
    PH_TRANSFORMATION_BANDELT2,
    PH_TRANSFORMATION_BANDELT2_JC } PH_TRANSFORMATION;


// --------------------------
//      class definitions
// --------------------------

class PH_root {
private:
    char    *use;

public:
    class AWT_graphic *display;
    GB_ERROR open(const char *db_server);
    GBDATA *gb_main;
};




long AP_timer();

struct elem;

class PHDATA {
    // connection to database
    // pointers to all elements and important values of the database

    struct PHENTRY
    {
        unsigned int    key;
        char           *name;
        char           *full_name;
        GBDATA         *gb_species_data_ptr;
        struct PHENTRY *next;
        struct PHENTRY *prev;
        int             group_members; /* >0: this elem is grouphead */
        struct elem    *first_member; /* !=NULL: elem is grouphead */
        bool            selected;
    };
    unsigned int last_key_number;
    long         seq_len;

    AW_root *aw_root;       // only link

public:
    GBDATA          *gb_main;
    char            *use;
    struct PHENTRY  *entries;
    struct PHENTRY **hash_elements;
    unsigned int     nentries;  // total number of entries
    static PHDATA   *ROOT;      // 'global' pointer
    AP_smatrix      *distance_table; // weights between different characters
    AP_smatrix      *matrix;    // calculated matrix
    float           *markerline;

    PHDATA(AW_root *awr);
    ~PHDATA();

    char     *load(char *use);  // open database and get pointers to it
    char     *unload();
    GB_ERROR  save(char *filename);
    void      print();
    GB_ERROR  calculate_matrix(const char *cancel, double alpha, PH_TRANSFORMATION transformation);
    long get_seq_len() { return seq_len; };

};

#else
#error phylo.hxx included twice
#endif // PHYLO_HXX
