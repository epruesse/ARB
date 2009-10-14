// =========================================================== //
//                                                             //
//   File      : ntree.hxx                                     //
//   Purpose   : global ARB_NTREE interface                    //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef NTREE_HXX
#define NTREE_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

#define NT_AW_TRUE  1
#define NT_AW_FALSE 0

class AWT_graphic_tree;

struct NT_global {
    AW_root           *awr;
    AWT_graphic_tree  *tree;
    AW_Window_Creator  window_creator;
    bool               extern_quit_button;
};

extern NT_global  GLOBAL_NT;
extern GBDATA    *GLOBAL_gb_main;


class NT_install_window_creator{
    int dummy;
public:
    NT_install_window_creator( AW_Window_Creator wc ) {
        GLOBAL_NT.window_creator = wc;
    };
};

void nt_main_startup_main_window(AW_root *aw_root);

#define AWAR_EXPORT_NDS                "tmp/export_nds"
#define AWAR_NT_REMOTE_BASE            "tmp/remote/ARB_NT"
#define AWAR_IMPORT_PROBE_GROUP_RESULT "tmp/pg_result"
#define AWAR_MARKED_SPECIES_COUNTER    "tmp/disp_marked_species"
#define AWAR_NTREE_TITLE_MODE          "tmp/title_mode"

#else
#error ntree.hxx included twice
#endif // NTREE_HXX
