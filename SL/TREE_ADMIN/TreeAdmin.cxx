// ============================================================= //
//                                                               //
//   File      : TreeAdmin.cxx                                   //
//   Purpose   : Common tree admin functionality                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "TreeAdmin.h"
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <arbdbt.h>
#include <arb_strbuf.h>
#include <cctype>
#include <aw_msg.hxx>
#include <arb_global_defs.h>
#include <awt_TreeAwars.hxx>

#define ta_assert(cond) arb_assert(cond)

#define AWAR_TREE_SOURCE "tmp/ad_tree/tree_source"
#define AWAR_TREE_DEST   "tmp/ad_tree/tree_dest"

namespace TreeAdmin {

    void create_awars(AW_root *root, AW_default aw_def, bool registerTreeAwar) {
        AW_awar *awar_srcTree = root->awar_string(AWAR_TREE_SOURCE, 0, aw_def)->set_srt(GBT_TREE_AWAR_SRT);
        if (registerTreeAwar) AWT_registerTreeAwarSimple(awar_srcTree);
        root->awar_string(AWAR_TREE_DEST, 0, aw_def)->set_srt(GBT_TREE_AWAR_SRT); // no need to register (awar always follows the tree selected in admin window!)
    }
    AW_awar *source_tree_awar(AW_root *root) {
        return root->awar(AWAR_TREE_SOURCE);
    }
    AW_awar *dest_tree_awar(AW_root *root) {
        return root->awar(AWAR_TREE_DEST);
    }

    AW_awar *Spec::tree_awar(AW_root *awr) const {
        return awr->awar(awar_selected_tree);
    }

    void delete_tree_cb(AW_window *aww, const Spec *spec) {
        AW_awar    *awar_tree = spec->tree_awar(aww->get_root());
        char       *name      = awar_tree->read_string();
        GBDATA     *gb_main   = spec->get_gb_main();
        GB_ERROR    error     = NULL;
        GBDATA     *gb_tree;

        // 1. TA: switch to next tree
        {
            GB_transaction ta(gb_main);
            gb_tree = GBT_find_tree(gb_main, name);
            if (!gb_tree) error = "Please select tree to delete";
            else {
                AWT_announce_tree_deleted(name);
                GBDATA *gb_next = GBT_find_next_tree(gb_tree);
                awar_tree->write_string(gb_next ? GBT_get_tree_name(gb_next) : NO_TREE_SELECTED);
            }
            error = ta.close(error);
        }

        // 2. TA: delete old tree
        if (!error) {
            GB_transaction ta(gb_main);
            error = GB_delete(gb_tree);
            error = ta.close(error);
        }

        if (error) {
            aw_message(error);
            awar_tree->write_string(name); // switch back to failed tree
        }

        free(name);
    }
    
    static GB_ERROR tree_append_remark(GBDATA *gb_tree, const char *add_to_remark) {
        GB_ERROR  error       = 0;
        GBDATA   *gb_remark   = GB_search(gb_tree, "remark", GB_STRING);
        if (!gb_remark) error = GB_await_error();
        else {
            char *old_remark       = GB_read_string(gb_remark);
            if (!old_remark) error = GB_await_error();
            else {
                GBS_strstruct *new_remark = GBS_stropen(2000);

                GBS_strcat(new_remark, old_remark);
                GBS_chrcat(new_remark, '\n');
                GBS_strcat(new_remark, add_to_remark);

                error = GB_write_string(gb_remark, GBS_mempntr(new_remark));

                GBS_strforget(new_remark);
            }
            free(old_remark);
        }
        return error;
    }

