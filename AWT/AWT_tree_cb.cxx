#include <stdio.h>
#include <stdlib.h>
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
// #include <awt_assert.hxx>
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
            text = "GROUP MODE   LEFT: group/ungroup group  RIGHT: create/destroy group";
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
            text = "BRANCH LENGTH MODE   LEFT: Select branch and drag to change length  RIGHT: -";
            break;
        case AWT_MODE_MOVE:
            text = "MOVE MODE   LEFT: move subtree and drag to new destination RIGHT: move group info only";
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

    ntw->awr->awar("tmp/LeftFooter")->write_string( text);
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
    if (count == 1) strcpy(buf, "There is 1 marked species");
    else sprintf(buf,"There are %li marked species", count);
    aw_message(buf);
}

// void NT_mark_all_cb(void *dummy, AWT_canvas *ntw)
// {
//     AWUSE(dummy);
//     GB_push_transaction(ntw->gb_main);
//     GBT_mark_all(ntw->gb_main,1);
//     ntw->refresh();
//     GB_pop_transaction(ntw->gb_main);
// }



// void NT_unmark_all_cb(void *dummy, AWT_canvas *ntw)
// {
//     AWUSE(dummy);
//     GB_push_transaction(ntw->gb_main);
//     GBT_mark_all(ntw->gb_main,0);
//     ntw->refresh();
//     GB_pop_transaction(ntw->gb_main);
// }

// void NT_invert_mark_all_cb(void *dummy, AWT_canvas *ntw)
// {
//     AWUSE(dummy);
//     GB_push_transaction(ntw->gb_main);
//     GBDATA *gb_species;
//     for (	gb_species = GBT_first_species(ntw->gb_main); gb_species; gb_species = GBT_next_species(gb_species)) {
//         long flag = GB_read_flag(gb_species);
//         GB_write_flag(gb_species,1-flag);
//     }
//     ntw->refresh();
//     GB_pop_transaction(ntw->gb_main);
// }


void NT_mark_all_cb(AW_window *, AW_CL cl_ntw, AW_CL cl_mark_mode)
{
    AWT_canvas *ntw       = (AWT_canvas*)cl_ntw;
    int         mark_mode = (int)cl_mark_mode;

    GB_transaction gb_dummy(ntw->gb_main);

    if (mark_mode == 1 || mark_mode == 0) {
        GBT_mark_all(ntw->gb_main,mark_mode);
    }
    else {
        for (GBDATA *gb_species = GBT_first_species(ntw->gb_main); gb_species; gb_species = GBT_next_species(gb_species)) {
            GB_write_flag(gb_species, !GB_read_flag(gb_species));
        }
    }

    ntw->refresh();
}

