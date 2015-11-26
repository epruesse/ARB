// =============================================================== //
//                                                                 //
//   File      : ad_trees.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"
#include "ad_trees.h"
#include "NT_tree_cmp.h"

#include <CT_ctree.hxx>

#include <TreeAdmin.h>
#include <TreeRead.h>
#include <TreeWrite.h>
#include <TreeCallbacks.hxx>

#include <awt_sel_boxes.hxx>
#include <awt_modules.hxx>
#include <awt_TreeAwars.hxx>

#include <aw_awars.hxx>
#include <aw_edit.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_select.hxx>

#include <arb_strbuf.h>
#include <arb_file.h>
#include <arb_diff.h>

#include <cctype>
#include <awt_config_manager.hxx>

#define AWAR_TREE_SAV "ad_tree/"
#define AWAR_TREE_TMP "tmp/ad_tree/"

#define AWAR_TREE_SECURITY         AWAR_TREE_TMP "tree_security"
#define AWAR_TREE_REM              AWAR_TREE_TMP "tree_rem"
#define AWAR_TREE_IMPORT           AWAR_TREE_TMP "import_tree"
#define AWAR_NODE_INFO_ONLY_MARKED AWAR_TREE_TMP "import_only_marked_node_info"

#define AWAR_TREE_EXPORT_FILEBASE AWAR_TREE_TMP "export_tree"
#define AWAR_TREE_EXPORT_FILTER   AWAR_TREE_EXPORT_FILEBASE "/filter"
#define AWAR_TREE_EXPORT_NAME     AWAR_TREE_EXPORT_FILEBASE "/file_name"

#define AWAR_TREE_EXPORT_SAV AWAR_TREE_SAV "export_tree/"

#define AWAR_TREE_EXPORT_FORMAT             AWAR_TREE_EXPORT_SAV "format"
#define AWAR_TREE_EXPORT_NDS                AWAR_TREE_EXPORT_SAV "NDS"
#define AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS AWAR_TREE_EXPORT_SAV "bootstraps"
#define AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS AWAR_TREE_EXPORT_SAV "branchlens"
#define AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES AWAR_TREE_EXPORT_SAV "groupnames"
#define AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS AWAR_TREE_EXPORT_SAV "hide_folded"
#define AWAR_TREE_EXPORT_QUOTEMODE          AWAR_TREE_EXPORT_SAV "quote_mode"
#define AWAR_TREE_EXPORT_REPLACE            AWAR_TREE_EXPORT_SAV "replace"


#define AWAR_TREE_CONSENSE_TMP AWAR_TREE_TMP "consense/"
#define AWAR_TREE_CONSENSE_SAV AWAR_TREE_SAV "consense/"

#define AWAR_TREE_CONSENSE_TREE     AWAR_TREE_CONSENSE_SAV "tree"
#define AWAR_TREE_CONSENSE_SELECTED AWAR_TREE_CONSENSE_TMP "selected"

static void tree_vars_callback(AW_root *aw_root) // Map tree vars to display objects
{
    if (GLOBAL.gb_main) {
        GB_push_transaction(GLOBAL.gb_main);
        char *treename = aw_root->awar(AWAR_TREE_NAME)->read_string();
        GBDATA *ali_cont = GBT_find_tree(GLOBAL.gb_main, treename);
        if (!ali_cont) {
            aw_root->awar(AWAR_TREE_SECURITY)->unmap();
            aw_root->awar(AWAR_TREE_REM)->unmap();
        }
        else {
            GBDATA *tree_prot = GB_search(ali_cont, "security", GB_FIND);
            if (!tree_prot) GBT_readOrCreate_int(ali_cont, "security", GB_read_security_write(ali_cont));
            tree_prot         = GB_search(ali_cont, "security", GB_INT);

            GBDATA *tree_rem = GB_search(ali_cont, "remark",   GB_STRING);
            aw_root->awar(AWAR_TREE_SECURITY)->map(tree_prot);
            aw_root->awar(AWAR_TREE_REM)     ->map(tree_rem);
        }
        char *suffix = aw_root->awar(AWAR_TREE_EXPORT_FILTER)->read_string();
        char *fname  = GBS_string_eval(treename, GBS_global_string("*=*1.%s:tree_*=*1", suffix), 0);
        aw_root->awar(AWAR_TREE_EXPORT_NAME)->write_string(fname); // create default file name
        free(fname);
        free(suffix);
        GB_pop_transaction(GLOBAL.gb_main);
        free(treename);
    }
}

static void update_default_treename_cb(AW_root *aw_root) {
    // update import tree name depending on file name
    GB_transaction ta(GLOBAL.gb_main);

    char *treename        = aw_root->awar(AWAR_TREE_IMPORT "/file_name")->read_string();
    char *treename_nopath = strrchr(treename, '/');

    if (treename_nopath) {
        ++treename_nopath;
    }
    else {
        treename_nopath = treename;
    }

    char *fname = GBS_string_eval(treename_nopath, "*.tree=tree_*1:*.ntree=tree_*1:*.xml=tree_*1:.=:-=_: =_", 0);
    aw_root->awar(AWAR_TREE_IMPORT "/tree_name")->write_string(fname);

    free(fname);
    free(treename);
}

static void ad_tree_set_security(AW_root *aw_root)
{
    if (GLOBAL.gb_main) {
        GB_transaction ta(GLOBAL.gb_main);
        char *treename = aw_root->awar(AWAR_TREE_NAME)->read_string();
        GBDATA *ali_cont = GBT_find_tree(GLOBAL.gb_main, treename);
        if (ali_cont) {
            long prot = aw_root->awar(AWAR_TREE_SECURITY)->read_int();
            long old;
            old = GB_read_security_delete(ali_cont);
            GB_ERROR error = 0;
            if (old != prot) {
                error = GB_write_security_delete(ali_cont, prot);
                if (!error)
                    error = GB_write_security_write(ali_cont, prot);
            }
            if (error) aw_message(error);
        }
        free(treename);
    }
}

enum ExportTreeType {
    AD_TREE_EXPORT_FORMAT_NEWICK,
    AD_TREE_EXPORT_FORMAT_XML,
    AD_TREE_EXPORT_FORMAT_NEWICK_PRETTY,
};

enum ExportNodeType {
    AD_TREE_EXPORT_NODE_SPECIES_NAME,
    AD_TREE_EXPORT_NODE_NDS
};

static void update_filter_cb(AW_root *root) {
    const char *filter_type = 0;

    switch (ExportTreeType(root->awar(AWAR_TREE_EXPORT_FORMAT)->read_int())) {
        case AD_TREE_EXPORT_FORMAT_XML: filter_type = "xml"; break;
        case AD_TREE_EXPORT_FORMAT_NEWICK:
        case AD_TREE_EXPORT_FORMAT_NEWICK_PRETTY:
            switch (ExportNodeType(root->awar(AWAR_TREE_EXPORT_NDS)->read_int())) {
                case AD_TREE_EXPORT_NODE_SPECIES_NAME:  filter_type = "tree"; break;
                case AD_TREE_EXPORT_NODE_NDS:           filter_type = "ntree"; break;
                default: nt_assert(0); break;
            }
            break;
        default: nt_assert(0); break;
    }

    nt_assert(filter_type);
    root->awar(AWAR_TREE_EXPORT_FILTER)->write_string(filter_type);
}

