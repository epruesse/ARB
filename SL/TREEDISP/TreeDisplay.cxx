#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_keysym.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>
#include <awt_nds.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>

#include "awt_attributes.hxx"
#include "TreeDisplay.hxx"

/***************************
      class AP_tree
****************************/

using namespace AW;

AW_gc_manager AWT_graphic_tree::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2)
{
    AW_gc_manager preset_window = 
        AW_manage_GC(aww,device,AWT_GC_CURSOR, AWT_GC_MAX, AW_GCM_DATA_AREA,
                     (AW_CB)AWT_resize_cb, (AW_CL)ntw, cd2,
                     true,      // define color groups
                     "#3be",

                     // Important note :
                     // Many gc indices are shared between ABR_NTREE and ARB_PARSIMONY
                     // e.g. the tree drawing routines use same gc's for drawing both trees
                     // (check PARS_dtree.cxx AWT_graphic_parsimony::init_devices)

                     "Cursor$white",
                     "Branch remarks$#b6ffb6",
                     "+-Bootstrap$#53d3ff",    "-B.(limited)$white",
                     "-GROUP_BRACKETS$#000",
                     "Marked$#ffe560",
                     "Some marked$#bb8833",
                     "Not marked$#622300",
                     "Zombies etc.$#977a0e",

                     "+-No probe$black",    "-Probes 1+2$yellow",
                     "+-Probe 1$red",       "-Probes 1+3$magenta",
                     "+-Probe 2$green",     "-Probes 2+3$cyan",
                     "+-Probe 3$blue",      "-All probes$white",
                     NULL );

    return preset_window;
}

AP_tree *AWT_graphic_tree::search(AP_tree *node, const char *name) {
    if (node) {
        if (node->is_leaf) {
            if (node->name && strcmp(name, node->name) == 0) {
                return node;
            }
        }
        else {
            AP_tree *result = search(node->get_leftson(), name);
            if (!result) result = search(node->get_rightson(), name);
            return result;
        }
    }
    return 0;
}

void AWT_graphic_tree::jump(AP_tree *at, const char *name)
{
    if (sort_is_list_style(tree_sort)) return;

    at = search(at,name);
    if(!at) return;
    if (tree_sort == AP_TREE_NORMAL) {
        tree_root_display = get_root_node();
    }
    else {
        while (at->father &&
               at->gr.view_sum<15 &&
               0 == at->gr.grouped)
        {
            at = at->get_father();
        }
        tree_root_display = at;
    }
}

void AWT_graphic_tree::mark_species_in_tree(AP_tree *at, int mark_mode) {
    /*
      mode      does

      0         unmark all
      1         mark all
      2         invert all marks
    */

    if (!at) return;

    GB_transaction ta(tree_static->get_gb_main());
    if (at->is_leaf) {
        if(at->gb_node) {       // not a zombie
            switch (mark_mode) {
                case 0: GB_write_flag(at->gb_node, 0); break;
                case 1: GB_write_flag(at->gb_node, 1); break;
                case 2: GB_write_flag(at->gb_node, !GB_read_flag(at->gb_node)); break;
                default : awt_assert(0);
            }
        }
    }

    mark_species_in_tree(at->get_leftson(), mark_mode);
    mark_species_in_tree(at->get_rightson(), mark_mode);
}

void AWT_graphic_tree::mark_species_in_tree_that(AP_tree *at, int mark_mode, int (*condition)(GBDATA*, void*), void *cd) {
    /*
      mark_mode does

      0         unmark all
      1         mark all
      2         invert all marks

      marks are only changed for those species for that condition() != 0
    */

    if (!at) return;

    GB_transaction dummy(tree_static->get_gb_main());
    if (at->is_leaf) {
        if(at->gb_node) {       // not a zombie
            int oldMark = GB_read_flag(at->gb_node);
            if (oldMark != mark_mode && condition(at->gb_node, cd)) {
                switch (mark_mode) {
                    case 0: GB_write_flag(at->gb_node, 0); break;
                    case 1: GB_write_flag(at->gb_node, 1); break;
                    case 2: GB_write_flag(at->gb_node, !oldMark); break;
                    default : awt_assert(0);
                }
            }
        }
    }

    mark_species_in_tree_that(at->get_leftson(), mark_mode, condition, cd);
    mark_species_in_tree_that(at->get_rightson(), mark_mode, condition, cd);
}


// same as mark_species_in_tree but works on rest of tree
void AWT_graphic_tree::mark_species_in_rest_of_tree(AP_tree *at, int mark_mode) {
    if (at) {
        AP_tree *pa = at->get_father();
        if (pa) {
            GB_transaction dummy(tree_static->get_gb_main());

            mark_species_in_tree(at->get_brother(), mark_mode);
            mark_species_in_rest_of_tree(pa, mark_mode);
        }
    }
}

// same as mark_species_in_tree_that but works on rest of tree
void AWT_graphic_tree::mark_species_in_rest_of_tree_that(AP_tree *at, int mark_mode, int (*condition)(GBDATA*, void*), void *cd) {
    if (at) {
        AP_tree *pa = at->get_father();
        if (pa) {
            GB_transaction dummy(tree_static->get_gb_main());

            mark_species_in_tree_that(at->get_brother(), mark_mode, condition, cd);
            mark_species_in_rest_of_tree_that(pa, mark_mode, condition, cd);
        }
    }
}

bool AWT_graphic_tree::tree_has_marks(AP_tree *at) {
    if (!at) return false;

    if (at->is_leaf) {
        if (!at->gb_node) return false; // zombie
        int marked = GB_read_flag(at->gb_node);
        return marked;
    }

    return tree_has_marks(at->get_leftson()) || tree_has_marks(at->get_rightson());
}

bool AWT_graphic_tree::rest_tree_has_marks(AP_tree *at) {
    if (!at) return false;

    AP_tree *pa = at->get_father();
    if (!pa) return false;

    return tree_has_marks(at->get_brother()) || rest_tree_has_marks(pa);
}

struct AWT_graphic_tree_group_state {
    int closed, opened;
    int closed_terminal, opened_terminal;
    int closed_with_marked, opened_with_marked;
    int marked_in_groups, marked_outside_groups;

    void clear() {
        closed                = 0;
        opened                = 0;
        closed_terminal       = 0;
        opened_terminal       = 0;
        closed_with_marked    = 0;
        opened_with_marked    = 0;
        marked_in_groups      = 0;
        marked_outside_groups = 0;
    }

    AWT_graphic_tree_group_state() { clear(); }

    bool has_groups() const { return closed+opened; }
    int marked() const { return marked_in_groups+marked_outside_groups; }

    bool all_opened() const { return closed == 0 && opened>0; }
    bool all_closed() const { return opened == 0 && closed>0; }
    bool all_terminal_closed() const { return opened_terminal == 0 && closed_terminal == closed; }
    bool all_marked_opened() const { return marked_in_groups > 0 && closed_with_marked == 0; }

    int next_expand_mode() const {
        if (closed_with_marked) return 1; // expand marked
        return 4; // expand all
    }

    int next_collapse_mode() const {
        if (all_terminal_closed()) return 0; // collapse all
        return 2; // group terminal
    }

    int next_group_mode() const {
        if (all_terminal_closed()) {
            if (marked_in_groups && !all_marked_opened()) return 1; // all but marked
            return 4; // expand all
        }
        if (all_marked_opened()) {
            if (all_opened()) return 0; // collapse all
            return 4;           // expand all
        }
        if (all_opened()) return 0; // collapse all
        if (!all_closed()) return 0; // collapse all

        if (!all_terminal_closed()) return 2; // group terminal
        if (marked_in_groups) return 1; // all but marked
        return 4; // expand all
    }

    void dump(void) {
        printf("closed=%i               opened=%i\n", closed, opened);
        printf("closed_terminal=%i      opened_terminal=%i\n", closed_terminal, opened_terminal);
        printf("closed_with_marked=%i   opened_with_marked=%i\n", closed_with_marked, opened_with_marked);
        printf("marked_in_groups=%i     marked_outside_groups=%i\n", marked_in_groups, marked_outside_groups);

        printf("all_opened=%i all_closed=%i all_terminal_closed=%i all_marked_opened=%i\n",
               all_opened(), all_closed(), all_terminal_closed(), all_marked_opened());
    }
};

void AWT_graphic_tree::detect_group_state(AP_tree *at, AWT_graphic_tree_group_state *state, AP_tree *skip_this_son) {
    if (!at) return;
    if (at->is_leaf) {
        if (at->gb_node && GB_read_flag(at->gb_node)) state->marked_outside_groups++; // count marks
        return;                 // leave are never grouped
    }

    if (at->gb_node) {          // i am a group
        AWT_graphic_tree_group_state sub_state;
        if (at->leftson != skip_this_son) detect_group_state(at->get_leftson(), &sub_state, skip_this_son);
        if (at->rightson != skip_this_son) detect_group_state(at->get_rightson(), &sub_state, skip_this_son);

        if (at->gr.grouped) {   // a closed group
            state->closed++;
            if (!sub_state.has_groups()) state->closed_terminal++;
            if (sub_state.marked()) state->closed_with_marked++;
            state->closed += sub_state.opened;
        }
        else { // an open group
            state->opened++;
            if (!sub_state.has_groups()) state->opened_terminal++;
            if (sub_state.marked()) state->opened_with_marked++;
            state->opened += sub_state.opened;
        }

        state->marked_in_groups += sub_state.marked();

        state->closed             += sub_state.closed;
        state->opened_terminal    += sub_state.opened_terminal;
        state->closed_terminal    += sub_state.closed_terminal;
        state->opened_with_marked += sub_state.opened_with_marked;
        state->closed_with_marked += sub_state.closed_with_marked;
    }
    else { // not a group
        if (at->leftson != skip_this_son) detect_group_state(at->get_leftson(), state, skip_this_son);
        if (at->rightson != skip_this_son) detect_group_state(at->get_rightson(), state, skip_this_son);
    }
}

void AWT_graphic_tree::group_rest_tree(AP_tree *at, int mode, int color_group) {
    if (at) {
        AP_tree *pa = at->get_father();
        if (pa) {
            group_tree(at->get_brother(), mode, color_group);
            group_rest_tree(pa, mode, color_group);
        }
    }
}

