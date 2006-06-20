#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt_canvas.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt_tree_cb.hxx>
#include <awt_advice.hxx>
#include <awt.hxx>


void
nt_mode_event( AW_window *aws, AWT_canvas *ntw, AWT_COMMAND_MODE mode)
{
    AWUSE(aws);
    const char *text;

    switch(mode){
        case AWT_MODE_SELECT:
            text = "SELECT MODE    LEFT: select species and open/close group";
            break;
        case AWT_MODE_MARK:
            text = "MARK MODE    LEFT: mark subtree   RIGHT: unmark subtree";
            break;
        case AWT_MODE_GROUP:
            text = "GROUP MODE   LEFT: fold/unfold group  RIGHT: create/rename/destroy group";
            break;
        case AWT_MODE_ZOOM:
            text = "ZOOM MODE    LEFT: press and drag to zoom   RIGHT: zoom out one step";
            break;
        case AWT_MODE_LZOOM:
            text = "LOGICAL ZOOM MODE   LEFT: show subtree  RIGHT: go up one step";
            break;
        case AWT_MODE_MOD:
            text = "MODIFY MODE   LEFT: select node M: assign info to internal node";
            break;
        case AWT_MODE_WWW:
            text = "CLICK NODE TO SEARCH WEB (See <PROPS/WWW...> also";
            break;
        case AWT_MODE_LINE:
            text = "LINE MODE    LEFT: reduce linewidth  RIGHT: increase linewidth";
            break;
        case AWT_MODE_ROT:
            text = "ROTATE MODE   LEFT: Select branch and drag to rotate";
            break;
        case AWT_MODE_SPREAD:
            text = "SPREAD MODE   LEFT: decrease angles  RIGHT: increase angles";
            break;
        case AWT_MODE_SWAP:
            text = "SWAP MODE    LEFT: swap branches";
            break;
        case AWT_MODE_LENGTH:
            text = "BRANCH LENGTH MODE   LEFT: Drag branch/ruler to change length  RIGHT: use discrete lengths";
            break;
        case AWT_MODE_MOVE:
            text = "MOVE MODE   LEFT: move subtree/ruler to new destination RIGHT: move group info only";
            break;
        case AWT_MODE_NNI:
            text = "NEAREST NEIGHBOUR INTERCHANGE OPTIMIZER  L: select subtree R: whole tree";
            break;
        case AWT_MODE_KERNINGHAN:
            text = "KERNIGHAN LIN OPTIMIZER   L: select subtree R: whole tree";
            break;
        case AWT_MODE_OPTIMIZE:
            text = "NNI & KL OPTIMIZER   L: select subtree R: whole tree";
            break;
        case AWT_MODE_SETROOT:
            text = "SET ROOT MODE   LEFT: set root";
            break;
        case AWT_MODE_RESET:
            text = "RESET MODE   LEFT: reset rotation  MIDDLE: reset angles  RIGHT: reset linewidth";
            break;

        default:
            text="No help for this mode available";
            break;
    }

    awt_assert(strlen(text) < AWAR_FOOTER_MAX_LEN); // text too long!

    ntw->awr->awar(AWAR_FOOTER)->write_string( text);
    ntw->set_mode(mode);
}

// ---------------------------------------
//      Basic mark/unmark callbacks :
// ---------------------------------------