void create_trees_var(AW_root *aw_root, AW_default aw_def) {
    AW_awar *awar_tree_name = aw_root->awar_string(AWAR_TREE_NAME, 0, aw_def)->set_srt(GBT_TREE_AWAR_SRT);

    TreeAdmin::create_awars(aw_root, aw_def, true);

    aw_root->awar_int   (AWAR_TREE_SECURITY, 0, aw_def);
    aw_root->awar_string(AWAR_TREE_REM,      0, aw_def);

    AW_create_fileselection_awars(aw_root, AWAR_TREE_EXPORT_FILEBASE, "", ".tree", "treefile");
    aw_root->awar_int(AWAR_TREE_EXPORT_FORMAT, AD_TREE_EXPORT_FORMAT_NEWICK, aw_def)-> add_callback(update_filter_cb);
    aw_root->awar_int(AWAR_TREE_EXPORT_NDS,  AD_TREE_EXPORT_NODE_SPECIES_NAME, aw_def)-> add_callback(update_filter_cb);

    aw_root->awar_int(AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS,  0, aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS,  1, aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS,  0, aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES,  1, aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_QUOTEMODE, TREE_SINGLE_QUOTES, aw_def); // old default behavior
    aw_root->awar_int(AWAR_TREE_EXPORT_REPLACE, 0, aw_def); // old default behavior

    AW_create_fileselection_awars(aw_root, AWAR_TREE_IMPORT, "", ".tree", "treefile");

    aw_root->awar_string(AWAR_TREE_IMPORT "/tree_name", "tree_",    aw_def)->set_srt(GBT_TREE_AWAR_SRT);

    aw_root->awar(AWAR_TREE_IMPORT "/file_name")->add_callback(update_default_treename_cb);
    awar_tree_name->add_callback(tree_vars_callback);
    awar_tree_name->map(AWAR_TREE);
    aw_root->awar(AWAR_TREE_SECURITY)->add_callback(ad_tree_set_security);

    aw_root->awar_int(AWAR_NODE_INFO_ONLY_MARKED, 0,    aw_def);

    aw_root->awar_string(AWAR_TREE_CONSENSE_TREE, "tree_consensus", aw_def)->set_srt(GBT_TREE_AWAR_SRT);
    AW_awar *ctree_awar = aw_root->awar_string(AWAR_TREE_CONSENSE_SELECTED, "", aw_def);
    AWT_registerTreeAwarSimple(ctree_awar);

    update_filter_cb(aw_root);
    tree_vars_callback(aw_root);
}

static void tree_save_cb(AW_window *aww) {
    AW_root  *aw_root   = aww->get_root();
    char     *tree_name = aw_root->awar(AWAR_TREE_NAME)->read_string();

    GB_ERROR error = 0;

    if (!tree_name || !strlen(tree_name)) {
        error = "Please select a tree first";
    }
    else {
        char *fname   = aw_root->awar(AWAR_TREE_EXPORT_NAME)->read_string();
        char *db_name = aw_root->awar(AWAR_DB_NAME)->read_string();

        bool                use_NDS    = ExportNodeType(aw_root->awar(AWAR_TREE_EXPORT_NDS)->read_int()) == AD_TREE_EXPORT_NODE_NDS;
        ExportTreeType      exportType = static_cast<ExportTreeType>(aw_root->awar(AWAR_TREE_EXPORT_FORMAT)->read_int());
        TREE_node_text_gen *node_gen   = use_NDS ? new TREE_node_text_gen(make_node_text_init, make_node_text_nds) : 0;

        switch (exportType) {
            case AD_TREE_EXPORT_FORMAT_XML:
                error = TREE_write_XML(GLOBAL.gb_main, db_name, tree_name, node_gen,
                                       aw_root->awar(AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS)->read_int(),
                                       fname);
                break;

            case AD_TREE_EXPORT_FORMAT_NEWICK:
            case AD_TREE_EXPORT_FORMAT_NEWICK_PRETTY:
                TREE_node_quoting quoteMode = TREE_node_quoting(aw_root->awar(AWAR_TREE_EXPORT_QUOTEMODE)->read_int());
                if (aw_root->awar(AWAR_TREE_EXPORT_REPLACE)->read_int()) {
                    quoteMode = TREE_node_quoting(quoteMode|TREE_FORCE_REPLACE);
                }

                error = TREE_write_Newick(GLOBAL.gb_main, tree_name, node_gen,
                                          aw_root->awar(AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS)->read_int(),
                                          aw_root->awar(AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS)->read_int(),
                                          aw_root->awar(AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES)->read_int(),
                                          exportType == AD_TREE_EXPORT_FORMAT_NEWICK_PRETTY,
                                          quoteMode,
                                          fname);
                break;
        }

        AW_refresh_fileselection(aw_root, AWAR_TREE_EXPORT_FILEBASE);

        delete node_gen;
        free(db_name);
        free(fname);
    }

    aww->hide_or_notify(error);
    free(tree_name);
}

static AWT_config_mapping_def tree_export_config_mapping[] = {
    { AWAR_TREE_EXPORT_FORMAT,             "format" },
    { AWAR_TREE_EXPORT_NDS,                "nodetype" },
    { AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS, "lengths" },
    { AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS, "bootstraps" },
    { AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES, "groupnames" },
    { AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS, "hidefolded" },
    { AWAR_TREE_EXPORT_QUOTEMODE,          "quotemode" },
    { AWAR_TREE_EXPORT_REPLACE,            "replacechars" },

    { 0, 0 }
};

static AW_window *create_tree_export_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "SAVE_TREE", "TREE SAVE");
    aws->load_xfig("tree_export.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("tr_export.hlp"));
    aws->create_button("HELP", "HELP", "H");

    AW_create_standard_fileselection(aws, AWAR_TREE_EXPORT_FILEBASE);

    aws->auto_space(10, 10);

    aws->at("user");
    aws->create_option_menu(AWAR_TREE_EXPORT_FORMAT, true);
    aws->insert_option("NEWICK TREE FORMAT",                   "N", AD_TREE_EXPORT_FORMAT_NEWICK);
    aws->insert_option("NEWICK TREE FORMAT (pretty, but big)", "P", AD_TREE_EXPORT_FORMAT_NEWICK_PRETTY);
    aws->insert_option("ARB_XML TREE FORMAT",                  "X", AD_TREE_EXPORT_FORMAT_XML);
    aws->update_option_menu();

    aws->at("user2");
    aws->label("Nodetype");
    aws->create_toggle_field(AWAR_TREE_EXPORT_NDS, 1);
    aws->insert_default_toggle("Species ID ('name')", "S", 0);
    aws->insert_toggle("NDS", "N", 1);
    aws->update_toggle_field();

    aws->at_newline(); aws->label("Save branch lengths"); aws->create_toggle(AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS);
    aws->at_newline(); aws->label("Save bootstrap values"); aws->create_toggle(AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS);
    aws->at_newline(); aws->label("Save group names"); aws->create_toggle(AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES);
    aws->at_newline(); aws->label("Hide folded groups (XML only)"); aws->create_toggle(AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS);

    aws->at_newline();
    aws->label("Name quoting (Newick only)");
    aws->create_option_menu(AWAR_TREE_EXPORT_QUOTEMODE, true);
    aws->insert_option("none",            "n", TREE_DISALLOW_QUOTES);
    aws->insert_option("single",          "s", TREE_SINGLE_QUOTES);
    aws->insert_option("double",          "d", TREE_DOUBLE_QUOTES);
    aws->insert_option("single (forced)", "i", TREE_SINGLE_QUOTES|TREE_FORCE_QUOTES);
    aws->insert_option("double (forced)", "o", TREE_DOUBLE_QUOTES|TREE_FORCE_QUOTES);
    aws->update_option_menu();

    aws->at_newline(); aws->label("Replace problem chars"); aws->create_toggle(AWAR_TREE_EXPORT_REPLACE);

    aws->at_newline();
    aws->button_length(10);

    aws->callback(tree_save_cb);
    aws->create_button("SAVE", "SAVE", "o");

    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "tree_export", tree_export_config_mapping);

    aws->window_fit();
    update_filter_cb(root);

    return aws;
}

static char *readXmlTree(char *fname) {
    // create a temp file
    char tempFile[]  = "newickXXXXXX";
    int createTempFile = mkstemp(tempFile);

    if (createTempFile) {
        GBS_strstruct *buf = GBS_stropen(strlen(fname));

        // extract path from fname in order to place a copy of dtd file required to validate xml file
        {
            char *tmpFname = strdup(fname);
            for (char *tok = strtok(tmpFname, "/"); tok;) {
                char *tmp = tok;
                tok = strtok(0, "/");
                if (tok) {
                    GBS_strcat(buf, "/");
                    GBS_strcat(buf, tmp);
                }
            }
            free(tmpFname);
        }

        char *path = GBS_strclose(buf);

        // linking arb_tree.dtd file to the Path from where xml file is loaded
#if defined(WARN_TODO)
#warning fix hack
#endif
        char *command = GBS_global_string_copy("ln -s %s/lib/dtd/arb_tree.dtd %s/.", GB_getenvARBHOME(), path);
        GB_xcmd(command, false, true);

        // execute xml2newick to convert xml format tree to newick format tree
        command = GBS_global_string_copy("xml2newick %s %s", fname, tempFile);
        GB_xcmd(command, false, true);

        free(command);
        free(path);

        // return newick format tree file
        return strdup(tempFile);
    }
    else {
        printf("Failed to create Temporary File to Parse xml file!\n");
        return 0;
    }
}

