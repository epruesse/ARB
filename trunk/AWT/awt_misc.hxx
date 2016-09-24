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
#ifndef ARBDB_H
#include <arbdb.h>
#endif

AW_window *AWT_create_IUPAC_info_window(AW_root *aw_root);

void AWT_insert_DBsaveType_selector(AW_window *aww, const char *awar_name);
void AWT_insert_DBcompression_selector(AW_window *aww, const char *awar_name);

// system/shell interface (for GUI apps):
// (note: AW_window parameter is an unused callback-dummy)
void AWT_system_cb(AW_window *, const char *command);
void AWT_console(AW_window*);
void AWT_system_in_console_cb(AW_window*, const char *command, XCMD_TYPE exectype);

// for direct calls use these:
inline void AWT_system_cb(const char *command) { AWT_system_cb(NULL, command); }
inline void AWT_console() { AWT_console(NULL); }
inline void AWT_system_in_console_cb(const char *command, XCMD_TYPE exectype) { AWT_system_in_console_cb(NULL, command, exectype); }

#else
#error awt_misc.hxx included twice
#endif // AWT_MISC_HXX