void NT_count_mark_all_cb(void *dummy, AW_CL cl_ntw)
{
    AWUSE(dummy);
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    GB_push_transaction(ntw->gb_main);

    GBDATA *gb_species_data = GB_search(ntw->gb_main,"species_data",GB_CREATE_CONTAINER);
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

static int nt_species_has_alignment(GBDATA *gb_species, void *cd_use) {
    return GBT_read_sequence(gb_species, (const char*)cd_use) != 0;
}

static int nt_sequence_is_partial(GBDATA *gb_species, void *cd_partial) {
    int wanted  = (int)cd_partial;
    awt_assert(wanted == 0 || wanted == 1);
    int partial = GBT_is_partial(gb_species, 1-wanted, 0);

    return partial == wanted;
}

#define MARK_MODE_LOWER_BITS (1|2)
#define MARK_MODE_UPPER_BITS (4|8|16)

void NT_mark_all_cb(AW_window *, AW_CL cl_ntw, AW_CL cl_mark_mode)
    // Bits 0 and 1 of mark_mode:
    //
    // mark_mode&3 == 0 -> unmark
    // mark_mode&3 == 1 -> mark
    // mark_mode&3 == 2 -> toggle mark
    //
    // Bits 2 .. 4 of mark_mode:
    //
    // mark_mode&12 == 4 -> affect only full sequences
    // mark_mode&12 == 8 -> affect only partial sequences
    // mark_mode&12 == 16 -> affect only species with data in current alignment
    // else -> affect all sequences
{
    AWT_canvas *ntw       = (AWT_canvas*)cl_ntw;
    int         mark_mode = (int)cl_mark_mode;

    GB_transaction gb_dummy(ntw->gb_main);

    switch (mark_mode&MARK_MODE_UPPER_BITS) {
        case 0:                 // all sequences
            GBT_mark_all(ntw->gb_main,mark_mode&MARK_MODE_LOWER_BITS);
            break;
        case 4:                 // full sequences only
            GBT_mark_all_that(ntw->gb_main,mark_mode&MARK_MODE_LOWER_BITS, nt_sequence_is_partial, (void*)0);
            break;
        case 8:                 // partial sequences only
            GBT_mark_all_that(ntw->gb_main,mark_mode&MARK_MODE_LOWER_BITS, nt_sequence_is_partial, (void*)1);
            break;
        case 16: {               // species with data in alignment only
            char *ali = GBT_get_default_alignment(ntw->gb_main);
            if (ali) GBT_mark_all_that(ntw->gb_main, mark_mode&MARK_MODE_LOWER_BITS, nt_species_has_alignment, (void*)ali);
            free(ali);
            break;
        }
        default :
            awt_assert(0); // illegal mode
            break;
    }

    ntw->refresh();
}

void NT_mark_tree_cb(AW_window *, AW_CL cl_ntw, AW_CL cl_mark_mode)
{
    AWT_canvas       *ntw       = (AWT_canvas*)cl_ntw;
    int               mark_mode = (int)cl_mark_mode;
    AWT_graphic_tree *gtree     = AWT_TREE(ntw);
    GB_transaction    gb_dummy(ntw->gb_main);

    gtree->check_update(ntw->gb_main);
    switch (mark_mode&MARK_MODE_UPPER_BITS) {
        case 0:                 // all sequences
            gtree->mark_species_in_tree(gtree->tree_root, mark_mode&MARK_MODE_LOWER_BITS);
            break;
        case 4:                 // full sequences only
            gtree->mark_species_in_tree_that(gtree->tree_root, mark_mode&MARK_MODE_LOWER_BITS, nt_sequence_is_partial, (void*)0);
            break;
        case 8:                 // partial sequences only
            gtree->mark_species_in_tree_that(gtree->tree_root, mark_mode&MARK_MODE_LOWER_BITS, nt_sequence_is_partial, (void*)1);
            break;
        case 16: {               // species with data in alignment only
            char *ali = GBT_get_default_alignment(ntw->gb_main);
            if (ali) gtree->mark_species_in_tree_that(gtree->tree_root, mark_mode&MARK_MODE_LOWER_BITS, nt_species_has_alignment, (void*)ali);
            free(ali);
            break;
        }
        default :
            awt_assert(0); // illegal mode
            break;
    }
    ntw->refresh();
}

struct mark_nontree_cb_data {
    int      mark_mode_upper_bits;
    char    *ali;               // current alignment (only if mark_mode_upper_bits == 16)
    GB_HASH *hash;
};

static int mark_nontree_cb(GBDATA *gb_species, void *cb_data) {
    struct mark_nontree_cb_data *data    = (mark_nontree_cb_data*)cb_data;
    const char                  *name    = GBT_read_name(gb_species);
    bool                         mark_me = false;

    if (GBS_read_hash(data->hash, name) == (long)gb_species) { // species is not in tree!
        switch (data->mark_mode_upper_bits) {
            case 0:             // all sequences
                mark_me = true;
                break;
            case 4:             // full sequences only
                mark_me = nt_sequence_is_partial(gb_species, (void*)0);
                break;
            case 8:             // partial sequences only
                mark_me = nt_sequence_is_partial(gb_species, (void*)1);
                break;
            case 16:            // species with data in alignment only
                mark_me = nt_species_has_alignment(gb_species, data->ali);
                break;
            default :
                awt_assert(0); // illegal mode
                break;
        }
    }

    return mark_me;
}

void NT_mark_nontree_cb(AW_window *, AW_CL cl_ntw, AW_CL cl_mark_mode)
{
    AWT_canvas                  *ntw       = (AWT_canvas*)cl_ntw;
    int                          mark_mode = (int)cl_mark_mode;
    AWT_graphic_tree            *gtree     = AWT_TREE(ntw);
    GB_transaction               gb_dummy(ntw->gb_main);
    struct mark_nontree_cb_data  cd;

    if ((mark_mode&MARK_MODE_LOWER_BITS) == 0) {   // unmark is much faster
        cd.hash = GBT_generate_marked_species_hash(ntw->gb_main); // because it only hashes marked species
    }
    else {
        cd.hash = GBT_generate_species_hash(ntw->gb_main, 0); // otherwise we have to hash ALL species
    }

    NT_remove_species_in_tree_from_hash(gtree->tree_root, cd.hash);

    cd.mark_mode_upper_bits = mark_mode&MARK_MODE_UPPER_BITS;
    cd.ali                  = cd.mark_mode_upper_bits == 16 ? GBT_get_default_alignment(ntw->gb_main) : 0;

    GBT_mark_all_that(ntw->gb_main, mark_mode&MARK_MODE_LOWER_BITS, mark_nontree_cb, (void*)&cd);

    free(cd.ali);

    ntw->refresh();
}

void NT_mark_color_cb(AW_window *, AW_CL cl_ntw, AW_CL cl_mark_mode)
{
    AWT_canvas *ntw       = (AWT_canvas*)cl_ntw;
    int         mark_mode = (int)cl_mark_mode;

    GB_transaction gb_dummy(ntw->gb_main);

    int color_group = mark_mode>>4;
    awt_assert(mark_mode&(4|8)); // either 4 or 8 has to be set
    bool mark_matching = (mark_mode&4) == 4;
    mark_mode    = mark_mode&3;

    for (GBDATA *gb_species = GBT_first_species(ntw->gb_main); gb_species; gb_species = GBT_next_species(gb_species)) {
        int my_color_group = AW_find_color_group(gb_species, AW_TRUE);

        if (mark_matching == (color_group == my_color_group)) {
            switch (mark_mode) {
                case 0: GB_write_flag(gb_species, 0); break;
                case 1: GB_write_flag(gb_species, 1); break;
                case 2: GB_write_flag(gb_species, !GB_read_flag(gb_species)); break;
                default : awt_assert(0); break;
            }
        }
    }

    ntw->refresh();
}


void NT_insert_color_mark_submenu(AW_window_menu_modes *awm, AWT_canvas *ntree_canvas, const char *menuname, int mark_basemode) {
#define MAXLABEL 40
#define MAXENTRY 20
    awm->insert_sub_menu(0, menuname, "");

    char        label_buf[MAXLABEL+1];
    char        entry_buf[MAXENTRY+1];
    char hotkey[]       = "x";
    const char *hotkeys = "N1234567890  ";

    const char *label_base = 0;
    switch (mark_basemode) {
        case 0: label_base = "all_unmark_color"; break;
        case 1: label_base = "all_mark_color"; break;
        case 2: label_base = "all_invert_mark_color"; break;
        default : awt_assert(0); break;
    }

    for (int all_but = 0; all_but <= 1; ++all_but) {
        const char *entry_prefix;
        if (all_but) entry_prefix = "all but";
        else        entry_prefix  = "all of";

        for (int i = 0; i <= AW_COLOR_GROUPS; ++i) {
            sprintf(label_buf, "%s_%i", label_base, i);

            if (i) {
                char *color_group_name = AW_get_color_group_name(awm->get_root(), i);
                sprintf(entry_buf, "%s '%s'", entry_prefix, color_group_name);
                free(color_group_name);
            }
            else {
                sprintf(entry_buf, "%s no color group", entry_prefix);
            }

            hotkey[0] = hotkeys[i];
            if (hotkey[0] == ' ' || all_but) hotkey[0] = 0;

            awm->insert_menu_topic(label_buf, entry_buf, hotkey, "markcolor.hlp", AWM_ALL,
                                   NT_mark_color_cb,
                                   (AW_CL)ntree_canvas,
                                   (AW_CL)mark_basemode|((all_but == 0) ? 4 : 8)|(i*16));
        }
        if (!all_but) awm->insert_separator();
    }

    awm->close_sub_menu();
#undef MAXLABEL
#undef MAXENTRY
}

static void nt_insert_mark_topic(AW_window_menu_modes *awm, const char *attrib, const char *label_suffix, const char *entry_template,
                                 const char *hotkey, const char *helpfile,
                                 AW_CB cb, AW_CL cl1, AW_CL cl2)
{
    const char *label     = 0;
    char       *label_tmp = 0;
    char       *entry     = 0;

    if (attrib) {
        bool append = attrib[0] == '-'; // if attrib starts with '-' then append (otherwise prepend)
        if (append) ++attrib; // skip '-'

        label_tmp = GBS_global_string_copy("%s_%s", attrib, label_suffix);
        label     = label_tmp;

        if (append) {
            char *spaced_attrib = GBS_global_string_copy(" %s", attrib);
            entry               = GBS_global_string_copy(entry_template, "", spaced_attrib);
            free(spaced_attrib);
        }
        else {
            char *spaced_attrib = GBS_global_string_copy("%s ", attrib);
            entry               = GBS_global_string_copy(entry_template, spaced_attrib, "");
            free(spaced_attrib);
        }
    }
    else {
        label = label_suffix;
        entry = GBS_global_string_copy(entry_template, "", "");
    }

    awm->insert_menu_topic(label, entry, hotkey, helpfile, AWM_ALL, cb, cl1, cl2);

    free(label_tmp);
    free(entry);
}

static void nt_insert_mark_topics(AW_window_menu_modes *awm, AWT_canvas *ntw, int affect, const char *attrib)
{
    awt_assert(affect == (affect&MARK_MODE_UPPER_BITS)); // only bits 2 .. 4 are allowed

    nt_insert_mark_topic(awm, attrib, "mark_all",            "Mark all %sSpecies%s",                    "M", "sp_mrk_all.hlp",    (AW_CB)NT_mark_all_cb,     (AW_CL)ntw, (AW_CL)(1+affect));
    nt_insert_mark_topic(awm, attrib, "unmark_all",          "Unmark all %sSpecies%s",                  "U", "sp_umrk_all.hlp",   (AW_CB)NT_mark_all_cb,     (AW_CL)ntw, (AW_CL)(0+affect));
    nt_insert_mark_topic(awm, attrib, "swap_marked",         "Invert marks of all %sSpecies%s",         "v", "sp_invert_mrk.hlp", (AW_CB)NT_mark_all_cb,     (AW_CL)ntw, (AW_CL)(2+affect));
    awm->insert_separator();
    nt_insert_mark_topic(awm, attrib, "mark_tree",           "Mark %sSpecies%s in Tree",                "T", "sp_mrk_tree.hlp",   (AW_CB)NT_mark_tree_cb,    (AW_CL)ntw, (AW_CL)(1+affect));
    nt_insert_mark_topic(awm, attrib, "unmark_tree",         "Unmark %sSpecies%s in Tree",              "n", "sp_umrk_tree.hlp",  (AW_CB)NT_mark_tree_cb,    (AW_CL)ntw, (AW_CL)(0+affect));
    nt_insert_mark_topic(awm, attrib, "swap_marked_tree",    "Invert marks of %sSpecies%s in Tree",     "",  "sp_invert_mrk.hlp", (AW_CB)NT_mark_tree_cb,    (AW_CL)ntw, (AW_CL)(2+affect));
    awm->insert_separator();
    nt_insert_mark_topic(awm, attrib, "mark_nontree",        "Mark %sSpecies%s NOT in Tree",            "",  "sp_mrk_tree.hlp",   (AW_CB)NT_mark_nontree_cb, (AW_CL)ntw, (AW_CL)(1+affect));
    nt_insert_mark_topic(awm, attrib, "unmark_nontree",      "Unmark %sSpecies%s NOT in Tree",          "",  "sp_umrk_tree.hlp",  (AW_CB)NT_mark_nontree_cb, (AW_CL)ntw, (AW_CL)(0+affect));
    nt_insert_mark_topic(awm, attrib, "swap_marked_nontree", "Invert marks of %sSpecies%s NOT in Tree", "",  "sp_invert_mrk.hlp", (AW_CB)NT_mark_nontree_cb, (AW_CL)ntw, (AW_CL)(2+affect));
}

void NT_insert_mark_submenus(AW_window_menu_modes *awm, AWT_canvas *ntw, int insert_as_submenu) {
    if (insert_as_submenu) {
        awm->insert_sub_menu(0, "Mark species", "M");
    }

    {
        awm->insert_menu_topic("count_marked",  "Count Marked Species",     "C","sp_count_mrk.hlp", AWM_ALL, (AW_CB)NT_count_mark_all_cb,       (AW_CL)ntw, 0 );
        awm->insert_separator();
        nt_insert_mark_topics(awm, ntw, 0, 0);
        awm->insert_separator();

        awm->insert_sub_menu(0, "Full sequences", "F");
        nt_insert_mark_topics(awm, ntw, 4, "full");
        awm->close_sub_menu();

        awm->insert_sub_menu(0, "Partial sequences", "P");
        nt_insert_mark_topics(awm, ntw, 8, "partial");
        awm->close_sub_menu();

        awm->insert_sub_menu(0, "Current Alignment", "A");
        nt_insert_mark_topics(awm, ntw, 16, "-with data");
        awm->close_sub_menu();
    }

    if (insert_as_submenu) {
        awm->close_sub_menu();
    }
}

// ---------------------------------------
//      Automated collapse/expand tree
// ---------------------------------------

void NT_group_tree_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 0, 0);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_group_not_marked_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 1, 0);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}
void NT_group_terminal_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 2, 0);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_ungroup_all_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 4, 0);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_group_not_color_cb(AW_window *, AW_CL cl_ntw, AW_CL cl_colornum) {
    AWT_canvas     *ntw      = (AWT_canvas*)cl_ntw;
    int             colornum = (int)cl_colornum;
    GB_transaction  gb_dummy(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 8, colornum);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}

