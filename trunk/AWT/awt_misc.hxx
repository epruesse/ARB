// ============================================================ //
//                                                              //
//   File      : awt_misc.hxx                                   //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2015   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef AWT_MISC_HXX
#define AWT_MISC_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

AW_window *AWT_create_IUPAC_info_window(AW_root *aw_root);

void AWT_insert_DBsaveType_selector(AW_window *aww, const char *awar_name);
void AWT_insert_DBcompression_selector(AW_window *aww, const char *awar_name);

void AWT_system_cb(AW_window *, const char *command);
void AWT_console(AW_window*);
void AWT_system_in_console_cb(AW_window*, const char *command);

#else
#error awt_misc.hxx included twice
#endif // AWT_MISC_HXX
