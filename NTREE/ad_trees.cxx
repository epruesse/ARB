// =============================================================== //
//                                                                 //
//   File      : ad_trees.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ad_trees.hxx"
#include "nt_tree_cmp.hxx"

#include <TreeRead.h>
#include <TreeWrite.h>
#include <awt_sel_boxes.hxx>
#include <awt_nds.hxx>
#include <awt.hxx>
#include <aw_awars.hxx>
#include <aw_global.hxx>

#define nt_assert(bed) arb_assert(bed)

extern GBDATA *GLOBAL_gb_main;

const char *AWAR_TREE_DEST     = "tmp/ad_tree/tree_dest";
const char *AWAR_TREE_SECURITY = "tmp/ad_tree/tree_security";
const char *AWAR_TREE_REM      = "tmp/ad_tree/tree_rem";

#define AWAR_TREE_EXPORT           "tmp/ad_tree/export_tree"
#define AWAR_TREE_IMPORT           "tmp/ad_tree/import_tree"
#define AWAR_NODE_INFO_ONLY_MARKED "tmp/ad_tree/import_only_marked_node_info"
#define AWAR_TREE_EXPORT_SAVE      "ad_tree/export_tree"

#define AWAR_TREE_EXPORT_FILTER             AWAR_TREE_EXPORT "/filter"
#define AWAR_TREE_EXPORT_FORMAT             AWAR_TREE_EXPORT_SAVE "/format"
#define AWAR_TREE_EXPORT_NDS                AWAR_TREE_EXPORT_SAVE "/NDS"
#define AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS AWAR_TREE_EXPORT_SAVE "/bootstraps"
#define AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS AWAR_TREE_EXPORT_SAVE "/branchlens"
#define AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES AWAR_TREE_EXPORT_SAVE "/groupnames"
#define AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS AWAR_TREE_EXPORT_SAVE "/hide_folded"
#define AWAR_TREE_EXPORT_QUOTEMODE          AWAR_TREE_EXPORT_SAVE "/quote_mode"
#define AWAR_TREE_EXPORT_REPLACE            AWAR_TREE_EXPORT_SAVE "/replace"

void tree_vars_callback(AW_root *aw_root) // Map tree vars to display objects
{
    if (GLOBAL_gb_main) {
        GB_push_transaction(GLOBAL_gb_main);
        char *treename = aw_root->awar(AWAR_TREE_NAME)->read_string();
        GBDATA *ali_cont = GBT_get_tree(GLOBAL_gb_main, treename);
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
        aw_root->awar(AWAR_TREE_EXPORT "/file_name")->write_string(fname); // create default file name
        free(fname);
        free(suffix);
        GB_pop_transaction(GLOBAL_gb_main);
        free(treename);
    }
}
/*  update import tree name depending on file name */
void tree_import_callback(AW_root *aw_root) {
    GB_transaction  dummy(GLOBAL_gb_main);
    char           *treename        = aw_root->awar(AWAR_TREE_IMPORT "/file_name")->read_string();
    char           *treename_nopath = strrchr(treename, '/');

    if (treename_nopath) {
        ++treename_nopath;
    }
    else {
        treename_nopath = treename;
    }

    char *fname = GBS_string_eval(treename_nopath, "*.tree=tree_*1:*.ntree=tree_*1:*.xml=tree_*1:.=", 0);
    aw_root->awar(AWAR_TREE_IMPORT "/tree_name")->write_string(fname);

    free(fname);
    free(treename);
}