static void tree_load_cb(AW_window *aww) {
    GB_ERROR  error     = 0;
    AW_root  *aw_root   = aww->get_root();
    char     *tree_name = aw_root->awar(AWAR_TREE_IMPORT "/tree_name")->read_string();

    {
        char *pcTreeFormat = aw_root->awar(AWAR_TREE_IMPORT "/filter")->read_string();
        char *fname        = aw_root->awar(AWAR_TREE_IMPORT "/file_name")->read_string();
        char *warnings     = 0;
        char *tree_comment = 0;

        TreeNode *tree;
        if (strcmp(pcTreeFormat, "xml") == 0) {
            char *tempFname = readXmlTree(fname);
            tree = TREE_load(tempFname, new SimpleRoot, &tree_comment, true, &warnings);
            GB_unlink_or_warn(tempFname, NULL);
            free(tempFname);
        }
        else {
            tree = TREE_load(fname, new SimpleRoot, &tree_comment, true, &warnings);
        }

        if (!tree) error = GB_await_error();
        else {
            if (warnings) GBT_message(GLOBAL.gb_main, warnings);

            {
                GB_transaction ta(GLOBAL.gb_main);
                error = GBT_write_tree_with_remark(GLOBAL.gb_main, tree_name, tree, tree_comment);
                error = ta.close(error);
            }

            if (!error) aw_root->awar(AWAR_TREE)->write_string(tree_name); // show new tree

            UNCOVERED();
            destroy(tree);
        }

        free(warnings);
        free(tree_comment);
        free(fname);
        free(pcTreeFormat);
    }

    aww->hide_or_notify(error);
    free(tree_name);
}

static AW_window *create_tree_import_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "LOAD_TREE", "TREE LOAD");
    aws->load_xfig("sel_box_tree.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("format");
    aws->label("Tree Format");
    aws->create_option_menu(AWAR_TREE_IMPORT "/filter", false);
    aws->insert_default_option("Newick", "t", "tree");
    aws->insert_option("XML", "x", "xml");
    aws->update_option_menu();

    aws->at("user");
    aws->label("Tree name");
    aws->create_input_field(AWAR_TREE_IMPORT "/tree_name", 15);

    AW_create_standard_fileselection(aws, AWAR_TREE_IMPORT);

    aws->at("save2");
    aws->callback(tree_load_cb);
    aws->create_button("LOAD", "LOAD", "o");

    aws->window_fit();

    return aws;
}

static void ad_move_tree_info(AW_window *aww, TreeInfoMode mode) {
    bool nodes_with_marked_only = false;

    char     *log_file = 0;
    GB_ERROR  error    = 0;

    if (mode == TREE_INFO_COPY || mode == TREE_INFO_ADD) {
        // move or add node-info writes a log file (containing errors)
        // compare_node_info only sets remark branches
        char *log_name       = GB_unique_filename("arb_node", "log");
        log_file             = GB_create_tempfile(log_name);
        if (!log_file) error = GB_await_error();
        free(log_name);

        nodes_with_marked_only = aww->get_root()->awar(AWAR_NODE_INFO_ONLY_MARKED)->read_int();
    }

    if (!error) {
        AW_root *awr      = aww->get_root();
        char    *src_tree = TreeAdmin::source_tree_awar(awr)->read_string();
        char    *dst_tree = TreeAdmin::dest_tree_awar(awr)->read_string();

        error = AWT_move_info(GLOBAL.gb_main, src_tree, dst_tree, log_file, mode, nodes_with_marked_only);
        if (log_file) {
            AW_edit(log_file);
            GB_remove_on_exit(log_file);
        }

        free(dst_tree);
        free(src_tree);
    }

    if (error) aw_message(error);
    else aww->hide();

    free(log_file);
}

static void swap_source_dest_cb(AW_window *aww) {
    AW_root *root = aww->get_root();

    AW_awar *s = TreeAdmin::source_tree_awar(root);
    AW_awar *d = TreeAdmin::dest_tree_awar(root);

    char *old_src = s->read_string();
    s->write_string(d->read_char_pntr());
    d->write_string(old_src);
    free(old_src);
}

static void copy_tree_awar_cb(UNFIXED, AW_awar *aw_source, AW_awar *aw_dest) {
    const char *tree = aw_source->read_char_pntr();
    if (tree && tree[0]) aw_dest->write_string(tree);
}

static AW_window_simple *create_select_two_trees_window(AW_root *root, const char *winId, const char *winTitle, const char *helpFile) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, winId, winTitle);
    aws->load_xfig("ad_two_trees.fig");

    aws->at("close");
    aws->auto_space(10, 3);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "Close", "C");

    aws->at("help");
    aws->callback(makeHelpCallback(helpFile));
    aws->create_button("HELP", "Help", "H");

    aws->at("tree1");
    awt_create_TREE_selection_list(GLOBAL.gb_main, aws, TreeAdmin::source_tree_awar(root)->awar_name, true);
    aws->at("tree2");
    awt_create_TREE_selection_list(GLOBAL.gb_main, aws, TreeAdmin::dest_tree_awar(root)->awar_name, false);

    AW_awar *awar_displayed_tree = root->awar(AWAR_TREE_NAME);

    aws->at("select1");
    aws->callback(makeWindowCallback(copy_tree_awar_cb, awar_displayed_tree, TreeAdmin::source_tree_awar(root)));  aws->create_autosize_button("SELECT_DISPLAYED1", "Use");
    aws->callback(makeWindowCallback(copy_tree_awar_cb, TreeAdmin::source_tree_awar(root), awar_displayed_tree));  aws->create_autosize_button("DISPLAY_SELECTED1", "Display");

    aws->callback(swap_source_dest_cb);
    aws->create_autosize_button("SWAP", "Swap");

    aws->at("select2");
    aws->callback(makeWindowCallback(copy_tree_awar_cb, awar_displayed_tree, TreeAdmin::dest_tree_awar(root)));  aws->create_autosize_button("SELECT_DISPLAYED2", "Use");
    aws->callback(makeWindowCallback(copy_tree_awar_cb, TreeAdmin::dest_tree_awar(root), awar_displayed_tree));  aws->create_autosize_button("DISPLAY_SELECTED2", "Display");

    aws->at("user");

    return aws;
}

static AW_window_simple *create_select_other_tree_window(AW_root *root, const char *winId, const char *winTitle, const char *helpFile, const char *displayed_tree_awarname) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, winId, winTitle);
    aws->load_xfig("ad_one_tree.fig");

    aws->at("close");
    aws->auto_space(10, 3);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "Close", "C");

    aws->at("help");
    aws->callback(makeHelpCallback(helpFile));
    aws->create_button("HELP", "Help", "H");

    AW_awar *awar_displayed_tree = root->awar(displayed_tree_awarname);

    aws->at("tree");
    awt_create_TREE_selection_list(GLOBAL.gb_main, aws, TreeAdmin::source_tree_awar(root)->awar_name, true);

    aws->at("select");
    aws->callback(makeWindowCallback(copy_tree_awar_cb, awar_displayed_tree, TreeAdmin::source_tree_awar(root)));  aws->create_autosize_button("SELECT_DISPLAYED", "Use");
    aws->callback(makeWindowCallback(copy_tree_awar_cb, TreeAdmin::source_tree_awar(root), awar_displayed_tree));  aws->create_autosize_button("DISPLAY_SELECTED", "Display");

    aws->at("user");

    return aws;
}

static AW_window *create_tree_diff_window(AW_root *root) {
    AW_window_simple *aws = create_select_two_trees_window(root, "CMP_TOPOLOGY", "Compare tree topologies", "tree_diff.hlp");

    aws->callback(makeWindowCallback(ad_move_tree_info, TREE_INFO_COMPARE));
    aws->create_autosize_button("CMP_TOPOLOGY", "Compare topologies");

    return aws;
}

static AW_window *create_tree_cmp_window(AW_root *root) {
    AW_window_simple *aws = create_select_two_trees_window(root, "COPY_NODE_INFO_OF_TREE", "Move tree node info", "tree_cmp.hlp");

    aws->button_length(11);

    aws->callback(makeWindowCallback(ad_move_tree_info, TREE_INFO_COPY));
    aws->create_button("COPY_INFO", "Copy info");

    aws->label("copy/add only info containing marked species");
    aws->create_toggle(AWAR_NODE_INFO_ONLY_MARKED);

    aws->at_newline();

    aws->callback(makeWindowCallback(ad_move_tree_info, TREE_INFO_ADD));
    aws->create_button("ADD_INFO", "Add info");

    return aws;
}

