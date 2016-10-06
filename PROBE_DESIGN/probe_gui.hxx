// ============================================================ //
//                                                              //
//   File      : probe_gui.hxx                                  //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                              //
// ============================================================ //

#ifndef PROBE_GUI_HXX
#define PROBE_GUI_HXX

class TREE_canvas;
class AW_window;
class AW_root;
class GBDATA;

AW_window *create_probe_design_window(AW_root *root, GBDATA *gb_main);
AW_window *create_probe_match_window(AW_root *root, GBDATA *gb_main);
AW_window *create_probe_admin_window(AW_root *root, GBDATA *gb_main);
AW_window *create_probe_match_with_specificity_window(AW_root *root, TREE_canvas *ntw);

#define AWAR_PROBE_ADMIN_PT_SERVER "tmp/probe_admin/pt_server"

#else
#error probe_gui.hxx included twice
#endif // PROBE_GUI_HXX