int AWT_graphic_tree::group_tree(AP_tree *at, int mode, int color_group)    // run on father !!!
{
    /*
      mode      does group

      0         all
      1         all but marked
      2         all but groups with subgroups
      4         none (ungroups all)
      8         all but color_group

    */

    if (!at) return 1;

    GB_transaction dummy(tree_static->get_gb_main());

    if (at->is_leaf) {
        int ungroup_me = 0;

        if (mode & 4) ungroup_me = 1;
        if (at->gb_node) { // not a zombie
            if (!ungroup_me && (mode & 1)) { // do not group marked
                if (GB_read_flag(at->gb_node)) { // i am a marked species
                    ungroup_me = 1;
                }
            }
            if (!ungroup_me && (mode & 8)) { // do not group one color_group
                int my_color_group = AW_find_color_group(at->gb_node, true);
                if (my_color_group == color_group) { // i am of that color group
                    ungroup_me = 1;
                }
            }
        }

        return ungroup_me;
    }

    int flag  = group_tree(at->get_leftson(), mode, color_group);
    flag     += group_tree(at->get_rightson(), mode, color_group);

    at->gr.grouped = 0;

    if (!flag) { // no son requests to be shown
        if (at->gb_node) { // i am a group
            GBDATA *gn = GB_entry(at->gb_node, "group_name");
            if (gn) {
                if (strlen(GB_read_char_pntr(gn))>0){ // and I have a name
                    at->gr.grouped     = 1;
                    if (mode & 2) flag = 1; // do not group non-terminal groups
                }
            }
        }
    }
    if (!at->father) get_root_node()->compute_tree(tree_static->get_gb_main());

    return flag;
}



int AWT_graphic_tree::resort_tree( int mode, struct AP_tree *at ) // run on father !!!
{
    /* mode

       0    to top
       1    to bottom
       2    center (to top)
       3    center (to bottom; don't use this - it's used internal)

       post-condition: leafname contains alphabetically "smallest" name of subtree

    */
    static const char *leafname;

    if (!at) {
        GB_transaction dummy(this->gb_main);
        at = get_root_node();
        if(!at) return 0;
        at->arb_tree_set_leafsum_viewsum();

        this->resort_tree(mode,at);
        at->compute_tree(this->gb_main);
        return 0;
    }

    if (at->is_leaf) {
        leafname = at->name;
        return 1;
    }
    int leftsize = at->get_leftson()->gr.leave_sum;
    int rightsize = at->get_rightson()->gr.leave_sum;

    if ( (mode &1) == 0 ) { // to top
        if (rightsize >leftsize ) {
            at->swap_sons();
        }
    }
    else {
        if (rightsize < leftsize ) {
            at->swap_sons();
        }
    }

    int lmode = mode;
    int rmode = mode;
    if ( (mode & 2) == 2){
        lmode = 2;
        rmode = 3;
    }

    resort_tree(lmode,at->get_leftson());
    awt_assert(leafname);
    const char *leftleafname = leafname;

    resort_tree(rmode,at->get_rightson());
    awt_assert(leafname);
    const char *rightleafname = leafname;

    awt_assert(leftleafname && rightleafname);

    if (leftleafname && rightleafname) {
        int name_cmp = strcmp(leftleafname,rightleafname);
        if (name_cmp<0) { // leftleafname < rightleafname
            leafname = leftleafname;
        }
        else { // (name_cmp>=0) aka: rightleafname <= leftleafname
            leafname = rightleafname;
            if (rightsize==leftsize && name_cmp>0) { // if sizes of subtrees are equal and rightleafname<leftleafname -> swap branches
                at->swap_sons();
            }
        }
    }
    else {
        if (leftleafname) leafname = leftleafname;
        else leafname = rightleafname;
    }

    return 0;
}


void AWT_graphic_tree::rot_show_line(AW_device *device)
{
    double             sx, sy;
    double             x, y;
    sx = (this->old_rot_cl.x0+this->old_rot_cl.x1)*.5;
    sy = (this->old_rot_cl.y0+this->old_rot_cl.y1)*.5;
    x = this->rot_cl.x0 * (1.0-this->rot_cl.length) + this->rot_cl.x1 * this->rot_cl.length;
    y = this->rot_cl.y0 * (1.0-this->rot_cl.length) + this->rot_cl.y1 * this->rot_cl.length;
    int gc = this->drag_gc;
    device->line(gc, sx, sy, x, y, AWT_F_ALL, 0, 0);
}

void AWT_graphic_tree::rot_show_triangle(AW_device *device)
{
    double   w;
    double   len;
    double   sx, sy;
    double   x1, y1, x2, y2;
    AP_tree *at;
    double   scale = 1.0;

    if (tree_sort == AP_TREE_IRS) scale = irs_tree_ruler_scale_factor;

    at = this->rot_at;

    if (!at || !at->father)
        return;

    sx = this->old_rot_cl.x0;
    sy = this->old_rot_cl.y0;

    if (at == at->father->leftson) len = at->father->leftlen;
    else len = at->father->rightlen;

    len *= scale;

    w = this->rot_orientation;  x1 = this->old_rot_cl.x0 + cos(w) * len;
    y1 = this->old_rot_cl.y0 + sin(w) * len;

    int gc = this->drag_gc;

    device->line(gc, sx, sy, x1, y1, AWT_F_ALL, 0, 0);


    if (!at->is_leaf) {
        sx = x1; sy = y1;
        len = at->gr.tree_depth;
        w = this->rot_orientation - this->rot_spread*.5*.5 + at->gr.right_angle;
        x1 = sx + cos(w) * len;
        y1 = sy + sin(w) * len;
        w = this->rot_orientation + this->rot_spread*.5*.5 + at->gr.left_angle;
        x2 = sx + cos(w) * len;
        y2 = sy + sin(w) * len;

        device->line(gc, sx, sy, x1, y1, AWT_F_ALL, 0, 0);
        device->line(gc, sx, sy, x2, y2, AWT_F_ALL, 0, 0);
        device->line(gc, x2, y2, x1, y1, AWT_F_ALL, 0, 0);

    }


}
void AWT_show_bootstrap_circle(AW_device *device, const char *bootstrap, double zoom_factor, double max_radius, double len, double x, double y, bool elipsoid, double elip_ysize, int filter, AW_CL cd1,AW_CL cd2){
    double radius           = .01 * atoi(bootstrap); // bootstrap values are given in % (0..100)
    if (radius < .1) radius = .1;

    radius  = 1.0 / sqrt(radius); // -> bootstrap->radius : 100% -> 1, 0% -> inf
    radius -= 1.0;              // -> bootstrap->radius : 100% -> 0, 0% -> inf
    radius *= 2; // diameter ?

    if (radius < 0) return;     // skip too small circles

    // Note : radius goes against infinite, if bootstrap values go against zero
    //        For this reason we do some limitation here:
#define BOOTSTRAP_RADIUS_LIMIT max_radius

    int gc = AWT_GC_BOOTSTRAP;

    if (radius > BOOTSTRAP_RADIUS_LIMIT) {
        radius = BOOTSTRAP_RADIUS_LIMIT;
        gc     = AWT_GC_BOOTSTRAP_LIMITED;
    }

    double     radiusx = radius * len * zoom_factor; // multiply with length of branch (and zoomfactor)
    double     radiusy;
    if (elipsoid) {
        radiusy = elip_ysize * zoom_factor;
        if (radiusy > radiusx) radiusy = radiusx;
    }
    else {
        radiusy = radiusx;
    }

    device->circle(gc, false, x, y, radiusx, radiusy, filter, cd1, (AW_CL)cd2);
}

double comp_rot_orientation(AW_clicked_line *cl)
{
    return atan2(cl->y1 - cl->y0, cl->x1 - cl->x0);
}

double comp_rot_spread(AP_tree *at, AWT_graphic_tree *ntw)
{
    double zw = 0;
    AP_tree *bt;

    if(!at) return 0;

    zw = 1.0;

    for (bt=at; bt->father && (bt!=ntw->tree_root_display); bt = bt->get_father()) {
        zw *= bt->gr.spread;
    }

    zw *= bt->gr.spread;
    zw *= (double)at->gr.view_sum / (double)bt->gr.view_sum;

    switch (ntw->tree_sort) {
        case AP_TREE_NORMAL:
            zw *= 0.5*M_PI;
            break;
        case AP_TREE_IRS:
            zw *= 0.5*M_PI * ntw->get_irs_tree_ruler_scale_factor();
            break;
        case AP_TREE_RADIAL:
            zw *= 2*M_PI;
            break;
        default:
            awt_assert(0);
    }

    return zw;
}

void AWT_graphic_tree_root_changed(void *cd, AP_tree *old, AP_tree *newroot) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    if (agt->tree_root_display == old || agt->tree_root_display->is_inside(old)) {
        agt->tree_root_display = newroot;
    }
}

void AWT_graphic_tree_node_deleted(void *cd, AP_tree *del) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    if (agt->tree_root_display == del) {
        agt->tree_root_display = agt->get_root_node();
    }
    if (agt->get_root_node() == del) {
        agt->tree_root_display = 0;
    }
}
void AWT_graphic_tree::toggle_group(AP_tree * at) {
    GB_ERROR error = NULL;
    
    if (at->gb_node) {                                            // existing group
        char *gname = GBT_read_string(at->gb_node, "group_name");
        if (gname) {
            const char *msg = GBS_global_string("What to do with group '%s'?",gname);

            switch (aw_question(msg, "Rename,Destroy,Cancel")) {
                case 0: {                                                    // rename
                    char *new_gname = aw_input("Rename group", "Change group name:", at->name);
                    if (new_gname) {
                        freeset(at->name, new_gname);
                        error = GBT_write_string(at->gb_node, "group_name", new_gname);
                    }
                    break;
                }

                case 1:                                      // destroy
                    at->gr.grouped = 0;
                    at->name       = 0;
                    error          = GB_delete(at->gb_node);
                    at->gb_node    = 0;
                    break;

                case 2:         // cancel
                    break;
            }

            free(gname);
        }
    }
    else {
        error = create_group(at); // create new group
        if (!error && at->name) at->gr.grouped = 1;
    }

    if (error) aw_message(error);
}

