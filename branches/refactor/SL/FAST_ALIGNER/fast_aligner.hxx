// =========================================================== //
//                                                             //
//   File      : fast_aligner.hxx                              //
//   Purpose   :                                               //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in 1998       //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef FAST_ALIGNER_HXX
#define FAST_ALIGNER_HXX

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif

#define INTEGRATED_ALIGNERS_TITLE "Integrated Aligners"

typedef char*   (*Aligner_get_consensus_func)(const char *species_name, int start_pos, int end_pos);
typedef int     (*Aligner_get_selected_range)(int *firstColumn, int *lastColumn);
typedef GBDATA* (*Aligner_get_first_selected_species)(int *total_no_of_selected_species);
typedef GBDATA* (*Aligner_get_next_selected_species)(void);

struct AlignDataAccess {
    int do_refresh;                                 // if do_refresh == TRUE then FastAligner_start() does a refresh
    void (*refresh_display)();                      // via calling refresh_display()

    Aligner_get_consensus_func         get_group_consensus;
    Aligner_get_selected_range         get_selected_range;
    Aligner_get_first_selected_species get_first_selected_species;
    Aligner_get_next_selected_species  get_next_selected_species;

    char   *helix_string;                           // currently only used for island hopping
    GBDATA *gb_main;                                // used by faligner
};

// --------------------------------------------------------------------------------

AW_window *FastAligner_create_window(AW_root *awr, const AlignDataAccess *data_access);

void FastAligner_create_variables(AW_root *root, AW_default db1);
void FastAligner_set_align_current(AW_root *root, AW_default db1);
void FastAligner_set_reference_species(AW_window *, AW_CL cl_AW_root);

void      FastAligner_start(AW_window *aw, AW_CL cl_AlignDataAccess);
ARB_ERROR FastAligner_delete_temp_entries(GBDATA *gb_main, GB_CSTR alignment);

// --------------------------------------------------------------------------------

#else
#error fast_aligner.hxx included twice
#endif // FAST_ALIGNER_HXX
