// =============================================================== //
//                                                                 //
//   File      : TreeCallbacks.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "TreeCallbacks.hxx"

#include <aw_awars.hxx>
#include <aw_advice.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <mode_text.h>

#include <cctype>

using namespace AW;

// AISC_MKPT_PROMOTE:#ifndef TREEDISPLAY_HXX
// AISC_MKPT_PROMOTE:#include <TreeDisplay.hxx>
// AISC_MKPT_PROMOTE:#endif

void nt_mode_event(UNFIXED, AWT_canvas *ntw, AWT_COMMAND_MODE mode) {
    const char *text;

    switch (mode) {
        case AWT_MODE_ZOOM:  text = MODE_TEXT_STANDARD_ZOOMMODE(); break;
        case AWT_MODE_EMPTY: text = MODE_TEXT_PLACEHOLDER();       break;

        case AWT_MODE_SELECT: text = MODE_TEXT_1BUTTON("SELECT", "select species or open/close group");                  break;
        case AWT_MODE_INFO:   text = MODE_TEXT_1BUTTON("INFO",   "click for info");                                      break;
        case AWT_MODE_WWW:    text = MODE_TEXT_1BUTTON("WEB",    "Launch node dependent URL (see <Properties/WWW...>)"); break;

        case AWT_MODE_SWAP:       text = MODE_TEXT_2BUTTONS("SWAP",         "swap child branches",        "flip whole subtree");          break;
        case AWT_MODE_MARK:       text = MODE_TEXT_2BUTTONS("MARK",         "mark subtree",               "unmark subtree");              break;
        case AWT_MODE_GROUP:      text = MODE_TEXT_2BUTTONS("GROUP",        "fold/unfold group",          "create/rename/destroy group"); break;
        case AWT_MODE_NNI:        text = MODE_TEXT_2BUTTONS("OPTI(NNI)",    "subtree",                    "whole tree");                  break;
        case AWT_MODE_KERNINGHAN: text = MODE_TEXT_2BUTTONS("OPTI(KL)",     "subtree",                    "whole tree");                  break;
        case AWT_MODE_OPTIMIZE:   text = MODE_TEXT_2BUTTONS("OPTI(NNI&KL)", "subtree",                    "whole tree");                  break;
        case AWT_MODE_SETROOT:    text = MODE_TEXT_2BUTTONS("REROOT",       "set root to clicked branch", "search optimal root");         break;

        case AWT_MODE_ROTATE: text = MODE_TEXT_1BUTTON_KEYS("ROTATE", "drag branch to rotate",         KEYINFO_ABORT_AND_RESET); break;
        case AWT_MODE_SPREAD: text = MODE_TEXT_1BUTTON_KEYS("SPREAD", "drag branch to spread subtree", KEYINFO_ABORT_AND_RESET); break;

        case AWT_MODE_LENGTH:    text = MODE_TEXT_2BUTTONS_KEYS("LENGTH",       "drag branch/ruler", "use discrete lengths", KEYINFO_ABORT_AND_RESET); break;
        case AWT_MODE_MULTIFURC: text = MODE_TEXT_2BUTTONS_KEYS("MULTIFURC",    "drag branch",       "use discrete lengths", KEYINFO_ABORT_AND_RESET); break;
        case AWT_MODE_LINE:      text = MODE_TEXT_2BUTTONS_KEYS("LINE",         "drag branch/ruler", "whole subtree",        KEYINFO_ABORT_AND_RESET); break;
        case AWT_MODE_MOVE:      text = MODE_TEXT_2BUTTONS_KEYS("MOVE",         "drag branch/ruler", "move groupinfo only",  KEYINFO_ABORT);           break;
        case AWT_MODE_LZOOM:     text = MODE_TEXT_2BUTTONS_KEYS("LOGICAL ZOOM", "show only subtree", "go up one step",       KEYINFO_RESET);           break;

        default: text = no_mode_text_defined(); break;
    }

    td_assert(strlen(text) < AWAR_FOOTER_MAX_LEN); // text too long!

    ntw->awr->awar(AWAR_FOOTER)->write_string(text);
    ntw->set_mode(mode);
}