GB_ERROR AWT_graphic_tree::create_group(AP_tree * at) {
    GB_ERROR error = NULL;
    if (!at->name) {
        char *gname = aw_input("Enter Name of Group");
        if (gname) {
            GBDATA         *gb_tree  = tree_static->get_gb_tree();
            GBDATA         *gb_mainT = GB_get_root(gb_tree);
            GB_transaction  ta(gb_mainT);

            if (!at->gb_node) {
                at->gb_node   = GB_create_container(gb_tree, "node");
                if (!at->gb_node) error = GB_await_error();
                else {
                    error = GBT_write_int(at->gb_node, "id", 0);
                    this->exports.save = !error;
                }
            }

            if (!error) {
                GBDATA *gb_name     = GB_search(at->gb_node, "group_name", GB_STRING);
                if (!gb_name) error = GB_await_error();
                else {
                    error = GBT_write_group_name(gb_name, gname);
                    if (!error) at->name = gname;
                }
            }
            error = ta.close(error);
        }
    }
    else if (!at->gb_node) {
        at->gb_node = GB_create_container(tree_static->get_gb_tree(), "node");

        if (!at->gb_node) error = GB_await_error();
        else {
            error = GBT_write_int(at->gb_node, "id", 0);
            this->exports.save = !error;
        }
    }

    return error;
}

void AWT_graphic_tree::key_command(AWT_COMMAND_MODE /*cmd*/, AW_key_mod key_modifier, char key_char,
                                   AW_pos /*x*/, AW_pos /*y*/, AW_clicked_line *cl, AW_clicked_text *ct)
{
    bool update_timer = true;
    bool calc_color   = true;
    bool refresh      = true;
    bool Save         = false;
    bool resize       = false;
    bool compute      = false;

    if (key_modifier != AW_KEYMODE_NONE) return;
    if (key_char == 0) return;

#if defined(DEBUG)
    printf("key_char=%i (=%c)\n", int(key_char), key_char);
#endif // DEBUG

    // ----------------------------------------
    //      commands independent of tree :
    // ----------------------------------------

    bool global_key = true;
    switch (key_char) {
        case 9:  {    // Ctrl-i = invert all
            GBT_mark_all(gb_main, 2);
            break;
        }
        case 13:  {    // Ctrl-m = mark/unmark all
            int mark   = GBT_first_marked_species(gb_main) == 0; // no species marked -> mark all
            GBT_mark_all(gb_main, mark);
            break;
        }
        default: {
            global_key = false;
            break;
        }
    }

    if (!global_key && tree_proto) {

        AP_tree    *at   = 0;
        const char *what = 0;

        if (ct->exists) {
            at   = (AP_tree*)ct->client_data1;
            what = (char*)ct->client_data2;
        }
        else if (cl->exists) {
            at   = (AP_tree*)cl->client_data1;
            what = (char*)cl->client_data2;
        }

#if defined(DEBUG)
        printf("what='%s' at=%p\n", what, at);
#endif // DEBUG

        if (what && strcmp(what, "species") == 0) {
            GBDATA *gb_species = 0;
            if (ct->exists) {
                gb_species = (GBDATA *)ct->client_data1;
            }
            else {
                awt_assert(0);
            }

            // ------------------------------------
            //      commands in species list :
            // ------------------------------------

            switch (key_char) {
                case 'i':
                case 'm':  {    // i/m = mark/unmark species
                    GB_write_flag(gb_species, 1-GB_read_flag(gb_species));
                    break;
                }
                case 'I':  {    // I = invert all but current
                    int mark = GB_read_flag(gb_species);
                    GBT_mark_all(gb_main, 2);
                    GB_write_flag(gb_species, mark);
                    break;
                }
                case 'M':  {    // M = mark/unmark all but current
                    int mark = GB_read_flag(gb_species);
                    GB_write_flag(gb_species, 0); // unmark current
                    GBT_mark_all(gb_main, GBT_first_marked_species(gb_main) == 0);
                    GB_write_flag(gb_species, mark); // restore mark of current
                    break;
                }
            }
        }
        else {
            if (!at) {
                // many commands work on whole tree if mouse does not point to subtree
                at = get_root_node();
            }

            // -------------------------------------
            //      command working with tree :
            // -------------------------------------

            if (at) {
                switch (key_char) {
                    case 'm':  {    // m = mark/unmark (sub)tree
                        mark_species_in_tree(at, !tree_has_marks(at));
                        break;
                    }
                    case 'M':  {    // M = mark/unmark all but (sub)tree
                        mark_species_in_rest_of_tree(at, !rest_tree_has_marks(at));
                        break;
                    }

                    case 'i':  {    // i = invert (sub)tree
                        mark_species_in_tree(at, 2);
                        break;
                    }
                    case 'I':  {    // I = invert all but (sub)tree
                        mark_species_in_rest_of_tree(at, 2);
                        break;
                    }
                    case 'c':
                    case 'x':  {
                        AWT_graphic_tree_group_state state;
                        detect_group_state(at, &state, 0);

                        if (!state.has_groups()) { // somewhere inside group
                        do_parent:
                            at = at->get_father();
                            while (at) {
                                if (at->gb_node) break;
                                at = at->get_father();
                            }

                            if (at) {
                                state.clear();
                                detect_group_state(at, &state, 0);
                            }
                        }

                        if (at) {
                            int next_group_mode;

                            if (key_char == 'x')  { // expand
                                next_group_mode = state.next_expand_mode();
                            }
                            else { // collapse
                                if (state.all_closed()) goto do_parent;
                                next_group_mode = state.next_collapse_mode();
                            }

                            /*int result = */
                            group_tree(at, next_group_mode, 0);

                            Save    = true;
                            resize  = true;
                            compute = true;
                        }
                        break;
                    }
                    case 'C':
                    case 'X':  {
                        AP_tree *root_node                  = at;
                        while (root_node->father) root_node = root_node->get_father(); // search father

                        awt_assert(root_node);

                        AWT_graphic_tree_group_state state;
                        detect_group_state(root_node, &state, at);

                        int next_group_mode;
                        if (key_char == 'X')  { // expand
                            next_group_mode = state.next_expand_mode();
                        }
                        else { // collapse
                            next_group_mode = state.next_collapse_mode();
                        }

                        /*int result = */
                        group_rest_tree(at, next_group_mode, 0);

                        Save    = true;
                        resize  = true;
                        compute = true;


                        break;
                    }
                }
            }
        }
    }

    if (Save) {
        update_timer = false;
        exports.save = 1;
    }

    if (compute && get_root_node()) {
        get_root_node()->compute_tree(gb_main);
        resize     = true;
        calc_color = false;
    }

    if (update_timer && tree_static) {
        tree_static->update_timers(); // do not reload the tree
    }

    if (resize) {
        exports.resize = 1;
        refresh        = true;
    }

    if (calc_color && get_root_node()) {
        get_root_node()->calc_color();
        refresh = true;
    }
    if (refresh) {
        exports.refresh = 1;
    }
}

inline double discrete_ruler_length(double analog_ruler_length, double min_length) {
    double drl = int(analog_ruler_length*10+0.5)/10.0;
    if (drl<min_length) {
        drl = min_length;
    }
    return drl;
}

