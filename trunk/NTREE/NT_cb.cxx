#include <stdio.h>
#include <stdlib.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>

#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include "ad_trees.hxx"
#include "ntree.hxx"
#include "nt_internal.h"
#include "nt_cb.hxx"


#define AWT_TREE(ntw) ((AWT_graphic_tree *)ntw->tree_disp)

void
NT_delete_mark_all_cb(void *dummy, AWT_canvas *ntw) {
    AWUSE(dummy);
    GB_ERROR error = 0;
    if (aw_message("Are you sure to delete species ??\n"
                   "This will destroy primary data !!!","YES,NO")) return;
    GB_begin_transaction(ntw->gb_main);
    GBDATA *gb_species,*gb_next;
    for (gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species = gb_next ) {
        gb_next = GBT_next_marked_species(gb_species);
        if (!error) error = GB_delete(gb_species);
        else    break;
    }
    if (error) {
        aw_message(error);
        GB_abort_transaction(ntw->gb_main);
    }else{
        GB_commit_transaction(ntw->gb_main);
    }
    ntw->refresh();
}


AW_window *
NT_open_select_tree_window(AW_root *awr,char *awar_tree)
{
    AW_window_simple *aws;

    aws = new AW_window_simple;
    aws->init( awr, "SELECT_TREE", "SELECT A TREE");
    aws->load_xfig("select_simple.fig");

    aws->at("selection");
//  aws->callback((AW_CB0)AW_POPDOWN);
    awt_create_selection_list_on_trees(gb_main,(AW_window *)aws,awar_tree);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("modify");
    aws->callback(AW_POPUP,(AW_CL)create_trees_window,0);
    aws->help_text("treeadm.hlp");
    aws->create_button("MODIFY","MODIFY","M");

    return (AW_window *)aws;
}

void NT_select_last_tree(AW_window *aww,char *awar_tree){
    GB_transaction dummy(gb_main);
    char *ltree = GBT_find_latest_tree(gb_main);
    if (ltree){
        aww->get_root()->awar(awar_tree)->write_string(ltree);
        free(ltree);
    }
}

AW_window *NT_open_select_alignment_window(AW_root *awr)
{
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( awr, "SELECT_ALIGNMENT", "SELECT AN ALIGNMENT");
    aws->load_xfig("select_simple.fig");

    aws->at("selection");
    //  aws->at(10,10);
    aws->auto_space(0,0);
    aws->callback((AW_CB0)AW_POPDOWN);
    awt_create_selection_list_on_ad(gb_main,(AW_window *)aws,AWAR_DEFAULT_ALIGNMENT,"*=");
    //  aws->at_newline();

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("modify");
    aws->callback(AW_POPUP,(AW_CL)NT_create_alignment_window, (AW_CL)aws);
    aws->help_text("ad_align.hlp");
    aws->create_button("MODIFY","MODIFY","M");

    aws->window_fit();
    return (AW_window *)aws;
}

void NT_system_cb(AW_window *aww, AW_CL command, AW_CL auto_help_file)
{
    char *sys = (char *)command;
    if (auto_help_file) {
        AW_POPUP_HELP(aww,auto_help_file);
    }
    GBCMC_system(gb_main,sys);
}

void NT_system_cb2(AW_window *aww, AW_CL command, AW_CL auto_help_file)
{
    char *sys = (char *)command;
    if (auto_help_file) {
        AW_POPUP_HELP(aww,auto_help_file);
    }
    GB_xcmd(sys,GB_TRUE, GB_FALSE);
}