    static void tree_copy_or_rename_cb(AW_window *aww, bool do_copy, const Spec& spec) {
        AW_root  *aw_root   = aww->get_root();
        AW_awar  *awar_tree = spec.tree_awar(aw_root);
        char     *source    = awar_tree->read_string();
        char     *dest      = aw_root->awar(AWAR_TREE_DEST)->read_string();
        GB_ERROR  error     = NULL;

        if (!error && !dest[0]) error = "Please enter new tree name";
        if (!error) {
            GBDATA *gb_main = spec.get_gb_main();
            error           = GB_begin_transaction(gb_main);
            if (!error) {
                if (do_copy) {
                    error = GBT_copy_tree(gb_main, source, dest);
                    if (!error) {
                        GBDATA *gb_new_tree = GBT_find_tree(gb_main, dest);
                        ta_assert(gb_new_tree);
                        error = tree_append_remark(gb_new_tree, GBS_global_string("[created as copy of '%s']", source));
                    }
                }
                else {
                    error = GBT_rename_tree(gb_main, source, dest);
                    if (!error) AWT_announce_tree_renamed(source, dest);
                }
            }

            if (!error) awar_tree->write_string(dest);
            error = GB_end_transaction(gb_main, error);
        }

        aww->hide_or_notify(error);

        free(dest);
        free(source);
    }

    static void tree_rename_cb(AW_window *aww, const Spec *spec) { tree_copy_or_rename_cb(aww, false, *spec); }
    static void tree_copy_cb  (AW_window *aww, const Spec *spec) { tree_copy_or_rename_cb(aww, true, *spec);  }

    static void current_as_dest_treename_cb(AW_window *aww, const Spec *spec) {
        AW_root    *awr  = aww->get_root();
        dest_tree_awar(awr)->write_string(spec->tree_awar(awr)->read_char_pntr());
    }

    static void make_dest_treename_unique_cb(AW_window *aww, const Spec *spec) {
        // generated a unique treename
        AW_root *awr       = aww->get_root();
        AW_awar *awar_dest = awr->awar(AWAR_TREE_DEST);

        char *name = awar_dest->read_string();
        int   len  = strlen(name);

        for (int p = len-1; p >= 0; --p) {
            bool auto_modified = isdigit(name[p]) || name[p] == '_';
            if (!auto_modified) break;
            name[p] = 0;
        }

        {
            GBDATA         *gb_main = spec->get_gb_main();
            GB_transaction  ta(gb_main);

            if (!GBT_find_tree(gb_main, name)) {
                awar_dest->write_string("");
                awar_dest->write_string(name);
            }
            else {
                for (int i = 2; ; i++) {
                    const char *testName = GBS_global_string("%s_%i", name, i);
                    if (!GBT_find_tree(gb_main, testName)) { // found unique name
                        awar_dest->write_string(testName);
                        break;
                    }
                }
            }
        }

        free(name);
    }

    static AW_window *create_copy_or_rename_window(AW_root *root, const char *win_id, const char *win_title, const char *go_label, void (*go_cb)(AW_window*, const Spec*), const Spec *spec) {
        AW_window_simple *aws = new AW_window_simple;
        aws->init(root, win_id, win_title);

        aws->at(10, 10);
        aws->auto_space(10, 10);

        aws->at_newline();
        aws->label("Current:");
        aws->callback(makeWindowCallback(current_as_dest_treename_cb, spec));
        aws->at_set_to(true, false, -10, 25);
        aws->create_button("use_current", spec->tree_awar(aws->get_root())->awar_name);

        aws->at_newline();
        aws->label("New:    ");
        aws->at_set_to(true, false, -10, 30);
        aws->create_input_field(AWAR_TREE_DEST);

        aws->at_newline();
        aws->callback(makeWindowCallback(go_cb, spec));
        aws->create_autosize_button("GO", go_label, "");

        aws->callback(AW_POPDOWN);
        aws->create_autosize_button("CLOSE", "Abort", "A");

        aws->callback(makeWindowCallback(make_dest_treename_unique_cb, spec));
        aws->create_autosize_button("UNIQUE", "Unique name", "U");
    
        aws->at_newline();

        return aws;
    }

    
    AW_window *create_rename_window(AW_root *root, const Spec *spec) {
        return create_copy_or_rename_window(root, "RENAME_TREE", "Rename Tree", "Rename Tree", tree_rename_cb, spec);
    }
    AW_window *create_copy_window(AW_root *root, const Spec *spec) {
        return create_copy_or_rename_window(root, "COPY_TREE",   "Copy Tree",   "Copy Tree",   tree_copy_cb, spec);
    }

    
};