void AWT_graphic_tree::command(AW_device *device, AWT_COMMAND_MODE cmd,
                               int button, AW_key_mod key_modifier, AW_key_code /*key_code*/, char key_char,
                               AW_event_type type, AW_pos x, AW_pos y,
                               AW_clicked_line *cl, AW_clicked_text *ct)
{
    static int rot_drag_flag, bl_drag_flag;

    if (type == AW_Keyboard_Press) {
        return key_command(cmd, key_modifier, key_char, x, y, cl, ct);
    }

    AP_tree *at;
    if ( ct->exists && ct->client_data2 && !strcmp((char *) ct->client_data2, "species")){
        // clicked on a species list :
        GBDATA *gbd     = (GBDATA *)ct->client_data1;
        bool    select  = false;
        bool    refresh = false;

        switch (cmd) {
            case AWT_MODE_MARK:  {
                if (type == AW_Mouse_Press) {
                    switch (button) {
                        case AWT_M_LEFT:
                            GB_write_flag(gbd, 1);
                            refresh = true;
                            select  = true;
                            break;
                        case AWT_M_RIGHT:
                            GB_write_flag(gbd, 0);
                            refresh = true;
                            break;
                    }
                }
                break;
            }
            default :  { // all other modes just select a species :
                select = true;
                break;
            }
        }

        if (select && type == AW_Mouse_Press) AD_map_viewer(gbd);
        if (refresh) this->exports.refresh = 1;

        return;
    }

    if (!this->tree_static) return;     // no tree -> no commands

    GBDATA *gb_tree = tree_static->get_gb_tree();
                        
    if ( (rot_ct.exists && (rot_ct.distance == 0) && (!rot_ct.client_data1) &&
          rot_ct.client_data2 && !strcmp((char *) rot_ct.client_data2, "ruler") ) ||
         (ct && ct->exists && (!ct->client_data1) &&
          ct->client_data2 && !strcmp((char *) ct->client_data2, "ruler")))
    {
        const char *tree_awar;
        char awar[256];
        float h;
        switch(cmd){
            case AWT_MODE_SELECT:
            case AWT_MODE_ROT:
            case AWT_MODE_SPREAD:
            case AWT_MODE_SWAP:
            case AWT_MODE_SETROOT:
            case AWT_MODE_LENGTH:
            case AWT_MODE_MOVE: // Move Ruler text
                switch(type){
                    case AW_Mouse_Press:
                        rot_ct          = *ct;
                        rot_ct.textArea.moveTo(Position(x, y));
                        break;
                    case AW_Mouse_Drag: {
                        double          scale = device->get_scale();
                        const Position &start = rot_ct.textArea.start();

                        tree_awar    = show_ruler(device, drag_gc);
                        sprintf(awar,"ruler/%s/text_x", tree_awar);

                        h = (x - start.xpos())/scale + *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                        GBT_write_float(gb_tree, awar, h);
                        sprintf(awar,"ruler/%s/text_y", tree_awar);
                        h = (y - start.ypos())/scale + *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                        GBT_write_float(gb_tree, awar, h);
                        rot_ct.textArea.moveTo(Position(x, y));
                        show_ruler(device, drag_gc);
                        break;
                    }
                    case AW_Mouse_Release:
                        rot_ct.exists = false;
                        exports.resize = 1;
                        break;
                    default: break;
                }
                return;
            default:    break;
        }
    }


    if ((rot_cl.exists && (!rot_cl.client_data1) &&
         rot_cl.client_data2 && !strcmp((char *) rot_cl.client_data2, "ruler") ) ||
        (cl && cl->exists && (!cl->client_data1) &&
         cl->client_data2 && !strcmp((char *) cl->client_data2, "ruler")))
    {
        const char *tree_awar;
        char awar[256];
        float h;
        switch(cmd){
            case AWT_MODE_ROT:
            case AWT_MODE_SPREAD:
            case AWT_MODE_SWAP:
            case AWT_MODE_SETROOT:
            case AWT_MODE_MOVE: // Move Ruler
                switch(type){
                    case AW_Mouse_Press:
                        rot_cl = *cl;
                        rot_cl.x0 = x;
                        rot_cl.y0 = y;
                        break;
                    case AW_Mouse_Drag:
                        tree_awar = show_ruler(device, this->drag_gc);
                        sprintf(awar,"ruler/%s/ruler_x",tree_awar);
                        h = (x - rot_cl.x0)/device->get_scale() + *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                        GBT_write_float(gb_tree, awar, h);
                        sprintf(awar,"ruler/%s/ruler_y",tree_awar);
                        h = (y - rot_cl.y0)/device->get_scale() + *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                        GBT_write_float(gb_tree, awar, h);

                        rot_cl.x0 = x;
                        rot_cl.y0 = y;
                        show_ruler(device, this->drag_gc);
                        break;
                    case AW_Mouse_Release:
                        rot_cl.exists = false;
                        this->exports.resize = 1;
                        break;
                    default:
                        break;
                }
                break;


            case AWT_MODE_LENGTH:
                switch(type){   // resize Ruler
                    case AW_Mouse_Press:
                        rot_cl = *cl;
                        rot_cl.x0 = x;
                        if (button==AWT_M_RIGHT) { // if right mouse button is used -> adjust to 1 digit behind comma
                            sprintf(awar,"ruler/size");
                            tree_awar = show_ruler(device, this->drag_gc);
                            double rulerSize = *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                            GBT_write_float(gb_tree, awar, discrete_ruler_length(rulerSize, 0.1));
                            tree_awar = show_ruler(device, this->drag_gc);
                        }
                        break;
                    case AW_Mouse_Drag: {
                        sprintf(awar,"ruler/size");
                        h = *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                        if (button == AWT_M_RIGHT) {
                            GBT_write_float(gb_tree, awar, discrete_ruler_length(h, 0.1));
                        }
                        tree_awar = show_ruler(device, this->drag_gc);

                        if (tree_sort == AP_TREE_IRS) {
                            double scale = device->get_scale() * irs_tree_ruler_scale_factor;
                            h += (rot_cl.x0 - x)/scale;
                        }
                        else {
                            h += (x - rot_cl.x0)/device->get_scale();
                        }
                        if (h<0.01) h = 0.01;

                        double h_rounded = h;
                        if (button==AWT_M_RIGHT) { // if right mouse button is used -> adjust to 1 digit behind comma
                            h_rounded = discrete_ruler_length(h, 0.1);
                            GBT_write_float(gb_tree, awar, h_rounded);
                            show_ruler(device, this->drag_gc);
                            GBT_write_float(gb_tree, awar, h);
                        }
                        else {
                            GBT_write_float(gb_tree, awar, h);
                            show_ruler(device, this->drag_gc);
                        }

                        rot_cl.x0 = x;
                        break;
                    }
                    case AW_Mouse_Release:
                        rot_cl.exists = false;
                        this->exports.refresh = 1;
                        if (button==AWT_M_RIGHT) { // if right mouse button is used -> adjust to 1 digit behind comma
                            sprintf(awar,"ruler/size");
                            double rulerSize = *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                            GBT_write_float(gb_tree, awar, discrete_ruler_length(rulerSize, 0.1));
                        }
                        break;
                    default:
                        break;
                }
                break;
            case AWT_MODE_LINE:
                if (type == AW_Mouse_Press) {
                    long i;
                    sprintf(awar,"ruler/ruler_width");
                    i = *GBT_readOrCreate_int(gb_tree, awar , 0);
                    switch(button){
                        case AWT_M_LEFT:
                            i --;
                            if (i<0) i= 0;
                            else{
                                GBT_write_int(gb_tree, awar,i);
                                this->exports.refresh = 1;
                            }
                            break;
                        case AWT_M_RIGHT:
                            i++;
                            GBT_write_int(gb_tree, awar,i);
                            show_ruler(device, AWT_GC_CURSOR);
                            break;
                        default: break;
                    }
                }
                break;
            default:    break;
        }
        return;
    }

    switch(cmd) {
        case AWT_MODE_MOVE:             // two point commands !!!!!
            if(button==AWT_M_MIDDLE){
                break;
            }
            switch(type){
                case AW_Mouse_Press:
                    if( !(cl && cl->exists) ){
                        break;
                    }

                    /*** check security level @@@ ***/
                    at = (AP_tree *)cl->client_data1;
                    if(at && at->father){
                        bl_drag_flag = 1;
                        this->rot_at = at;
                        this->rot_cl = *cl;
                        this->old_rot_cl = *cl;
                    }
                    break;

                case AW_Mouse_Drag:
                    if( bl_drag_flag && this->rot_at && this->rot_at->father){
                        this->rot_show_line(device);
                        if (cl->exists) {
                            this->rot_cl = *cl;
                        }
                        else {
                            rot_cl = old_rot_cl;
                        }
                        this->rot_show_line(device);
                    }
                    break;
                case AW_Mouse_Release:
                    if( bl_drag_flag && this->rot_at && this->rot_at->father){
                        this->rot_show_line(device);
                        AP_tree *dest= 0;
                        if (cl->exists) dest = (AP_tree *)cl->client_data1;
                        AP_tree *source = rot_at;
                        if (!(source && dest)) {
                            aw_message("Please Drag Line to a valid branch");
                            break;
                        }
                        if (source == dest) {
                            aw_message("Please drag mouse from source to destination");
                            break;
                        }

                        GB_ERROR error;
                        switch(button){
                            case AWT_M_LEFT:
                                error = source->cantMoveTo(dest);
                                if (!error) source->moveTo(dest,cl->length);
                                break;
                                
                            case AWT_M_RIGHT:
                                error = source->move_group_info(dest);
                                break;
                            default:
                                error = "????? 45338";
                        }

                        if (error){
                            aw_message(error);
                        }
                        else {
                            this->exports.refresh = 1;
                            this->exports.save    = 1;
                            this->exports.resize  = 1;
                            ASSERT_VALID_TREE(get_root_node());
                            get_root_node()->compute_tree(gb_main);
                        }
                    }
                    break;
                default:
                    break;
            }
            break;


        case AWT_MODE_LENGTH:
            if (button == AWT_M_MIDDLE) {
                break;
            }
            switch(type){
                case AW_Mouse_Press:
                    if( cl && cl->exists ){
                        at = (AP_tree *)cl->client_data1;
                        if (!at) break;
                        bl_drag_flag = 1;
                        this->rot_at = at;
                        this->rot_cl = *cl;
                        this->old_rot_cl = *cl;
                        device->transform(cl->x0, cl->y0, rot_cl.x0, rot_cl.y0);
                        device->transform(cl->x1, cl->y1, rot_cl.x1, rot_cl.y1);

                        this->rot_orientation = comp_rot_orientation(&rot_cl);
                        this->rot_spread      = comp_rot_spread(at, this);
#if defined(DEBUG)
                        printf("rot_spread=%f\n", rot_spread);
#endif // DEBUG
                        rot_show_triangle(device);
                    }
                    break;

                case AW_Mouse_Drag:
                    if( bl_drag_flag && this->rot_at && this->rot_at->father){
                        double len, ex, ey, scale;

                        rot_show_triangle(device);

                        if (rot_at == rot_at->father->leftson){
                            len = rot_at->father->leftlen;
                        }
                        else {
                            len = rot_at->father->rightlen;
                        }

                        ex = x-rot_cl.x0;
                        ey = y-rot_cl.y0;

                        len = ex * cos(this->rot_orientation) +
                            ey * sin(this->rot_orientation);

                        if (button==AWT_M_RIGHT) { // if right mouse button is used -> adjust to 1 digit behind comma
                            len = discrete_ruler_length(len, 0.0);
                        }
                        else if (len<0.0) {
                            len = 0.0;
                        }

                        scale = device->get_scale();
                        if (tree_sort == AP_TREE_IRS) {
                            scale *= irs_tree_ruler_scale_factor;
                        }
                        len = len/scale;

                        if (rot_at == rot_at->father->leftson) {
                            rot_at->father->leftlen = len;
                        }
                        else {
                            rot_at->father->rightlen = len;
                        }

                        rot_show_triangle(device);
                    }
                    break;

                case AW_Mouse_Release:
                    exports.refresh = 1;
                    exports.save    = 1;
                    rot_drag_flag   = 0;
                    get_root_node()->compute_tree(gb_main);
                    break;
                default:
                    break;
            }
            break;

        case AWT_MODE_ROT:
            if(button!=AWT_M_LEFT){
                break;
            }
            switch(type){
                case AW_Mouse_Press:
                    if( cl && cl->exists ){
                        at = (AP_tree *)cl->client_data1;
                        if (!at) break;
                        rot_drag_flag = 1;
                        this->rot_at = at;
                        this->rot_cl = *cl;
                        this->old_rot_cl = *cl;
                        device->transform(cl->x0, cl->y0, rot_cl.x0, rot_cl.y0);
                        device->transform(cl->x1, cl->y1, rot_cl.x1, rot_cl.y1);

                        this->rot_orientation = comp_rot_orientation(&rot_cl);
                        this->rot_spread = comp_rot_spread(at, this);
                        rot_show_triangle(device);
                    }
                    break;

                case AW_Mouse_Drag:
                    if( rot_drag_flag && this->rot_at && this->rot_at->father){
                        rot_show_triangle(device);

                        double new_a     = atan2(y - rot_cl.y0, x - rot_cl.x0) - rot_orientation;
                        rot_orientation += new_a;
                        
                        AP_tree *rot_father = rot_at->get_father();
                        if (rot_at == rot_father->leftson) {
                            rot_father->gr.left_angle += new_a;
                            if (!rot_father->father) rot_father->gr.right_angle += new_a;
                        }
                        else {
                            rot_father->gr.right_angle += new_a;
                            if (!rot_father->father) rot_father->gr.left_angle += new_a;
                        }
                        rot_show_triangle(device);
                    }
                    break;

                case AW_Mouse_Release:
                    this->exports.refresh = 1;
                    this->exports.save = 1;
                    rot_drag_flag = 0;
                    break;
                default:    break;
            }

            break;

        case AWT_MODE_LZOOM:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (at) {
                                tree_root_display  = at;
                                exports.zoom_reset = 1;
                                exports.refresh    = 1;
                            }
                        }
                        break;
                    case AWT_M_RIGHT:
                        if (tree_root_display->father) {
                            tree_root_display  = tree_root_display->get_father();
                            exports.refresh    = 1;
                            exports.zoom_reset = 1;
                        }
                        break;
                }
                get_root_node()->compute_tree(gb_main);
            } /* if type */
            break;

        case AWT_MODE_RESET:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        /** reset rotation **/
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (at) {
                                at->reset_rotation();
                                exports.save    = 1;
                                exports.refresh = 1;
                            }
                        }
                        break;
                    case AWT_M_MIDDLE:
                        /** reset spread **/
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (at) {
                                at->reset_spread();
                                exports.save    = 1;
                                exports.refresh = 1;
                            }
                        }
                        break;
                    case AWT_M_RIGHT:
                        /** reset linewidth **/
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (at) {
                                at->reset_line_width();   
                                AP_tree *at_fath = at->get_father();
                                if (at_fath) { // reset clicked branch
                                    if (at_fath->leftson == at) at_fath->gr.left_linewidth = 0;
                                    else at_fath->gr.right_linewidth                       = 0;
                                }
                                exports.save    = 1;
                                exports.refresh = 1;
                            }
                        }
                        break;
                }
                get_root_node()->compute_tree(gb_main);
            } /* if press */
            break;

        case AWT_MODE_LINE:
            if (type==AW_Mouse_Press) {
                if (cl->exists) {
                    at = (AP_tree *)cl->client_data1;
                    if (at) {
                        AP_tree *at_fath = at->get_father();
                        if (at_fath) {
                            char &linewidth = (at_fath->leftson == at)
                                ? at_fath->gr.left_linewidth
                                : at_fath->gr.right_linewidth;

                            switch (button) {
                                case AWT_M_LEFT:
                                    linewidth       = (linewidth <= 1) ? 0 : linewidth-1;
                                    exports.save    = 1;
                                    exports.refresh = 1;
                                    break;
                                    
                                case AWT_M_RIGHT:
                                    linewidth += 1;
                                    exports.save    = 1;
                                    exports.refresh = 1;
                                    break;
                            }
                        }
                    }
                }
            } /* if type */
            break;

        case AWT_MODE_SPREAD:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->gr.spread -= PH_CLICK_SPREAD;
                            if(at->gr.spread<0) at->gr.spread = 0;
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                    case AWT_M_RIGHT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->gr.spread += PH_CLICK_SPREAD;
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                }
            } /* if type */
            break;

    act_like_group:
        case AWT_MODE_GROUP:
            if(type==AW_Mouse_Press){
                if(cl->exists){
                    at = (AP_tree *)cl->client_data1;
                    if (!at) break;
                }else break;

                switch(button){
                    case AWT_M_LEFT:
                        if ( (!at->gr.grouped) && (!at->name)) {
                            break; // not grouped and no name
                        }

                        if (at->is_leaf) break; // don't touch zombies

                        if (gb_tree) {
                            GB_ERROR error = this->create_group(at);
                            if (error) aw_message(error);
                        }
                        if (at->name) {
                            at->gr.grouped  ^= 1;
                            exports.refresh  = 1;
                            exports.save     = 1;
                            exports.resize   = 1;
                            get_root_node()->compute_tree(gb_main);
                        }
                        break;
                    case AWT_M_RIGHT:
                        if (gb_tree) {
                            this->toggle_group(at);
                        }
                        exports.refresh = 1;
                        exports.save    = 1;
                        exports.resize  = 1;
                        get_root_node()->compute_tree(gb_main);

                        break;
                    default:
                        break;
                }
            } /* if type */
            break;

        case AWT_MODE_SETROOT:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            
                            at->set_root();
                            exports.save       = 1;
                            exports.zoom_reset = 1;
                            get_root_node()->compute_tree(gb_main);
                        }
                        break;
                    case AWT_M_RIGHT:
                        DOWNCAST(AP_tree*, tree_static->find_innermost_edge().son())->set_root();
                        exports.save       = 1;
                        exports.zoom_reset = 1;
                        get_root_node()->compute_tree(gb_main);
                        break;
                }
            } /* if type */
            break;

        case AWT_MODE_SWAP:
            if(type==AW_Mouse_Press){
                switch(button){
                    case AWT_M_LEFT:
                        if(cl->exists){
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->swap_sons();

                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                    case AWT_M_RIGHT:
                        break;
                }
            } /* if type */
            break;

        case AWT_MODE_MARK:
            if (type == AW_Mouse_Press && (cl->exists || ct->exists)) {
                at = (AP_tree *)(cl->exists ? cl->client_data1 : ct->client_data1);
                if (at) {
                    GB_transaction ta(tree_static->get_gb_main());

                    if (type == AW_Mouse_Press) {
                        switch (button) {
                            case AWT_M_LEFT:
                                mark_species_in_tree(at, 1);
                                break;
                            case AWT_M_RIGHT:
                                mark_species_in_tree(at, 0);
                                break;
                        }
                    }
                    exports.refresh = 1;
                    tree_static->update_timers(); // do not reload the tree
                    get_root_node()->calc_color();
                }
            }
            break;

        case AWT_MODE_NONE:
        case AWT_MODE_SELECT:
            if(type==AW_Mouse_Press && (cl->exists || ct->exists) && button != AWT_M_MIDDLE){
                GB_transaction ta(tree_static->get_gb_main());
                at = (AP_tree *)(cl->exists ? cl->client_data1 : ct->client_data1);
                if (!at) break;

                this->exports.refresh = 1;        // No refresh needed !! AD_map_viewer will do the refresh (needed by arb_pars)
                AD_map_viewer(at->gb_node, ADMVT_SELECT);

                if (button == AWT_M_LEFT) goto act_like_group; // now do the same like in AWT_MODE_GROUP
            }
            break;

        case AWT_MODE_EDIT:
            if(type==AW_Mouse_Press && (cl->exists || ct->exists) ){
                GB_transaction ta(tree_static->get_gb_main());
                at = (AP_tree *)(cl->exists ? cl->client_data1 : ct->client_data1);
                if (!at) break;
                AD_map_viewer(at->gb_node,ADMVT_INFO);
            }
            break;
        case AWT_MODE_WWW:
            if(type==AW_Mouse_Press && (cl->exists || ct->exists) ){
                GB_transaction ta(tree_static->get_gb_main());
                at = (AP_tree *)(cl->exists ? cl->client_data1 : ct->client_data1);
                if (!at) break;
                AD_map_viewer(at->gb_node,ADMVT_WWW);
            }
            break;
        default:
            break;
    }
}

