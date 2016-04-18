// =============================================================== //
//                                                                 //
//   File      : NT_cb.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include "ad_trees.h"

#include <awt_canvas.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>

#include <map>
#include <AliAdmin.h>


// AISC_MKPT_PROMOTE:#ifndef ARBDB_BASE_H
// AISC_MKPT_PROMOTE:#include <arbdb_base.h>
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:#ifndef AW_BASE_HXX
// AISC_MKPT_PROMOTE:#include <aw_base.hxx>
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:class AWT_canvas;

#define AWT_TREE(ntw)  DOWNCAST(AWT_graphic_tree *, (ntw)->tree_disp)

void NT_delete_mark_all_cb(AW_window*, AWT_canvas *ntw) {
    if (aw_ask_sure("delete_marked_species",
                    "Are you sure to delete species ??\n"
                    "This will destroy primary data !!!"))
    {
        {
            GB_ERROR       error = 0;
            GB_transaction ta(ntw->gb_main);

            GBDATA *gb_species, *gb_next;
            for (gb_species = GBT_first_marked_species(ntw->gb_main);
                 gb_species && !error;
                 gb_species = gb_next)
            {
                gb_next = GBT_next_marked_species(gb_species);
                error   = GB_delete(gb_species);
            }

            if (error) {
                error = ta.close(error);
                aw_message(error);
            }
        }
        ntw->refresh();
    }
}


AW_window *NT_create_select_tree_window(AW_root *awr, const char *awar_tree) {
    static std::map<std::string,AW_window*> tree_select_windows;
    if (tree_select_windows.find(awar_tree) == tree_select_windows.end()) {
        AW_window_simple *aws = new AW_window_simple;

        const char *id = strrchr(awar_tree, '/');
        nt_assert(id);
        id++; // use name-part of awar_tree as id (results in 'SELECT_tree_name', 'SELECT_tree_name_1', ...)

        aws->init(awr, GBS_global_string("SELECT_%s", id), "SELECT A TREE");
        aws->load_xfig("select_simple.fig");

        aws->at("selection");
        awr->awar(awar_tree)->add_callback(makeRootCallback(awt_auto_popdown_cb, aws));
        awt_create_TREE_selection_list(GLOBAL.gb_main, aws, awar_tree, true);

        aws->auto_space(5, 5);
        aws->button_length(6);

        aws->at("button");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(popup_tree_admin_window);
        aws->help_text("treeadm.hlp");
        aws->create_button("MODIFY", "ADMIN", "A");

        tree_select_windows[awar_tree] = aws;
    }
    return tree_select_windows[awar_tree];
}

void NT_select_bottom_tree(AW_window *aww, const char *awar_tree) {
    GB_transaction ta(GLOBAL.gb_main);
    const char *ltree = GBT_name_of_bottom_tree(GLOBAL.gb_main);
    if (ltree) aww->get_root()->awar(awar_tree)->write_string(ltree);
}

void NT_create_alignment_vars(AW_root *aw_root, AW_default aw_def, GBDATA *gb_main) {
    // map awar containing selected alignment with db-entry (both contain same value; historical)
    // - allows access via AWAR_DEFAULT_ALIGNMENT and GBT_get_default_alignment

    AW_awar        *awar_def_ali = aw_root->awar_string(AWAR_DEFAULT_ALIGNMENT, "", aw_def);
    GB_transaction  ta(gb_main);
    GBDATA         *gb_use       = GB_search(gb_main, GB_DEFAULT_ALIGNMENT, GB_STRING);

    awar_def_ali->map(gb_use);
}

AW_window *NT_create_alignment_admin_window(AW_root *root, AW_window *aw_popmedown) {
    // if 'aw_popmedown' points to a window, that window is popped down
    if (aw_popmedown) aw_popmedown->hide();

    static AliAdmin *ntreeAliAdmin    = NULL;
    if (!ntreeAliAdmin) ntreeAliAdmin = new AliAdmin(MAIN_ADMIN, GLOBAL.gb_main, AWAR_DEFAULT_ALIGNMENT, "tmp/presets/");

    return ALI_create_admin_window(root, ntreeAliAdmin);
}

AW_window *NT_create_select_alignment_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(awr, "SELECT_ALIGNMENT", "SELECT AN ALIGNMENT");
        aws->load_xfig("select_simple.fig");

        aws->at("selection");
        awr->awar(AWAR_DEFAULT_ALIGNMENT)->add_callback(makeRootCallback(awt_auto_popdown_cb, aws));
        awt_create_ALI_selection_list(GLOBAL.gb_main, aws, AWAR_DEFAULT_ALIGNMENT, "*=");

        aws->auto_space(5, 5);
        aws->button_length(6);

        aws->at("button");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeCreateWindowCallback(NT_create_alignment_admin_window, static_cast<AW_window*>(aws)));
        aws->help_text("ad_align.hlp");
        aws->create_button("MODIFY", "ADMIN", "A");

        aws->window_fit();
    }
    return aws;
}

void NT_system_cb(AW_window *, const char *command) {
    aw_message_if(GBK_system(command));
}
void NT_xterm(AW_window*) {
    GB_xterm();
}
void NT_system_in_xterm_cb(AW_window*, const char *command) {
    GB_ERROR error = GB_xcmd(command, true, true);
    if (error) aw_message(error);
}
