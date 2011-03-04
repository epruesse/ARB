// ============================================================ //
//                                                              //
//   File      : ad_spec.hxx                                    //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#ifndef AD_SPEC_HXX
#define AD_SPEC_HXX

#define AWAR_SPECIES_DEST "tmp/adspec/dest"
#define AWAR_SPECIES_INFO "tmp/adspec/info"
#define AWAR_SPECIES_KEY  "tmp/adspec/key"

#define AWAR_FIELD_CREATE_NAME "tmp/adfield/name"
#define AWAR_FIELD_CREATE_TYPE "tmp/adfield/type"
#define AWAR_FIELD_DELETE      "tmp/adfield/source"

#define AWAR_FIELD_CONVERT_SOURCE "tmp/adconvert/source"
#define AWAR_FIELD_CONVERT_NAME   "tmp/adconvert/name"
#define AWAR_FIELD_CONVERT_TYPE   "tmp/adconvert/type"

#define AWAR_FIELD_REORDER_SOURCE "tmp/ad_reorder/source"
#define AWAR_FIELD_REORDER_DEST   "tmp/ad_reorder/dest"

#ifndef AWT_HXX
#include <awt.hxx>
#endif

AW_window *NTX_create_organism_window(AW_root *aw_root, AW_CL cl_gb_main);
AW_window *NTX_create_query_window(AW_root *aw_root);

#else
#error ad_spec.hxx included twice
#endif // AD_SPEC_HXX
