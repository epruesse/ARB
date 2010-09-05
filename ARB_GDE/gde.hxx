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

typedef unsigned char uchar;
typedef enum { CGSS_WT_DEFAULT, CGSS_WT_EDIT, CGSS_WT_EDIT4 } gde_cgss_window_type;

void create_gde_var(AW_root *aw_root, AW_default aw_def,
                    char *(*get_sequences)(void *THIS, GBDATA **&the_species,
                                           uchar **&the_names,
                                           uchar **&the_sequences,
                                           long &numberspecies,long &maxalignlen)=0,
                    gde_cgss_window_type wt=CGSS_WT_DEFAULT,
                    void *THIS = 0);

void GDE_load_menu(AW_window *awm,AW_active, const char *menulabel=0,const char *menuitemlabel=0 );

#else
#error gde.hxx included twice
#endif // GDE_HXX