void AWT_graphic_tree::set_tree_type(AP_tree_sort type)
{
    if (sort_is_list_style(type)) {
        if (tree_sort == type) { // we are already in wanted view
            nds_show_all = !nds_show_all; // -> toggle between 'marked' and 'all'
        }
        else {
            nds_show_all = true; // default to all
        }
    }
    tree_sort = type;
    switch(type) {
        case AP_TREE_RADIAL:
            exports.dont_fit_x      = 0;
            exports.dont_fit_y      = 0;
            exports.dont_fit_larger = 0;
            exports.left_offset     = 150;
            exports.right_offset    = 150;
            exports.top_offset      = 30;
            exports.bottom_offset   = 30;
            exports.dont_scroll     = 0;
            break;

        case AP_LIST_SIMPLE:
        case AP_LIST_NDS:
            exports.dont_fit_x      = 1;
            exports.dont_fit_y      = 1;
            exports.dont_fit_larger = 0;
            exports.left_offset     = short(2*NT_SELECTED_WIDTH+0.5);
            exports.right_offset    = 300;
            exports.top_offset      = 30;
            exports.bottom_offset   = 30;
            exports.dont_scroll     = 0;
            break;

        case AP_TREE_IRS: // folded dendrogram
            exports.dont_fit_x      = 1;
            exports.dont_fit_y      = 1;
            exports.dont_fit_larger = 0;
            exports.left_offset     = 0;
            exports.right_offset    = 300;
            exports.top_offset      = 30;
            exports.bottom_offset   = 30;
            exports.dont_scroll     = 1;
            break;

        case AP_TREE_NORMAL: // normal dendrogram
            exports.dont_fit_x      = 0;
            exports.dont_fit_y      = 1;
            exports.dont_fit_larger = 0;
            exports.left_offset     = 0;
            exports.right_offset    = 300;
            exports.top_offset      = 30;
            exports.bottom_offset   = 30;
            exports.dont_scroll     = 0;
            break;
    }
}

AWT_graphic_tree::AWT_graphic_tree(AW_root *aw_rooti, GBDATA *gb_maini) : AWT_graphic() {
    line_filter       = AW_SCREEN|AW_CLICK|AW_CLICK_DRAG|AW_SIZE|AW_PRINTER;
    vert_line_filter  = AW_SCREEN|AW_PRINTER;
    text_filter       = AW_SCREEN|AW_CLICK|AW_PRINTER;
    mark_filter       = AW_SCREEN|AW_PRINTER_EXT;
    ruler_filter      = AW_SCREEN|AW_CLICK|AW_PRINTER|AW_SIZE;
    root_filter       = AW_SCREEN|AW_CLICK|AW_PRINTER_EXT;
    set_tree_type(AP_TREE_NORMAL);
    tree_root_display = 0;
    y_pos             = 0;
    tree_proto        = 0;
    link_to_database  = false;
    tree_static       = 0;
    baselinewidth     = 0;
    species_name      = 0;
    this->aw_root     = aw_rooti;
    this->gb_main     = gb_maini;
    rot_ct.exists     = false;
    rot_cl.exists     = false;
    nds_show_all      = true;
}

AWT_graphic_tree::~AWT_graphic_tree() {
    delete tree_proto;
    delete tree_static;
}

void AWT_graphic_tree::init(const AP_tree& tree_prototype, AliView *aliview, AP_sequence *seq_prototype, bool link_to_database_, bool insert_delete_cbs) {
    tree_static = new AP_tree_root(aliview, tree_prototype, seq_prototype, insert_delete_cbs);

    awt_assert(!insert_delete_cbs || link_to_database); // inserting delete callbacks w/o linking to DB has no effect!
    link_to_database = link_to_database_;
}

void AWT_graphic_tree::unload() {
    delete tree_static->get_root_node();
    rot_at            = 0;
    tree_root_display = 0;
}

