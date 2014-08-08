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

class AW_window;
class AW_root;

void MG_alignment_vars_callback(AW_root *aw_root, int db_nr);
void MG_create_alignment_awars(AW_root *aw_root, AW_default aw_def);
AW_window *MG_create_alignment_window(AW_root *root, int db_nr);

#else
#error MG_ali_admin.hxx included twice
#endif // MG_ALI_ADMIN_HXX