// ----------------------------------------------------------------------------------------------------
//      void NT_insert_color_collapse_submenu(AW_window_menu_modes *awm, AWT_canvas *ntree_canvas)
// ----------------------------------------------------------------------------------------------------
void NT_insert_color_collapse_submenu(AW_window_menu_modes *awm, AWT_canvas *ntree_canvas) {
#define MAXLABEL 30
#define MAXENTRY (AW_COLOR_GROUP_NAME_LEN+9)

    awt_assert(ntree_canvas != 0);

    awm->insert_sub_menu(0, "Group all except Color ...", "C");

    char        label_buf[MAXLABEL+1];
    char        entry_buf[MAXENTRY+1];
    char hotkey[]       = "x";
    const char *hotkeys = "N1234567890  ";

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

        awm->insert_menu_topic(label_buf, entry_buf, hotkey, "tgroupcolor.hlp", AWM_ALL, NT_group_not_color_cb, (AW_CL)ntree_canvas, (AW_CL)i);
    }

    awm->close_sub_menu();

#undef MAXLABEL
#undef MAXENTRY
}

// ------------------------
//      tree sorting :
// ------------------------

void NT_resort_tree_cb(void *dummy, AWT_canvas *ntw,int type)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    int stype;
    switch(type){
        case 0: stype = 0; break;
        case 1: stype = 2; break;
        default:stype = 1; break;
    }
    AWT_TREE(ntw)->resort_tree(stype);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_reset_lzoom_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->tree_root_display = AWT_TREE(ntw)->tree_root;
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_reset_pzoom_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_set_tree_style(void *dummy, AWT_canvas *ntw, AP_tree_sort type)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->set_tree_type(type);
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_remove_leafs(void *dummy, AWT_canvas *ntw, long mode) // if dummy == 0 -> no message
{
    AWUSE(dummy);
    {
        GB_transaction gb_dummy(ntw->gb_main);
        AWT_TREE(ntw)->check_update(ntw->gb_main);

        GB_ERROR error = AWT_TREE(ntw)->tree_root->remove_leafs(ntw->gb_main, (int)mode);
        if (error) aw_message(error);
        if (AWT_TREE(ntw)->tree_root) AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    }
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_remove_bootstrap(AW_window*, AW_CL cl_ntw, AW_CL) // delete all bootstrap values
{
    AWT_canvas     *ntw = (AWT_canvas*)cl_ntw;
    GB_transaction  gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);

    AP_tree *tree_root = AWT_TREE(ntw)->tree_root;
    if (tree_root) {
        tree_root->remove_bootstrap(ntw->gb_main);
        tree_root->compute_tree(ntw->gb_main);
        AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    }
    ntw->zoom_reset();
    ntw->refresh();
}
void NT_reset_branchlengths(AW_window*, AW_CL cl_ntw, AW_CL) // set all branchlengths to 0.1
{
    AWT_canvas     *ntw = (AWT_canvas*)cl_ntw;
    GB_transaction  gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);

    AP_tree *tree_root = AWT_TREE(ntw)->tree_root;
    if (tree_root){
        tree_root->reset_branchlengths(ntw->gb_main);
        tree_root->compute_tree(ntw->gb_main);
        AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    }
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_move_boot_branch(AW_window*, AW_CL cl_ntw, AW_CL cl_direction) // copy branchlengths to bootstraps (or vice versa)
{
    AWT_canvas     *ntw       = (AWT_canvas*)cl_ntw;
    int             direction = (int)cl_direction;
    GB_transaction  gb_dummy(ntw->gb_main);

    AWT_TREE(ntw)->check_update(ntw->gb_main);

    AP_tree *tree_root = AWT_TREE(ntw)->tree_root;
    if (tree_root){
        if (direction == 0) tree_root->bootstrap2branchlen(ntw->gb_main);
        else                tree_root->branchlen2bootstrap(ntw->gb_main);

        tree_root->compute_tree(ntw->gb_main);
        AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);

        char *adviceText = GBS_global_string_copy("Please note, that you just overwrote your existing %s.",
                                                  direction ? "bootstrap values" : "branchlengths");
        AWT_advice(adviceText, AWT_ADVICE_TOGGLE|AWT_ADVICE_HELP, 0, "tbl_boot2len.hlp");
        free(adviceText);
    }
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_scale_tree(AW_window*, AW_CL cl_ntw, AW_CL) // scale branchlengths
{
    char *answer = aw_input("Enter scale factor", "Scale branchlengths by factor:", 0, "100");
    if (answer) {
        AWT_canvas *ntw    = (AWT_canvas*)cl_ntw;
        double      factor = atof(answer);
        GB_transaction ta(ntw->gb_main);

        AP_tree *tree_root = AWT_TREE(ntw)->tree_root;
        if (tree_root){
            tree_root->scale_branchlengths(ntw->gb_main, factor);
            tree_root->compute_tree(ntw->gb_main);
            AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
        }
        ntw->zoom_reset();
        ntw->refresh();

        free(answer);
    }
}