// ---------------------------------------
//      Basic mark/unmark callbacks :

static void count_mark_all_cb(UNFIXED, AWT_canvas *ntw) {
    GB_push_transaction(ntw->gb_main);

    GBDATA *gb_species_data = GBT_get_species_data(ntw->gb_main);
    long    count           = GB_number_of_marked_subentries(gb_species_data);

    GB_pop_transaction(ntw->gb_main);

    char buf[256];
    switch (count) {
        case 0: strcpy(buf, "There are NO marked species"); break;
        case 1: strcpy(buf, "There is 1 marked species"); break;
        default: sprintf(buf, "There are %li marked species", count); break;
    }
    strcat(buf, ". (The number of species is displayed in the top area as well)");
    aw_message(buf);
}

static int species_has_alignment(GBDATA *gb_species, void *cd_use) {
    return GBT_read_sequence(gb_species, (const char*)cd_use) != 0;
}

static int sequence_is_partial(GBDATA *gb_species, void *cd_partial) {
    long wanted  = (long)cd_partial;
    td_assert(wanted == 0 || wanted == 1);
    int partial = GBT_is_partial(gb_species, 1-wanted, 0);

    return partial == wanted;
}

#define MARK_MODE_LOWER_BITS (1|2)
#define MARK_MODE_UPPER_BITS (4|8|16)

void NT_mark_all_cb(UNFIXED, AWT_canvas *ntw, int mark_mode) {
    // Bits 0 and 1 of mark_mode:
    //
    // mark_mode&3  == 0 -> unmark
    // mark_mode&3  == 1 -> mark
    // mark_mode&3  == 2 -> toggle mark
    //
    // Bits 2 .. 4 of mark_mode:
    //
    // mark_mode&12 == 4 -> affect only full sequences
    // mark_mode&12 == 8 -> affect only partial sequences
    // mark_mode&12 == 16 -> affect only species with data in current alignment
    // else -> affect all sequences

    GB_transaction ta(ntw->gb_main);

    switch (mark_mode&MARK_MODE_UPPER_BITS) {
        case 0:                 // all sequences
            GBT_mark_all(ntw->gb_main, mark_mode&MARK_MODE_LOWER_BITS);
            break;
        case 4:                 // full sequences only
            GBT_mark_all_that(ntw->gb_main, mark_mode&MARK_MODE_LOWER_BITS, sequence_is_partial, (void*)0);
            break;
        case 8:                 // partial sequences only
            GBT_mark_all_that(ntw->gb_main, mark_mode&MARK_MODE_LOWER_BITS, sequence_is_partial, (void*)1);
            break;
        case 16: {               // species with data in alignment only
            char *ali = GBT_get_default_alignment(ntw->gb_main);
            if (ali) GBT_mark_all_that(ntw->gb_main, mark_mode&MARK_MODE_LOWER_BITS, species_has_alignment, (void*)ali);
            free(ali);
            break;
        }
        default:
            td_assert(0); // illegal mode
            break;
    }

    ntw->refresh();
}

static void mark_tree_cb(UNFIXED, AWT_canvas *ntw, int mark_mode) {
    AWT_graphic_tree *gtree = AWT_TREE(ntw);
    GB_transaction    ta(ntw->gb_main);

    gtree->check_update(ntw->gb_main);
    AP_tree *tree_root = gtree->get_root_node();

    switch (mark_mode&MARK_MODE_UPPER_BITS) {
        case 0:                 // all sequences
            gtree->mark_species_in_tree(tree_root, mark_mode&MARK_MODE_LOWER_BITS);
            break;
        case 4:                 // full sequences only
            gtree->mark_species_in_tree_that(tree_root, mark_mode&MARK_MODE_LOWER_BITS, sequence_is_partial, (void*)0);
            break;
        case 8:                 // partial sequences only
            gtree->mark_species_in_tree_that(tree_root, mark_mode&MARK_MODE_LOWER_BITS, sequence_is_partial, (void*)1);
            break;
        case 16: {               // species with data in alignment only
            char *ali = GBT_get_default_alignment(ntw->gb_main);
            if (ali) gtree->mark_species_in_tree_that(tree_root, mark_mode&MARK_MODE_LOWER_BITS, species_has_alignment, (void*)ali);
            free(ali);
            break;
        }
        default:
            td_assert(0); // illegal mode
            break;
    }
    ntw->refresh();
}

