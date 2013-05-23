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

#define MAX_NT_WINDOWS          5
#define MAX_NT_WINDOWS_NULLINIT NULL,NULL,NULL,NULL,NULL

class AWT_graphic_tree;

struct NT_global {
    AW_root           *aw_root;
    GBDATA            *gb_main;
    AW_Window_Creator  window_creator;

    NT_global()
        : aw_root(NULL),
          gb_main(NULL),
          window_creator(NULL)
    {}
};

extern NT_global GLOBAL;

#define AWAR_EXPORT_NDS                "tmp/export_nds"
#define AWAR_EXPORT_NDS_SEPARATOR      "tmp/export_nds/separator"
#define AWAR_NT_REMOTE_BASE            "tmp/remote/ARB_NT"
#define AWAR_IMPORT_PROBE_GROUP_RESULT "tmp/pg_result"
#define AWAR_MARKED_SPECIES_COUNTER    "tmp/disp_marked_species"
#define AWAR_NTREE_TITLE_MODE          "tmp/title_mode"

#else
#error ntree.hxx included twice
#endif // NTREE_HXX