static void reorder_trees_cb(AW_window *aww, awt_reorder_mode dest) {
    // moves the tree in the list of trees

    char     *tree_name = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();
    GB_ERROR  error     = NULL;

    GB_transaction ta(GLOBAL.gb_main);
    GBDATA *gb_treedata   = GBT_get_tree_data(GLOBAL.gb_main);
    GBDATA *gb_moved_tree = GB_entry(gb_treedata, tree_name);

    if (!gb_moved_tree) {
        error = "No tree selected";
    }
    else {
        GBT_ORDER_MODE  move_mode;
        GBDATA         *gb_target_tree = NULL;

        switch (dest) {
            case ARM_UP:
                move_mode      = GBT_INFRONTOF;
                gb_target_tree = GBT_tree_infrontof(gb_moved_tree);
                if (gb_target_tree) break;
                // fall-through (move top-tree up = move to bottom)
            case ARM_BOTTOM:
                move_mode      = GBT_BEHIND;
                gb_target_tree = GBT_find_bottom_tree(GLOBAL.gb_main);
                break;

            case ARM_DOWN:
                move_mode      = GBT_BEHIND;
                gb_target_tree = GBT_tree_behind(gb_moved_tree);
                if (gb_target_tree) break;
                // fall-through (move bottom-tree down = move to top)
            case ARM_TOP:
                move_mode      = GBT_INFRONTOF;
                gb_target_tree = GBT_find_top_tree(GLOBAL.gb_main);
                break;
        }

        if (gb_target_tree && gb_target_tree != gb_moved_tree) {
            error = GBT_move_tree(gb_moved_tree, move_mode, gb_target_tree);
        }
    }

    if (error) aw_message(error);
    free(tree_name);
}

void popup_tree_admin_window(AW_window *aws) {
    popup_tree_admin_window(aws->get_root());
}
void popup_tree_admin_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "TREE_ADMIN", "TREE ADMIN");
        aws->load_xfig("ad_tree.fig");

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("treeadm.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->button_length(40);

        aws->at("sel");
        aws->create_button(0, AWAR_TREE_NAME, 0, "+");

        aws->at("security");
        aws->create_option_menu(AWAR_TREE_SECURITY, true);
        aws->insert_option("0", "0", 0);
        aws->insert_option("1", "1", 1);
        aws->insert_option("2", "2", 2);
        aws->insert_option("3", "3", 3);
        aws->insert_option("4", "4", 4);
        aws->insert_option("5", "5", 5);
        aws->insert_default_option("6", "6", 6);
        aws->update_option_menu();

        aws->at("rem");
        aws->create_text_field(AWAR_TREE_REM);


        aws->button_length(20);

        static TreeAdmin::Spec spec(GLOBAL.gb_main, AWAR_TREE_NAME);

        aws->at("delete");
        aws->callback(makeWindowCallback(TreeAdmin::delete_tree_cb, &spec));
        aws->create_button("DELETE", "Delete", "D");

        aws->at("rename");
        aws->callback(makeCreateWindowCallback(TreeAdmin::create_rename_window, &spec));
        aws->create_button("RENAME", "Rename", "R");

        aws->at("copy");
        aws->callback(makeCreateWindowCallback(TreeAdmin::create_copy_window, &spec));
        aws->create_button("COPY", "Copy", "C");

        aws->at("move");
        aws->callback(create_tree_cmp_window);
        aws->create_button("MOVE_NODE_INFO", "Move node info", "C");

        aws->at("cmp");
        aws->callback(create_tree_diff_window);
        aws->sens_mask(AWM_EXP);
        aws->create_button("CMP_TOPOLOGY", "Compare topology", "T");
        aws->sens_mask(AWM_ALL);

        aws->at("export");
        aws->callback(create_tree_export_window);
        aws->create_button("EXPORT", "Export", "E");

        aws->at("import");
        aws->callback(create_tree_import_window);
        aws->create_button("IMPORT", "Import", "I");

        aws->button_length(0);

        aws->at("list");
        awt_create_TREE_selection_list(GLOBAL.gb_main, aws, AWAR_TREE_NAME, true);

        aws->at("sort");
        awt_create_order_buttons(aws, reorder_trees_cb);
    }

    aws->activate();
}

// -----------------------
//      consense tree


static void create_consense_tree_cb(AW_window *aww, AW_selection *selected_trees) {
    AW_root  *aw_root = aww->get_root();
    GB_ERROR  error   = NULL;

    const char *cons_tree_name = aw_root->awar(AWAR_TREE_CONSENSE_TREE)->read_char_pntr();
    if (!cons_tree_name || !cons_tree_name[0]) {
        error = "No name specified for the consensus tree";
    }
    else {
        StrArray tree_names;
        selected_trees->get_values(tree_names);

        if (tree_names.size()<2) {
            error = "Not enough trees selected (at least 2 needed)";
        }
        else {
            GBDATA *gb_main = GLOBAL.gb_main;
            GB_transaction ta(gb_main);

            {
                arb_progress progress("Building consensus tree", 2); // 2 steps: deconstruct, reconstruct
                ConsensusTreeBuilder tree_builder;

                progress.subtitle("loading input trees");
                for (size_t t = 0; t<tree_names.size() && !error; ++t) {
                    TreeRoot      *root = new SizeAwareRoot; // will be deleted when tree gets deleted
                    SizeAwareTree *tree = DOWNCAST(SizeAwareTree*, GBT_read_tree(gb_main, tree_names[t], root));
                    if (!tree) {
                        error = GB_await_error();
                    }
                    else {
                        tree_builder.add(tree, tree_names[t], 1.0);
                    }
                }

                if (!error) {
                    size_t    species_count;
                    TreeNode *cons_tree = tree_builder.get(species_count, error); // triggers 2 implicit progress increments

                    if (!error && progress.aborted()) {
                        error = "user abort";
                    }

                    nt_assert(contradicted(cons_tree, error));
                    if (cons_tree) {
                        char *comment = tree_builder.get_remark();
                        error         = GBT_write_tree_with_remark(gb_main, cons_tree_name, cons_tree, comment);
                        free(comment);
                        UNCOVERED();
                        destroy(cons_tree);
                    }
                }
                if (error) progress.done();
            }
            error = ta.close(error);
        }
    }

    if (!error) {
        aw_root->awar(AWAR_TREE_NAME)->write_string(cons_tree_name); // show in main window
    }

    aw_message_if(error);
}

static void use_selected_as_target_cb(AW_window *aww) {
    AW_root *aw_root = aww->get_root();
    aw_root->awar(AWAR_TREE_CONSENSE_TREE)->write_string(aw_root->awar(AWAR_TREE_CONSENSE_SELECTED)->read_char_pntr());
}

AW_window *NT_create_consense_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "CONSENSE_TREE", "Consensus Tree");
        aws->load_xfig("ad_cons_tree.fig");

        aws->auto_space(10, 10);

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("consense_tree.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->at("list");
        AW_DB_selection *all_trees      = awt_create_TREE_selection_list(GLOBAL.gb_main, aws, AWAR_TREE_CONSENSE_SELECTED, true);
        AW_selection    *selected_trees = awt_create_subset_selection_list(aws, all_trees->get_sellist(), "selected", "add", "sort");

        aws->at("name");
        aws->create_input_field(AWAR_TREE_CONSENSE_TREE);

        aws->callback(use_selected_as_target_cb);
        aws->create_button("USE_AS_TARGET", "#moveLeft.xpm", 0);

        aws->at("build");
        aws->callback(makeWindowCallback(create_consense_tree_cb, selected_trees));
        aws->create_autosize_button("BUILD", "Build consensus tree", "B");
    }
    return aws;
}

class PosInfo {
    // describes relative position of a given subtree in a tree

    double relpos_sum; // sum of all involved relative positions
    size_t count;      // amount of positions involved

    PosInfo() : relpos_sum(0), count(0) {} // = no position
public:

    static PosInfo unknown() { return PosInfo(); }

    explicit PosInfo(double relpos_sum_) // create leaf-info
        : relpos_sum(relpos_sum_),
          count(1)
    {}
    PosInfo(const PosInfo& p1, const PosInfo& p2)
        : relpos_sum(p1.relpos_sum + p2.relpos_sum),
          count(p1.count + p2.count)
    {}

    bool is_known() const { return count; }
    double value() const {
        nt_assert(is_known());
        return relpos_sum / count;
    }
    int compare(const PosInfo &right) const {
        return double_cmp(value(), right.value());
    }
};

class CombinedPosInfo {
    // combines relative positions of a subtree in 2 trees (source- and target-tree).
    // provides compare operations for SortByTopo