GB_ERROR AWT_graphic_tree::load(GBDATA *, const char *name, AW_CL /*cl_link_to_database*/, AW_CL /*cl_insert_delete_cbs*/) {
    GB_ERROR error = 0;

    if (name[0] == 0 || strcmp(name, "tree_????") == 0) { // no tree selected
        unload();
        zombies    = 0;
        duplicates = 0;
    }
    else {
        {
            char *name_dup = strdup(name); // name might be freed by unload()
            unload();
            error = tree_static->loadFromDB(name_dup);
            free(name_dup);
        }
        
        if (!error && link_to_database) {
            error = tree_static->linkToDB(&zombies, &duplicates);
        }

        if (error) {
            delete tree_static->get_root_node();
        }
        else {
            tree_root_display = get_root_node();

            get_root_node()->compute_tree(gb_main);

            tree_static->set_root_changed_callback(AWT_graphic_tree_root_changed, this);
            tree_static->set_node_deleted_callback(AWT_graphic_tree_node_deleted, this);
        }
    }

    return error;
}

GB_ERROR AWT_graphic_tree::save(GBDATA */*dummy*/, const char */*name*/, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GB_ERROR error = NULL;
    if (get_root_node()) {
        error = tree_static->saveToDB();
    }
    else if (tree_static && tree_static->get_tree_name()) {
        if (tree_static->gb_tree_gone) {
            GB_transaction ta(gb_main);
            error = GB_delete(tree_static->gb_tree_gone);
            error = ta.close(error);

            if (!error) {
                aw_message(GBS_global_string("Tree '%s' lost all leaves and has been deleted", tree_static->get_tree_name()));
#if defined(DEVEL_RALF)
#warning somehow update selected tree

                // solution: currently selected tree (in NTREE, maybe also in PARSIMONY)
                // needs to add a delete callback on treedata in DB

#endif // DEVEL_RALF
            }

            tree_static->gb_tree_gone = 0; // do not delete twice
        }
    }
    return error;
}

int AWT_graphic_tree::check_update(GBDATA *) {
    AP_UPDATE_FLAGS flags(AP_UPDATE_OK);

    if (tree_static) {
        AP_tree *tree_root = get_root_node();
        if (!tree_root) {
            flags = AP_UPDATE_ERROR;
        }
        else {
            GB_transaction ta(gb_main);

            flags = tree_root->check_update();
            switch (flags) {
                case AP_UPDATE_OK:
                case AP_UPDATE_ERROR:
                    break;

                case AP_UPDATE_RELOADED: {
                    const char *name = tree_static->get_tree_name();
                    if (name) {
                        GB_ERROR error = load(gb_main, name, 1, 0);
                        if (error) aw_message(error);
                        exports.resize = 1;
                    }
                    break;
                }
                case AP_UPDATE_RELINKED: {
                    GB_ERROR error = tree_root->relink();
                    if (!error) tree_root->compute_tree(gb_main);
                    if (error) aw_message(error);
                    break;
                }
            }
        }
    }

    return (int)flags;
}

void AWT_graphic_tree::update(GBDATA *) {
    if (tree_static) {
        AP_tree *root = get_root_node();
        if (root) {
            GB_transaction ta(gb_main);
            root->update();
        }
    }
}

void AWT_graphic_tree::NT_scalebox(int gc, double x, double y, double width)
{
    double diam  = width/disp_device->get_scale();
    double diam2 = diam+diam;
    disp_device->set_fill(gc, this->grey_level);
    disp_device->box(gc, true, x-diam, y-diam, diam2, diam2, mark_filter, 0, 0);
}

void AWT_graphic_tree::NT_emptybox(int gc, double x, double y, double width)
{
    double diam  = width/disp_device->get_scale();
    double diam2 = diam+diam;
    disp_device->set_line_attributes(gc, 0.0, AW_SOLID);
    disp_device->box(gc, false, x-diam, y-diam, diam2, diam2, mark_filter, 0, 0);
}

void AWT_graphic_tree::NT_rotbox(int gc, double x, double y, double width) // box with one corner down
{
    double diam = width / disp_device->get_scale();
    double x1   = x-diam;
    double x2   = x+diam;
    double y1   = y-diam;
    double y2   = y+diam;

    disp_device->line(gc, x1, y, x, y1, mark_filter, 0, 0);
    disp_device->line(gc, x2, y, x, y1, mark_filter, 0, 0);
    disp_device->line(gc, x1, y, x, y2, mark_filter, 0, 0);
    disp_device->line(gc, x2, y, x, y2, mark_filter, 0, 0);
}

bool AWT_show_remark_branch(AW_device *device, const char *remark_branch, bool is_leaf, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    // returns true if a bootstrap was DISPLAYED
    char       *end          = 0;
    int         bootstrap    = int(strtol(remark_branch, &end, 10));
    bool        is_bootstrap = end[0] == '%' && end[1] == 0;
    bool        show         = true;
    const char *text         = 0;

    if (is_bootstrap) {
        if (bootstrap == 100) {
            show           = !is_leaf; // do not show 100% bootstraps at leafs
            if (show) text = "100%";
        }
        else {
            if (bootstrap == 0) {
                text = "<1%"; // show instead of '0%'
            }
            else {
                text = GBS_global_string("%i%%", bootstrap);
            }
        }
    }
    else {
        text = remark_branch;
    }

    if (show) {
        awt_assert(text != 0);
        device->text(AWT_GC_BRANCH_REMARK, text, x, y, alignment, filteri, cd1, cd2);
    }

    return is_bootstrap && show;
}


double AWT_graphic_tree::show_dendrogram(AP_tree *at, double x_father, double x_son)
{
    double ny0, ny1, nx0, nx1, ry, l_min, l_max, xoffset, yoffset;
    AWUSE(x_father);
    ny0 = y_pos;

    if (disp_device->type() != AW_DEVICE_SIZE){ // tree below cliprect bottom can be cut
        AW_pos xs = 0;
        AW_pos ys = y_pos - scaled_branch_distance *2.0;
        AW_pos X,Y;
        disp_device->transform(xs,ys,X,Y);
        if ( Y > disp_device->clip_rect.b){
            y_pos += scaled_branch_distance;
            return ny0;
        }
        ys = y_pos + scaled_branch_distance * (at->gr.view_sum+2);
        disp_device->transform(xs,ys,X,Y);

        if ( Y  < disp_device->clip_rect.t){
            y_pos += scaled_branch_distance*at->gr.view_sum;
            return ny0;
        }
    }

    if (at->is_leaf) {
        /* draw mark box */
        if (at->gb_node && GB_read_flag(at->gb_node)){
            NT_scalebox(at->gr.gc, x_son, ny0, NT_BOX_WIDTH);
        }
        if (at->name &&
            at->name[0] == this->species_name[0] &&
            strcmp(at->name,this->species_name) == 0)
        {
            x_cursor = x_son;
            y_cursor = ny0;
        }

        if (at->name && (disp_device->filter & text_filter) ){
            // display text
            const char                *data     = make_node_text_nds(this->gb_main, at->gb_node,0,at->get_gbt_tree(), tree_static->get_tree_name());
            const AW_font_information *fontinfo = disp_device->get_font_information(at->gr.gc,'A');

#if defined(DEBUG) && 0
            static bool dumped = false;
            if (!dumped) {
                for (int c = 32; c <= 255; c++) {
                    const AW_font_information *fi = disp_device->get_font_information(at->gr.gc,(char)c);
                    printf("fontinfo: %3i '%c' ascent=%2i descent=%2i width=%2i\n",
                           c, (char)c,
                           fi->this_letter.ascent,
                           fi->this_letter.descent,
                           fi->this_letter.width
                           );
                    if (c == 255) {
                        printf("fontinfo (all-maximas): ascent=%2i descent=%2i width=%2i\n",
                               fi->max_all_letter.ascent,
                               fi->max_all_letter.descent,
                               fi->max_all_letter.width
                               );
                        printf("fontinfo (ascii-maximas): ascent=%2i descent=%2i width=%2i\n",
                               fi->max_letter.ascent,
                               fi->max_letter.descent,
                               fi->max_letter.width
                               );
                    }
                }

                dumped = true;
            }
#endif // DEBUG


            double unscale = 1.0/disp_device->get_scale();
            yoffset = scaled_font.ascent*.5;
            xoffset = ((fontinfo->max_letter.width * 0.5) + NT_BOX_WIDTH) * unscale;

            disp_device->text(at->gr.gc,data ,
                              (AW_pos) x_son+xoffset,(AW_pos) ny0+yoffset,
                              (AW_pos) 0 , text_filter,
                              (AW_CL) at , (AW_CL) 0 );
        }


        y_pos += scaled_branch_distance;
        return ny0;
    }

    if (at->gr.grouped) {
        l_min = at->gr.min_tree_depth;
        l_max = at->gr.tree_depth;
        ny1 = y_pos += scaled_branch_distance*(at->gr.view_sum-1);
        nx0 = x_son + l_max;
        nx1 = x_son + l_min;

        int linewidth = 0;
        if (at->father) {
            AP_tree *at_fath = at->get_father();
            if (at_fath->leftson == at) linewidth = at_fath->gr.left_linewidth;
            else                        linewidth = at_fath->gr.right_linewidth;
        }

        disp_device->set_line_attributes(at->gr.gc,linewidth+baselinewidth,AW_SOLID);

        AW_pos q[8];
        q[0] = x_son;   q[1] = ny0;
        q[2] = x_son;   q[3] = ny1;
        q[4] = nx1;     q[5] = ny1;
        q[6] = nx0;     q[7] = ny0;

        disp_device->set_fill(at->gr.gc, grey_level);
        disp_device->filled_area(at->gr.gc, 4, &q[0], line_filter, (AW_CL)at,0);

        const AW_font_information *fontinfo    = disp_device->get_font_information(at->gr.gc,'A');
        double                     text_ascent = fontinfo->max_letter.ascent/ disp_device->get_scale() ;

        yoffset = (ny1-ny0+text_ascent)*.5;
        xoffset = text_ascent*.5;

        if (at->gb_node && (disp_device->filter & text_filter)){
            const char *data = make_node_text_nds(this->gb_main, at->gb_node,0,at->get_gbt_tree(), tree_static->get_tree_name());

            disp_device->text(at->gr.gc,data ,
                              (AW_pos) nx0+xoffset,(AW_pos) ny0+yoffset,
                              (AW_pos) 0 , text_filter,
                              (AW_CL) at , (AW_CL) 0 );
        }
        disp_device->text(at->gr.gc,(char *)GBS_global_string(" %i",at->gr.leave_sum),
                          x_son+xoffset, ny0+yoffset,
                          0,text_filter,(AW_CL)at,0);

        y_pos += scaled_branch_distance;

        return (ny0+ny1)*.5;
    }
    nx0 = (x_son +  at->leftlen) ;
    nx1 = (x_son +  at->rightlen) ;
    ny0 = show_dendrogram(at->get_leftson(), x_son, nx0);
    ry  = y_pos - .5*scaled_branch_distance;
    ny1 = show_dendrogram(at->get_rightson(), x_son, nx1);

    if (at->name) {
        NT_rotbox(at->gr.gc,x_son, ry, NT_BOX_WIDTH*2);
    }

    if (at->leftson->remark_branch ) {
        bool bootstrap_shown = AWT_show_remark_branch(disp_device, at->leftson->remark_branch, at->leftson->is_leaf, nx0, ny0-scaled_font.ascent*0.1, 1, text_filter, (AW_CL)at, 0);

        if (show_circle && bootstrap_shown){
            AWT_show_bootstrap_circle(disp_device,at->leftson->remark_branch, circle_zoom_factor, circle_max_size, at->leftlen,nx0, ny0, use_ellipse, scaled_branch_distance, text_filter, (AW_CL) at->leftson, (AW_CL) 0);
        }
    }

    if (at->rightson->remark_branch ) {
        bool bootstrap_shown = AWT_show_remark_branch(disp_device, at->rightson->remark_branch, at->rightson->is_leaf, nx1, ny1-scaled_font.ascent*0.1, 1, text_filter, (AW_CL)at, 0);

        if (show_circle && bootstrap_shown){
            AWT_show_bootstrap_circle(disp_device,at->rightson->remark_branch,circle_zoom_factor, circle_max_size, at->rightlen,nx1, ny1, use_ellipse, scaled_branch_distance, text_filter, (AW_CL) at->rightson, (AW_CL) 0);
        }
    }

#if defined(DEBUG) && 0
    // display group-node-address
    if (at->gb_node) {
        disp_device->text(at->gr.gc, GBS_global_string("%p(gb_node)", at->gb_node), x_father, (ny0+ny1)/2, 0, text_filter, (AW_CL)at, 0);
    }
#endif // DEBUG


    int          lw = at->gr.left_linewidth+baselinewidth;
    unsigned int gc = at->get_leftson()->gr.gc;
    
    disp_device->set_line_attributes(gc, lw, AW_SOLID);
    disp_device->line(gc, x_son, ny0, nx0,   ny0, line_filter,      (AW_CL)at->leftson, 0);
    disp_device->line(gc, x_son, ny0, x_son, ry,  vert_line_filter, (AW_CL)at,          0);

    lw = at->gr.right_linewidth+baselinewidth;
    gc = at->get_rightson()->gr.gc;

    disp_device->set_line_attributes(gc, lw, AW_SOLID);
    disp_device->line(gc, x_son, ny1, nx1,   ny1, line_filter,      (AW_CL)at->rightson, 0);
    disp_device->line(gc, x_son, ry,  x_son, ny1, vert_line_filter, (AW_CL)at,           0);

    return ry;
}