struct mark_nontree_cb_data {
    int      mark_mode_upper_bits;
    char    *ali;               // current alignment (only if mark_mode_upper_bits == 16)
    GB_HASH *hash;
};

static int are_not_in_tree(GBDATA *gb_species, void *cb_data) {
    struct mark_nontree_cb_data *data    = (mark_nontree_cb_data*)cb_data;
    const char                  *name    = GBT_read_name(gb_species);
    bool                         mark_me = false;

    if (GBS_read_hash(data->hash, name) == (long)gb_species) { // species is not in tree!
        switch (data->mark_mode_upper_bits) {
            case 0:             // all sequences
                mark_me = true;
                break;
            case 4:             // full sequences only
                mark_me = sequence_is_partial(gb_species, (void*)0);
                break;
            case 8:             // partial sequences only
                mark_me = sequence_is_partial(gb_species, (void*)1);
                break;
            case 16:            // species with data in alignment only
                mark_me = species_has_alignment(gb_species, data->ali);
                break;
            default:
                td_assert(0); // illegal mode
                break;
        }
    }

    return mark_me;
}

static void mark_nontree_cb(UNFIXED, AWT_canvas *ntw, int mark_mode) {
    AWT_graphic_tree            *gtree = AWT_TREE(ntw);
    GB_transaction               ta(ntw->gb_main);
    struct mark_nontree_cb_data  cd;

    if ((mark_mode&MARK_MODE_LOWER_BITS) == 0) {   // unmark is much faster
        cd.hash = GBT_create_marked_species_hash(ntw->gb_main); // because it only hashes marked species
    }
    else {
        cd.hash = GBT_create_species_hash(ntw->gb_main); // for mark we have to hash ALL species
    }

    NT_remove_species_in_tree_from_hash(gtree->get_root_node(), cd.hash);

    cd.mark_mode_upper_bits = mark_mode&MARK_MODE_UPPER_BITS;
    cd.ali                  = cd.mark_mode_upper_bits == 16 ? GBT_get_default_alignment(ntw->gb_main) : 0;

    GBT_mark_all_that(ntw->gb_main, mark_mode&MARK_MODE_LOWER_BITS, are_not_in_tree, (void*)&cd);

    free(cd.ali);

    ntw->refresh();
}

static char *create_mark_menu_entry(const char *attrib, const char *entry_template) {
    char *entry = 0;
    if (attrib) {
        bool append = attrib[0] == '-'; // if attrib starts with '-' then append (otherwise prepend)
        if (append) ++attrib; // skip '-'

        if (append) {
            char *spaced_attrib = GBS_global_string_copy(" %s", attrib);
            entry               = GBS_global_string_copy(entry_template, "", spaced_attrib);
            free(spaced_attrib);
        }
        else {
            char *spaced_attrib = GBS_global_string_copy("%s ", attrib);
            entry               = GBS_global_string_copy(entry_template, spaced_attrib, "");

            if (islower(entry[0])) entry[0] = toupper(entry[0]); // Caps prepended lowercase 'attrib'

            free(spaced_attrib);
        }
    }
    else {
        entry = GBS_global_string_copy(entry_template, "", "");
    }
    return entry;
}
static char *create_mark_menu_id(const char *attrib, const char *id_suffix) {
    char *id = 0;
    if (attrib) {
        id = GBS_global_string_copy("%s_%s", attrib[0] == '-' ? attrib+1 : attrib, id_suffix);
    }
    else {
        id = strdup(id_suffix);
    }
    return id;
}

