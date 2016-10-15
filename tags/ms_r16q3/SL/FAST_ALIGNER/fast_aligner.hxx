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

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif
#ifndef POS_RANGE_H
#include <pos_range.h>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif

#define INTEGRATED_ALIGNERS_TITLE "Integrated Aligners"

typedef char*   (*Aligner_get_consensus_func)(const char *species_name, PosRange range);
typedef bool    (*Aligner_get_selected_range)(PosRange& range);
typedef GBDATA* (*Aligner_get_first_selected_species)(int *total_no_of_selected_species);
typedef GBDATA* (*Aligner_get_next_selected_species)(void);
typedef char* (*Aligner_get_helix_string)(GBDATA *gb_main, const char *alignment_name); // returns heap-copy!

struct AlignDataAccess {
    GBDATA *gb_main;

    std::string alignment_name;

    bool do_refresh;                                // if do_refresh == true then FastAligner_start() does a refresh
    void (*refresh_display)();                      // via calling refresh_display()

    Aligner_get_consensus_func         get_group_consensus; // changed behavior in [8165]: returns only given range
    Aligner_get_selected_range         get_selected_range;
    Aligner_get_first_selected_species get_first_selected_species;
    Aligner_get_next_selected_species  get_next_selected_species;
    Aligner_get_helix_string           get_helix_string;

    char *getHelixString() const { return get_helix_string(gb_main, alignment_name.c_str()); }

    AlignDataAccess(GBDATA                             *gb_main_,
                    const char                         *alignment_name_,
                    bool                                do_refresh_,
                    void                              (*refresh_display_)(),
                    Aligner_get_consensus_func          get_group_consensus_,
                    Aligner_get_selected_range          get_selected_range_,
                    Aligner_get_first_selected_species  get_first_selected_species_,
                    Aligner_get_next_selected_species   get_next_selected_species_,
                    Aligner_get_helix_string            get_helix_string_)
        : gb_main(gb_main_),
          alignment_name(alignment_name_),
          do_refresh(do_refresh_),
          refresh_display(refresh_display_),
          get_group_consensus(get_group_consensus_),
          get_selected_range(get_selected_range_),
          get_first_selected_species(get_first_selected_species_),
          get_next_selected_species(get_next_selected_species_),
          get_helix_string(get_helix_string_)
    {}

    AlignDataAccess(const AlignDataAccess& other)
        : gb_main(other.gb_main),
          alignment_name(other.alignment_name),
          do_refresh(other.do_refresh),
          refresh_display(other.refresh_display),
          get_group_consensus(other.get_group_consensus),
          get_selected_range(other.get_selected_range),
          get_first_selected_species(other.get_first_selected_species),
          get_next_selected_species(other.get_next_selected_species),
          get_helix_string(other.get_helix_string)
    {}

    DECLARE_ASSIGNMENT_OPERATOR(AlignDataAccess);
};

// --------------------------------------------------------------------------------

AW_window *FastAligner_create_window(AW_root *awr, const AlignDataAccess *data_access);

void FastAligner_create_variables(AW_root *root, AW_default db1);
void FastAligner_set_align_current(AW_root *root, AW_default db1);
void FastAligner_set_reference_species(AW_root *root);

void      FastAligner_start(AW_window *aw, const AlignDataAccess *data_access);
ARB_ERROR FastAligner_delete_temp_entries(GBDATA *gb_main, const char *alignment);

// --------------------------------------------------------------------------------

#else
#error fast_aligner.hxx included twice
#endif // FAST_ALIGNER_HXX
