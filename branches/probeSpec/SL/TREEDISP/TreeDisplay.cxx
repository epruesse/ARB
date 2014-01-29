// =============================================================== //
//                                                                 //
//   File      : TreeDisplay.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "TreeDisplay.hxx"

#include <nds.h>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>

#include <awt_attributes.hxx>
#include <arb_defs.h>
#include <arb_strarray.h>

#include <unistd.h>
#include <iostream>
#include <arb_global_defs.h>

/*!*************************
  class AP_tree
****************************/

AW_color_idx  MatchProbeColourIndex[16] = {AW_WINDOW_BG};
char*         MatchProbeColours[16]     = {"#77211F",
                                           "#75771F",
                                           "#1F7726",
                                           "#2D9495",
                                           "#526ACD",
                                           "#8F4DE7",
                                           "#AC60AB",
                                           "#A91759",
                                           "#996417",
                                           "#58832C",
                                           "#27934A",
                                           "#276293",
                                           "#404DD7",
                                           "#9840D7",
                                           "#CA69AD",
                                           "#AD4A4D"};

const int MATCH_COL_WIDTH = 3;

using namespace AW;

AW_gc_manager AWT_graphic_tree::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2)
{
    AW_gc_manager gc_manager =
        AW_manage_GC(aww,
                     ntw->get_gc_base_name(),
                     device, AWT_GC_CURSOR, AWT_GC_MAX, AW_GCM_DATA_AREA,
                     makeWindowCallback(AWT_resize_cb, ntw, cd2),
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
                     NULL);

    // Add colours for identifying probes in multi-probe matching
    unsigned long colour_index = aww->color_table_size - AW_STD_COLOR_IDX_MAX;

    for (int cn = 0 ; cn < 16 ; cn++)
    {
      MatchProbeColourIndex[cn] = aww->alloc_named_data_color(colour_index, MatchProbeColours[cn]);

      colour_index++;
    }

    return gc_manager;
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

    at = search(at, name);
    if (!at) return;
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
        if (at->gb_node) {      // not a zombie
            switch (mark_mode) {
                case 0: GB_write_flag(at->gb_node, 0); break;
                case 1: GB_write_flag(at->gb_node, 1); break;
                case 2: GB_write_flag(at->gb_node, !GB_read_flag(at->gb_node)); break;
                default: td_assert(0);
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
        if (at->gb_node) {      // not a zombie
            int oldMark = GB_read_flag(at->gb_node);
            if (oldMark != mark_mode && condition(at->gb_node, cd)) {
                switch (mark_mode) {
                    case 0: GB_write_flag(at->gb_node, 0); break;
                    case 1: GB_write_flag(at->gb_node, 1); break;
                    case 2: GB_write_flag(at->gb_node, !oldMark); break;
                    default: td_assert(0);
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

    void dump() {
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
        return;                 // leafs never get grouped
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
                if (strlen(GB_read_char_pntr(gn))>0) { // and I have a name
                    at->gr.grouped     = 1;
                    if (mode & 2) flag = 1; // do not group non-terminal groups
                }
            }
        }
    }
    if (!at->father) get_root_node()->compute_tree(tree_static->get_gb_main());

    return flag;
}



int AWT_graphic_tree::resort_tree(int mode, AP_tree *at)   // run on father !!!
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
        if (!at) return 0;
        at->arb_tree_set_leafsum_viewsum();

        this->resort_tree(mode, at);
        at->compute_tree(this->gb_main);
        return 0;
    }

    if (at->is_leaf) {
        leafname = at->name;
        return 1;
    }
    int leftsize  = at->get_leftson() ->gr.leaf_sum;
    int rightsize = at->get_rightson()->gr.leaf_sum;

    if ((mode &1) == 0) {   // to top
        if (rightsize >leftsize) {
            at->swap_sons();
        }
    }
    else {
        if (rightsize < leftsize) {
            at->swap_sons();
        }
    }

    int lmode = mode;
    int rmode = mode;
    if ((mode & 2) == 2) {
        lmode = 2;
        rmode = 3;
    }

    resort_tree(lmode, at->get_leftson());
    td_assert(leafname);
    const char *leftleafname = leafname;

    resort_tree(rmode, at->get_rightson());
    td_assert(leafname);
    const char *rightleafname = leafname;

    td_assert(leftleafname && rightleafname);

    if (leftleafname && rightleafname) {
        int name_cmp = strcmp(leftleafname, rightleafname);
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


void AWT_graphic_tree::rot_show_line(AW_device *device) {
    double sx = (old_rot_cl.x0+old_rot_cl.x1)*.5;
    double sy = (old_rot_cl.y0+old_rot_cl.y1)*.5;
    double x  = rot_cl.x0 * (1.0-rot_cl.nearest_rel_pos) + rot_cl.x1 * rot_cl.nearest_rel_pos;
    double y  = rot_cl.y0 * (1.0-rot_cl.nearest_rel_pos) + rot_cl.y1 * rot_cl.nearest_rel_pos;
    device->line(drag_gc, sx, sy, x, y);
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

    device->line(gc, sx, sy, x1, y1);


    if (!at->is_leaf) {
        sx = x1; sy = y1;
        len = at->gr.tree_depth;
        w = this->rot_orientation - this->rot_spread*.5*.5 + at->gr.right_angle;
        x1 = sx + cos(w) * len;
        y1 = sy + sin(w) * len;
        w = this->rot_orientation + this->rot_spread*.5*.5 + at->gr.left_angle;
        x2 = sx + cos(w) * len;
        y2 = sy + sin(w) * len;

        device->line(gc, sx, sy, x1, y1);
        device->line(gc, sx, sy, x2, y2);
        device->line(gc, x2, y2, x1, y1);
    }
}

static void show_bootstrap_circle(AW_device *device, const char *bootstrap, double zoom_factor, double max_radius, double len, const Position& center, bool elipsoid, double elip_ysize, int filter) {
    double radius           = .01 * atoi(bootstrap); // bootstrap values are given in % (0..100)
    if (radius < .1) radius = .1;

    radius  = 1.0 / sqrt(radius); // -> bootstrap->radius : 100% -> 1, 0% -> inf
    radius -= 1.0;              // -> bootstrap->radius : 100% -> 0, 0% -> inf
    radius *= 2; // diameter ?

    // Note : radius goes against infinite, if bootstrap values go against zero
    //        For this reason we do some limitation here:
#define BOOTSTRAP_RADIUS_LIMIT max_radius

    int gc = AWT_GC_BOOTSTRAP;

    if (radius > BOOTSTRAP_RADIUS_LIMIT) {
        radius = BOOTSTRAP_RADIUS_LIMIT;
        gc     = AWT_GC_BOOTSTRAP_LIMITED;
    }

    double radiusx = radius * len * zoom_factor;     // multiply with length of branch (and zoomfactor)
    if (radiusx<0 || nearlyZero(radiusx)) return;    // skip too small circles

    double radiusy;
    if (elipsoid) {
        radiusy = elip_ysize * zoom_factor;
        if (radiusy > radiusx) radiusy = radiusx;
    }
    else {
        radiusy = radiusx;
    }

    device->circle(gc, false, center, Vector(radiusx, radiusy), filter);
    // device->arc(gc, false, center, Vector(radiusx, radiusy), 45, -90, filter); // @@@ use to test AW_device_print::arc_impl
}

static double comp_rot_orientation(AW_clicked_line *cl)
{
    return atan2(cl->y1 - cl->y0, cl->x1 - cl->x0);
}

static double comp_rot_spread(AP_tree *at, AWT_graphic_tree *ntw)
{
    double zw = 0;
    AP_tree *bt;

    if (!at) return 0;

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
            td_assert(0);
    }

    return zw;
}

static void AWT_graphic_tree_root_changed(void *cd, AP_tree *old, AP_tree *newroot) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    if (agt->tree_root_display == old || agt->tree_root_display->is_inside(old)) {
        agt->tree_root_display = newroot;
    }
}

static void AWT_graphic_tree_node_deleted(void *cd, AP_tree *del) {
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
            const char *msg = GBS_global_string("What to do with group '%s'?", gname);

            switch (aw_question(NULL, msg, "Rename,Destroy,Cancel")) {
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

/**
 * Called from AWT_graphic_tree::command() to handle AW_Event_Type=AW_Keyboard_Press.
 */
void AWT_graphic_tree::key_command(AWT_COMMAND_MODE /* cmd */, AW_key_mod key_modifier, char key_char,
                                   AW_pos /* x */, AW_pos /* y */, AW_clicked_line *cl, AW_clicked_text *ct)
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

    bool global_key = true;
    switch (key_char) {
        case 9: {     // Ctrl-i = invert all
            GBT_mark_all(gb_main, 2);
            break;
        }
        case 13: {     // Ctrl-m = mark/unmark all
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
                td_assert(0);
            }

            // ------------------------------------
            //      commands in species list :

            switch (key_char) {
                case 'i':
                case 'm': {     // i/m = mark/unmark species
                    GB_write_flag(gb_species, 1-GB_read_flag(gb_species));
                    break;
                }
                case 'I': {     // I = invert all but current
                    int mark = GB_read_flag(gb_species);
                    GBT_mark_all(gb_main, 2);
                    GB_write_flag(gb_species, mark);
                    break;
                }
                case 'M': {     // M = mark/unmark all but current
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

            if (at) {
                switch (key_char) {
                    case 'm': {     // m = mark/unmark (sub)tree
                        mark_species_in_tree(at, !tree_has_marks(at));
                        break;
                    }
                    case 'M': {     // M = mark/unmark all but (sub)tree
                        mark_species_in_rest_of_tree(at, !rest_tree_has_marks(at));
                        break;
                    }

                    case 'i': {     // i = invert (sub)tree
                        mark_species_in_tree(at, 2);
                        break;
                    }
                    case 'I': {     // I = invert all but (sub)tree
                        mark_species_in_rest_of_tree(at, 2);
                        break;
                    }
                    case 'c':
                    case 'x': {
                        AWT_graphic_tree_group_state state;
                        detect_group_state(at, &state, 0);

                        if (!state.has_groups()) { // somewhere inside group
                        do_parent :
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

                            if (key_char == 'x') {  // expand
                                next_group_mode = state.next_expand_mode();
                            }
                            else { // collapse
                                if (state.all_closed()) goto do_parent;
                                next_group_mode = state.next_collapse_mode();
                            }

                            // int result =
                            group_tree(at, next_group_mode, 0);

                            Save    = true;
                            resize  = true;
                            compute = true;
                        }
                        break;
                    }
                    case 'C':
                    case 'X': {
                        AP_tree *root_node                  = at;
                        while (root_node->father) root_node = root_node->get_father(); // search father

                        td_assert(root_node);

                        AWT_graphic_tree_group_state state;
                        detect_group_state(root_node, &state, at);

                        int next_group_mode;
                        if (key_char == 'X') {  // expand
                            next_group_mode = state.next_expand_mode();
                        }
                        else { // collapse
                            next_group_mode = state.next_collapse_mode();
                        }

                        // int result =
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

static bool command_on_GBDATA(GBDATA *gbd, AWT_COMMAND_MODE cmd, AW_event_type type, int button, AD_map_viewer_cb map_viewer_cb) {
    // modes listed here are available in ALL tree-display-modes (i.e. as well in listmode)

    bool refresh = false;

    if (type == AW_Mouse_Press && button != AW_BUTTON_MIDDLE) {
        bool select = false;

        switch (cmd) {
            case AWT_MODE_WWW:
                map_viewer_cb(gbd, ADMVT_WWW);
                break;

            case AWT_MODE_MARK:
                switch (button) {
                    case AW_BUTTON_LEFT:
                        GB_write_flag(gbd, 1);
                        select  = true;
                        break;
                    case AW_BUTTON_RIGHT:
                        GB_write_flag(gbd, 0);
                        break;
                }
                refresh = true;
                break;

            default :
                select = true;
                break;
        }

        if (select) {
            map_viewer_cb(gbd, ADMVT_INFO);
        }
    }


    return refresh;
}

/**
 * Handle input_events
 *
 * Overrides pure virtual in AWT_graphic, which in turn is called by the
 * input_event() handler registered by AWT_graphic.
 */
void AWT_graphic_tree::command(AW_device *device, AWT_COMMAND_MODE cmd,
                               int button, AW_key_mod key_modifier, AW_key_code /* key_code */, char key_char,
                               AW_event_type type, AW_pos x, AW_pos y,
                               AW_clicked_line *cl, AW_clicked_text *ct)
{
    static int rot_drag_flag, bl_drag_flag;

    if((type   == AW_Mouse_Press) &&
       (button == AW_BUTTON_LEFT))
    {
        clickNotifyWhichProbe(device, x, y);
    }

    if (type == AW_Keyboard_Press) {
        return key_command(cmd, key_modifier, key_char, x, y, cl, ct);
    }

    if (ct->exists && ct->client_data2 && !strcmp((char *) ct->client_data2, "species")) {
        // clicked on a species in list-mode:
        GBDATA *gbd = (GBDATA *)ct->client_data1;
        if (command_on_GBDATA(gbd, cmd, type, button, map_viewer_cb)) {
            exports.refresh = 1;
        }
        return;
    }

    if (!this->tree_static) return;     // no tree -> no commands

    GBDATA *gb_tree = tree_static->get_gb_tree();

    if ((rot_ct.exists && (rot_ct.distance == 0) && (!rot_ct.client_data1) &&
          rot_ct.client_data2 && !strcmp((char *) rot_ct.client_data2, "ruler")) ||
         (ct && ct->exists && (!ct->client_data1) &&
          ct->client_data2 && !strcmp((char *) ct->client_data2, "ruler")))
    {
        const char *tree_awar;
        char awar[256];
        float h;
        switch (cmd) {
            case AWT_MODE_SELECT:
            case AWT_MODE_ROT:
            case AWT_MODE_SPREAD:
            case AWT_MODE_SWAP:
            case AWT_MODE_SETROOT:
            case AWT_MODE_LENGTH:
            case AWT_MODE_MOVE: // Move Ruler text
                switch (type) {
                    case AW_Mouse_Press:
                        rot_ct          = *ct;
                        rot_ct.textArea.moveTo(Position(x, y));
                        break;
                    case AW_Mouse_Drag: {
                        double          scale = device->get_scale();
                        const Position &start = rot_ct.textArea.start();

                        tree_awar    = show_ruler(device, drag_gc);
                        sprintf(awar, "ruler/%s/text_x", tree_awar);

                        h = (x - start.xpos())/scale + *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                        GBT_write_float(gb_tree, awar, h);
                        sprintf(awar, "ruler/%s/text_y", tree_awar);
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
         rot_cl.client_data2 && !strcmp((char *) rot_cl.client_data2, "ruler")) ||
        (cl && cl->exists && (!cl->client_data1) &&
         cl->client_data2 && !strcmp((char *) cl->client_data2, "ruler")))
    {
        const char *tree_awar;
        char awar[256];
        float h;
        switch (cmd) {
            case AWT_MODE_ROT:
            case AWT_MODE_SPREAD:
            case AWT_MODE_SWAP:
            case AWT_MODE_SETROOT:
            case AWT_MODE_MOVE: // Move Ruler
                switch (type) {
                    case AW_Mouse_Press:
                        rot_cl = *cl;
                        rot_cl.x0 = x;
                        rot_cl.y0 = y;
                        break;
                    case AW_Mouse_Drag:
                        tree_awar = show_ruler(device, this->drag_gc);
                        sprintf(awar, "ruler/%s/ruler_x", tree_awar);
                        h = (x - rot_cl.x0)*device->get_unscale() + *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                        GBT_write_float(gb_tree, awar, h);
                        sprintf(awar, "ruler/%s/ruler_y", tree_awar);
                        h = (y - rot_cl.y0)*device->get_unscale() + *GBT_readOrCreate_float(gb_tree, awar, 0.0);
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
                switch (type) { // resize Ruler
                    case AW_Mouse_Press:
                        rot_cl = *cl;
                        rot_cl.x0 = x;
                        if (button==AW_BUTTON_RIGHT) { // if right mouse button is used -> adjust to 1 digit behind comma
                            sprintf(awar, "ruler/size");
                            /*tree_awar =*/ show_ruler(device, this->drag_gc);
                            double rulerSize = *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                            GBT_write_float(gb_tree, awar, discrete_ruler_length(rulerSize, 0.1));
                            /*tree_awar =*/ show_ruler(device, this->drag_gc);
                        }
                        break;
                    case AW_Mouse_Drag: {
                        sprintf(awar, "ruler/size");
                        h = *GBT_readOrCreate_float(gb_tree, awar, 0.0);
                        if (button == AW_BUTTON_RIGHT) {
                            GBT_write_float(gb_tree, awar, discrete_ruler_length(h, 0.1));
                        }
                        /*tree_awar = */ show_ruler(device, this->drag_gc);

                        if (tree_sort == AP_TREE_IRS) {
                            double scale = device->get_scale() * irs_tree_ruler_scale_factor;
                            h += (rot_cl.x0 - x)/scale;
                        }
                        else {
                            h += (x - rot_cl.x0)*device->get_unscale();
                        }
                        if (h<0.01) h = 0.01;

                        if (button==AW_BUTTON_RIGHT) { // if right mouse button is used -> adjust to 1 digit behind comma
                            double h_rounded = discrete_ruler_length(h, 0.1);
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
                        if (button==AW_BUTTON_RIGHT) { // if right mouse button is used -> adjust to 1 digit behind comma
                            sprintf(awar, "ruler/size");
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
                    sprintf(awar, "ruler/ruler_width");
                    long i = *GBT_readOrCreate_int(gb_tree, awar,  0);
                    switch (button) {
                        case AW_BUTTON_LEFT:
                            if (i>0) {
                                GBT_write_int(gb_tree, awar, i-1);
                                this->exports.refresh = 1;
                            }
                            break;
                        case AW_BUTTON_RIGHT:
                            GBT_write_int(gb_tree, awar, i+1);
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

    AP_tree *at;
    switch (cmd) {
        case AWT_MODE_MOVE:             // two point commands !!!!!
            if (button==AW_BUTTON_MIDDLE) {
                break;
            }
            switch (type) {
                case AW_Mouse_Press:
                    if (!(cl && cl->exists)) {
                        break;
                    }

                    //!* check security level @@@ **
                    at = (AP_tree *)cl->client_data1;
                    if (at && at->father) {
                        bl_drag_flag = 1;
                        this->rot_at = at;
                        this->rot_cl = *cl;
                        this->old_rot_cl = *cl;
                    }
                    break;

                case AW_Mouse_Drag:
                    if (bl_drag_flag && this->rot_at && this->rot_at->father) {
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
                    if (bl_drag_flag && this->rot_at && this->rot_at->father) {
                        this->rot_show_line(device);
                        AP_tree *dest = 0;
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
                        switch (button) {
                            case AW_BUTTON_LEFT:
                                error = source->cantMoveTo(dest);
                                if (!error) source->moveTo(dest, cl->nearest_rel_pos);
                                break;

                            case AW_BUTTON_RIGHT:
                                error = source->move_group_info(dest);
                                break;
                            default:
                                error = "????? 45338";
                        }

                        if (error) {
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
            if (button == AW_BUTTON_MIDDLE) {
                break;
            }
            switch (type) {
                case AW_Mouse_Press:
                    if (cl && cl->exists) {
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
                    if (bl_drag_flag && this->rot_at && this->rot_at->father) {
                        double len, ex, ey, scale;

                        rot_show_triangle(device);

                        if (rot_at == rot_at->father->leftson) {
                            len = rot_at->father->leftlen; // @@@ unused
                        }
                        else {
                            len = rot_at->father->rightlen; // @@@ unused
                        }

                        ex = x-rot_cl.x0;
                        ey = y-rot_cl.y0;

                        len = ex * cos(this->rot_orientation) +
                            ey * sin(this->rot_orientation);

                        if (button==AW_BUTTON_RIGHT) { // if right mouse button is used -> adjust to 1 digit behind comma
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
            if (button!=AW_BUTTON_LEFT) {
                break;
            }
            switch (type) {
                case AW_Mouse_Press:
                    if (cl && cl->exists) {
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
                    if (rot_drag_flag && this->rot_at && this->rot_at->father) {
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
            if (type==AW_Mouse_Press) {
                switch (button) {
                    case AW_BUTTON_LEFT:
                        if (cl->exists) {
                            at = (AP_tree *)cl->client_data1;
                            if (at) {
                                tree_root_display = at;
                                exports.refresh   = 1;
                            }
                        }
                        break;
                    case AW_BUTTON_RIGHT:
                        if (tree_root_display->father) {
                            tree_root_display  = tree_root_display->get_father();
                            exports.zoom_reset = 1;
                        }
                        break;
                }
                get_root_node()->compute_tree(gb_main);
            }
            break;

        case AWT_MODE_RESET:
            if (type==AW_Mouse_Press) {
                switch (button) {
                    case AW_BUTTON_LEFT:
                        //! reset rotation *
                        if (cl->exists) {
                            at = (AP_tree *)cl->client_data1;
                            if (at) {
                                at->reset_rotation();
                                exports.save    = 1;
                                exports.refresh = 1;
                            }
                        }
                        break;
                    case AW_BUTTON_MIDDLE:
                        //! reset spread *
                        if (cl->exists) {
                            at = (AP_tree *)cl->client_data1;
                            if (at) {
                                at->reset_spread();
                                exports.save    = 1;
                                exports.refresh = 1;
                            }
                        }
                        break;
                    case AW_BUTTON_RIGHT:
                        //! reset linewidth *
                        if (cl->exists) {
                            at = (AP_tree *)cl->client_data1;
                            if (at) {
                                at->reset_child_linewidths();
                                at->set_linewidth(1);
                                exports.save    = 1;
                                exports.refresh = 1;
                            }
                        }
                        break;
                }
                get_root_node()->compute_tree(gb_main);
            }
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
                                case AW_BUTTON_LEFT:
                                    linewidth       = (linewidth <= 1) ? 0 : linewidth-1;
                                    exports.save    = 1;
                                    exports.refresh = 1;
                                    break;

                                case AW_BUTTON_RIGHT:
                                    linewidth += 1;
                                    exports.save    = 1;
                                    exports.refresh = 1;
                                    break;
                            }
                        }
                    }
                }
            }
            break;

        case AWT_MODE_SPREAD:
            if (type==AW_Mouse_Press) {
                switch (button) {
                    case AW_BUTTON_LEFT:
                        if (cl->exists) {
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->gr.spread -= PH_CLICK_SPREAD;
                            if (at->gr.spread<0) at->gr.spread = 0;
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                    case AW_BUTTON_RIGHT:
                        if (cl->exists) {
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->gr.spread += PH_CLICK_SPREAD;
                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                }
            }
            break;

    act_like_group :
        case AWT_MODE_GROUP:
            if (type==AW_Mouse_Press) {
                if (cl->exists) {
                    at = (AP_tree *)cl->client_data1;
                    if (!at) break;
                } else break;

                switch (button) {
                    case AW_BUTTON_LEFT:
                        if ((!at->gr.grouped) && (!at->name)) {
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
                    case AW_BUTTON_RIGHT:
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
            }
            break;

        case AWT_MODE_SETROOT:
            if (type==AW_Mouse_Press) {
                switch (button) {
                    case AW_BUTTON_LEFT:
                        if (cl->exists) {
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;

                            at->set_root();
                            exports.save       = 1;
                            exports.zoom_reset = 1;
                            get_root_node()->compute_tree(gb_main);
                        }
                        break;
                    case AW_BUTTON_RIGHT:
                        DOWNCAST(AP_tree*, tree_static->find_innermost_edge().son())->set_root();
                        exports.save       = 1;
                        exports.zoom_reset = 1;
                        get_root_node()->compute_tree(gb_main);
                        break;
                }
            }
            break;

        case AWT_MODE_SWAP:
            if (type==AW_Mouse_Press) {
                switch (button) {
                    case AW_BUTTON_LEFT:
                        if (cl->exists) {
                            at = (AP_tree *)cl->client_data1;
                            if (!at) break;
                            at->swap_sons();

                            this->exports.refresh = 1;
                            this->exports.save = 1;
                        }
                        break;
                    case AW_BUTTON_RIGHT:
                        break;
                }
            }
            break;

        case AWT_MODE_MARK:
            if (type == AW_Mouse_Press && (cl->exists || ct->exists)) {
                at = (AP_tree *)(cl->exists ? cl->client_data1 : ct->client_data1);
                if (at) {
                    GB_transaction ta(tree_static->get_gb_main());

                    if (type == AW_Mouse_Press) {
                        switch (button) {
                            case AW_BUTTON_LEFT:
                                mark_species_in_tree(at, 1);
                                break;
                            case AW_BUTTON_RIGHT:
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
            if (type==AW_Mouse_Press && (cl->exists || ct->exists) && button != AW_BUTTON_MIDDLE) {
                GB_transaction ta(tree_static->get_gb_main());
                at = (AP_tree *)(cl->exists ? cl->client_data1 : ct->client_data1);
                if (!at) break;

                this->exports.refresh = 1;        // No refresh needed !! AD_map_viewer will do the refresh (needed by arb_pars)
                map_viewer_cb(at->gb_node, ADMVT_SELECT);

                if (button == AW_BUTTON_LEFT) goto act_like_group; // now do the same like in AWT_MODE_GROUP
            }
            break;

        case AWT_MODE_EDIT:
        case AWT_MODE_WWW:
            // now handle all modes which only act on tips (aka species) and
            // shall perform idetically in tree- and list-modes
            at = (AP_tree *)(cl->exists ? cl->client_data1 : (ct->exists ? ct->client_data1 : NULL));
            if (at && at->gb_node) {
                if (command_on_GBDATA(at->gb_node, cmd, type, button, map_viewer_cb)) {
                    exports.refresh = 1;
                }
            }
            break;

        default:
            break;
    }
}

void AWT_graphic_tree::set_tree_type(AP_tree_sort type, AWT_canvas *ntw) {
    if (sort_is_list_style(type)) {
        if (tree_sort == type) { // we are already in wanted view
            nds_show_all = !nds_show_all; // -> toggle between 'marked' and 'all'
        }
        else {
            nds_show_all = true; // default to all
        }
    }
    tree_sort = type;
    apply_zoom_settings_for_treetype(ntw); // sets default padding

    exports.fit_mode  = AWT_FIT_LARGER;
    exports.zoom_mode = AWT_ZOOM_BOTH;

    exports.dont_scroll = 0;

    switch (type) {
        case AP_TREE_RADIAL:
            break;

        case AP_LIST_SIMPLE:
        case AP_LIST_NDS:
            exports.fit_mode  = AWT_FIT_NEVER;
            exports.zoom_mode = AWT_ZOOM_NEVER;

            break;

        case AP_TREE_IRS:    // folded dendrogram
            exports.fit_mode    = AWT_FIT_X;
            exports.zoom_mode   = AWT_ZOOM_X;
            exports.dont_scroll = 1;
            break;

        case AP_TREE_NORMAL: // normal dendrogram
            exports.fit_mode  = AWT_FIT_X;
            exports.zoom_mode = AWT_ZOOM_X;
            break;
    }
    exports.resize = 1;
}

AWT_graphic_tree::AWT_graphic_tree(AW_root *aw_root_, GBDATA *gb_main_, AD_map_viewer_cb map_viewer_cb_)
    : AWT_graphic()
{
    line_filter          = AW_SCREEN|AW_CLICK|AW_CLICK_DRAG|AW_SIZE|AW_PRINTER;
    vert_line_filter     = AW_SCREEN|AW_PRINTER;
    group_bracket_filter = AW_SCREEN|AW_PRINTER|AW_CLICK|AW_SIZE_UNSCALED;
    text_filter          = AW_SCREEN|AW_CLICK|AW_PRINTER|AW_SIZE_UNSCALED;
    mark_filter          = AW_SCREEN|AW_PRINTER_EXT;
    ruler_filter         = AW_SCREEN|AW_CLICK|AW_PRINTER; // appropriate size-filter added manually in code
    root_filter          = AW_SCREEN|AW_CLICK|AW_PRINTER_EXT;

    set_tree_type(AP_TREE_NORMAL, NULL);
    tree_root_display = 0;
    tree_proto        = 0;
    link_to_database  = false;
    tree_static       = 0;
    baselinewidth     = 1;
    species_name      = 0;
    aw_root           = aw_root_;
    gb_main           = gb_main_;
    rot_ct.exists     = false;
    rot_cl.exists     = false;
    nds_show_all      = true;
    map_viewer_cb     = map_viewer_cb_;
}

AWT_graphic_tree::~AWT_graphic_tree() {
    free(species_name);
    delete tree_proto;
    delete tree_static;
}

void AWT_graphic_tree::init(const AP_tree& tree_prototype, AliView *aliview, AP_sequence *seq_prototype, bool link_to_database_, bool insert_delete_cbs) {
    tree_static = new AP_tree_root(aliview, tree_prototype, seq_prototype, insert_delete_cbs);

    td_assert(!insert_delete_cbs || link_to_database); // inserting delete callbacks w/o linking to DB has no effect!
    link_to_database = link_to_database_;
}

void AWT_graphic_tree::unload() {
    delete tree_static->get_root_node();
    rot_at            = 0;
    tree_root_display = 0;
}

GB_ERROR AWT_graphic_tree::load(GBDATA *, const char *name, AW_CL /* cl_link_to_database */, AW_CL /* cl_insert_delete_cbs */) {
    GB_ERROR error = 0;

    if (name[0] == 0 || strcmp(name, NO_TREE_SELECTED) == 0) {
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

GB_ERROR AWT_graphic_tree::save(GBDATA * /* dummy */, const char * /* name */, AW_CL /* cd1 */, AW_CL /* cd2 */) {
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
                aw_message(GBS_global_string("Tree '%s' lost all leafs and has been deleted", tree_static->get_tree_name()));
#if defined(WARN_TODO)
#warning somehow update selected tree

                // solution: currently selected tree (in NTREE, maybe also in PARSIMONY)
                // needs to add a delete callback on treedata in DB

#endif
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
        if (tree_root) {
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

    if ((aw_root->awar_no_error("probe_collection/do_refresh")    != 0) &&
        (aw_root->awar("probe_collection/do_refresh")->read_int() != 0))
    {
        aw_root->awar("probe_collection/do_refresh")->write_int(0);

        if (flags == AP_UPDATE_OK)
        {
            flags = AP_UPDATE_RELOADED;
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

void AWT_graphic_tree::enumerateClade(AP_tree *at, int* pMatchCounts, int& nCladeSize, int nNumProbes)
{
  if (at->is_leaf)
  {
    if (at->name && (disp_device->get_filter() & text_filter))
    {
      const char* pDB_MatchString = GBT_read_char_pntr(at->gb_node, "matched_string");

      if ((pDB_MatchString != 0) && (strlen(pDB_MatchString) == nNumProbes))
      {
        for (int cn = 0 ; cn < nNumProbes ; cn++)
        {
          switch (pDB_MatchString[cn])
          {
            case '1':
            {
              pMatchCounts[cn] += 1;
              break;
            }

            default:
            case '0':
            {
              break;
            }
          }
        }

        nCladeSize++;
      }
    }
  }
  else
  {
    enumerateClade(at->get_leftson(), pMatchCounts, nCladeSize, nNumProbes);
    enumerateClade(at->get_rightson(), pMatchCounts, nCladeSize, nNumProbes);
  }
}

void AWT_graphic_tree::drawMatchFlag(AP_tree *at, bool bPartial, int nProbe, int nProbeOffset, double y1, double y2)
{
  double    dHalfWidth = 0.5 * MATCH_COL_WIDTH / disp_device->get_scale();
  double    dWidth     = dHalfWidth * 2;
  double    x          = dHalfWidth + (nProbe - nProbeOffset - 0.5) * dWidth;
  int       nColour    = nProbe % 16;

  disp_device->set_foreground_color(at->gr.gc, MatchProbeColourIndex[nColour]);
  disp_device->set_grey_level(at->gr.gc, this->grey_level);

  if (bPartial)
  {
    disp_device->set_fill_stipple(at->gr.gc);
  }

  Position  pb(x, y1);
  Vector    sizeb(dWidth, y2 - y1);

  disp_device->box(at->gr.gc, true, pb, sizeb, mark_filter);

  if (bPartial)
  {
    disp_device->set_fill_solid(at->gr.gc);
  }
}

void AWT_graphic_tree::drawMatchFlag(AP_tree *at, const char* pName, double y1, double y2)
{
  if ((aw_root->awar_no_error("probe_collection/has_results")     != 0) &&
      (aw_root->awar("probe_collection/has_results")->read_int()  != 0))
  {
    AW_color_idx  LastColor;
    bool          bChanged      = false;
    int           nNumProbes    = aw_root->awar("probe_collection/number_of_probes")->read_int();
    int           nCladeSize    = 0;
    int           nProbeOffset  = nNumProbes + 1;
    int*          pMatchCounts  = new int[nNumProbes];
    int           nProbe;

    LastColor = disp_device->get_foreground_color(at->gr.gc);

    if (pMatchCounts != 0)
    {
      memset(pMatchCounts, 0, nNumProbes * sizeof(*pMatchCounts));

      enumerateClade(at, pMatchCounts, nCladeSize, nNumProbes);

      if (!at->is_leaf)
      {
        double    dCladeMarkedThreshold           = aw_root->awar("probe_collection/clade_marked_threshold")->read_float();
        double    dCladePartiallyMarkedThreshold  = aw_root->awar("probe_collection/clade_partially_marked_threshold")->read_float();

        if (nCladeSize > 0)
        {
          int             nMatchedSize          = (int)(nCladeSize * dCladeMarkedThreshold + 0.5);
          int             nPartiallyMatchedSize = (int)(nCladeSize * dCladePartiallyMarkedThreshold + 0.5);

          for (nProbe = 0 ; nProbe < nNumProbes ; nProbe++)
          {
            bool  bMatched      = (pMatchCounts[nProbe] >= nMatchedSize);
            bool  bPartialMatch = false;

            // Only check for partial match if we don't have a match. If a partial
            // match is found then isMatched() should return true.
            if (!bMatched)
            {
              bPartialMatch = (pMatchCounts[nProbe] >= nPartiallyMatchedSize);
              bMatched      = bPartialMatch;
            }

            if (bMatched)
            {
              drawMatchFlag(at, bPartialMatch, nProbe, nProbeOffset, y1, y2);

              bChanged = true;
            }
          }
        }
      }
      else
      {
        for (nProbe = 0 ; nProbe < nNumProbes ; nProbe++)
        {
          if (pMatchCounts[nProbe] > 0)
          {
            drawMatchFlag(at, false, nProbe, nProbeOffset, y1, y2);

            bChanged = true;
          }
        }
      }
    }


    if (bChanged)
    {
      disp_device->set_foreground_color(at->gr.gc, LastColor);
    }
  }
}

void AWT_graphic_tree::drawMatchFlagNames(AP_tree *at, Position& Pen)
{
  if ((aw_root->awar_no_error("probe_collection/has_results")     != 0) &&
      (aw_root->awar("probe_collection/has_results")->read_int()  != 0))
  {
    double        x;
    double        dHalfWidth    = 0.5 * MATCH_COL_WIDTH / disp_device->get_scale();
    double        dWidth        = dHalfWidth * 2;
    double        y_root        = Pen.ypos();
    AW_color_idx  LastColor     = disp_device->get_foreground_color(at->gr.gc);
    int           nNumProbes    = aw_root->awar("probe_collection/number_of_probes")->read_int();
    int           nProbeOffset  = nNumProbes + 1;
    int           nProbe;
    int           nColour;

    Pen.movey(2 * scaled_branch_distance);

    // Loop through probes in reverse probe collection order so the match columns
    // match the probe list
    for (nProbe = nNumProbes - 1 ; nProbe >= 0 ; nProbe--)
    {
      nColour = nProbe % 16;
      x       = dWidth + (nProbe - nProbeOffset - 0.5) * dWidth;

      Pen.movey(scaled_branch_distance);

      char  sAWAR[32] = {0};

      sprintf(sAWAR, "probe_collection/probe%d/Name", nProbe);

      char* pProbeName = aw_root->awar(sAWAR)->read_string();

      if (pProbeName != 0)
      {
        Position  pl1(x, y_root);
        Position  pl2(x, Pen.ypos() - 2 * scaled_branch_distance);
        Position  pb(x - dHalfWidth, Pen.ypos() - 2 * scaled_branch_distance);
        Vector    sizeb(dWidth, scaled_branch_distance);
        Position  pt(x + dHalfWidth * 3, Pen.ypos() - scaled_branch_distance);

        disp_device->set_foreground_color(at->gr.gc, MatchProbeColourIndex[nColour]);
        disp_device->line(at->gr.gc, pl1, pl2, mark_filter);
        disp_device->box(at->gr.gc, true, pb, sizeb, mark_filter);
        disp_device->text(at->gr.gc, pProbeName, pt, 0, text_filter, strlen(pProbeName));

        free(pProbeName);
      }
    }

    Pen.movey(scaled_branch_distance);

    disp_device->set_foreground_color(at->gr.gc, LastColor);

    // This hack is needed to ensure that the screen size is scaled correctly
    // to fit the match results which are draw with negative x values.
    if (disp_device->type() == AW_DEVICE_SIZE)
    {
      AW_device_size* SizeDevice = (AW_device_size*)disp_device;
      AW::Rectangle   world;
      AW_screen_area  rect;
      AW_pos          width;
      AW_pos          net_window_width;
      double          dOffsetX;
      double          dK;

      world = SizeDevice->get_size_information();
      rect  = SizeDevice->get_area_size();

      width             = world.width();
      net_window_width  = rect.r - rect.l - exports.get_x_padding();
      dK                = -MATCH_COL_WIDTH * (nProbeOffset + 1);

      if (net_window_width < AWT_MIN_WIDTH)
      {
        net_window_width = AWT_MIN_WIDTH;
      }

      if (-dK / net_window_width < 0.9)
      {
        dOffsetX  = (dK * world.right()) / (dK + net_window_width);
      }
      else
      {
        dK        = -0.9 * net_window_width;
        dOffsetX  = (dK * world.right()) / (dK + net_window_width);
      }

      dOffsetX /= disp_device->get_scale();

      Position  pl1(dOffsetX, Pen.ypos());
      Position  pl2(dOffsetX, Pen.ypos());

      disp_device->line(at->gr.gc, pl1, pl2, line_filter);
    }
  }
}

void AWT_graphic_tree::clickNotifyWhichProbe(AW_device* device, AW_pos click_x, AW_pos click_y)
{
  if ((aw_root->awar_no_error("probe_collection/has_results")     != 0) &&
      (aw_root->awar("probe_collection/has_results")->read_int()  != 0))
  {
    int     nNumProbes    = aw_root->awar("probe_collection/number_of_probes")->read_int();
    double  dHalfWidth    = 0.5 * MATCH_COL_WIDTH / device->get_scale();
    double  dWidth        = dHalfWidth * 2;
    int     nProbeOffset  = nNumProbes + 1;
    int     nProbe;

    for (nProbe = nNumProbes - 1 ; nProbe >= 0 ; nProbe--)
    {
      double        x = dWidth + (nProbe - nProbeOffset - 0.5) * dWidth;
      AW::Position  probe_posH(device->transform(AW::Position(x + dHalfWidth, 0)));
      AW::Position  probe_posL(device->transform(AW::Position(x - dHalfWidth, 0)));

      if ((click_x >= probe_posL.xpos())  &&
          (click_x <  probe_posH.xpos()))
      {
        char  sAWAR[32] = {0};

        sprintf(sAWAR, "probe_collection/probe%d/Name", nProbe);

        char* pProbeName = aw_root->awar(sAWAR)->read_string();

        if (pProbeName != 0)
        {
          aw_message(pProbeName);
          free(pProbeName);
        }
        break;
      }
    }
  }
}

void AWT_graphic_tree::box(int gc, const AW::Position& pos, int pixel_width, bool filled) {
    double diameter = disp_device->rtransform_pixelsize(pixel_width);
    Vector diagonal(diameter, diameter);

    if (filled) disp_device->set_grey_level(gc, grey_level);
    else        disp_device->set_line_attributes(gc, 1, AW_SOLID);

    disp_device->box(gc, filled, pos-0.5*diagonal, diagonal, mark_filter);
}

void AWT_graphic_tree::diamond(int gc, const Position& pos, int pixel_width) {
    // box with one corner down
    double diameter = disp_device->rtransform_pixelsize(pixel_width);
    double radius  = diameter*0.5;

    Position t(pos.xpos(), pos.ypos()-radius);
    Position b(pos.xpos(), pos.ypos()+radius);
    Position l(pos.xpos()-radius, pos.ypos());
    Position r(pos.xpos()+radius, pos.ypos());

    disp_device->line(gc, l, t, mark_filter);
    disp_device->line(gc, r, t, mark_filter);
    disp_device->line(gc, l, b, mark_filter);
    disp_device->line(gc, r, b, mark_filter);
}

bool AWT_show_branch_remark(AW_device *device, const char *remark_branch, bool is_leaf, const Position& pos, AW_pos alignment, AW_bitset filteri) {
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
        td_assert(text != 0);
        device->text(AWT_GC_BRANCH_REMARK, text, pos, alignment, filteri);
    }

    return is_bootstrap && show;
}

bool AWT_show_branch_remark(AW_device *device, const char *remark_branch, bool is_leaf, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri) {
    return AWT_show_branch_remark(device, remark_branch, is_leaf, Position(x, y), alignment, filteri);
}

void AWT_graphic_tree::show_dendrogram(AP_tree *at, Position& Pen, DendroSubtreeLimits& limits) {
    // 'Pen' points to the upper-left corner of the area into which subtree gets painted
    // after return 'Pen' is Y-positioned for next tree-tip (X is undefined)

    if (disp_device->type() != AW_DEVICE_SIZE) { // tree below cliprect bottom can be cut
        Position p(0, Pen.ypos() - scaled_branch_distance *2.0);
        Position s = disp_device->transform(p);

        bool   is_clipped = false;
        double offset     = 0.0;
        if (disp_device->is_below_clip(s.ypos())) {
            offset     = scaled_branch_distance;
            is_clipped = true;
        }
        else {
            p.sety(Pen.ypos() + scaled_branch_distance * (at->gr.view_sum+2));
            s = disp_device->transform(p);;

            if (disp_device->is_above_clip(s.ypos())) {
                offset     = scaled_branch_distance*at->gr.view_sum;
                is_clipped = true;
            }
        }

        if (is_clipped) {
            limits.x_right  = Pen.xpos();
            limits.y_branch = Pen.ypos();
            Pen.movey(offset);
            limits.y_top    = limits.y_bot = Pen.ypos();
            return;
        }
    }

    AW_click_cd cd(disp_device, (AW_CL)at);
    if (at->is_leaf) {
        if (at->gb_node && GB_read_flag(at->gb_node)) {
            set_line_attributes_for(at);
            filled_box(at->gr.gc, Pen, NT_BOX_WIDTH);
        }
        if (at->hasName(species_name)) cursor = Pen;

        if (at->name && (disp_device->get_filter() & text_filter)) {
            // display text
            const char            *data       = make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at->get_gbt_tree(), tree_static->get_tree_name());
            const AW_font_limits&  charLimits = disp_device->get_font_limits(at->gr.gc, 'A');

            double   unscale  = disp_device->get_unscale();
            size_t   data_len = strlen(data);
            Position textPos  = Pen + 0.5*Vector((charLimits.width+NT_BOX_WIDTH)*unscale, scaled_font.ascent);

            drawMatchFlag(at, data, Pen.ypos() - scaled_branch_distance * 0.495, Pen.ypos() + scaled_branch_distance * 0.495);
            disp_device->text(at->gr.gc, data, textPos, 0.0, text_filter, data_len);

            double textsize = disp_device->get_string_size(at->gr.gc, data, data_len) * unscale;
            limits.x_right = textPos.xpos() + textsize;
        }
        else {
            limits.x_right = Pen.xpos();
        }

        limits.y_top = limits.y_bot = limits.y_branch = Pen.ypos();
        Pen.movey(scaled_branch_distance);
    }

    //   s0-------------n0
    //   |
    //   attach (to father)
    //   |
    //   s1------n1

    else if (at->gr.grouped) {
        double height     = scaled_branch_distance * at->gr.view_sum;
        double box_height = height-scaled_branch_distance;

        Position s0(Pen);
        Position s1(s0);  s1.movey(box_height);
        Position n0(s0);  n0.movex(at->gr.tree_depth);
        Position n1(s1);  n1.movex(at->gr.min_tree_depth);

        Position group[4] = { s0, s1, n1, n0 };

        set_line_attributes_for(at);

        drawMatchFlag(at, 0, Pen.ypos() - scaled_branch_distance * 0.495, Pen.ypos() + height + scaled_branch_distance * 0.495);

        disp_device->set_grey_level(at->gr.gc, grey_level);
        disp_device->filled_area(at->gr.gc, 4, group, line_filter);

        const AW_font_limits& charLimits  = disp_device->get_font_limits(at->gr.gc, 'A');
        double                text_ascent = charLimits.ascent * disp_device->get_unscale();

        Vector text_offset = 0.5 * Vector(text_ascent, text_ascent+box_height);

        limits.x_right = n0.xpos();

        if (at->gb_node && (disp_device->get_filter() & text_filter)) {
            const char *data     = make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at->get_gbt_tree(), tree_static->get_tree_name());
            size_t      data_len = strlen(data);

            Position textPos = n0+text_offset;
            disp_device->text(at->gr.gc, data, textPos, 0.0, text_filter, data_len);

            double textsize = disp_device->get_string_size(at->gr.gc, data, data_len) * disp_device->get_unscale();
            limits.x_right  = textPos.xpos()+textsize;
        }

        Position    countPos = s0+text_offset;
        const char *count    = GBS_global_string(" %i", at->gr.leaf_sum);
        disp_device->text(at->gr.gc, count, countPos, 0.0, text_filter);

        limits.y_top    = s0.ypos();
        limits.y_bot    = s1.ypos();
        limits.y_branch = centroid(limits.y_top, limits.y_bot);

        Pen.movey(height);
    }
    else { // furcation
        Position s0(Pen);

        Pen.movex(at->leftlen);
        Position n0(Pen);

        show_dendrogram(at->get_leftson(), Pen, limits); // re-use limits for left branch

        n0.sety(limits.y_branch);
        s0.sety(limits.y_branch);

        Pen.setx(s0.xpos());
        Position attach(Pen); attach.movey(- .5*scaled_branch_distance);
        Pen.movex(at->rightlen);
        Position n1(Pen);
        {
            DendroSubtreeLimits right_lim;
            show_dendrogram(at->get_rightson(), Pen, right_lim);
            n1.sety(right_lim.y_branch);
            limits.combine(right_lim);
        }

        Position s1(s0.xpos(), n1.ypos());

        if (at->name) {
            diamond(at->gr.gc, attach, NT_BOX_WIDTH*2);

            if (show_brackets) {
                double                unscale          = disp_device->get_unscale();
                const AW_font_limits& charLimits       = disp_device->get_font_limits(at->gr.gc, 'A');
                double                half_text_ascent = charLimits.ascent * unscale * 0.5;

                double x1 = limits.x_right + scaled_branch_distance*0.1;
                double x2 = x1 + scaled_branch_distance * 0.3;
                double y1 = limits.y_top - half_text_ascent * 0.5;
                double y2 = limits.y_bot + half_text_ascent * 0.5;

                Rectangle bracket(Position(x1, y1), Position(x2, y2));

                set_line_attributes_for(at);

                unsigned int gc = at->gr.gc;
                disp_device->line(gc, bracket.upper_edge(), group_bracket_filter);
                disp_device->line(gc, bracket.lower_edge(), group_bracket_filter);
                disp_device->line(gc, bracket.right_edge(), group_bracket_filter);

                limits.x_right = x2;

                if (at->gb_node && (disp_device->get_filter() & text_filter)) {
                    const char *data     = make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at->get_gbt_tree(), tree_static->get_tree_name());
                    size_t      data_len = strlen(data);

                    LineVector worldBracket = disp_device->transform(bracket.right_edge());
                    LineVector clippedWorldBracket;
                    bool       visible      = disp_device->clip(worldBracket, clippedWorldBracket);
                    if (visible) {
                        LineVector clippedBracket = disp_device->rtransform(clippedWorldBracket);

                        Position textPos = clippedBracket.centroid()+Vector(half_text_ascent, half_text_ascent);
                        disp_device->text(at->gr.gc, data, textPos, 0.0, text_filter, data_len);

                        double textsize = disp_device->get_string_size(at->gr.gc, data, data_len) * unscale;
                        limits.x_right  = textPos.xpos()+textsize;
                    }
                }
            }
        }

        for (int right = 0; right<2; ++right) {
            AP_tree         *son;
            GBT_LEN          len;
            const Position&  n = right ? n1 : n0;
            const Position&  s = right ? s1 : s0;

            if (right) {
                son = at->get_rightson();
                len = at->rightlen;
            }
            else {
                son = at->get_leftson();
                len = at->leftlen;
            }

            AW_click_cd cds(disp_device, (AW_CL)son);
            if (son->remark_branch) {
                Position remarkPos(n);
                remarkPos.movey(-scaled_font.ascent*0.1);
                bool bootstrap_shown = AWT_show_branch_remark(disp_device, son->remark_branch, son->is_leaf, remarkPos, 1, text_filter);
                if (show_circle && bootstrap_shown) {
                    show_bootstrap_circle(disp_device, son->remark_branch, circle_zoom_factor, circle_max_size, len, n, use_ellipse, scaled_branch_distance, text_filter);
                }
            }

            set_line_attributes_for(son);
            unsigned int gc = son->gr.gc;
            disp_device->line(gc, s, n, line_filter);
            disp_device->line(gc, attach, s, vert_line_filter);
        }
        limits.y_branch = attach.ypos();
    }
}


void AWT_graphic_tree::scale_text_koordinaten(AW_device *device, int gc, double& x, double& y, double orientation, int flag) {
    if (flag!=1) {
        const AW_font_limits& charLimits  = device->get_font_limits(gc, 'A');
        double                text_height = charLimits.height * disp_device->get_unscale();
        double                dist        = charLimits.height * disp_device->get_unscale();

        x += cos(orientation) * dist;
        y += sin(orientation) * dist + 0.3*text_height;
    }
}

void AWT_graphic_tree::show_radial_tree(AP_tree * at, double x_center,
                                        double y_center, double tree_spread, double tree_orientation,
                                        double x_root, double y_root)
{
    double l, r, w, z, l_min, l_max;

    AW_click_cd cd(disp_device, (AW_CL)at);
    set_line_attributes_for(at);
    disp_device->line(at->gr.gc, x_root, y_root, x_center, y_center, line_filter);

    // draw mark box
    if (at->gb_node && GB_read_flag(at->gb_node)) {
        filled_box(at->gr.gc, Position(x_center, y_center), NT_BOX_WIDTH);
    }

    if (at->is_leaf) {
        if (at->name && (disp_device->get_filter() & text_filter)) {
            if (at->hasName(species_name)) cursor = Position(x_center, y_center);
            scale_text_koordinaten(disp_device, at->gr.gc, x_center, y_center, tree_orientation, 0);
            const char *data =  make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at->get_gbt_tree(), tree_static->get_tree_name());
            disp_device->text(at->gr.gc, data,
                              (AW_pos)x_center, (AW_pos) y_center,
                              (AW_pos) .5 - .5 * cos(tree_orientation),
                              text_filter);
        }
        return;
    }

    if (at->gr.grouped) {
        l_min = at->gr.min_tree_depth;
        l_max = at->gr.tree_depth;

        r    = l = 0.5;
        AW_pos q[6];
        q[0] = x_center;
        q[1] = y_center;
        w    = tree_orientation + r*0.5*tree_spread + at->gr.right_angle;
        q[2] = x_center+l_min*cos(w);
        q[3] = y_center+l_min*sin(w);
        w    = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
        q[4] = x_center+l_max*cos(w);
        q[5] = y_center+l_max*sin(w);

        disp_device->set_grey_level(at->gr.gc, grey_level);
        disp_device->filled_area(at->gr.gc, 3, &q[0], line_filter);

        if (at->gb_node && (disp_device->get_filter() & text_filter)) {
            w = tree_orientation + at->gr.right_angle;
            l_max = (l_max+l_min)*.5;
            x_center = x_center+l_max*cos(w);
            y_center = y_center+l_max*sin(w);
            scale_text_koordinaten(disp_device, at->gr.gc, x_center, y_center, w, 0);

            // insert text (e.g. name of group)
            const char *data = make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at->get_gbt_tree(), tree_static->get_tree_name());
            disp_device->text(at->gr.gc, data,
                              (AW_pos)x_center, (AW_pos) y_center,
                              (AW_pos).5 - .5 * cos(tree_orientation),
                              text_filter);
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

            //!* left branch **
            w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
            z = at->leftlen;
            show_radial_tree(at_leftson,
                             x_center + z * cos(w),
                             y_center + z * sin(w),
                             (at->leftson->is_leaf) ? 1.0 : tree_spread * l * at_leftson->gr.spread,
                             w,
                             x_center, y_center);

            //!* right branch **
            w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
            z = at->rightlen;
            show_radial_tree(at_rightson,
                             x_center + z * cos(w),
                             y_center + z * sin(w),
                             (at->rightson->is_leaf) ? 1.0 : tree_spread * r * at_rightson->gr.spread,
                             w,
                             x_center, y_center);
        }
        else {
            //!* right branch **
            w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
            z = at->rightlen;
            show_radial_tree(at_rightson,
                             x_center + z * cos(w),
                             y_center + z * sin(w),
                             (at->rightson->is_leaf) ? 1.0 : tree_spread * r * at_rightson->gr.spread,
                             w,
                             x_center, y_center);

            //!* left branch **
            w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
            z = at->leftlen;
            show_radial_tree(at_leftson,
                             x_center + z * cos(w),
                             y_center + z * sin(w),
                             (at->leftson->is_leaf) ? 1.0 : tree_spread * l * at_leftson->gr.spread,
                             w,
                             x_center, y_center);
        }
    }
    if (show_circle) {
        if (at->leftson->remark_branch) {
            AW_click_cd cdl(disp_device, (AW_CL)at->leftson);
            w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
            z = at->leftlen * .5;
            Position center(x_center + z * cos(w), y_center + z * sin(w));
            show_bootstrap_circle(disp_device, at->leftson->remark_branch, circle_zoom_factor, circle_max_size, at->leftlen, center, false, 0, text_filter);
        }
        if (at->rightson->remark_branch) {
            AW_click_cd cdr(disp_device, (AW_CL)at->rightson);
            w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
            z = at->rightlen * .5;
            Position center(x_center + z * cos(w), y_center + z * sin(w));
            show_bootstrap_circle(disp_device, at->rightson->remark_branch, circle_zoom_factor, circle_max_size, at->rightlen, center, false, 0, text_filter);
        }
    }
}

const char *AWT_graphic_tree::show_ruler(AW_device *device, int gc) {
    const char *tree_awar = 0;

    char    awar[256];
    GBDATA *gb_tree = tree_static->get_gb_tree();

    if (!gb_tree) return 0;                         // no tree -> no ruler

    GB_transaction dummy(gb_tree);

    sprintf(awar, "ruler/size");
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
            td_assert(0);
            tree_awar = 0;
            break;
    }

    if (tree_awar) {
        sprintf(awar, "ruler/%s/ruler_y", tree_awar);
        if (!GB_search(gb_tree, awar, GB_FIND)) {
            if (device->type() == AW_DEVICE_SIZE) {
                AW_world world;
                DOWNCAST(AW_device_size*, device)->get_size_information(&world);
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

        sprintf(awar, "ruler/%s/ruler_x", tree_awar);
        ruler_x = ruler_add_x + *GBT_readOrCreate_float(gb_tree, awar, ruler_x);

        sprintf(awar, "ruler/%s/text_x", tree_awar);
        ruler_text_x = *GBT_readOrCreate_float(gb_tree, awar, ruler_text_x);

        sprintf(awar, "ruler/%s/text_y", tree_awar);
        ruler_text_y = *GBT_readOrCreate_float(gb_tree, awar, ruler_text_y);

        sprintf(awar, "ruler/ruler_width");
        int ruler_width = *GBT_readOrCreate_int(gb_tree, awar, 0);

        device->set_line_attributes(gc, ruler_width+baselinewidth, AW_SOLID);

        AW_click_cd cd(device, 0, (AW_CL)"ruler");
        device->line(gc,
                     ruler_x - half_ruler_width, ruler_y,
                     ruler_x + half_ruler_width, ruler_y,
                     this->ruler_filter|AW_SIZE);

        char ruler_text[20];
        sprintf(ruler_text, "%4.2f", ruler_size);
        device->text(gc, ruler_text,
                     ruler_x + ruler_text_x,
                     ruler_y + ruler_text_y,
                     0.5,
                     this->ruler_filter|AW_SIZE_UNSCALED);
    }
    return tree_awar;
}

struct Column : virtual Noncopyable {
    char   *text;
    size_t  len;
    double  print_width;
    bool    is_numeric; // also true for empty text

    Column() : text(NULL) {}
    ~Column() { free(text); }

    void init(const char *text_, AW_device& device, int gc) {
        text        = strdup(text_);
        len         = strlen(text);
        print_width = device.get_string_size(gc, text, len);
        is_numeric  = (strspn(text, "0123456789.") == len);
    }
};

class ListDisplayRow : virtual Noncopyable {
    GBDATA *gb_species;
    AW_pos  y_position;
    int     gc;
    size_t  part_count;                             // NDS columns
    Column *column;

public:
    ListDisplayRow(GBDATA *gb_main, GBDATA *gb_species_, AW_pos y_position_, int gc_, AW_device& device, bool use_nds, const char *tree_name)
        : gb_species(gb_species_)
        , y_position(y_position_)
        , gc(gc_)
    {
        const char *nds = use_nds
            ? make_node_text_nds(gb_main, gb_species, NDS_OUTPUT_TAB_SEPARATED, 0, tree_name)
            : GBT_read_name(gb_species);

        ConstStrArray parts;
        GBT_split_string(parts, nds, "\t", false);
        part_count = parts.size();

        column = new Column[part_count];
        for (size_t i = 0; i<part_count; ++i) {
            column[i].init(parts[i], device, gc);
        }
    }

    ~ListDisplayRow() { delete [] column; }

    size_t get_part_count() const { return part_count; }
    const Column& get_column(size_t p) const {
        td_assert(p<part_count);
        return column[p];
    }
    double get_print_width(size_t p) const { return get_column(p).print_width; }
    const char *get_text(size_t p, size_t& len) const {
        const Column& col = get_column(p);
        len = col.len;
        return col.text;
    }
    int get_gc() const { return gc; }
    double get_ypos() const { return y_position; }
    GBDATA *get_species() const { return gb_species; }
};

void AWT_graphic_tree::show_nds_list(GBDATA *, bool use_nds) {
    AW_pos y_position = scaled_branch_distance;
    AW_pos x_position = NT_SELECTED_WIDTH * disp_device->get_unscale();

    disp_device->text(nds_show_all ? AWT_GC_CURSOR : AWT_GC_SELECTED,
                      GBS_global_string("%s of %s species", use_nds ? "NDS List" : "Simple list", nds_show_all ? "all" : "marked"),
                      (AW_pos) x_position, (AW_pos) 0,
                      (AW_pos) 0, text_filter);

    double max_x         = 0;
    double text_y_offset = scaled_font.ascent*.5;

    GBDATA *selected_species;
    {
        GBDATA *selected_name = GB_find_string(GBT_get_species_data(gb_main), "name", this->species_name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
        selected_species      = selected_name ? GB_get_father(selected_name) : NULL;
    }

    const char *tree_name = tree_static ? tree_static->get_tree_name() : NULL;

    AW_pos y1, y2;
    {
        const AW_screen_area& clip_rect = disp_device->get_cliprect();

        AW_pos Y1 = clip_rect.t;
        AW_pos Y2 = clip_rect.b;

        AW_pos x;
        disp_device->rtransform(0, Y1, x, y1);
        disp_device->rtransform(0, Y2, x, y2);
    }

    y1 -= 2*scaled_branch_distance;                 // add two lines for safety
    y2 += 2*scaled_branch_distance;

    size_t           displayed_rows = (y2-y1)/scaled_branch_distance+1;
    ListDisplayRow **row            = new ListDisplayRow*[displayed_rows];

    size_t species_count = 0;
    size_t max_parts     = 0;

    GBDATA *gb_species = nds_show_all ? GBT_first_species(gb_main) : GBT_first_marked_species(gb_main);
    if (gb_species) {
        int skip_over = (y1-y_position)/scaled_branch_distance-2;
        if (skip_over>0) {
            gb_species  = nds_show_all
                ? GB_followingEntry(gb_species, skip_over-1)
                : GB_following_marked(gb_species, "species", skip_over-1);
            y_position += skip_over*scaled_branch_distance;
        }
    }

    for (; gb_species; gb_species = nds_show_all ? GBT_next_species(gb_species) : GBT_next_marked_species(gb_species)) {
        y_position += scaled_branch_distance;
        if (gb_species == selected_species) cursor = Position(0, y_position);
        if (y_position>y1) {
            if (y_position>y2) break;           // no need to examine rest of species

            bool is_marked = nds_show_all ? GB_read_flag(gb_species) : true;
            if (is_marked) {
                disp_device->set_line_attributes(AWT_GC_SELECTED, baselinewidth, AW_SOLID);
                filled_box(AWT_GC_SELECTED, Position(0, y_position), NT_BOX_WIDTH);
            }

            int gc                            = AWT_GC_NSELECTED;
            if (nds_show_all && is_marked) gc = AWT_GC_SELECTED;
            else {
                int color_group     = AWT_species_get_dominant_color(gb_species);
                if (color_group) gc = AWT_GC_FIRST_COLOR_GROUP+color_group-1;
            }
            ListDisplayRow *curr = new ListDisplayRow(gb_main, gb_species, y_position+text_y_offset, gc, *disp_device, use_nds, tree_name);
            max_parts            = std::max(max_parts, curr->get_part_count());
            row[species_count++] = curr;
        }
    }

    td_assert(species_count <= displayed_rows);

    // calculate column offsets and detect column alignment
    double *max_part_width = new double[max_parts];
    bool   *align_right    = new bool[max_parts];

    for (size_t p = 0; p<max_parts; ++p) {
        max_part_width[p] = 0;
        align_right[p]    = true;
    }

    for (size_t s = 0; s<species_count; ++s) {
        size_t parts = row[s]->get_part_count();
        for (size_t p = 0; p<parts; ++p) {
            const Column& col = row[s]->get_column(p);
            max_part_width[p] = std::max(max_part_width[p], col.print_width);
            align_right[p]    = align_right[p] && col.is_numeric;
        }
    }

    double column_space = scaled_branch_distance;

    double *part_x_pos = new double[max_parts];
    for (size_t p = 0; p<max_parts; ++p) {
        part_x_pos[p]  = x_position;
        x_position    += max_part_width[p]+column_space;
    }
    max_x = x_position;

    // draw

    for (size_t s = 0; s<species_count; ++s) {
        const ListDisplayRow& Row = *row[s];

        size_t parts = Row.get_part_count();
        int    gc    = Row.get_gc();
        AW_pos y     = Row.get_ypos();

        GBDATA      *gb_sp = Row.get_species();
        AW_click_cd  cd(disp_device, (AW_CL)gb_sp, (AW_CL)"species");

        for (size_t p = 0; p<parts; ++p) {
            const Column& col = Row.get_column(p);

            AW_pos x               = part_x_pos[p];
            if (align_right[p]) x += max_part_width[p] - col.print_width;

            disp_device->text(gc, col.text, x, y, 0.0, text_filter, col.len);
        }
    }

    delete [] part_x_pos;
    delete [] align_right;
    delete [] max_part_width;

    for (size_t s = 0; s<species_count; ++s) delete row[s];
    delete [] row;

    disp_device->invisible(Origin);  // @@@ remove when size-dev works
    disp_device->invisible(Position(max_x, y_position+scaled_branch_distance));  // @@@ remove when size-dev works
}

void AWT_graphic_tree::read_tree_settings() {
    scaled_branch_distance = aw_root->awar(AWAR_DTREE_VERICAL_DIST)->read_float(); // not final value!
    grey_level             = aw_root->awar(AWAR_DTREE_GREY_LEVEL)->read_int()*.01;
    baselinewidth          = aw_root->awar(AWAR_DTREE_BASELINEWIDTH)->read_int();
    show_brackets          = aw_root->awar(AWAR_DTREE_SHOW_BRACKETS)->read_int();
    show_circle            = aw_root->awar(AWAR_DTREE_SHOW_CIRCLE)->read_int();
    circle_zoom_factor     = aw_root->awar(AWAR_DTREE_CIRCLE_ZOOM)->read_float();
    circle_max_size        = aw_root->awar(AWAR_DTREE_CIRCLE_MAX_SIZE)->read_float();
    use_ellipse            = aw_root->awar(AWAR_DTREE_USE_ELLIPSE)->read_int();

    freeset(species_name, aw_root->awar(AWAR_SPECIES_NAME)->read_string());
}

void AWT_graphic_tree::apply_zoom_settings_for_treetype(AWT_canvas *ntw) {
    exports.set_standard_default_padding();

    if (ntw) {
        bool zoom_fit_text       = false;
        int  left_padding  = 0;
        int  right_padding = 0;

        switch (tree_sort) {
            case AP_TREE_RADIAL:
                zoom_fit_text = aw_root->awar(AWAR_DTREE_RADIAL_ZOOM_TEXT)->read_int();
                left_padding  = aw_root->awar(AWAR_DTREE_RADIAL_XPAD)->read_int();
                right_padding = left_padding;
                break;

            case AP_TREE_NORMAL:
            case AP_TREE_IRS:
                zoom_fit_text = aw_root->awar(AWAR_DTREE_DENDRO_ZOOM_TEXT)->read_int();
                left_padding  = STANDARD_PADDING;
                right_padding = aw_root->awar(AWAR_DTREE_DENDRO_XPAD)->read_int();
                break;

            default :
                break;
        }

        exports.set_default_padding(STANDARD_PADDING, STANDARD_PADDING, left_padding, right_padding);

        ntw->set_consider_text_for_zoom_reset(zoom_fit_text);
    }
}

void AWT_graphic_tree::show(AW_device *device) {
    if (tree_static && tree_static->get_gb_tree()) {
        check_update(gb_main);
    }

    read_tree_settings();

    disp_device = device;
    disp_device->reset_style();

    const AW_font_limits& charLimits = disp_device->get_font_limits(AWT_GC_SELECTED, 0);

    scaled_font.init(charLimits, device->get_unscale());
    scaled_branch_distance *= scaled_font.height;

    make_node_text_init(gb_main);

    cursor = Origin;

    if (!tree_root_display && sort_is_tree_style(tree_sort)) { // if there is no tree, but display style needs tree
        static const char *no_tree_text[] = {
            "No tree (selected)",
            "",
            "In the top area you may click on",
            "- the listview-button to see a plain list of species",
            "- the tree-selection-button to select a tree",
            NULL
        };

        Position p0(0, -3*scaled_branch_distance);
        cursor = p0;
        for (int i = 0; no_tree_text[i]; ++i) {
            cursor.movey(scaled_branch_distance);
            device->text(AWT_GC_CURSOR, no_tree_text[i], cursor);
        }
        device->line(AWT_GC_CURSOR, p0, cursor);
    }
    else {
        switch (tree_sort) {
            case AP_TREE_NORMAL: {
                DendroSubtreeLimits limits;
                Position pen(0, 0.05);
                show_dendrogram(tree_root_display, pen, limits);
                drawMatchFlagNames(tree_root_display, pen);
                list_tree_ruler_y = pen.ypos() + 3.0 * scaled_branch_distance;
                break;
            }
            case AP_TREE_RADIAL:
                empty_box(tree_root_display->gr.gc, Origin, NT_ROOT_WIDTH);
                show_radial_tree(tree_root_display, 0, 0, 2*M_PI, 0.0, 0, 0);
                break;

            case AP_TREE_IRS:
                show_irs_tree(tree_root_display, scaled_branch_distance);
                break;

            case AP_LIST_NDS:       // this is the list all/marked species mode (no tree)
                show_nds_list(gb_main, true);
                break;

            case AP_LIST_SIMPLE:    // simple list of names (used at startup only)
                show_nds_list(gb_main, false);
                break;
        }
        if (are_distinct(Origin, cursor)) empty_box(AWT_GC_CURSOR, cursor, NT_SELECTED_WIDTH);
        if (sort_is_tree_style(tree_sort)) show_ruler(disp_device, AWT_GC_CURSOR);
    }

    disp_device = NULL;
}

void AWT_graphic_tree::info(AW_device */*device*/, AW_pos /*x*/, AW_pos /*y*/, AW_clicked_line */*cl*/, AW_clicked_text */*ct*/) {
    aw_message("INFO MESSAGE");
}

AWT_graphic_tree *NT_generate_tree(AW_root *root, GBDATA *gb_main, AD_map_viewer_cb map_viewer_cb) {
    AWT_graphic_tree *apdt    = new AWT_graphic_tree(root, gb_main, map_viewer_cb);
    apdt->init(AP_tree(0), new AliView(gb_main), NULL, true, false); // tree w/o sequence data
    return apdt;
}

void awt_create_dtree_awars(AW_root *aw_root, AW_default def)
{
    aw_root->awar_int(AWAR_DTREE_BASELINEWIDTH, 1, def)->set_minmax(1, 10);
    aw_root->awar_float(AWAR_DTREE_VERICAL_DIST, 1.0, def)->set_minmax(0.01, 30);
    aw_root->awar_int(AWAR_DTREE_AUTO_JUMP, 1, def);

    aw_root->awar_int(AWAR_DTREE_SHOW_BRACKETS, 1, def);
    aw_root->awar_int(AWAR_DTREE_SHOW_CIRCLE, 0, def);
    aw_root->awar_int(AWAR_DTREE_USE_ELLIPSE, 1, def);

    aw_root->awar_float(AWAR_DTREE_CIRCLE_ZOOM, 1.0, def)   ->set_minmax(0.01, 20);
    aw_root->awar_float(AWAR_DTREE_CIRCLE_MAX_SIZE, 1.5, def) ->set_minmax(0.01, 200);
    aw_root->awar_int(AWAR_DTREE_GREY_LEVEL, 20, def)       ->set_minmax(0, 100);

    aw_root->awar_int(AWAR_DTREE_RADIAL_ZOOM_TEXT, 0, def);
    aw_root->awar_int(AWAR_DTREE_RADIAL_XPAD, 150, def);
    aw_root->awar_int(AWAR_DTREE_DENDRO_ZOOM_TEXT, 0, def);
    aw_root->awar_int(AWAR_DTREE_DENDRO_XPAD, 300, def);

    aw_root->awar_int(AWAR_TREE_REFRESH, 0, def);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>
#include <../../WINDOW/aw_common.hxx>

static void fake_AD_map_viewer_cb(GBDATA *, AD_MAP_VIEWER_TYPE ) {}

static AW_rgb colors_def[] = {
    AW_NO_COLOR, AW_NO_COLOR, AW_NO_COLOR, AW_NO_COLOR, AW_NO_COLOR, AW_NO_COLOR,
    0x30b0e0,
    0xff8800, // AWT_GC_CURSOR
    0xa3b3cf, // AWT_GC_BRANCH_REMARK
    0x53d3ff, // AWT_GC_BOOTSTRAP
    0x808080, // AWT_GC_BOOTSTRAP_LIMITED
    0x000000, // AWT_GC_GROUPS
    0xf0c000, // AWT_GC_SELECTED
    0xbb8833, // AWT_GC_UNDIFF
    0x622300, // AWT_GC_NSELECTED
    0x977a0e, // AWT_GC_SOME_MISMATCHES
    0x000000, // AWT_GC_BLACK
    0xffff00, // AWT_GC_YELLOW
    0xff0000, // AWT_GC_RED
    0xff00ff, // AWT_GC_MAGENTA
    0x00ff00, // AWT_GC_GREEN
    0x00ffff, // AWT_GC_CYAN
    0x0000ff, // AWT_GC_BLUE
    0x808080, // AWT_GC_WHITE
    0xd50000, // AWT_GC_FIRST_COLOR_GROUP
    0x00c0a0,
    0x00ff77,
    0xc700c7,
    0x0000ff,
    0xffce5b,
    0xab2323,
    0x008888,
    0x008800,
    0x880088,
    0x000088,
    0x888800,
    AW_NO_COLOR
};
static AW_rgb *fcolors       = colors_def;
static AW_rgb *dcolors       = colors_def;
static long    dcolors_count = ARRAY_ELEMS(colors_def);

class fake_AW_GC : public AW_GC {
    virtual void wm_set_fill_solid() OVERRIDE {  }
    virtual void wm_set_fill_stipple() OVERRIDE {  }
    virtual void wm_set_foreground_color(AW_rgb /*col*/) OVERRIDE {  }
    virtual void wm_set_function(AW_function /*mode*/) OVERRIDE { td_assert(0); }
    virtual void wm_set_lineattributes(short /*lwidth*/, AW_linestyle /*lstyle*/) OVERRIDE {}
    virtual void wm_set_font(AW_font /*font_nr*/, int size, int */*found_size*/) OVERRIDE {
        unsigned int i;
        for (i = AW_FONTINFO_CHAR_ASCII_MIN; i <= AW_FONTINFO_CHAR_ASCII_MAX; i++) {
            set_char_size(i, size, 0, size-2); // good fake size for Courier 8pt
        }
    }
public:
    fake_AW_GC(AW_common *common_) : AW_GC(common_) {}
    virtual int get_available_fontsizes(AW_font /*font_nr*/, int */*available_sizes*/) const OVERRIDE {
        td_assert(0);
        return 0;
    }
};

class fake_AW_common : public AW_common {
public:
    fake_AW_common()
        : AW_common(fcolors, dcolors, dcolors_count)
    {
        for (int gc = 0; gc < dcolors_count; ++gc) { // gcs used in this example
            new_gc(gc);
            AW_GC *gcm = map_mod_gc(gc);
            gcm->set_line_attributes(1, AW_SOLID);
            gcm->set_function(AW_COPY);
            gcm->set_font(12, 8, NULL); // 12 is Courier (use monospaced here, cause font limits are faked)

            gcm->set_fg_color(colors_def[gc+AW_STD_COLOR_IDX_MAX]);
        }
    }
    virtual ~fake_AW_common() OVERRIDE {}

    virtual AW_GC *create_gc() {
        return new fake_AW_GC(this);
    }
};

class fake_AWT_graphic_tree : public AWT_graphic_tree {
    int var_mode;

    virtual void read_tree_settings() OVERRIDE {
        scaled_branch_distance = 1.0; // not final value!
        // var_mode is in range [0..3]
        // it is used to vary tree settings such that many different combinations get tested
        grey_level         = 20*.01;
        baselinewidth      = (var_mode == 3)+1;
        show_brackets      = (var_mode != 2);
        show_circle        = var_mode%3;
        use_ellipse        = var_mode%2;
        circle_zoom_factor = 1.3;
        circle_max_size    = 1.5;
    }

public:
    fake_AWT_graphic_tree(GBDATA *gbmain, const char *selected_species)
        : AWT_graphic_tree(NULL, gbmain, fake_AD_map_viewer_cb),
          var_mode(0)
    {
        species_name = strdup(selected_species);
    }

    void set_var_mode(int mode) { var_mode = mode; }
    void test_show_tree(AW_device *device) { show(device); }

    void test_print_tree(AW_device_print *print_device, AP_tree_sort type, bool show_handles) {
        const int      SCREENSIZE = 541; // use a prime as screensize to reduce rounding errors
        AW_device_size size_device(print_device->get_common());

        size_device.reset();
        size_device.zoom(1.0);
        size_device.set_filter(AW_SIZE|AW_SIZE_UNSCALED);
        test_show_tree(&size_device);

        Rectangle drawn = size_device.get_size_information();

        td_assert(drawn.surface() >= 0.0);

        double zoomx = SCREENSIZE/drawn.width();
        double zoomy = SCREENSIZE/drawn.height();
        double zoom  = 0.0;

        switch (type) {
            case AP_LIST_SIMPLE:
            case AP_TREE_RADIAL:
                zoom = std::max(zoomx, zoomy);
                break;

            case AP_TREE_NORMAL:
            case AP_TREE_IRS:
                zoom = zoomx;
                break;

            case AP_LIST_NDS:
                zoom = 1.0;
                break;
        }

        if (!nearlyEqual(zoom, 1.0)) {
            // recalculate size
            size_device.clear();
            size_device.reset();
            size_device.zoom(zoom);
            size_device.set_filter(AW_SIZE|AW_SIZE_UNSCALED);
            test_show_tree(&size_device);
        }

        drawn = size_device.get_size_information();

        const AW_borders& text_overlap = size_device.get_unscaleable_overlap();
        Rectangle         drawn_text   = size_device.get_size_information_inclusive_text();

        int            EXTRA = SCREENSIZE*0.05;
        AW_screen_area clipping;

        clipping.l = 0; clipping.r = drawn.width()+text_overlap.l+text_overlap.r + 2*EXTRA;
        clipping.t = 0; clipping.b = drawn.height()+text_overlap.t+text_overlap.b + 2*EXTRA;

        print_device->get_common()->set_screen(clipping);
        print_device->set_filter(AW_PRINTER|(show_handles ? AW_PRINTER_EXT : 0));
        print_device->reset();

        print_device->zoom(zoom);

        Rectangle drawn_world      = print_device->rtransform(drawn);
        Rectangle drawn_text_world = print_device->rtransform(drawn_text);

        Vector extra_shift  = Vector(EXTRA, EXTRA);
        Vector corner_shift = -Vector(drawn.upper_left_corner());
        Vector text_shift = Vector(text_overlap.l, text_overlap.t);

        Vector offset(extra_shift+corner_shift+text_shift);
        print_device->set_offset(offset/(zoom*zoom)); // dont really understand this, but it does the right shift

        test_show_tree(print_device);
        print_device->box(AWT_GC_CURSOR, false, drawn_world);
        print_device->box(AWT_GC_GROUPS, false, drawn_text_world);
    }
};

void fake_AW_init_color_groups();
void AW_init_color_groups(AW_root *awr, AW_default def);


void TEST_treeDisplay() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("../../demo.arb", "r");

    fake_AWT_graphic_tree agt(gb_main, "OctSprin");
    fake_AW_common        fake_common;

    AW_device_print print_dev(&fake_common);
    AW_init_color_group_defaults(NULL);
    fake_AW_init_color_groups();

    AP_tree proto(0);
    agt.init(proto, new AliView(gb_main), NULL, true, false);

    {
        GB_transaction ta(gb_main);
        ASSERT_RESULT(const char *, NULL, agt.load(NULL, "tree_test", 0, 0));
    }

    const char *spoolnameof[] = {
        "dendro",
        "radial",
        "irs",
        "nds",
        NULL, // "simple", (too simple, need no test)
    };

    for (int show_handles = 0; show_handles <= 1; ++show_handles) {
        for (int color = 0; color <= 1; ++color) {
            print_dev.set_color_mode(color);
            // for (AP_tree_sort type = AP_TREE_NORMAL; type <= AP_LIST_SIMPLE; type = AP_tree_sort(type+1)) {
            for (AP_tree_sort type = AP_LIST_SIMPLE; type >= AP_TREE_NORMAL; type = AP_tree_sort(type-1)) { // now passes
                if (spoolnameof[type]) {
                    char *spool_name     = GBS_global_string_copy("display/%s_%c%c", spoolnameof[type], "MC"[color], "NH"[show_handles]);
                    char *spool_file     = GBS_global_string_copy("%s_curr.fig", spool_name);
                    char *spool_expected = GBS_global_string_copy("%s.fig", spool_name);


// #define TEST_AUTO_UPDATE // dont test, instead update expected results

                    agt.set_tree_type(type, NULL);

#if defined(TEST_AUTO_UPDATE)
#warning TEST_AUTO_UPDATE is active (non-default)
                    TEST_EXPECT_NO_ERROR(print_dev.open(spool_expected));
#else
                    TEST_EXPECT_NO_ERROR(print_dev.open(spool_file));
#endif

                    {
                        GB_transaction ta(gb_main);
                        agt.set_var_mode(show_handles+2*color);
                        agt.test_print_tree(&print_dev, type, show_handles);
                    }

                    print_dev.close();

#if !defined(TEST_AUTO_UPDATE)
                    // if (strcmp(spool_expected, "display/irs_CH.fig") == 0) {
                        TEST_EXPECT_TEXTFILES_EQUAL(spool_expected, spool_file);
                    // }
                    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink(spool_file));
#endif
                    free(spool_expected);
                    free(spool_file);
                    free(spool_name);
                }
            }
        }
    }

    GB_close(gb_main);
}


#endif // UNIT_TESTS