void AWT_graphic_tree::scale_text_koordinaten(AW_device *device, int gc,double& x,double& y,double orientation,int flag)
{
    const AW_font_information *fontinfo    = device->get_font_information(gc,'A');
    double                     text_height = fontinfo->max_letter.height/ disp_device->get_scale() ;
    double                     dist        = fontinfo->max_letter.height/ disp_device->get_scale();
    
    if (flag==1) {
        dist += 1;
    }
    else {
        x += cos(orientation) * dist;
        y += sin(orientation) * dist + 0.3*text_height;
    }
    return;
}


//  ********* shell and radial tree
void AWT_graphic_tree::show_radial_tree(AP_tree * at, double x_center,
                                        double y_center, double tree_spread,double tree_orientation,
                                        double x_root, double y_root, int linewidth)
{
    double l,r,w,z,l_min,l_max;

    disp_device->set_line_attributes(at->gr.gc,linewidth+baselinewidth,AW_SOLID);
    disp_device->line(at->gr.gc, x_root, y_root, x_center, y_center, line_filter, (AW_CL)at,0);

    // draw mark box
    if (at->gb_node && GB_read_flag(at->gb_node)) {
        NT_scalebox(at->gr.gc, x_center, y_center, NT_BOX_WIDTH);
    }

    if( at->is_leaf){
        if (at->name && (disp_device->filter & text_filter) ){
            if (at->name[0] == this->species_name[0] &&
                !strcmp(at->name,this->species_name)) {
                x_cursor = x_center; y_cursor = y_center;
            }
            scale_text_koordinaten(disp_device,at->gr.gc, x_center,y_center,tree_orientation,0);
            const char *data =  make_node_text_nds(this->gb_main,at->gb_node,0,at->get_gbt_tree(), tree_static->get_tree_name());
            disp_device->text(at->gr.gc,data,
                              (AW_pos)x_center,(AW_pos) y_center,
                              (AW_pos) .5 - .5 * cos(tree_orientation),
                              text_filter,  (AW_CL) at , (AW_CL) 0 );
        }
        return;
    }

    if( at->gr.grouped){
        l_min = at->gr.min_tree_depth;
        l_max = at->gr.tree_depth;

        r    = l = 0.5;
        AW_pos q[6];
        q[0] = x_center;
        q[1] = y_center;
        w    = tree_orientation + r*0.5*tree_spread+ at->gr.right_angle;
        q[2] = x_center+l_min*cos(w);
        q[3] = y_center+l_min*sin(w);
        w    = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
        q[4] = x_center+l_max*cos(w);
        q[5] = y_center+l_max*sin(w);

        disp_device->set_fill(at->gr.gc, grey_level);
        disp_device->filled_area(at->gr.gc, 3, &q[0], line_filter, (AW_CL)at,0);

        if (at->gb_node && (disp_device->filter & text_filter) ){
            w = tree_orientation + at->gr.right_angle;
            l_max = (l_max+l_min)*.5;
            x_center= x_center+l_max*cos(w);
            y_center= y_center+l_max*sin(w);
            scale_text_koordinaten(disp_device,at->gr.gc,x_center,y_center,w,0);

            /* insert text (e.g. name of group) */
            const char *data = make_node_text_nds(this->gb_main, at->gb_node,0,at->get_gbt_tree(), tree_static->get_tree_name());
            disp_device->text(at->gr.gc,data,
                              (AW_pos)x_center,(AW_pos) y_center,
                              (AW_pos).5 - .5 * cos(tree_orientation),
                              text_filter,
                              (AW_CL) at , (AW_CL) 0 );
        }
        return;
    }
    l = (double) at->get_leftson()->gr.view_sum / (double)at->gr.view_sum;
    r = 1.0 - (double)l;

    {
        AP_tree *at_leftson  = at->get_leftson();
        AP_tree *at_rightson = at->get_rightson();

        if (at_leftson->gr.gc > at_rightson->gr.gc) {
            // bring selected gc to front

            /*** left branch ***/
            w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
            z = at->leftlen;
            show_radial_tree(at_leftson,
                             x_center+ z * cos(w),
                             y_center+ z * sin(w),
                             (at->leftson->is_leaf) ? 1.0 : tree_spread * l * at_leftson->gr.spread,
                             w,
                             x_center, y_center, at->gr.left_linewidth );

            /*** right branch ***/
            w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
            z = at->rightlen;
            show_radial_tree(at_rightson,
                             x_center+ z * cos(w),
                             y_center+ z * sin(w),
                             (at->rightson->is_leaf) ? 1.0 : tree_spread * r * at_rightson->gr.spread,
                             w,
                             x_center, y_center, at->gr.right_linewidth);
        }
        else {
            /*** right branch ***/
            w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
            z = at->rightlen;
            show_radial_tree(at_rightson,
                             x_center+ z * cos(w),
                             y_center+ z * sin(w),
                             (at->rightson->is_leaf) ? 1.0 : tree_spread * r * at_rightson->gr.spread,
                             w,
                             x_center, y_center, at->gr.right_linewidth);

            /*** left branch ***/
            w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
            z = at->leftlen;
            show_radial_tree(at_leftson,
                             x_center+ z * cos(w),
                             y_center+ z * sin(w),
                             (at->leftson->is_leaf) ? 1.0 : tree_spread * l * at_leftson->gr.spread,
                             w,
                             x_center, y_center, at->gr.left_linewidth );
        }
    }
    if (show_circle){
        /*** left branch ***/
        if (at->leftson->remark_branch){
            w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
            z = at->leftlen * .5;
            AWT_show_bootstrap_circle(disp_device,at->leftson->remark_branch,circle_zoom_factor, circle_max_size, at->leftlen,x_center+ z * cos(w),y_center+ z * sin(w), false, 0, text_filter, (AW_CL)at,0);
        }
        if (at->rightson->remark_branch){
            /*** right branch ***/
            w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
            z = at->rightlen * .5;
            AWT_show_bootstrap_circle(disp_device,at->rightson->remark_branch,circle_zoom_factor, circle_max_size, at->rightlen,x_center+ z * cos(w),y_center+ z * sin(w), false, 0, text_filter, (AW_CL)at,0);
        }
    }
}

