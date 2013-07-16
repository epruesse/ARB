// ============================================================= //
//                                                               //
//   File      : dbserver.hxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2013   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef DBSERVER_HXX
#define DBSERVER_HXX

#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARB_CORE_H
#include <arb_core.h>
#endif

class AW_root;
struct GBDATA;

__ATTR__USERESULT GB_ERROR startup_dbserver(AW_root *aw_root, const char *application_id, GBDATA *gb_main);
__ATTR__USERESULT GB_ERROR reconfigure_dbserver(const char *application_id, GBDATA *gb_main);

#else
#error dbserver.hxx included twice
#endif // DBSERVER_HXX