static void insert_mark_topic(AW_window_menu_modes *awm, AW_active mask, const char *attrib, const char *id_suffix, const char *entry_template,
                              const char *hotkey, const char *helpfile, const WindowCallback& wcb)
{
    char *entry = create_mark_menu_entry(attrib, entry_template);
    char *id    = create_mark_menu_id(attrib, id_suffix);

    awm->insert_menu_topic(id, entry, hotkey, helpfile, mask, wcb);

    free(id);
    free(entry);
}

static void insert_mark_topics(AW_window_menu_modes *awm, AW_active mask, AWT_canvas *ntw, int affect, const char *attrib)
{
    td_assert(affect == (affect&MARK_MODE_UPPER_BITS)); // only bits 2 .. 4 are allowed

    insert_mark_topic(awm, mask, attrib, "mark_all",    "Mark all %sSpecies%s",            "M", "sp_mrk_all.hlp",    makeWindowCallback(NT_mark_all_cb, ntw, 1+affect));
    insert_mark_topic(awm, mask, attrib, "unmark_all",  "Unmark all %sSpecies%s",          "U", "sp_umrk_all.hlp",   makeWindowCallback(NT_mark_all_cb, ntw, 0+affect));
    insert_mark_topic(awm, mask, attrib, "swap_marked", "Invert marks of all %sSpecies%s", "I", "sp_invert_mrk.hlp", makeWindowCallback(NT_mark_all_cb, ntw, 2+affect));
    awm->sep______________();

    char *label = create_mark_menu_entry(attrib, "%sSpecies%s in Tree");

    awm->insert_sub_menu(label, "T");
    insert_mark_topic(awm, mask, attrib, "mark_tree",        "Mark %sSpecies%s in Tree",            "M", "sp_mrk_tree.hlp",   makeWindowCallback(mark_tree_cb, ntw, 1+affect));
    insert_mark_topic(awm, mask, attrib, "unmark_tree",      "Unmark %sSpecies%s in Tree",          "U", "sp_umrk_tree.hlp",  makeWindowCallback(mark_tree_cb, ntw, 0+affect));
    insert_mark_topic(awm, mask, attrib, "swap_marked_tree", "Invert marks of %sSpecies%s in Tree", "I", "sp_invert_mrk.hlp", makeWindowCallback(mark_tree_cb, ntw, 2+affect));
    awm->close_sub_menu();

    freeset(label, create_mark_menu_entry(attrib, "%sSpecies%s NOT in Tree"));

    awm->insert_sub_menu(label, "N");
    insert_mark_topic(awm, mask, attrib, "mark_nontree",        "Mark %sSpecies%s NOT in Tree",            "M", "sp_mrk_tree.hlp",   makeWindowCallback(mark_nontree_cb, ntw, 1+affect));
    insert_mark_topic(awm, mask, attrib, "unmark_nontree",      "Unmark %sSpecies%s NOT in Tree",          "U", "sp_umrk_tree.hlp",  makeWindowCallback(mark_nontree_cb, ntw, 0+affect));
    insert_mark_topic(awm, mask, attrib, "swap_marked_nontree", "Invert marks of %sSpecies%s NOT in Tree", "I", "sp_invert_mrk.hlp", makeWindowCallback(mark_nontree_cb, ntw, 2+affect));
    awm->close_sub_menu();

    free(label);
}

void NT_insert_mark_submenus(AW_window_menu_modes *awm, AWT_canvas *ntw, int insert_as_submenu) {
    if (insert_as_submenu) {
        awm->insert_sub_menu("Mark species", "M");
    }

    {
        awm->insert_menu_topic("count_marked", "Count Marked Species", "C", "sp_count_mrk.hlp", AWM_ALL, makeWindowCallback(count_mark_all_cb, ntw));
        awm->sep______________();
        insert_mark_topics(awm, AWM_ALL, ntw, 0, 0);
        awm->sep______________();

        awm->insert_sub_menu("Complete sequences", "o");
        insert_mark_topics(awm, AWM_EXP, ntw, 4, "complete");
        awm->close_sub_menu();

        awm->insert_sub_menu("Partial sequences", "P");
        insert_mark_topics(awm, AWM_EXP, ntw, 8, "partial");
        awm->close_sub_menu();

        awm->insert_sub_menu("Current Alignment", "A");
        insert_mark_topics(awm, AWM_EXP, ntw, 16, "-with data");
        awm->close_sub_menu();
    }

    if (insert_as_submenu) {
        awm->close_sub_menu();
    }
}

