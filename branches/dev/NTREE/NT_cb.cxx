// =============================================================== //
//                                                                 //
//   File      : NT_cb.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "nt_internal.h"
#include "nt_cb.hxx"
#include "ntree.hxx"
#include "ad_trees.hxx"

#include <awt_canvas.hxx>
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>

// AISC_MKPT_PROMOTE:#ifndef ARBDB_BASE_H
// AISC_MKPT_PROMOTE:#include <arbdb_base.h>
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:#ifndef AW_BASE_HXX
// AISC_MKPT_PROMOTE:#include <aw_base.hxx>
// AISC_MKPT_PROMOTE:#endif
// AISC_MKPT_PROMOTE:
// AISC_MKPT_PROMOTE:class AW_window;
// AISC_MKPT_PROMOTE:class AWT_canvas;

#define AWT_TREE(ntw) DOWNCAST(AWT_graphic_tree *, (ntw)->tree_disp)

void NT_delete_mark_all_cb(void *, AWT_canvas *ntw) {
    if (aw_ask_sure("Are you sure to delete species ??\n"
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


AW_window * NT_open_select_tree_window(AW_root *awr, char *awar_tree) {
    AW_window_simple *aws;

    aws = new AW_window_simple;
    aws->init(awr, "SELECT_TREE", "SELECT A TREE");
    aws->load_xfig("select_simple.fig");

    aws->at("selection");
    awt_create_selection_list_on_trees(GLOBAL_gb_main, (AW_window *)aws, awar_tree);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("modify");
    aws->callback(AW_POPUP, (AW_CL)create_trees_window, 0);
    aws->help_text("treeadm.hlp");
    aws->create_button("MODIFY", "MODIFY", "M");

    return (AW_window *)aws;
}

void NT_select_last_tree(AW_window *aww, char *awar_tree) {
    GB_transaction dummy(GLOBAL_gb_main);
    char *ltree = GBT_find_latest_tree(GLOBAL_gb_main);
    if (ltree) {
        aww->get_root()->awar(awar_tree)->write_string(ltree);
        free(ltree);
    }
}

AW_window *NT_open_select_alignment_window(AW_root *awr)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init(awr, "SELECT_ALIGNMENT", "SELECT AN ALIGNMENT");
    aws->load_xfig("select_simple.fig");

    aws->at("selection");
    aws->auto_space(0, 0);
    aws->callback((AW_CB0)AW_POPDOWN);
    awt_create_selection_list_on_alignments(GLOBAL_gb_main, (AW_window *)aws, AWAR_DEFAULT_ALIGNMENT, "*=");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("modify");
    aws->callback(AW_POPUP, (AW_CL)NT_create_alignment_window, (AW_CL)aws);
    aws->help_text("ad_align.hlp");
    aws->create_button("MODIFY", "MODIFY", "M");

    aws->window_fit();
    return aws;
}

void NT_system_cb(AW_window *aww, AW_CL cl_command, AW_CL cl_auto_help_file) {
    AW_system(aww, GLOBAL_gb_main, (const char *)cl_command, (const char *)cl_auto_help_file);
}