void NT_mark_tree_cb(AW_window *, AW_CL cl_ntw, AW_CL cl_mark_mode)
{
    AWT_canvas *ntw       = (AWT_canvas*)cl_ntw;
    int         mark_mode = (int)cl_mark_mode;

    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->mark_tree(AWT_TREE(ntw)->tree_root, mark_mode, 0);
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

    const char *label_base;
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
                const char *color_group_name = AW_get_color_group_name(awm->get_root(), i);
                sprintf(entry_buf, "%s '%s'", entry_prefix, color_group_name);
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

void NT_insert_mark_submenus(AW_window_menu_modes *awm, AWT_canvas *ntw) {
    awm->insert_menu_topic("count_marked",	"Count Marked Species",		"C","sp_count_mrk.hlp",	AWM_ALL, (AW_CB)NT_count_mark_all_cb,		(AW_CL)ntw, 0 );
    awm->insert_separator();
    awm->insert_menu_topic("mark_all",	"Mark all Species",		"M","sp_mrk_all.hlp",	AWM_ALL, (AW_CB)NT_mark_all_cb,			(AW_CL)ntw, (AW_CL)1 );
    awm->insert_menu_topic("mark_tree",	"Mark Species in Tree",		"T","sp_mrk_tree.hlp",	AWM_EXP, (AW_CB)NT_mark_tree_cb,		(AW_CL)ntw, (AW_CL)1 );
    NT_insert_color_mark_submenu(awm, ntw, "Mark colored species", 1);
    awm->insert_separator();
    awm->insert_menu_topic("unmark_all",	"Unmark all Species",		"U","sp_umrk_all.hlp",	AWM_ALL, (AW_CB)NT_mark_all_cb,		(AW_CL)ntw, 0 );
    awm->insert_menu_topic("unmark_tree",	"Unmark Species in Tree",	"n","sp_umrk_tree.hlp",	AWM_EXP, (AW_CB)NT_mark_tree_cb,		(AW_CL)ntw, (AW_CL)0 );
    NT_insert_color_mark_submenu(awm, ntw, "Unmark colored Species", 0);
    awm->insert_separator();
    awm->insert_menu_topic("swap_marked",	"Swap marks of all Species",		"w","sp_invert_mrk.hlp",AWM_ALL, (AW_CB)NT_mark_all_cb,		(AW_CL)ntw, (AW_CL)2 );
    awm->insert_menu_topic("swap_marked",	"Swap marks of Species in Tree",		"p","sp_invert_mrk.hlp",AWM_ALL, (AW_CB)NT_mark_tree_cb,		(AW_CL)ntw, (AW_CL)2 );
    NT_insert_color_mark_submenu(awm, ntw, "Swap marks of colored Species", 2);
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

        if (i) {
            const char *color_group_name = AW_get_color_group_name(awm->get_root(), i);
            sprintf(entry_buf, "group '%s'", color_group_name);
        }
        else {
            strcpy(entry_buf, "No color group");
        }

        hotkey[0]                       = hotkeys[i];
        if (hotkey[0] == ' ') hotkey[0] = 0;

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
        case 0:	stype = 0; break;
        case 1:	stype = 2; break;
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
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);

    GB_ERROR error = AWT_TREE(ntw)->tree_root->remove_leafs(ntw->gb_main, (int)mode);
    if (error) aw_message(error);
    if (AWT_TREE(ntw)->tree_root) AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_remove_bootstrap(void *dummy, AWT_canvas *ntw) // delete all bootstrap values
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);

    if (AWT_TREE(ntw)->tree_root){
        AWT_TREE(ntw)->tree_root->remove_bootstrap(ntw->gb_main);
        AWT_TREE(ntw)->tree_root->compute_tree(ntw->gb_main);
        AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    }
    ntw->zoom_reset();
    ntw->refresh();
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
            case AP_IRS_TREE:
            case AP_LIST_TREE:{
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
            case AP_NDS_TREE:
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
            case AP_RADIAL_TREE:{
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
    delete name;
}

void NT_jump_cb_auto(AW_window *dummy, AWT_canvas *ntw){	// jump only if auto jump is set
    if (AWT_TREE(ntw)->tree_sort == AP_LIST_TREE || AWT_TREE(ntw)->tree_sort == AP_NDS_TREE) {
        if (ntw->aww->get_root()->awar(AWAR_DTREE_AUTO_JUMP)->read_int()) {
            NT_jump_cb(dummy,ntw,0);
            return;
        }
    }
    ntw->refresh();
}


void NT_reload_tree_event(AW_root *awr, AWT_canvas *ntw, GB_BOOL set_delete_cbs)
{
    GB_push_transaction(ntw->gb_main);
    char *tree_name = awr->awar(ntw->user_awar)->read_string();

    GB_ERROR error = ntw->tree_disp->load(ntw->gb_main, tree_name,1,set_delete_cbs);	// linked
    if (error) {
        aw_message(error);
    }
    delete tree_name;
    ntw->zoom_reset();
    AWT_expose_cb(0,ntw,0);
    GB_pop_transaction(ntw->gb_main);
}


