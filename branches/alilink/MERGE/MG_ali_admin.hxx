// ============================================================== //
//                                                                //
//   File      : MG_ali_admin.hxx                                 //
//   Purpose   : interface to ali admin (merge)                   //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2014   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef MG_ALI_ADMIN_HXX
#define MG_ALI_ADMIN_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif

class AliAdmin;

AW_window *MG_create_AliAdmin_window(AW_root *root, AliAdmin *admin);

#else
#error MG_ali_admin.hxx included twice
#endif // MG_ALI_ADMIN_HXX
