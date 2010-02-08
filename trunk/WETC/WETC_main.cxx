#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <awt.hxx>


AW_HEADER_MAIN


int main(int argc, char **argv) {
    if (argc <1) {
        GB_export_error("Syntax: arb_wetc [-fileedit]");
        GB_print_error();
        exit(-1);
    }

    const char *com        = argv[1];
    AW_root    *aw_root    = new AW_root;
    AW_default  aw_default = AWT_open_properties(aw_root, ".arb_prop/ntree.arb");
    
    aw_root->init_variables(aw_default);
    aw_root->init_root("ARB_WETC", false);

    if (!strcmp(com, "-fileedit")) {
        AWT_show_file(aw_root, argv[2]);
    }
    else {
        GB_export_error("wrong parameter, allowed: [-fileedit] file");
        GB_print_error();
        exit(-1);
    }
    aw_root->window_hide();
    AWT_install_cb_guards();
    aw_root->main_loop();
    return 0;
}

void AD_map_viewer(GBDATA *, AD_MAP_VIEWER_TYPE)
{
    ;
}