void ad_tree_set_security(AW_root *aw_root)
{
    if (GLOBAL_gb_main) {
        GB_transaction dummy(GLOBAL_gb_main);
        char *treename = aw_root->awar(AWAR_TREE_NAME)->read_string();
        GBDATA *ali_cont = GBT_get_tree(GLOBAL_gb_main, treename);
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

void update_filter_cb(AW_root *root) {
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

void create_trees_var(AW_root *aw_root, AW_default aw_def)
{
    AW_awar *awar_tree_name = aw_root->awar_string(AWAR_TREE_NAME,        0,  aw_def)->set_srt(GBT_TREE_AWAR_SRT);
    aw_root->awar_string(AWAR_TREE_DEST,        0,  aw_def) ->set_srt(GBT_TREE_AWAR_SRT);
    aw_root->awar_int(AWAR_TREE_SECURITY,       0,  aw_def);
    aw_root->awar_string(AWAR_TREE_REM,         0,  aw_def);

    aw_create_selection_box_awars(aw_root, AWAR_TREE_EXPORT, "", ".tree", "treefile", aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_FORMAT, AD_TREE_EXPORT_FORMAT_NEWICK, aw_def)-> add_callback(update_filter_cb);
    aw_root->awar_int(AWAR_TREE_EXPORT_NDS,  AD_TREE_EXPORT_NODE_SPECIES_NAME, aw_def)-> add_callback(update_filter_cb);

    aw_root->awar_int(AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS,  0, aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS,  1, aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS,  0, aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES,  1, aw_def);
    aw_root->awar_int(AWAR_TREE_EXPORT_QUOTEMODE, TREE_SINGLE_QUOTES, aw_def); // old default behavior
    aw_root->awar_int(AWAR_TREE_EXPORT_REPLACE, 0, aw_def); // old default behavior

    aw_create_selection_box_awars(aw_root, AWAR_TREE_IMPORT, "", ".tree", "treefile", aw_def);

    aw_root->awar_string(AWAR_TREE_IMPORT "/tree_name", "tree_",    aw_def) ->set_srt(GBT_TREE_AWAR_SRT);

    aw_root->awar(AWAR_TREE_IMPORT "/file_name")->add_callback(tree_import_callback);
    awar_tree_name->add_callback(tree_vars_callback);
    awar_tree_name->map(AWAR_TREE);
    aw_root->awar(AWAR_TREE_SECURITY)->add_callback(ad_tree_set_security);

    aw_root->awar_int(AWAR_NODE_INFO_ONLY_MARKED, 0,    aw_def);

    update_filter_cb(aw_root);
    tree_vars_callback(aw_root);
}

void tree_rename_cb(AW_window *aww) {
    AW_root  *aw_root   = aww->get_root();
    AW_awar  *awar_tree = aw_root->awar(AWAR_TREE_NAME);
    char     *source    = awar_tree->read_string();
    char     *dest      = aw_root->awar(AWAR_TREE_DEST)->read_string();
    GB_ERROR  error     = GBT_check_tree_name(dest);

    if (!error) {
        error = GB_begin_transaction(GLOBAL_gb_main);
        if (!error) {
            GBDATA *gb_tree_data = GB_search(GLOBAL_gb_main, "tree_data", GB_CREATE_CONTAINER);

            if (!gb_tree_data) error = GB_await_error();
            else {
                GBDATA *gb_tree_name = GB_entry(gb_tree_data, source);
                GBDATA *gb_dest      = GB_entry(gb_tree_data, dest);

                if (gb_dest) error = GBS_global_string("Tree '%s' already exists", dest);
                else if (!gb_tree_name) error = "Please select a tree";
                else {
                    GBDATA *gb_new_tree = GB_create_container(gb_tree_data, dest);

                    if (!gb_new_tree) error = GB_await_error();
                    else {
                        error = GB_copy(gb_new_tree, gb_tree_name);
                        if (!error) error = GB_delete(gb_tree_name);
                    }
                }
            }
        }

        if (!error) awar_tree->write_string(dest);
        error = GB_end_transaction(GLOBAL_gb_main, error);
    }

    aww->hide_or_notify(error);

    free(dest);
    free(source);
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

void tree_copy_cb(AW_window *aww) {
    char     *source = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();
    char     *dest   = aww->get_root()->awar(AWAR_TREE_DEST)->read_string();
    GB_ERROR  error  = GBT_check_tree_name(dest);

    if (!error) {
        error = GB_begin_transaction(GLOBAL_gb_main);
        if (!error) {
            GBDATA *gb_tree_data     = GB_search(GLOBAL_gb_main, "tree_data", GB_CREATE_CONTAINER);
            if (!gb_tree_data) error = GB_await_error();
            else {
                GBDATA *gb_tree_name = GB_entry(gb_tree_data, source);
                GBDATA *gb_dest      = GB_entry(gb_tree_data, dest);

                if (gb_dest) error = GBS_global_string("Tree '%s' already exists", dest);
                else if (!gb_tree_name) error = "Please select a tree";
                else {
                    gb_dest             = GB_create_container(gb_tree_data, dest);
                    if (!gb_dest) error = GB_await_error();
                    else {
                        error = GB_copy(gb_dest, gb_tree_name);
                        if (!error) error = tree_append_remark(gb_dest, GBS_global_string("[created as copy of '%s']", source));
                    }
                }
            }
        }
        error = GB_end_transaction(GLOBAL_gb_main, error);
    }

    aww->hide_or_notify(error);

    free(dest);
    free(source);
}

static void tree_save_cb(AW_window *aww) {
    AW_root  *aw_root   = aww->get_root();
    char     *tree_name = aw_root->awar(AWAR_TREE_NAME)->read_string();

    GB_ERROR error = 0;

    if (!tree_name || !strlen(tree_name)) {
        error = "Please select a tree first";
    }
    else {
        char *fname   = aw_root->awar(AWAR_TREE_EXPORT "/file_name")->read_string();
        char *db_name = aw_root->awar(AWAR_DB_NAME)->read_string();

        bool                use_NDS    = ExportNodeType(aw_root->awar(AWAR_TREE_EXPORT_NDS)->read_int()) == AD_TREE_EXPORT_NODE_NDS;
        ExportTreeType      exportType = static_cast<ExportTreeType>(aw_root->awar(AWAR_TREE_EXPORT_FORMAT)->read_int());
        TREE_node_text_gen *node_gen   = use_NDS ? new TREE_node_text_gen(make_node_text_init, make_node_text_nds) : 0;

        switch (exportType) {
            case AD_TREE_EXPORT_FORMAT_XML:
                error = TREE_write_XML(GLOBAL_gb_main, db_name, tree_name, node_gen,
                                       aw_root->awar(AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS)->read_int(),
                                       fname);
                break;

            case AD_TREE_EXPORT_FORMAT_NEWICK:
            case AD_TREE_EXPORT_FORMAT_NEWICK_PRETTY:
                TREE_node_quoting quoteMode = TREE_node_quoting(aw_root->awar(AWAR_TREE_EXPORT_QUOTEMODE)->read_int());
                if (aw_root->awar(AWAR_TREE_EXPORT_REPLACE)->read_int()) {
                    quoteMode = TREE_node_quoting(quoteMode|TREE_FORCE_REPLACE);
                }

                error = TREE_write_Newick(GLOBAL_gb_main, tree_name, node_gen,
                                          aw_root->awar(AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS)->read_int(),
                                          aw_root->awar(AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS)->read_int(),
                                          aw_root->awar(AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES)->read_int(),
                                          exportType == AD_TREE_EXPORT_FORMAT_NEWICK_PRETTY,
                                          quoteMode,
                                          fname);
                break;
        }

        awt_refresh_selection_box(aw_root, AWAR_TREE_EXPORT);

        delete node_gen;
        free(db_name);
        free(fname);
    }

    aww->hide_or_notify(error);
    free(tree_name);
}

AW_window *create_tree_export_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "SAVE_TREE", "TREE SAVE");
    aws->load_xfig("sel_box_user2.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"tr_export.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at("user");
    aws->create_option_menu(AWAR_TREE_EXPORT_FORMAT, 0, 0);
    aws->insert_option("NEWICK TREE FORMAT",                   "N", AD_TREE_EXPORT_FORMAT_NEWICK);
    aws->insert_option("NEWICK TREE FORMAT (pretty, but big)", "P", AD_TREE_EXPORT_FORMAT_NEWICK_PRETTY);
    aws->insert_option("ARB_XML TREE FORMAT",                  "X", AD_TREE_EXPORT_FORMAT_XML);
    aws->update_option_menu();

    awt_create_selection_box((AW_window *)aws, AWAR_TREE_EXPORT "");

    aws->at("user2");
    aws->auto_space(10, 10);
    aws->label("Nodetype");
    aws->create_toggle_field(AWAR_TREE_EXPORT_NDS, 1);
    aws->insert_default_toggle("Species name", "S", 0);
    aws->insert_toggle("NDS", "N", 1);
    aws->update_toggle_field();

    aws->at_newline(); aws->label("Save branch lengths"); aws->create_toggle(AWAR_TREE_EXPORT_INCLUDE_BRANCHLENS);
    aws->at_newline(); aws->label("Save bootstrap values"); aws->create_toggle(AWAR_TREE_EXPORT_INCLUDE_BOOTSTRAPS);
    aws->at_newline(); aws->label("Save group names"); aws->create_toggle(AWAR_TREE_EXPORT_INCLUDE_GROUPNAMES);
    aws->at_newline(); aws->label("Hide folded groups (XML only)"); aws->create_toggle(AWAR_TREE_EXPORT_HIDE_FOLDED_GROUPS);

    aws->at_newline(); aws->label("Name quoting (Newick only)");
    aws->create_option_menu(AWAR_TREE_EXPORT_QUOTEMODE, 0, 0);
    aws->insert_option("none",            "n", TREE_DISALLOW_QUOTES);
    aws->insert_option("single",          "s", TREE_SINGLE_QUOTES);
    aws->insert_option("double",          "d", TREE_DOUBLE_QUOTES);
    aws->insert_option("single (forced)", "i", TREE_SINGLE_QUOTES|TREE_FORCE_QUOTES);
    aws->insert_option("double (forced)", "o", TREE_DOUBLE_QUOTES|TREE_FORCE_QUOTES);
    aws->update_option_menu();

    aws->at_newline(); aws->label("Replace problem chars"); aws->create_toggle(AWAR_TREE_EXPORT_REPLACE);

    aws->at_newline();
    aws->callback(tree_save_cb);
    aws->create_button("SAVE", "SAVE", "o");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CANCEL", "CANCEL", "C");

    aws->window_fit();
    update_filter_cb(root);

    return (AW_window *)aws;
}

char *readXmlTree(char *fname) {
    // create a temp file
    char tempFile[]  = "newickXXXXXX";
    int createTempFile = mkstemp(tempFile);

    if (createTempFile) {
        char          *tmpFname = strdup(fname);
        char          *tmp      = 0;
        GBS_strstruct *buf      = GBS_stropen(strlen(fname));

        // extract path from fname in order to place a copy of dtd file required to validate xml file
        for (char *tok = strtok(tmpFname, "/"); tok;) {
            tmp = tok;
            tok = strtok(0, "/");
            if (tok) {
                GBS_strcat(buf, "/");
                GBS_strcat(buf, tmp);
            }
        }
        char *path = GBS_strclose(buf);

        // linking arb_tree.dtd file to the Path from where xml file is loaded
#if defined(DEVEL_RALF)
#warning fix hack
#endif // DEVEL_RALF
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

void tree_load_cb(AW_window *aww) {
    GB_ERROR  error     = 0;
    AW_root  *aw_root   = aww->get_root();
    char     *tree_name = aw_root->awar(AWAR_TREE_IMPORT "/tree_name")->read_string();

    {
        char *pcTreeFormat = aw_root->awar(AWAR_TREE_IMPORT "/filter")->read_string();
        char *fname        = aw_root->awar(AWAR_TREE_IMPORT "/file_name")->read_string();
        char *scaleWarning = 0;
        char *tree_comment = 0;

        GBT_TREE *tree;
        if (strcmp(pcTreeFormat, "xml") == 0) {
            char *tempFname = readXmlTree(fname);
            tree = TREE_load(tempFname, sizeof(GBT_TREE), &tree_comment, 1, &scaleWarning);
            GB_unlink_or_warn(tempFname, NULL);
            free(tempFname);
        }
        else {
            tree = TREE_load(fname, sizeof(GBT_TREE), &tree_comment, 1, &scaleWarning);
        }

        if (!tree) error = GB_await_error();
        else {
            if (scaleWarning) GBT_message(GLOBAL_gb_main, scaleWarning);

            GB_transaction ta(GLOBAL_gb_main);
            error = GBT_write_tree(GLOBAL_gb_main, 0, tree_name, tree);

            if (!error && tree_comment) {
                error = GBT_write_tree_rem(GLOBAL_gb_main, tree_name, tree_comment);
            }

            if (error) error = ta.close(error);
            else aw_root->awar(AWAR_TREE)->write_string(tree_name); // show new tree

            GBT_delete_tree(tree);
        }

        free(scaleWarning);
        free(tree_comment);
        free(fname);
        free(pcTreeFormat);
    }

    aww->hide_or_notify(error);
    free(tree_name);
}

AW_window *create_tree_import_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "LOAD_TREE", "TREE LOAD");
    aws->load_xfig("sel_box_tree.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("format");
    aws->label("Tree Format :");
    aws->create_option_menu(AWAR_TREE_IMPORT "/filter");
    aws->insert_default_option("Newick", "t", "tree");
    aws->insert_option("XML", "x", "xml");
    aws->update_option_menu();

    aws->at("user");
    aws->label("tree_name:");
    aws->create_input_field(AWAR_TREE_IMPORT "/tree_name", 15);

    awt_create_selection_box(aws, AWAR_TREE_IMPORT "");

    aws->at("save2");
    aws->callback(tree_load_cb);
    aws->create_button("LOAD", "LOAD", "o");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("cancel2");
    aws->create_button("CANCEL", "CANCEL", "C");

    aws->window_fit();

    return (AW_window *)aws;
}

AW_window *create_tree_rename_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "RENAME_TREE", "TREE RENAME");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the new name\nof the tree");

    aws->at("input");
    aws->create_input_field(AWAR_TREE_DEST, 15);

    aws->at("ok");
    aws->callback(tree_rename_cb);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

AW_window *create_tree_copy_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "COPY_TREE", "TREE COPY");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the new tree");

    aws->at("input");
    aws->create_input_field(AWAR_TREE_DEST, 15);

    aws->at("ok");
    aws->callback(tree_copy_cb);
    aws->create_button("GO", "GO", "G");

    return (AW_window *)aws;
}

void ad_move_tree_info(AW_window *aww, AW_CL mode) {
    /* mode == 0 -> move info (=overwrite info from source tree)
     * mode == 1 -> compare info
     * mode == 2 -> add info
     */

    bool compare_node_info      = mode==1;
    bool delete_old_nodes       = mode==0;
    bool nodes_with_marked_only = (mode==0 || mode==2) && aww->get_root()->awar(AWAR_NODE_INFO_ONLY_MARKED)->read_int();

    char     *log_file = 0;
    GB_ERROR  error    = 0;

    if (!compare_node_info) {
        // move or add node-info writes a log file (containing errors)
        // compare_node_info only sets remark branches
        char *log_name       = GB_unique_filename("arb_node", "log");
        log_file             = GB_create_tempfile(log_name);
        if (!log_file) error = GB_await_error();
        free(log_name);
    }

    if (!error) {
        char *t1 = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();
        char *t2 = aww->get_root()->awar(AWAR_TREE_DEST)->read_string();

        AWT_move_info(GLOBAL_gb_main, t1, t2, log_file, compare_node_info, delete_old_nodes, nodes_with_marked_only);
        if (log_file) {
            AWT_edit(log_file);
            GB_remove_on_exit(log_file);
        }

        free(t2);
        free(t1);
    }

    if (error) aw_message(error);
    else aww->hide();

    free(log_file);
}

AW_window *create_tree_diff_window(AW_root *root) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;
    GB_transaction dummy(GLOBAL_gb_main);
    aws = new AW_window_simple;
    aws->init(root, "CMP_TOPOLOGY", "COMPARE TREE TOPOLOGIES");
    aws->load_xfig("ad_tree_cmp.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"tree_diff.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    root->awar_string(AWAR_TREE_NAME);
    root->awar_string(AWAR_TREE_DEST);

    aws->at("tree1");
    awt_create_selection_list_on_trees(GLOBAL_gb_main, (AW_window *)aws, AWAR_TREE_NAME);
    aws->at("tree2");
    awt_create_selection_list_on_trees(GLOBAL_gb_main, (AW_window *)aws, AWAR_TREE_DEST);

    aws->button_length(20);

    aws->at("move");
    aws->callback(ad_move_tree_info, 1); // compare
    aws->create_button("CMP_TOPOLOGY", "COMPARE TOPOLOGY");

    return aws;
}
AW_window *create_tree_cmp_window(AW_root *root) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;
    GB_transaction dummy(GLOBAL_gb_main);
    aws = new AW_window_simple;
    aws->init(root, "COPY_NODE_INFO_OF_TREE", "TREE COPY INFO");
    aws->load_xfig("ad_tree_cmp.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"tree_cmp.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    root->awar_string(AWAR_TREE_NAME);
    root->awar_string(AWAR_TREE_DEST);

    aws->at("tree1");
    awt_create_selection_list_on_trees(GLOBAL_gb_main, (AW_window *)aws, AWAR_TREE_NAME);
    aws->at("tree2");
    awt_create_selection_list_on_trees(GLOBAL_gb_main, (AW_window *)aws, AWAR_TREE_DEST);

    aws->at("move");
    aws->callback(ad_move_tree_info, 0); // move
    aws->create_button("COPY_INFO", "COPY INFO");

    aws->at("cmp");
    aws->callback(ad_move_tree_info, 2); // add
    aws->create_button("ADD_INFO", "ADD INFO");

    aws->at("only_marked");
    aws->label("only info containing marked species");
    aws->create_toggle(AWAR_NODE_INFO_ONLY_MARKED);

    return aws;
}
void ad_tr_delete_cb(AW_window *aww) {
    AW_awar  *awar_tree = aww->get_root()->awar(AWAR_TREE_NAME);
    char     *source    = awar_tree->read_string();

    // 1. switch to next tree
    {
        GB_transaction ta(GLOBAL_gb_main);
        char *newname = GBT_get_next_tree_name(GLOBAL_gb_main, source);

        awar_tree->write_string(!newname || (strcmp(newname, source) == 0) ? "" : newname);
        free(newname);
    }

    // 2. delete old tree
    {
        GB_ERROR  error   = GB_begin_transaction(GLOBAL_gb_main);
        GBDATA   *gb_tree = GBT_get_tree(GLOBAL_gb_main, source);
        if (gb_tree) {
            char *newname = GBT_get_next_tree_name(GLOBAL_gb_main, source);
            error = GB_delete(gb_tree);
            if (!error && newname) {
                awar_tree->write_string(strcmp(newname, source) == 0 ? "" : newname);
            }
            free(newname);
        }
        else {
            error = "Please select a tree first";
        }

        error = GB_end_transaction(GLOBAL_gb_main, error);
        if (error) {
            aw_message(error);
            awar_tree->write_string(source); // switch back to failed tree
        }
    }

    free(source);
}

static void create_tree_last_window(AW_window *aww) {
    GB_ERROR  error  = 0;
    char     *source = aww->get_root()->awar(AWAR_TREE_NAME)->read_string();

    GB_begin_transaction(GLOBAL_gb_main);

    GBDATA *gb_tree_data = GB_search(GLOBAL_gb_main, "tree_data", GB_CREATE_CONTAINER);
    GBDATA *gb_tree_name = GB_entry(gb_tree_data, source);

    if (!gb_tree_name) {
        error = GB_export_error("No tree selected.");
    }
    else {
        GBDATA *gb_dest   = GB_create_container(gb_tree_data, source);
        error             = GB_copy(gb_dest, gb_tree_name);
        if (!error) error = GB_delete(gb_tree_name);
    }

    if (!error) GB_commit_transaction(GLOBAL_gb_main);
    else GB_abort_transaction(GLOBAL_gb_main);

    if (error) aw_message(error);

    free(source);
}

void move_tree_pos(AW_window *aww, AW_CL cl_offset) {
    // moves the tree in the list of trees
    int   offset = (int)cl_offset;

    if (offset == 9999) {
        create_tree_last_window(aww);
    }
    else {
        aw_message("Not implemented yet.");
        // @@@ FIXME: implement other cases
    }
}

AW_window *create_trees_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "TREE_ADMIN", "TREE ADMIN");
        aws->load_xfig("ad_tree.fig");

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(AW_POPUP_HELP, (AW_CL)"treeadm.hlp");
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->button_length(20);

        aws->at("delete");
        aws->callback(ad_tr_delete_cb);
        aws->create_button("DELETE", "Delete", "D");

        aws->at("rename");
        aws->callback(AW_POPUP, (AW_CL)create_tree_rename_window, 0);
        aws->create_button("RENAME", "Rename", "R");

        aws->at("copy");
        aws->callback(AW_POPUP, (AW_CL)create_tree_copy_window, 0);
        aws->create_button("COPY", "Copy", "C");

        aws->at("move");
        aws->callback(AW_POPUP, (AW_CL)create_tree_cmp_window, 0);
        aws->create_button("MOVE_NODE_INFO", "Move node info", "C");

        aws->at("cmp");
        aws->callback(AW_POPUP, (AW_CL)create_tree_diff_window, 0);
        aws->sens_mask(AWM_EXP);
        aws->create_button("CMP_TOPOLOGY", "Compare topology", "T");
        aws->sens_mask(AWM_ALL);

        aws->at("export");
        aws->callback(AW_POPUP, (AW_CL)create_tree_export_window, 0);
        aws->create_button("EXPORT", "Export", "E");

        aws->at("import");
        aws->callback(AW_POPUP, (AW_CL)create_tree_import_window, 0);
        aws->create_button("IMPORT", "Import", "I");

        aws->button_length(0);

#if defined(DEBUG)
#if defined(DEVEL_RALF)
#warning implement tree move buttons
#endif // DEVEL_RALF
        aws->at("upall");
        aws->callback(move_tree_pos, (AW_CL)-9999);
        aws->create_button("moveUpAll", "#moveUpAll.bitmap", 0);

        aws->at("up");
        aws->callback(move_tree_pos, (AW_CL)-1);
        aws->create_button("moveUp", "#moveUp.bitmap", 0);

        aws->at("down");
        aws->callback(move_tree_pos, (AW_CL)1);
        aws->create_button("moveDown", "#moveDown.bitmap", 0);
#endif // DEBUG

        aws->at("downall");
        aws->callback(move_tree_pos, (AW_CL)9999);
        aws->create_button("moveDownAll", "#moveDownAll.bitmap", 0);


        aws->at("list");
        awt_create_selection_list_on_trees(GLOBAL_gb_main, (AW_window *)aws, AWAR_TREE_NAME);

        aws->at("security");
        aws->create_option_menu(AWAR_TREE_SECURITY);
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
    }

    return (AW_window *)aws;
}