    PosInfo source; // in source tree ("ordering tree")
    PosInfo target; // in target tree ("modified tree")

public:

    CombinedPosInfo(const PosInfo& s, const PosInfo& t)
        : source(s),
          target(t)
    {
        nt_assert(target.is_known());
    }
    CombinedPosInfo(const CombinedPosInfo& c1, const CombinedPosInfo& c2)
        : source(c1.source, c2.source),
          target(c1.target, c2.target)
    {}

    int compare(const CombinedPosInfo &right) const {
        // result similar to strcmp(this, right)
        if (!source.is_known() || !right.source.is_known()) {
            // one subtree is completely unknown in source-tree
            // => keep target-tree order
            return target.compare(right.target);
        }
        return source.compare(right.source);
    }
};


class SpeciesPosition {
    // provides relative position of species inside a tree

    typedef std::map<std::string, unsigned> PosMap;
    PosMap spos;

    void fillFromTree(const TreeNode *node) {
        if (node->is_leaf) {
            size_t pos       = spos.size();
            spos[node->name] = pos;
        }
        else {
            fillFromTree(node->get_leftson());
            fillFromTree(node->get_rightson());
        }
    }
public:
    explicit SpeciesPosition(const TreeNode *tree) {
        fillFromTree(tree);
    }

    PosInfo relative(const char *name) const {
        /*! returns PosInfo of species 'name' inside tree.
         */
        PosMap::const_iterator found = spos.find(name);
        return (found == spos.end()) ? PosInfo::unknown() : PosInfo(found->second/double(spos.size()-1));
    }
};


class SortByTopo : virtual Noncopyable {
    SpeciesPosition        source_pos; // in ordering topology
    const SpeciesPosition *target_pos; // in target topology (used where source_pos does not provide order)

    CombinedPosInfo reorder_subtree_rec(TreeNode *node) { // similar to ../ARBDB/TreeNode.cxx@reorder_subtree
        static const char *smallest_leafname; // has to be set to the alphabetically smallest name (when function exits)

        if (node->is_leaf) {
            smallest_leafname = node->name;
            return CombinedPosInfo(source_pos.relative(node->name),
                                   target_pos->relative(node->name));
        }

        CombinedPosInfo  leftInfo       = reorder_subtree_rec(node->get_leftson());
        const char      *smallest_left  = smallest_leafname;
        CombinedPosInfo  rightInfo      = reorder_subtree_rec(node->get_rightson());
        const char      *smallest_right = smallest_leafname;

        bool left_leafname_bigger = strcmp(smallest_left, smallest_right)>0;
        smallest_leafname         = left_leafname_bigger ? smallest_right : smallest_left;

        {
            int cmp = leftInfo.compare(rightInfo);
            if (cmp>0 || (cmp == 0 && left_leafname_bigger)) {
                node->swap_sons();
            }
        }

        return CombinedPosInfo(leftInfo, rightInfo);
    }
public:

    SortByTopo(const TreeNode *by)
        : source_pos(by),
          target_pos(NULL)
    {}

#if defined(UNIT_TESTS)
    PosInfo sourcePos(const char *name) { return source_pos.relative(name); }
#endif

    void reorder_subtree(TreeNode *tree) {
        SpeciesPosition tpos(tree);
        LocallyModify<const SpeciesPosition*> provide(target_pos, &tpos);
        reorder_subtree_rec(tree);
    }
};

static GB_ERROR sort_tree_by_other_tree(GBDATA *gb_main, TreeNode *tree, const char *other_tree) {
    GB_ERROR       error = NULL;
    GB_transaction ta(gb_main);

    TreeNode *otherTree   = GBT_read_tree(gb_main, other_tree, new SimpleRoot);
    if (!otherTree) error = GB_await_error();
    else {
        SortByTopo sorter(otherTree);
        destroy(otherTree);
        sorter.reorder_subtree(tree);
    }
    return error;
}

static bool sort_dtree_by_other_tree_cb(TreeNode *tree, GB_ERROR& error) {
    const char *other_tree = TreeAdmin::source_tree_awar(AW_root::SINGLETON)->read_char_pntr();
    error = sort_tree_by_other_tree(GLOBAL.gb_main, tree, other_tree);
    return !error;
}

static void sort_tree_by_other_tree_cb(UNFIXED, AWT_canvas *ntw) {
    GB_ERROR error = NT_with_displayed_tree_do(ntw, sort_dtree_by_other_tree_cb);
    aw_message_if(error);
}

AW_window *NT_create_sort_tree_by_other_tree_window(AW_root *aw_root, AWT_canvas *ntw) {
    AW_window_simple *aws = create_select_other_tree_window(aw_root, ntw->aww->local_id("SORT_BY_OTHER"), "Sort tree by other tree", "resortbyother.hlp", ntw->user_awar);

    aws->callback(makeWindowCallback(sort_tree_by_other_tree_cb, ntw));
    aws->create_autosize_button("RESORT", "Sort according to source tree");

    return aws;
}

// ---------------------------
//      multifurcate tree

#define AWAR_MFURC                    "tree/multifurc/"
#define AWAR_MFURC_CONSIDER_BOOTSTRAP AWAR_MFURC "use_bs"
#define AWAR_MFURC_CONSIDER_LENGTH    AWAR_MFURC "use_len"
#define AWAR_MFURC_CONSIDER_TERMINALS AWAR_MFURC "terminals"
#define AWAR_MFURC_LENGTH_LIMIT       AWAR_MFURC "len"
#define AWAR_MFURC_BOOTSTRAP_LIMIT    AWAR_MFURC "bs"

void NT_create_multifurcate_tree_awars(AW_root *aw_root, AW_default props) {
    aw_root->awar_int  (AWAR_MFURC_CONSIDER_BOOTSTRAP, 0,   props);
    aw_root->awar_int  (AWAR_MFURC_CONSIDER_LENGTH,    1,   props);
    aw_root->awar_int  (AWAR_MFURC_CONSIDER_TERMINALS, 0,   props);
    aw_root->awar_float(AWAR_MFURC_LENGTH_LIMIT,       0.1, props);
    aw_root->awar_float(AWAR_MFURC_BOOTSTRAP_LIMIT,    50,  props);
}
static void multifurcation_cb(UNFIXED, AWT_canvas *ntw) {
    AW_root *aw_root = ntw->aww->get_root();

    double below_bootstrap = 101.0;
    double below_length    = 1000000.0;
    bool   applyAtLeafs    = aw_root->awar(AWAR_MFURC_CONSIDER_TERMINALS)->read_int();

    if (aw_root->awar(AWAR_MFURC_CONSIDER_BOOTSTRAP)->read_int()) below_bootstrap = aw_root->awar(AWAR_MFURC_BOOTSTRAP_LIMIT)->read_float();
    if (aw_root->awar(AWAR_MFURC_CONSIDER_LENGTH)   ->read_int()) below_length    = aw_root->awar(AWAR_MFURC_LENGTH_LIMIT)   ->read_float();

    NT_multifurcate_tree(ntw, TreeNode::multifurc_limits(below_bootstrap, below_length, applyAtLeafs));
}
AW_window *NT_create_multifurcate_tree_window(AW_root *aw_root, AWT_canvas *ntw) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, ntw->aww->local_id("multifurcate"), "Multifurcate tree");
    aws->at(10, 10);
    aws->auto_space(10, 10);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("multifurcate.hlp"));
    aws->create_button("HELP", "HELP", "H");

    const int LABEL_LENGTH = 46;
    aws->label_length(LABEL_LENGTH);

    aws->at_newline();
    aws->label("Multifurcate branches with branchlength below");
    aws->create_toggle(AWAR_MFURC_CONSIDER_LENGTH);
    aws->create_input_field(AWAR_MFURC_LENGTH_LIMIT, 10);

    aws->at_newline();
    aws->label("                          AND bootstrap below");
    aws->create_toggle(AWAR_MFURC_CONSIDER_BOOTSTRAP);
    aws->create_input_field(AWAR_MFURC_BOOTSTRAP_LIMIT, 10);

    aws->label_length(0);
    aws->at_newline();
    aws->label("Also apply to terminal branches");
    aws->create_toggle(AWAR_MFURC_CONSIDER_TERMINALS);

    aws->at_newline();
    aws->callback(makeWindowCallback(multifurcation_cb, ntw));
    aws->create_autosize_button("MULTIFURCATE", "Multifurcate", "M");

    return aws;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static GB_ERROR sort_namedtree_by_other_tree(GBDATA *gb_main, const char *tree, const char *other_tree) {
    GB_ERROR        error = NULL;
    GB_transaction  ta(gb_main);
    SizeAwareTree  *Tree  = DOWNCAST(SizeAwareTree*, GBT_read_tree(gb_main, tree, new SizeAwareRoot));
    if (!Tree) error      = GB_await_error();
    else {
        Tree->compute_tree();
        error             = sort_tree_by_other_tree(gb_main, Tree, other_tree);
        if (!error) error = GBT_write_tree(gb_main, tree, Tree);
    }
    destroy(Tree);
    return error;
}