void NT_jump_cb(AW_window *dummy, AWT_canvas *ntw, AW_CL auto_expand_groups)
{
    AWUSE(dummy);
    AW_window *aww = ntw->aww;
    if (!AWT_TREE(ntw)) return;
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    char *name = aww->get_root()->awar(AWAR_SPECIES_NAME)->read_string();
    if (name[0]){
        AP_tree * found = AWT_TREE(ntw)->search(AWT_TREE(ntw)->tree_root_display,name);
        if (!found && AWT_TREE(ntw)->tree_root_display != AWT_TREE(ntw)->tree_root){
            found = AWT_TREE(ntw)->search(AWT_TREE(ntw)->tree_root,name);
            if (found) {
                // now i found a species outside logical zoomed tree
                aw_message("Species found outside displayed subtree: zoom reset done");
                AWT_TREE(ntw)->tree_root_display = AWT_TREE(ntw)->tree_root;
                ntw->zoom_reset();
            }
        }
        switch (AWT_TREE(ntw)->tree_sort) {
            case AP_TREE_IRS:
            case AP_TREE_NORMAL:{
                if (auto_expand_groups) {
                    AW_BOOL         changed = AW_FALSE;
                    while (found) {
                        if (found->gr.grouped) {
                            found->gr.grouped = 0;
                            changed = AW_TRUE;
                        }
                        found = found->father;
                    }
                    if (changed) {
                        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
                        AWT_TREE(ntw)->save(ntw->gb_main, 0, 0, 0);
                        ntw->zoom_reset();
                    }
                }
                AW_device      *device = aww->get_size_device(AW_MIDDLE_AREA);
                device->set_filter(AW_SIZE);
                device->reset();
                ntw->init_device(device);
                ntw->tree_disp->show(device);
                AW_rectangle    screen;
                device->get_area_size(&screen);
                AW_pos          ys = AWT_TREE(ntw)->y_cursor;
                AW_pos          xs = 0;
                if (AWT_TREE(ntw)->x_cursor != 0.0 || ys != 0.0) {
                    AW_pos          x, y;
                    device->transform(xs, ys, x, y);
                    if (y < 0.0) {
                        ntw->scroll(aww, 0, (int) (y - screen.b * .5));
                    } else if (y > screen.b) {
                        ntw->scroll(aww, 0, (int) (y - screen.b * .5));
                    }
                }else{
                    if (auto_expand_groups){
                        aw_message(GBS_global_string("Sorry, I didn't find the species '%s' in this tree", name));
                    }
                }
                ntw->refresh();
                break;
            }
            case AP_LIST_NDS:
                {
                    AW_device      *device = aww->get_size_device(AW_MIDDLE_AREA);
                    device->set_filter(AW_SIZE);
                    device->reset();
                    ntw->init_device(device);
                    ntw->tree_disp->show(device);
                    AW_rectangle    screen;
                    device->get_area_size(&screen);
                    AW_pos          ys = AWT_TREE(ntw)->y_cursor;
                    AW_pos          xs = 0;
                    if (AWT_TREE(ntw)->x_cursor != 0.0 || ys != 0.0) {
                        AW_pos          x, y;
                        device->transform(xs, ys, x, y);
                        if (y < 0.0) {
                            ntw->scroll(aww, 0, (int) (y - screen.b * .5));
                        } else if (y > screen.b) {
                            ntw->scroll(aww, 0, (int) (y - screen.b * .5));
                        }
                    }else{
                        if (auto_expand_groups){
                            aw_message(GBS_global_string("Sorry, your species '%s' is not marked and therefore not in this list", name));
                        }
                    }
                    ntw->refresh();
                    break;
                }
            case AP_TREE_RADIAL:{
                AWT_TREE(ntw)->tree_root_display = 0;
                AWT_TREE(ntw)->jump(AWT_TREE(ntw)->tree_root, name);
                if (!AWT_TREE(ntw)->tree_root_display) {
                    aw_message(GBS_global_string("Sorry, I didn't find the species '%s' in this tree", name));
                    AWT_TREE(ntw)->tree_root_display = AWT_TREE(ntw)->tree_root;
                }
                ntw->zoom_reset();
                ntw->refresh();
                break;
            }
            default : awt_assert(0); break;
        }
    }
    free(name);
}

