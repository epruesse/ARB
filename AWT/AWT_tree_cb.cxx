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
#include <awt_assert.hxx>


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

void NT_mark_all_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_push_transaction(ntw->gb_main);
    GBT_mark_all(ntw->gb_main,1);
    ntw->refresh();
    GB_pop_transaction(ntw->gb_main);
}



void NT_unmark_all_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_push_transaction(ntw->gb_main);
    GBT_mark_all(ntw->gb_main,0);
    ntw->refresh();
    GB_pop_transaction(ntw->gb_main);
}

void NT_invert_mark_all_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_push_transaction(ntw->gb_main);
    GBDATA *gb_species;
    for (	gb_species = GBT_first_species(ntw->gb_main);
            gb_species;
            gb_species = GBT_next_species(gb_species) ){
        long flag = GB_read_flag(gb_species);
        GB_write_flag(gb_species,1-flag);
    }
    ntw->refresh();
    GB_pop_transaction(ntw->gb_main);
}

void NT_count_mark_all_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_push_transaction(ntw->gb_main);
    long count=0;
    GBDATA *gb_species_data =
        GB_search(ntw->gb_main,"species_data",GB_CREATE_CONTAINER);
    count = GB_number_of_marked_subentries(gb_species_data);
    GB_pop_transaction(ntw->gb_main);
    char buf[256];
    sprintf(buf,"There are %li marked species",count);
    aw_message(buf);
    ntw=ntw;
}

void NT_mark_tree_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->mark_tree(AWT_TREE(ntw)->tree_root, 1);
    ntw->refresh();
}

void NT_group_tree_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 0);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}

void NT_group_not_marked_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 1);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}
void NT_group_terminal_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 2);
    AWT_TREE(ntw)->save(ntw->gb_main,0,0,0);
    ntw->zoom_reset();
    ntw->refresh();
}

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

void NT_ungroup_all_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->group_tree(AWT_TREE(ntw)->tree_root, 4);
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

void NT_unmark_all_tree_cb(void *dummy, AWT_canvas *ntw)
{
    AWUSE(dummy);
    GB_transaction gb_dummy(ntw->gb_main);
    AWT_TREE(ntw)->check_update(ntw->gb_main);
    AWT_TREE(ntw)->mark_tree(AWT_TREE(ntw)->tree_root, 0);
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