const char *AWT_graphic_tree::show_ruler(AW_device *device, int gc) {
    const char *tree_awar = 0;

    char    awar[256];
    GBDATA *gb_tree = tree_static->get_gb_tree();

    if (!gb_tree) return 0;                         // no tree -> no ruler
    
    GB_transaction dummy(gb_tree);

    sprintf(awar,"ruler/size");
    float ruler_size = *GBT_readOrCreate_float(gb_tree, awar, 0.1);
    float ruler_x = 0.0;
    float ruler_y = 0.0;
    float ruler_text_x = 0.0;
    float ruler_text_y = 0.0;
    float ruler_add_y = 0.0;
    float ruler_add_x = 0.0;

    switch (tree_sort) {
        case AP_TREE_NORMAL:
            tree_awar = "LIST";
            break;
        case AP_TREE_RADIAL:
            tree_awar = "RADIAL";
            break;
        case AP_TREE_IRS:
            tree_awar = "IRS";
            break;
        case AP_LIST_SIMPLE:
        case AP_LIST_NDS:
            // rulers not allowed in these display modes
            awt_assert(0);
            tree_awar = 0;
            break;
    }

    if (tree_awar) {
        sprintf(awar,"ruler/%s/ruler_y",tree_awar);
        if (!GB_search(gb_tree,awar,GB_FIND)){
            if (device->type() == AW_DEVICE_SIZE) {
                AW_world world;
                device ->get_size_information(&world);
                ruler_y = world.b * 1.3;
            }
        }

        double half_ruler_width = ruler_size*0.5;

        switch (tree_sort) {
            case AP_TREE_IRS:
                // scale is different for IRS tree -> adjust:
                half_ruler_width *= irs_tree_ruler_scale_factor;
                ruler_y     = 0;
                ruler_add_y = this->list_tree_ruler_y;
                ruler_add_x = -half_ruler_width;
                break;
            case AP_TREE_NORMAL:
                ruler_y     = 0;
                ruler_add_y = this->list_tree_ruler_y;
                ruler_add_x = half_ruler_width;
                break;
            default:
                break;
        }


        ruler_y = ruler_add_y + *GBT_readOrCreate_float(gb_tree, awar, ruler_y);

        sprintf(awar,"ruler/%s/ruler_x",tree_awar);
        ruler_x = ruler_add_x + *GBT_readOrCreate_float(gb_tree, awar, ruler_x);

        sprintf(awar,"ruler/%s/text_x", tree_awar);
        ruler_text_x = *GBT_readOrCreate_float(gb_tree, awar, ruler_text_x);

        sprintf(awar,"ruler/%s/text_y", tree_awar);
        ruler_text_y = *GBT_readOrCreate_float(gb_tree, awar, ruler_text_y);

        sprintf(awar,"ruler/ruler_width");
        double ruler_width = *GBT_readOrCreate_int(gb_tree, awar, 0);

        device->set_line_attributes(gc, ruler_width+baselinewidth, AW_SOLID);

        device->line(gc,
                     ruler_x - half_ruler_width, ruler_y,
                     ruler_x + half_ruler_width, ruler_y,
                     this->ruler_filter,
                     0, (AW_CL)"ruler");
        char ruler_text[20];
        sprintf(ruler_text,"%4.2f",ruler_size);

        device->text(gc, ruler_text,
                     ruler_x + ruler_text_x,
                     ruler_y + ruler_text_y,
                     0.5,
                     this->ruler_filter & ~AW_SIZE,
                     0, (AW_CL)"ruler");
    }
    return tree_awar;
}

void AWT_graphic_tree::show_nds_list(GBDATA * dummy, bool use_nds)
{
    AWUSE(dummy);
    AW_pos y_position = scaled_branch_distance;
    AW_pos x_position = 2*NT_SELECTED_WIDTH / disp_device->get_scale();
    long   max_strlen = 0;

    disp_device->text(nds_show_all ? AWT_GC_CURSOR : AWT_GC_SELECTED,
                      GBS_global_string("%s of %s species", use_nds ? "NDS List" : "Simple list", nds_show_all ? "all" : "marked"),
                      (AW_pos) x_position, (AW_pos) 0,
                      (AW_pos) 0, text_filter,
                      (AW_CL) 0, (AW_CL) 0);

    const char *tree_name = tree_static ? tree_static->get_tree_name() : NULL;

    for (GBDATA *gb_species = nds_show_all ? GBT_first_species(gb_main) : GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = nds_show_all ? GBT_next_species(gb_species) : GBT_next_marked_species(gb_species))
    {
        y_position += scaled_branch_distance;

        const char *name = GBT_read_name(gb_species);

        if (strcmp(name, this->species_name) == 0) {
            x_cursor = 0;
            y_cursor = y_position;
        }

        bool is_marked = GB_read_flag(gb_species);
        if (is_marked) NT_scalebox(AWT_GC_SELECTED, 0, y_position, NT_BOX_WIDTH);

        {
            AW_pos xs = 0;
            AW_pos X,Y;
            disp_device->transform(xs, y_position+scaled_branch_distance, X, Y);
            if ( Y  < disp_device->clip_rect.t) continue;
            disp_device->transform(xs, y_position-scaled_branch_distance, X, Y);
            if ( Y > disp_device->clip_rect.b) continue;
        }

        if (disp_device->type() != AW_DEVICE_SIZE){ // tree below cliprect bottom can be cut
            const char *data;
            if (use_nds) {
                data = make_node_text_nds(gb_main, gb_species, 1, 0, tree_name);
            }
            else {
                data = name;
            }

            long slen = strlen(data);
            int  gc   = AWT_GC_NSELECTED;

            if (nds_show_all && is_marked) {
                gc = AWT_GC_SELECTED;
            }
            else {
                int color_group     = AWT_species_get_dominant_color(gb_species);
                if (color_group) gc = AWT_GC_FIRST_COLOR_GROUP+color_group-1;
            }

            disp_device->text(gc, data,
                              (AW_pos) x_position, (AW_pos) y_position + scaled_font.ascent*.5,
                              (AW_pos) 0, text_filter,
                              (AW_CL) gb_species, (AW_CL) "species", slen);

            if (slen> max_strlen) max_strlen = slen;
        }
    }

    disp_device->invisible(AWT_GC_CURSOR, 0, 0, -1, 0, 0);
    disp_device->invisible(AWT_GC_CURSOR, max_strlen*scaled_font.width, y_position+scaled_branch_distance, -1, 0, 0);
}

void AWT_graphic_tree::show(AW_device *device)  {
    if (tree_static && tree_static->get_gb_tree()) {
        check_update(gb_main);
    }

    disp_device = device;

    const AW_font_information *fontinfo = disp_device->get_font_information(AWT_GC_SELECTED, 0);

    scaled_font.init(fontinfo->max_letter, 1/device->get_scale());
    scaled_branch_distance = scaled_font.height * aw_root->awar(AWAR_DTREE_VERICAL_DIST)->read_float();

    make_node_text_init(gb_main);

    grey_level         = aw_root->awar(AWAR_DTREE_GREY_LEVEL)->read_int()*.01;
    baselinewidth      = (int)aw_root->awar(AWAR_DTREE_BASELINEWIDTH)->read_int();
    show_circle        = (int)aw_root->awar(AWAR_DTREE_SHOW_CIRCLE)->read_int();
    circle_zoom_factor = aw_root->awar(AWAR_DTREE_CIRCLE_ZOOM)->read_float();
    circle_max_size    = aw_root->awar(AWAR_DTREE_CIRCLE_MAX_SIZE)->read_float();
    use_ellipse        = aw_root->awar(AWAR_DTREE_USE_ELLIPSE)->read_int();

    freeset(species_name, aw_root->awar(AWAR_SPECIES_NAME)->read_string());

    x_cursor = y_cursor = 0.0;

    if (!tree_root_display && sort_is_tree_style(tree_sort)) { // if there is no tree, but display style needs tree
        static const char *no_tree_text[] = {
            "No tree (selected)",
            "",
            "In the top area you may click on",
            "- the listview-button to see a plain list of species",
            "- the tree-selection-button to select a tree",
            NULL
        };

        double y_start = y_cursor - 2*scaled_branch_distance;
        for (int i = 0; no_tree_text[i]; ++i) {
            device->text(AWT_GC_CURSOR, no_tree_text[i], x_cursor, y_cursor);
            y_cursor += scaled_branch_distance;
        }
        double y_end = y_cursor;

        x_cursor -= scaled_branch_distance; 
        device->line(AWT_GC_CURSOR, x_cursor, y_start, x_cursor, y_end);
    }
    else {
        switch (tree_sort) {
            case AP_TREE_NORMAL:
                y_pos             = 0.05;
                show_dendrogram(tree_root_display, 0, 0);
                list_tree_ruler_y = y_pos + 2.0 * scaled_branch_distance;
                break;

            case AP_TREE_RADIAL:
                NT_emptybox(tree_root_display->gr.gc, 0, 0, NT_ROOT_WIDTH);
                show_radial_tree(tree_root_display, 0,0,2*M_PI, 0.0,0, 0, tree_root_display->gr.left_linewidth);
                break;

            case AP_TREE_IRS:
                show_irs_tree(tree_root_display,disp_device,fontinfo->max_letter.height);
                list_tree_ruler_y = y_pos;
                break;

            case AP_LIST_NDS:       // this is the list all/marked species mode (no tree)
                show_nds_list(gb_main, true);
                break;

            case AP_LIST_SIMPLE:    // simple list of names (used at startup only)
                show_nds_list(gb_main, false);
                break;            
        }
        if (x_cursor != 0.0 || y_cursor != 0.0) {
            NT_emptybox(AWT_GC_CURSOR, x_cursor, y_cursor, NT_SELECTED_WIDTH);
        }
        if (sort_is_tree_style(tree_sort)) { // show rulers in tree-style display modes
            show_ruler(device, AWT_GC_CURSOR);
        }
    }
}

void AWT_graphic_tree::info(AW_device *device, AW_pos x, AW_pos y,
                            AW_clicked_line *cl, AW_clicked_text *ct)
{
    aw_message("INFO MESSAGE");
    AWUSE(device);
    AWUSE(x);
    AWUSE(y);
    AWUSE(cl);
    AWUSE(ct);

}

AWT_graphic_tree *NT_generate_tree(AW_root *root, GBDATA *gb_main) {
    AWT_graphic_tree *apdt    = new AWT_graphic_tree(root, gb_main);
    apdt->init(AP_tree(0), new AliView(gb_main), NULL, true, false); // tree w/o sequence data
    return apdt;
}

void awt_create_dtree_awars(AW_root *aw_root,AW_default def)
{
    aw_root->awar_int(AWAR_DTREE_BASELINEWIDTH,1,def)->set_minmax(1, 10);
    aw_root->awar_float(AWAR_DTREE_VERICAL_DIST,1.0,def)->set_minmax(0.01,30);
    aw_root->awar_int(AWAR_DTREE_AUTO_JUMP,1,def);

    aw_root->awar_int(AWAR_DTREE_SHOW_CIRCLE,0,def);
    aw_root->awar_int(AWAR_DTREE_USE_ELLIPSE, 1, def);

    aw_root->awar_float(AWAR_DTREE_CIRCLE_ZOOM,1.0,def)     ->set_minmax(0.01,20);
    aw_root->awar_float(AWAR_DTREE_CIRCLE_MAX_SIZE,1.5,def) ->set_minmax(0.01,200);
    aw_root->awar_int(AWAR_DTREE_GREY_LEVEL,20,def)         ->set_minmax(0,100);

    aw_root->awar_int(AWAR_DTREE_REFRESH,0,def);
}