void NT_jump_cb_auto(AW_window *dummy, AWT_canvas *ntw){    // jump only if auto jump is set
    if (AWT_TREE(ntw)->tree_sort == AP_TREE_NORMAL || AWT_TREE(ntw)->tree_sort == AP_LIST_NDS) {
        if (ntw->aww->get_root()->awar(AWAR_DTREE_AUTO_JUMP)->read_int()) {
            NT_jump_cb(dummy,ntw,0);
            return;
        }
    }
    ntw->refresh();
}

inline const char *plural(int val) {
    return "s"+(val == 1);
}

void NT_reload_tree_event(AW_root *awr, AWT_canvas *ntw, GB_BOOL set_delete_cbs)
{
    GB_push_transaction(ntw->gb_main);
    char *tree_name = awr->awar(ntw->user_awar)->read_string();

    GB_ERROR error = ntw->tree_disp->load(ntw->gb_main, tree_name,1,set_delete_cbs);    // linked
    if (error) {
        aw_message(error);
    }
    else {
        int zombies, duplicates;
        ((AWT_graphic_tree*)ntw->tree_disp)->get_zombies_and_duplicates(zombies, duplicates);

        if (zombies || duplicates) {
            const char *msg = 0;
            if (duplicates) {
                if (zombies) msg = GBS_global_string("%i zombie%s and %i duplicate%s", zombies, plural(zombies), duplicates, plural(duplicates));
                else msg         = GBS_global_string("%i duplicate%s", duplicates, plural(duplicates));
            }
            else {
                awt_assert(zombies);
                msg = GBS_global_string("%i zombie%s", zombies, plural(zombies));
            }
            aw_message(GBS_global_string("%s in '%s'", msg, tree_name));
        }
    }
    free(tree_name);
    ntw->zoom_reset();
    AWT_expose_cb(0,ntw,0);
    GB_pop_transaction(ntw->gb_main);
}

void NT_recompute_cb(AW_window *,AWT_canvas *ntw, AW_CL cl2) {
    AWT_graphic_tree *gt = dynamic_cast<AWT_graphic_tree*>(ntw->tree_disp);
    awt_assert(gt);

    gt->tree_root->compute_tree(ntw->gb_main);
    AWT_expose_cb (ntw->aww, ntw, cl2 );
}

void NT_remove_species_in_tree_from_hash(AP_tree *tree,GB_HASH *hash) {
    if (!tree) return;
    if (tree->is_leaf && tree->name) {
        GBS_write_hash(hash,tree->name,0);  // delete species in hash table
    }else{
        NT_remove_species_in_tree_from_hash(tree->leftson,hash);
        NT_remove_species_in_tree_from_hash(tree->rightson,hash);
    }
}