void TEST_sort_tree_by_other_tree() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_trees.arb", "rw");
    TEST_REJECT_NULL(gb_main);

    const char *topo_test   = "(((((((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,CloTyrob:0.009):0.274,CloInnoc:0.371):0.057,CloBifer:0.388):0.124,(((CloButy2:0.009,CloButyr:0.000):0.564,CloCarni:0.120):0.010,CloPaste:0.179):0.131):0.081,((((CorAquat:0.084,CurCitre:0.058):0.103,CorGluta:0.522):0.053,CelBiazo:0.059):0.207,CytAquat:0.711):0.081);";
    const char *topo_center = "(((CloPaste:0.179,((CloButy2:0.009,CloButyr:0.000):0.564,CloCarni:0.120):0.010):0.131,((CloInnoc:0.371,((CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017,CloTyrob:0.009):0.274):0.057,CloBifer:0.388):0.124):0.081,((CelBiazo:0.059,((CorAquat:0.084,CurCitre:0.058):0.103,CorGluta:0.522):0.053):0.207,CytAquat:0.711):0.081);";
    const char *topo_bottom = "((CytAquat:0.711,(CelBiazo:0.059,(CorGluta:0.522,(CorAquat:0.084,CurCitre:0.058):0.103):0.053):0.207):0.081,((CloPaste:0.179,(CloCarni:0.120,(CloButy2:0.009,CloButyr:0.000):0.564):0.010):0.131,(CloBifer:0.388,(CloInnoc:0.371,(CloTyrob:0.009,(CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017):0.274):0.057):0.124):0.081);";

    const char *topo_vs_nj_bs = "(((((((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,CloTyrob:0.009):0.274,CloInnoc:0.371):0.057,CloBifer:0.388):0.124,(((CloButyr:0.000,CloButy2:0.009):0.564,CloCarni:0.120):0.010,CloPaste:0.179):0.131):0.081,(((CorGluta:0.522,(CorAquat:0.084,CurCitre:0.058):0.103):0.053,CelBiazo:0.059):0.207,CytAquat:0.711):0.081);";

    TEST_EXPECT_DIFFERENT(topo_test,   topo_center);
    TEST_EXPECT_DIFFERENT(topo_test,   topo_bottom);
    TEST_EXPECT_DIFFERENT(topo_center, topo_bottom);

    // create sorted copies of tree_test
    {
        GB_transaction  ta(gb_main);
        SizeAwareTree  *tree = DOWNCAST(SizeAwareTree*, GBT_read_tree(gb_main, "tree_test", new SizeAwareRoot));
        TEST_REJECT_NULL(tree);
        TEST_EXPECT_NEWICK(nLENGTH, tree, topo_test);

        tree->reorder_tree(BIG_BRANCHES_TO_CENTER); TEST_EXPECT_NO_ERROR(GBT_write_tree(gb_main, "tree_sorted_center", tree)); TEST_EXPECT_NEWICK(nLENGTH, tree, topo_center);
        tree->reorder_tree(BIG_BRANCHES_TO_BOTTOM); TEST_EXPECT_NO_ERROR(GBT_write_tree(gb_main, "tree_sorted_bottom", tree)); TEST_EXPECT_NEWICK(nLENGTH, tree, topo_bottom);

        // test SortByTopo
        {
            SortByTopo   sbt(tree);
            const double EPSILON = 0.0001;

            TEST_EXPECT_SIMILAR(sbt.sourcePos("CytAquat").value(), 0.0, EPSILON); // leftmost species (in topo_bottom)
            TEST_EXPECT_SIMILAR(sbt.sourcePos("CloTyro4").value(), 1.0, EPSILON); // rightmost species

            TEST_EXPECT_SIMILAR(sbt.sourcePos("CurCitre").value(), 0.2857, EPSILON); // (5 of 15)
            TEST_EXPECT_SIMILAR(sbt.sourcePos("CloButy2").value(), 0.5,    EPSILON); // center species (8 of 15)
            TEST_EXPECT_SIMILAR(sbt.sourcePos("CloTyrob").value(), 0.7857, EPSILON); // (12 of 15)

            TEST_REJECT(sbt.sourcePos("Un-Known").is_known()); // unknown species
        }

        tree->reorder_tree(BIG_BRANCHES_TO_EDGE); TEST_EXPECT_NO_ERROR(GBT_write_tree(gb_main, "tree_work", tree));

        destroy(tree);
    }


    TEST_EXPECT_NO_ERROR(sort_namedtree_by_other_tree(gb_main, "tree_work", "tree_sorted_center")); TEST_EXPECT_SAVED_NEWICK(nLENGTH, gb_main, "tree_work", topo_center);
    TEST_EXPECT_NO_ERROR(sort_namedtree_by_other_tree(gb_main, "tree_work", "tree_sorted_bottom")); TEST_EXPECT_SAVED_NEWICK(nLENGTH, gb_main, "tree_work", topo_bottom);
    TEST_EXPECT_NO_ERROR(sort_namedtree_by_other_tree(gb_main, "tree_work", "tree_test"));          TEST_EXPECT_SAVED_NEWICK(nLENGTH, gb_main, "tree_work", topo_test);
    TEST_EXPECT_NO_ERROR(sort_namedtree_by_other_tree(gb_main, "tree_work", "tree_nj_bs"));         TEST_EXPECT_SAVED_NEWICK(nLENGTH, gb_main, "tree_work", topo_vs_nj_bs);

    // TEST_EXPECT_NO_ERROR(GB_save_as(gb_main, "TEST_trees_save.arb", "b")); // test-save db to examine tree (do not commit)

    GB_close(gb_main);
}
TEST_PUBLISH(TEST_sort_tree_by_other_tree);

void TEST_move_node_info() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_trees.arb", "r");

#define GROUP_TEST "(CloTyrob,(CloTyro2,(CloTyro3,CloTyro4)))"

