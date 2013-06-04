// =============================================================== //
//                                                                 //
//   File      : NT_cb.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ntree.hxx"
#include "nt_internal.h"
#include "nt_cb.hxx"
#include "ad_trees.hxx"

#include <awt_canvas.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>

#include <map>


// AISC_MKPT_PROMOTE:#ifndef ARBDB_BASE_H
// AISC_MKPT_PROMOTE:#include <arbdb_base.h>
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:#ifndef AW_BASE_HXX
// AISC_MKPT_PROMOTE:#include <aw_base.hxx>
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:class AWT_canvas;

#define nt_assert(bed) arb_assert(bed)
#define AWT_TREE(ntw)  DOWNCAST(AWT_graphic_tree *, (ntw)->tree_disp)

void NT_delete_mark_all_cb(void *, AWT_canvas *ntw) {
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
        awt_create_selection_list_on_trees(GLOBAL.gb_main, (AW_window *)aws, awar_tree);

        aws->auto_space(5, 5);
        aws->button_length(6);

        aws->at("button");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(popup_tree_admin_window, (AW_CL)aws);
        aws->help_text("treeadm.hlp");
        aws->create_button("MODIFY", "ADMIN", "A");

        tree_select_windows[awar_tree] = aws;
    }
    return tree_select_windows[awar_tree];
}

void NT_select_bottom_tree(AW_window *aww, const char *awar_tree) {
    GB_transaction dummy(GLOBAL.gb_main);
    const char *ltree = GBT_name_of_bottom_tree(GLOBAL.gb_main);
    if (ltree) aww->get_root()->awar(awar_tree)->write_string(ltree);
}

AW_window *NT_open_select_alignment_window(AW_root *awr)
{
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(awr, "SELECT_ALIGNMENT", "SELECT AN ALIGNMENT");
        aws->load_xfig("select_simple.fig");

        aws->at("selection");
        aws->callback((AW_CB0)AW_POPDOWN);
        awt_create_selection_list_on_alignments(GLOBAL.gb_main, (AW_window *)aws, AWAR_DEFAULT_ALIGNMENT, "*=");

        aws->auto_space(5, 5);
        aws->button_length(6);

        aws->at("button");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(AW_POPUP, (AW_CL)NT_create_alignment_window, (AW_CL)aws);
        aws->help_text("ad_align.hlp");
        aws->create_button("MODIFY", "ADMIN", "A");

        aws->window_fit();
    }
    return aws;
}

void NT_system_cb(AW_window *aww, AW_CL cl_command, AW_CL cl_auto_help_file) {
    AW_system(aww, (const char *)cl_command, (const char *)cl_auto_help_file);
}

void NT_system_in_xterm_cb(AW_window *aww, AW_CL cl_command, AW_CL cl_auto_help_file) {
    char *command = (char *)cl_command;
    if (cl_auto_help_file) AW_POPUP_HELP(aww, cl_auto_help_file);

    GB_ERROR error = GB_xcmd(command, true, true);
    if (error) aw_message(error);
}
