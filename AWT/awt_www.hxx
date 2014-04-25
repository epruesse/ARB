// =========================================================== //
//                                                             //
//   File      : awt_www.hxx                                   //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_WWW_HXX
#define AWT_WWW_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif


AW_window *AWT_create_www_window(AW_root *aw_root, GBDATA *gb_main);
void       awt_openDefaultURL_on_species(AW_window *aww, GBDATA *gb_main);
void       awt_create_aww_vars(AW_root *aw_root, AW_default aw_def);
GB_ERROR   awt_openURL_by_gbd(AW_root *aw_root, GBDATA *gb_main, GBDATA *gbd, const char *name);
GB_ERROR   awt_open_ACISRT_URL_by_gbd(AW_root *aw_root, GBDATA *gb_main, GBDATA *gbd, const char *name, const char *url_srt);

void AWT_openURL(AW_window *aww, const char *url);

#else
#error awt_www.hxx included twice
#endif // AWT_WWW_HXX
