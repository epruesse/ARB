#ifndef ntree_hxx_included
#define ntree_hxx_included


#define NT_AW_TRUE 1
#define NT_AW_FALSE 0
class AWT_graphic_tree;

struct NT_global {
    AW_root *awr;
    AWT_graphic_tree *tree;
    AW_Window_Creator window_creator;
    AW_BOOL	extern_quit_button;
};

extern NT_global nt;
extern GBDATA *gb_main;


class NT_install_window_creator{
    int dummy;
public:
    NT_install_window_creator( AW_Window_Creator wc ) {
        nt.window_creator = wc;
    };
};

void nt_main_startup_main_window(AW_root *aw_root);

#define AWAR_EXPORT_NDS                "tmp/export_nds"
#define AWAR_NT_REMOTE_BASE            "tmp/remote/ARB_NT"
#define AWAR_IMPORT_PROBE_GROUP_RESULT "tmp/pg_result"
#define AWAR_MARKED_SPECIES_COUNTER    "tmp/disp_marked_species"
#define AWAR_NTREE_TITLE_MODE          "tmp/title_mode"

#endif