static void save_changed_tree(AWT_canvas *ntw) {
    GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main, 0, 0, 0);
    if (error) aw_message(error);
    ntw->zoom_reset_and_refresh();
}

// ----------------------------------------
//      Automated collapse/expand tree

static void group_and_save_tree(AWT_canvas *ntw, CollapseMode mode, int color_group) {
    GB_transaction ta(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->get_root_node(), mode, color_group);
    save_changed_tree(ntw);
}

void NT_group_tree_cb      (UNFIXED, AWT_canvas *ntw) { group_and_save_tree(ntw, COLLAPSE_ALL,      0); }
void NT_group_not_marked_cb(UNFIXED, AWT_canvas *ntw) { group_and_save_tree(ntw, EXPAND_MARKED,     0); }
void NT_group_terminal_cb  (UNFIXED, AWT_canvas *ntw) { group_and_save_tree(ntw, COLLAPSE_TERMINAL, 0); }
void NT_ungroup_all_cb     (UNFIXED, AWT_canvas *ntw) { group_and_save_tree(ntw, EXPAND_ALL,        0); }

static void NT_group_not_color_cb(UNFIXED, AWT_canvas *ntw, int colornum) { group_and_save_tree(ntw, EXPAND_COLOR, colornum); }

void NT_insert_color_collapse_submenu(AW_window_menu_modes *awm, AWT_canvas *ntree_canvas) {
#define MAXLABEL 30
#define MAXENTRY (AW_COLOR_GROUP_NAME_LEN+10)

    td_assert(ntree_canvas != 0);

    awm->insert_sub_menu("Group all except Color ...", "C");

    char        label_buf[MAXLABEL+1];
    char        entry_buf[MAXENTRY+1];
    char hotkey[]       = "x";
    const char *hotkeys = "N1234567890AB";

    for (int i = 0; i <= AW_COLOR_GROUPS; ++i) {
        sprintf(label_buf, "tree_group_not_color_%i", i);

        hotkey[0]                       = hotkeys[i];
        if (hotkey[0] == ' ') hotkey[0] = 0;

        if (i) {
            char *color_group_name = AW_get_color_group_name(awm->get_root(), i);
            sprintf(entry_buf, "%s group '%s'", hotkey, color_group_name);
            free(color_group_name);
        }
        else {
            strcpy(entry_buf, "No color group");
        }

        awm->insert_menu_topic(awm->local_id(label_buf), entry_buf, hotkey, "tgroupcolor.hlp", AWM_ALL, makeWindowCallback(NT_group_not_color_cb, ntree_canvas, i));
    }

    awm->close_sub_menu();

#undef MAXLABEL
#undef MAXENTRY
}

// ------------------------
//      tree sorting :

GB_ERROR NT_with_displayed_tree_do(AWT_canvas *ntw, bool (*displayed_tree_cb)(RootedTree *tree, GB_ERROR& error)) {
    // 'displayed_tree_cb' has to return true if tree was changed and needs to be saved

    GB_transaction ta(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);

    GB_ERROR error = NULL;
    if (displayed_tree_cb(AWT_TREE(ntw)->get_root_node(), error)) {
        save_changed_tree(ntw);
    }
    return error;
}

