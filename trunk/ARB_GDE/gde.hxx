// ================================================================ //
//                                                                  //
//   File      : gde.hxx                                            //
//   Purpose   : external interface of GDE functionality            //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef GDE_HXX
#define GDE_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif


typedef unsigned char uchar;
enum gde_window_type {
    GDE_WINDOWTYPE_DEFAULT,
    GDE_WINDOWTYPE_EDIT4, 
};

typedef char *(*GDE_get_sequences_cb)(AW_CL     cd,
                                      GBDATA **&the_species,
                                      uchar  **&the_names,
                                      uchar  **&the_sequences,
                                      long     &numberspecies,
                                      long     &maxalignlen);

void GDE_create_var(AW_root              *aw_root,
                    AW_default            aw_def,
                    GBDATA               *gb_main,
                    GDE_get_sequences_cb  get_sequences = 0,
                    gde_window_type       wt            = GDE_WINDOWTYPE_DEFAULT,
                    AW_CL                 client_data   = 0);

void GDE_load_menu(AW_window *awm, AW_active, const char *menulabel=0, const char *menuitemlabel=0);

#else
#error gde.hxx included twice
#endif // GDE_HXX
