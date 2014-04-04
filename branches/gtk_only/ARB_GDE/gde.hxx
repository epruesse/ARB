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
#ifndef ARB_CORE_H
#include <arb_core.h>
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

GB_ERROR GDE_create_var(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main, GDE_get_sequences_cb get_sequences, gde_window_type window_type, AW_CL client_data);
void GDE_load_menu(AW_window *awm, AW_active, const char *menulabel);

#else
#error gde.hxx included twice
#endif // GDE_HXX