void NT_resort_tree_cb(UNFIXED, AWT_canvas *ntw, TreeOrder order) {
    GB_transaction ta(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->reorder_tree(order);
    save_changed_tree(ntw);
}

void NT_reset_lzoom_cb(UNFIXED, AWT_canvas *ntw) {
    GB_transaction ta(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->displayed_root = AWT_TREE(ntw)->get_root_node();
    ntw->zoom_reset_and_refresh();
}

void NT_reset_pzoom_cb(UNFIXED, AWT_canvas *ntw) {
    GB_transaction ta(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    ntw->zoom_reset_and_refresh();
}

void NT_set_tree_style(UNFIXED, AWT_canvas *ntw, AP_tree_display_type type) {
    GB_transaction ta(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->set_tree_type(type, ntw);
    ntw->zoom_reset_and_refresh();
}

void NT_remove_leafs(UNFIXED, AWT_canvas *ntw, AWT_RemoveType mode) {
    GB_transaction ta(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AP_tree *tree_root = AWT_TREE(ntw)->get_root_node();
    if (tree_root) {
        AWT_TREE(ntw)->tree_static->remove_leafs(mode);

        tree_root = AWT_TREE(ntw)->get_root_node(); // root might have changed -> get again
        if (tree_root) tree_root->compute_tree();
        save_changed_tree(ntw);
    }
    else {
        aw_message("Got no tree");
    }
}

void NT_remove_bootstrap(UNFIXED, AWT_canvas *ntw) { // delete all bootstrap values
    GB_transaction ta(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);

    AP_tree *tree_root = AWT_TREE(ntw)->get_root_node();
    if (tree_root) {
        tree_root->remove_bootstrap();
        tree_root->compute_tree();
        save_changed_tree(ntw);
    }
    else {
        aw_message("Got no tree");
    }
}
void NT_toggle_bootstrap100(UNFIXED, AWT_canvas *ntw) { // toggle 100% bootstrap values
    GB_transaction ta(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);

    AP_tree *tree_root = AWT_TREE(ntw)->get_root_node();
    if (tree_root) {
        tree_root->toggle_bootstrap100();
        tree_root->compute_tree();
        save_changed_tree(ntw);
    }
    else {
        aw_message("Got no tree");
    }
}

void NT_reset_branchlengths(UNFIXED, AWT_canvas *ntw) { // set all branchlengths to tree_defaults::LENGTH
    GB_transaction ta(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);

    AP_tree *tree_root = AWT_TREE(ntw)->get_root_node();
    if (tree_root) {
        tree_root->reset_branchlengths();
        tree_root->compute_tree();
        save_changed_tree(ntw);
    }
    else {
        aw_message("Got no tree");
    }
}

void NT_multifurcate_tree(AWT_canvas *ntw, const RootedTree::multifurc_limits& below) {
    GB_transaction ta(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);

    RootedTree *tree = AWT_TREE(ntw)->get_root_node();
    if (tree) {
        tree->multifurcate_whole_tree(below);
        save_changed_tree(ntw);
    }
    else {
        aw_message("Got no tree");
    }
}

void NT_move_boot_branch(UNFIXED, AWT_canvas *ntw, int direction) { // copy branchlengths to bootstraps (or vice versa)
    GB_transaction ta(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);

    AP_tree *tree_root = AWT_TREE(ntw)->get_root_node();
    if (tree_root) {
        if (direction == 0) tree_root->bootstrap2branchlen();
        else                tree_root->branchlen2bootstrap();

        tree_root->compute_tree();
        save_changed_tree(ntw);

        char *adviceText = GBS_global_string_copy("Please note, that you just overwrote your existing %s.",
                                                  direction ? "bootstrap values" : "branchlengths");
        AW_advice(adviceText, AW_ADVICE_TOGGLE_AND_HELP, 0, "tbl_boot2len.hlp");
        free(adviceText);
    }
    else {
        aw_message("Got no tree");
    }
}

void NT_scale_tree(UNFIXED, AWT_canvas *ntw) { // scale branchlengths
    char *answer = aw_input("Enter scale factor", "Scale branchlengths by factor:", "100");
    if (answer) {
        double factor = atof(answer);
        GB_transaction ta(ntw->gb_main);

        AP_tree *tree_root = AWT_TREE(ntw)->get_root_node();
        if (tree_root) {
            tree_root->scale_branchlengths(factor);
            tree_root->compute_tree();
            save_changed_tree(ntw);
        }
        else {
            aw_message("Got no tree");
        }
        free(answer);
    }
}

inline AP_tree *common_ancestor(AP_tree *t1, AP_tree *t2) {
    return DOWNCAST(AP_tree*, t1->ancestor_common_with(t2));
}

static bool make_node_visible(AWT_canvas *ntw, AP_tree *node) {
    bool changed = false;
    while (node) {
        if (node->gr.grouped) {
            node->gr.grouped = 0;
            changed          = true;
        }
        node = node->get_father();
    }
    if (changed) {
        AWT_TREE(ntw)->get_root_node()->compute_tree();
        GB_ERROR error = AWT_TREE(ntw)->save(ntw->gb_main, 0, 0, 0);
        if (error) {
            aw_message(error);
            return false;
        }
        ntw->zoom_reset();
    }
    return true;
}

void NT_jump_cb(UNFIXED, AWT_canvas *ntw, AP_tree_jump_type jumpType) {
    AW_window        *aww   = ntw->aww;
    AWT_graphic_tree *gtree = AWT_TREE(ntw);
    
    if (!gtree) return;
    if (!gtree->displayed_root) return;

    GB_transaction ta(ntw->gb_main);
    gtree->check_update(ntw->gb_main);

    const char *name     = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_char_pntr();
    char       *msg      = NULL;
    bool        verboose = jumpType & AP_JUMP_BE_VERBOOSE;

    if (name[0]) {
        AP_tree *found   = NULL;
        bool     is_tree = sort_is_tree_style(gtree->tree_sort);

        if (is_tree) {
            found = gtree->displayed_root->findLeafNamed(name);
            if (!found && gtree->is_logically_zoomed()) {
                found = gtree->get_root_node()->findLeafNamed(name);
                if (found) { // species is invisible because it is outside logically zoomed tree
                    if (jumpType & AP_JUMP_UNFOLD_GROUPS) {
                        gtree->displayed_root = common_ancestor(found, gtree->displayed_root);
                        ntw->zoom_reset();
                    }
                    else {
                        if (verboose) msg = GBS_global_string_copy("Species '%s' is outside logical zoomed subtree", name);

                        found = NULL;
                    }
                }
            }

            if (found) {
                if (jumpType&AP_JUMP_UNFOLD_GROUPS) {
                    if (!make_node_visible(ntw, found)) found = NULL;
                }
                else if (found->is_inside_folded_group()) {
                    found = NULL;
                }
            }
        }

        if (found || !is_tree) {
            bool repeat   = gtree->tree_sort == AP_TREE_IRS;
            bool do_jump  = true;
            while (do_jump) {
                do_jump = false;

                AW_device_size *device = aww->get_size_device(AW_MIDDLE_AREA);
                device->set_filter(AW_SIZE|AW_SIZE_UNSCALED);
                device->reset();
                ntw->init_device(device);
                ntw->gfx->show(device);

                const AW_screen_area& screen = device->get_area_size();

                const Position& cursor = gtree->get_cursor();
                if (are_distinct(Origin, cursor)) {
                    Position S = device->transform(cursor);

                    int scroll_x = 0;
                    int scroll_y = 0;

                    if (S.xpos()<0.0) scroll_x      = (int)(S.xpos() - screen.r * .1);
                    if (S.xpos()>screen.r) scroll_x = (int)(S.xpos() - screen.r * .5);

                    if (gtree->tree_sort == AP_TREE_IRS) {
                        // always scroll IRS tree
                        // position a bit below vertical center
                        scroll_y = (int) (S.ypos() - screen.b * .6);
                    }
                    else if (S.ypos()<0.0 || S.ypos()>screen.b) {
                        scroll_y = (int) (S.ypos() - screen.b * .5);
                    }

                    if (jumpType & AP_JUMP_CENTER_IF_VISIBLE) {
                        // additional JUMP button clicks -> center vertically, then horizontally
                        if (!scroll_x && !scroll_y) {
                            scroll_y                = (int) (S.ypos() - screen.b * .5);
                            if (!scroll_y) scroll_x = (int) (S.xpos() - screen.r * (is_tree ? .5 : .02));
                        }
                    }

                    if (scroll_x || scroll_y) ntw->scroll(scroll_x, scroll_y);
                    if (repeat) {
                        // reposition jump in IRS tree (reduces jump failure rate)
                        repeat  = false;
                        do_jump = true;
                    }
                }
                else {
                    td_assert(!is_tree);
                    if (verboose) msg = GBS_global_string_copy("Species '%s' is no member of this list", name);
                }
            }
        }

        if (!found && is_tree && verboose && !msg) {
            msg = GBS_global_string_copy("Species '%s' is no member of this %s", name, gtree->tree_sort == AP_LIST_NDS ? "list" : "tree");
        }
    }
    else if (verboose) {
        msg = strdup("No species selected");
    }

    ntw->refresh(); // always do refresh to show change of selected species

    if (msg) {
        td_assert(verboose);
        aw_message(msg);
        free(msg);
    }
}

void TREE_auto_jump_cb(UNFIXED, AWT_canvas *ntw) {   // jump only if auto jump is set
    if (ntw->aww->get_root()->awar(AWAR_DTREE_AUTO_JUMP)->read_int()) {
        NT_jump_cb(NULL, ntw, AP_JUMP_AUTO);
    }
}

inline const char *plural(int val) {
    return "s"+(val == 1);
}

void NT_reload_tree_event(AW_root *awr, AWT_canvas *ntw, bool expose) {
    GB_push_transaction(ntw->gb_main);
    char     *tree_name = awr->awar(ntw->user_awar)->read_string();
    GB_ERROR  error     = ntw->gfx->load(ntw->gb_main, tree_name, 0, 0);
    if (error) {
        aw_message(error);
    }
    else {
        int zombies, duplicates;
        DOWNCAST(AWT_graphic_tree*, ntw->gfx)->get_zombies_and_duplicates(zombies, duplicates);

        if (zombies || duplicates) {
            const char *msg = 0;
            if (duplicates) {
                if (zombies) msg = GBS_global_string("%i zombie%s and %i duplicate%s", zombies, plural(zombies), duplicates, plural(duplicates));
                else msg         = GBS_global_string("%i duplicate%s", duplicates, plural(duplicates));
            }
            else {
                td_assert(zombies);
                msg = GBS_global_string("%i zombie%s", zombies, plural(zombies));
            }
            aw_message(GBS_global_string("%s in '%s'", msg, tree_name));
        }
    }
    free(tree_name);
    if (expose) {
        ntw->zoom_reset();
        AWT_expose_cb(NULL, ntw);
    }
    GB_pop_transaction(ntw->gb_main);
}

void TREE_recompute_cb(UNFIXED, AWT_canvas *ntw) {
    AWT_graphic_tree *gt = DOWNCAST(AWT_graphic_tree*, ntw->gfx);
    gt->get_root_node()->compute_tree();
    AWT_expose_cb(NULL, ntw);
}

void NT_reinit_treetype(UNFIXED, AWT_canvas *ntw) {
    AWT_graphic_tree *gt = DOWNCAST(AWT_graphic_tree*, ntw->gfx);
    gt->set_tree_type(gt->tree_sort, ntw);
    AWT_resize_cb(NULL, ntw);
}

void NT_remove_species_in_tree_from_hash(AP_tree *tree, GB_HASH *hash) {
    if (!tree) return;
    if (tree->is_leaf && tree->name) {
        GBS_write_hash(hash, tree->name, 0); // delete species in hash table
    }
    else {
        NT_remove_species_in_tree_from_hash(tree->get_leftson(), hash);
        NT_remove_species_in_tree_from_hash(tree->get_rightson(), hash);
    }
}

