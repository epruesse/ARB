// ============================================================= //
//                                                               //
//   File      : macros.hxx                                      //
//   Purpose   : macro interface                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef MACROS_HXX
#define MACROS_HXX

#ifndef ARB_CORE_H
#include <arb_core.h>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

class AW_window;
class AW_root;
class UserActionTracker;

struct GBDATA;

// tracker factory:
UserActionTracker          *need_macro_ability();
__ATTR__USERESULT GB_ERROR  configure_macro_recording(AW_root *aw_root, const char *client_id, GBDATA *gb_main); // replaces active tracker
void                        shutdown_macro_recording(AW_root *aw_root);

bool got_macro_ability(AW_root *aw_root);

// gui-interface:
void insert_macro_menu_entry(AW_window *awm, bool prepend_separator);
void execute_macro(AW_root *root, const char *macroname);

#else
#error macros.hxx included twice
#endif // MACROS_HXX