#define NAMED_GROUP_TEST       GROUP_TEST "'test'"
#define OVERWRITTEN_GROUP_TEST GROUP_TEST "'g2 [was: test]'"

    const char *org_topo = "((CloInnoc," GROUP_TEST "),(CloBifer,((CloCarni,CurCitre),((CloPaste,(Zombie1,(CloButy2,CloButyr))),(CytAquat,(CelBiazo,(CorGluta,(CorAquat,Zombie2))))))));";

    const char *unwanted_topo1 = "((CytAquat,(CelBiazo,(CorGluta,(CorAquat,Zombie2)))),((CloPaste,(Zombie1,(CloButy2,CloButyr))),((CloCarni,CurCitre),(CloBifer,(CloInnoc," NAMED_GROUP_TEST ")))));";
    const char *unwanted_topo2 = "((CloButy2,CloButyr),(Zombie1,(CloPaste,((((CloInnoc," OVERWRITTEN_GROUP_TEST "),CloBifer),(CloCarni,CurCitre)),(CytAquat,(CelBiazo,(CorGluta,(CorAquat,Zombie2)))))))'outer');";

    const char *sorted_topo1 = "(((((CloInnoc," NAMED_GROUP_TEST "),CloBifer),(CloCarni,CurCitre)),(CloPaste,(Zombie1,(CloButy2,CloButyr)))),(CytAquat,(CelBiazo,(CorGluta,(CorAquat,Zombie2)))));";
    const char *sorted_topo2 = "(((((((CloInnoc," OVERWRITTEN_GROUP_TEST "),CloBifer),(CloCarni,CurCitre)),(CytAquat,(CelBiazo,(CorGluta,(CorAquat,Zombie2))))),CloPaste),Zombie1)'outer',(CloButy2,CloButyr));";

    const char *compared_topo = "(((((((CloInnoc,(CloTyrob,(CloTyro2,(CloTyro3,CloTyro4)))),CloBifer),(CloCarni,CurCitre)'# 2')'# 2',(CytAquat,(CelBiazo,(CorGluta,(CorAquat,Zombie2)'# 1')'# 1')'# 1')'# 1')'# 1',CloPaste),Zombie1),(CloButy2,CloButyr));";

    const char *LOG = "move_node_info.log";

    // create copy of 'tree_removal'
    {
        GB_transaction  ta(gb_main);
        TreeNode       *tree = GBT_read_tree(gb_main, "tree_removal", new SimpleRoot);

        TEST_EXPECT_NEWICK(nSIMPLE, tree, org_topo);
        TEST_EXPECT_NO_ERROR(GBT_write_tree(gb_main, "tree_removal_copy", tree));
        destroy(tree);
    }

    // move node info
    {
        TEST_EXPECT_NO_ERROR(AWT_move_info(gb_main, "tree_test", "tree_removal", LOG, TREE_INFO_COPY, false));

        TEST_EXPECT_SAVED_NEWICK__BROKEN(nSIMPLE, gb_main, "tree_removal", org_topo); // @@@ moving node info modifies topology (might be necessary to insert groups)
        TEST_EXPECT_SAVED_NEWICK(nGROUP, gb_main, "tree_removal", unwanted_topo1);

        // @@@ when we have a function to set the root according to another tree (#449),
        // use that function here. sorting tree after that, should again result in 'org_topo'!

        TEST_EXPECT_NO_ERROR(sort_namedtree_by_other_tree(gb_main, "tree_removal", "tree_removal_copy"));
        TEST_EXPECT_SAVED_NEWICK(nGROUP, gb_main, "tree_removal", sorted_topo1);
    }

    // add node info
    {
        TEST_EXPECT_NO_ERROR(AWT_move_info(gb_main, "tree_tree2", "tree_removal", LOG, TREE_INFO_ADD, false));

        TEST_EXPECT_SAVED_NEWICK__BROKEN(nSIMPLE, gb_main, "tree_removal", org_topo); // @@@ moving node info modifies topology (might be necessary to insert groups)
        TEST_EXPECT_SAVED_NEWICK(nGROUP, gb_main, "tree_removal", unwanted_topo2);

        // @@@ when we have a function to set the root according to another tree (#449),
        // use that function here. sorting tree after that, should again result in 'org_topo'!

        TEST_EXPECT_NO_ERROR(sort_namedtree_by_other_tree(gb_main, "tree_removal", "tree_removal_copy"));
        TEST_EXPECT_SAVED_NEWICK(nGROUP, gb_main, "tree_removal", sorted_topo2);
    }

    // compare node info
    {
        TEST_EXPECT_NO_ERROR(AWT_move_info(gb_main, "tree_test", "tree_removal", NULL, TREE_INFO_COMPARE, false));
        TEST_EXPECT_SAVED_NEWICK(nREMARK, gb_main, "tree_removal", compared_topo);
    }

    GB_unlink(LOG);
    GB_close(gb_main);
}

void TEST_edges() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_trees.arb", "rw");
    TEST_REJECT_NULL(gb_main);

    {
        GB_transaction  ta(gb_main);
        TreeNode       *tree = GBT_read_tree(gb_main, "tree_test", new SizeAwareRoot);

        TreeNode *left  = tree->findLeafNamed("CloTyro3"); TEST_REJECT_NULL(left);
        TreeNode *node  = left->get_father();              TEST_REJECT_NULL(node);
        TreeNode *right = node->findLeafNamed("CloTyro4"); TEST_REJECT_NULL(right);

        TEST_EXPECT(node == right->get_father());
        TEST_EXPECT(node->get_leftson()  == left);
        TEST_EXPECT(node->get_rightson() == right);

        TreeNode *parent  = node->get_father();                TEST_REJECT_NULL(parent);
        TreeNode *brother = parent->findLeafNamed("CloTyro2"); TEST_REJECT_NULL(brother);

        TEST_EXPECT(node->get_brother() == brother);

        TreeNode *grandpa  = parent->get_father(); TEST_REJECT_NULL(grandpa);

        // topology:
        //
        //            grandpa
        //              /
        //             /
        //            /
        //          parent
        //           /\              .
        //          /  \             .
        //         /    \            .
        //       node  brother
        //        /\                 .
        //       /  \                .
        //      /    \               .
        //    left right

        // test next() and otherNext() for inner edge 'node->parent'
        {
            ARB_edge nodeUp = parentEdge(node);

            TEST_EXPECT(node->is_leftson()); // if child is left son..
            TEST_EXPECT(nodeUp.next().dest()      == grandpa); // .. next() continues rootwards
            TEST_EXPECT(nodeUp.otherNext().dest() == brother);

            ARB_edge brotherUp = parentEdge(brother);

            TEST_EXPECT(brother->is_rightson());               // if child is right son..
            TEST_EXPECT(brotherUp.next().dest()      == node); // .. next() continues with other son
            TEST_EXPECT(brotherUp.otherNext().dest() == grandpa);

            ARB_edge down = nodeUp.inverse();

            TEST_EXPECT(down.next().dest()      == right); // next descends into right son
            TEST_EXPECT(down.otherNext().dest() == left);

            ARB_edge toLeaf(node, left);
            TEST_EXPECT(toLeaf.at_leaf());

            // both iterators should turn around at leaf:
            TEST_EXPECT(toLeaf.next().dest()      == node);
            TEST_EXPECT(toLeaf.otherNext().dest() == node);

            // test adjacent_distance
            const double EPSILON = 0.000001;

            const double NLEN = 0.025806;
            const double BLEN = 0.017316;
            const double PLEN = 0.017167;
            const double LLEN = 1.045690;
            const double RLEN = 0.060606;

            TEST_EXPECT_SIMILAR(node->get_branchlength(),             NLEN, EPSILON);
            TEST_EXPECT_SIMILAR(nodeUp.length(),                      NLEN, EPSILON);
            TEST_EXPECT_SIMILAR(down.length(),                        NLEN, EPSILON);
            TEST_EXPECT_SIMILAR(nodeUp.length_or_adjacent_distance(), NLEN, EPSILON);
            TEST_EXPECT_SIMILAR(down.length_or_adjacent_distance(),   NLEN, EPSILON);

            TEST_EXPECT_SIMILAR(brother->get_branchlength(), BLEN,      EPSILON);
            TEST_EXPECT_SIMILAR(parent ->get_branchlength(), PLEN,      EPSILON);
            TEST_EXPECT_SIMILAR(nodeUp.adjacent_distance(),  BLEN+PLEN, EPSILON);

            TEST_EXPECT_SIMILAR(left ->get_branchlength(), LLEN,      EPSILON);
            TEST_EXPECT_SIMILAR(right->get_branchlength(), RLEN,      EPSILON);
            TEST_EXPECT_SIMILAR(down.adjacent_distance(),  LLEN+RLEN, EPSILON);

            // modify lengths
            const double MOD_NLEN = 0.123456;
            const double MOD_LLEN = 0.246802;

            toLeaf.set_length(MOD_LLEN);
            nodeUp.set_length(MOD_NLEN);

            TEST_EXPECT_SIMILAR(toLeaf.length(), MOD_LLEN, EPSILON);
            TEST_EXPECT_SIMILAR(nodeUp.length(), MOD_NLEN, EPSILON);
            TEST_EXPECT_SIMILAR(down.length(),   MOD_NLEN, EPSILON);
        }

        destroy(tree);
    }

    GB_close(gb_main);
}

void TEST_toggle_bootstraps100() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_trees.arb", "rw");
    TEST_REJECT_NULL(gb_main);

    {
        GB_transaction  ta(gb_main);
        TreeNode       *tree = GBT_read_tree(gb_main, "tree_test", new SizeAwareRoot);
        TEST_REJECT_NULL(tree);

        const char *topo_org   = "(((((((CloTyro3,CloTyro4)'40%',CloTyro2)'0%',CloTyrob)'97%',CloInnoc)'0%',CloBifer)'53%',(((CloButy2,CloButyr)'100%',CloCarni)'33%',CloPaste)'97%')'100%',((((CorAquat,CurCitre)'100%',CorGluta)'17%',CelBiazo)'40%',CytAquat)'100%');";
        const char *topo_no100 = "(((((((CloTyro3,CloTyro4)'40%',CloTyro2)'0%',CloTyrob)'97%',CloInnoc)'0%',CloBifer)'53%',(((CloButy2,CloButyr)"    ",CloCarni)'33%',CloPaste)'97%')"    ",((((CorAquat,CurCitre)"    ",CorGluta)'17%',CelBiazo)'40%',CytAquat)"    ");";
        const char *topo_rem   = "(((((((CloTyro3,CloTyro4),CloTyro2),CloTyrob),CloInnoc),CloBifer),(((CloButy2,CloButyr),CloCarni),CloPaste)),((((CorAquat,CurCitre),CorGluta),CelBiazo),CytAquat));";

        TEST_EXPECT_NEWICK(nREMARK, tree, topo_org);

        tree->toggle_bootstrap100();
        TEST_EXPECT_NEWICK(nREMARK, tree, topo_no100);

        tree->toggle_bootstrap100();
        TEST_EXPECT_NEWICK(nREMARK, tree, topo_org);

        tree->remove_bootstrap();
        TEST_EXPECT_NEWICK(nREMARK, tree, topo_rem);

        destroy(tree);
    }

    GB_close(gb_main);
}

void TEST_multifurcate_tree() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_trees.arb", "rw");
    TEST_REJECT_NULL(gb_main);

    const char *topo_test               = "(((((((CloTyro3:1.046,CloTyro4:0.061)'40%':0.026,CloTyro2:0.017)'0%':0.017,CloTyrob:0.009)'97%:test':0.274,CloInnoc:0.371)'0%':0.057,CloBifer:0.388)'53%':0.124,(((CloButy2:0.009,CloButyr:0.000)'100%':0.564,CloCarni:0.120)'33%':0.010,CloPaste:0.179)'97%':0.131)'100%':0.081,((((CorAquat:0.084,CurCitre:0.058)'100%':0.103,CorGluta:0.522)'17%':0.053,CelBiazo:0.059)'40%':0.207,CytAquat:0.711)'100%':0.081);";
    // changes                          = "                                                                                                    +0.307         -0.371     +0.064 "
    const char *topo_single             = "(((((((CloTyro3:1.046,CloTyro4:0.061)'40%':0.026,CloTyro2:0.017)'0%':0.017,CloTyrob:0.009)'97%:test':0.581,CloInnoc:0.000)'0%':0.121,CloBifer:0.388)'53%':0.124,(((CloButy2:0.009,CloButyr:0.000)'100%':0.564,CloCarni:0.120)'33%':0.010,CloPaste:0.179)'97%':0.131)'100%':0.081,((((CorAquat:0.084,CurCitre:0.058)'100%':0.103,CorGluta:0.522)'17%':0.053,CelBiazo:0.059)'40%':0.207,CytAquat:0.711)'100%':0.081);";
    const char *topo_bs_less_101_005    = "(((((((CloTyro3:1.098,CloTyro4:0.064)"   ":0.000,CloTyro2:0.000)"  ":0.000,CloTyrob:0.000)'97%:test':0.287,CloInnoc:0.371)'0%':0.057,CloBifer:0.388)'53%':0.124,(((CloButy2:0.000,CloButyr:0.000)'100%':0.578,CloCarni:0.121)"   ":0.000,CloPaste:0.181)'97%':0.132)'100%':0.081,((((CorAquat:0.084,CurCitre:0.058)'100%':0.103,CorGluta:0.522)'17%':0.053,CelBiazo:0.059)'40%':0.207,CytAquat:0.711)'100%':0.081);";
    const char *topo_bs_less_101_005_NT = "(((((((CloTyro3:1.078,CloTyro4:0.062)"   ":0.000,CloTyro2:0.018)"  ":0.000,CloTyrob:0.009)'97%:test':0.282,CloInnoc:0.371)'0%':0.057,CloBifer:0.388)'53%':0.124,(((CloButy2:0.009,CloButyr:0.000)'100%':0.570,CloCarni:0.121)"   ":0.000,CloPaste:0.181)'97%':0.132)'100%':0.081,((((CorAquat:0.084,CurCitre:0.058)'100%':0.103,CorGluta:0.522)'17%':0.053,CelBiazo:0.059)'40%':0.207,CytAquat:0.711)'100%':0.081);";
    const char *topo_bs_less_30_005     = "(((((((CloTyro3:1.046,CloTyro4:0.061)'40%':0.027,CloTyro2:0.018)"  ":0.000,CloTyrob:0.009)'97%:test':0.288,CloInnoc:0.371)'0%':0.057,CloBifer:0.388)'53%':0.124,(((CloButy2:0.009,CloButyr:0.000)'100%':0.564,CloCarni:0.120)'33%':0.010,CloPaste:0.179)'97%':0.131)'100%':0.081,((((CorAquat:0.084,CurCitre:0.058)'100%':0.103,CorGluta:0.522)'17%':0.053,CelBiazo:0.059)'40%':0.207,CytAquat:0.711)'100%':0.081);";
    const char *topo_bs_less_30         = "(((((((CloTyro3:1.046,CloTyro4:0.061)'40%':0.027,CloTyro2:0.018)"  ":0.000,CloTyrob:0.009)'97%:test':0.302,CloInnoc:0.390)"  ":0.000,CloBifer:0.407)'53%':0.131,(((CloButy2:0.009,CloButyr:0.000)'100%':0.564,CloCarni:0.120)'33%':0.010,CloPaste:0.179)'97%':0.131)'100%':0.081,((((CorAquat:0.084,CurCitre:0.058)'100%':0.109,CorGluta:0.554)"   ":0.000,CelBiazo:0.062)'40%':0.220,CytAquat:0.711)'100%':0.081);";
    const char *topo_all                = "(((((((CloTyro3:0.000,CloTyro4:0.000)"   ":0.000,CloTyro2:0.000)"  ":0.000,CloTyrob:0.000)'"  "test':0.000,CloInnoc:0.000)"  ":0.000,CloBifer:0.000)"   ":0.000,(((CloButy2:0.000,CloButyr:0.000)"    ":0.000,CloCarni:0.000)"   ":0.000,CloPaste:0.000)"   ":0.000)"    ":0.000,((((CorAquat:0.000,CurCitre:0.000)"    ":0.000,CorGluta:0.000)"   ":0.000,CelBiazo:0.000)"   ":0.000,CytAquat:0.000)"    ":0.000);";

    const double STABLE_LENGTH = 5.362750;
    const double EPSILON       = 0.000001;

    for (int test = 1; test<=6; ++test) {
        GB_transaction  ta(gb_main);
        TreeNode       *tree = GBT_read_tree(gb_main, "tree_test", new SizeAwareRoot);

        TEST_REJECT_NULL(tree);
        if (test == 1) {
            TEST_EXPECT_NEWICK(nALL, tree, topo_test);
            TEST_EXPECT_SIMILAR(tree->sum_child_lengths(), STABLE_LENGTH, EPSILON);
        }

        switch (test) {
            case 1:
                tree->multifurcate_whole_tree(TreeNode::multifurc_limits(101, 0.05, true));
                TEST_EXPECT_NEWICK(nALL, tree, topo_bs_less_101_005);
                TEST_EXPECT_SIMILAR(tree->sum_child_lengths(), STABLE_LENGTH, EPSILON);
                break;
            case 6:
                tree->multifurcate_whole_tree(TreeNode::multifurc_limits(101, 0.05, false));
                TEST_EXPECT_NEWICK(nALL, tree, topo_bs_less_101_005_NT);
                TEST_EXPECT_SIMILAR(tree->sum_child_lengths(), STABLE_LENGTH, EPSILON);
                break;
            case 2:
                tree->multifurcate_whole_tree(TreeNode::multifurc_limits(30, 0.05, true));
                TEST_EXPECT_NEWICK(nALL, tree, topo_bs_less_30_005);
                TEST_EXPECT_SIMILAR(tree->sum_child_lengths(), STABLE_LENGTH, EPSILON);
                break;
            case 3:
                tree->multifurcate_whole_tree(TreeNode::multifurc_limits(30, 1000, true));
                TEST_EXPECT_NEWICK(nALL, tree, topo_bs_less_30);
                TEST_EXPECT_SIMILAR(tree->sum_child_lengths(), STABLE_LENGTH, EPSILON);
                break;
            case 4:
                tree->multifurcate_whole_tree(TreeNode::multifurc_limits(101, 1000, true)); // multifurcate all
                TEST_EXPECT_NEWICK(nALL, tree, topo_all);
                TEST_EXPECT_SIMILAR(tree->sum_child_lengths(), 0.0, EPSILON);
                break;
            case 5: {
                TreeNode *CloInnoc = tree->findLeafNamed("CloInnoc");
                TEST_REJECT_NULL(CloInnoc);

                parentEdge(CloInnoc).multifurcate();
                TEST_EXPECT_NEWICK(nALL, tree, topo_single);

                TEST_EXPECT_SIMILAR(tree->sum_child_lengths(), STABLE_LENGTH, EPSILON);
                break;
            }
            default:
                nt_assert(0);
                break;
        }

        destroy(tree);
    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

