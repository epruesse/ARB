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
#include <arb_diff.h>

#include <unistd.h>
#include <iostream>
#include <arb_global_defs.h>
#include <cfloat>

/*!*************************
  class AP_tree
****************************/

#define RULER_LINEWIDTH "ruler/ruler_width" // line width of ruler
#define RULER_SIZE      "ruler/size"

#define DEFAULT_RULER_LINEWIDTH tree_defaults::LINEWIDTH
#define DEFAULT_RULER_LENGTH    tree_defaults::LENGTH

using namespace AW;

AW_gc_manager AWT_graphic_tree::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw) {
    AW_gc_manager gc_manager =
        AW_manage_GC(aww,
                     ntw->get_gc_base_name(),
                     device, AWT_GC_CURSOR, AWT_GC_MAX, AW_GCM_DATA_AREA,
                     makeWindowCallback(AWT_resize_cb, ntw),
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

    return gc_manager;
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

    GB_transaction ta(tree_static->get_gb_main());
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
            GB_transaction ta(tree_static->get_gb_main());

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
            GB_transaction ta(tree_static->get_gb_main());

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

    CollapseMode next_expand_mode() const {
        if (closed_with_marked) return EXPAND_MARKED;
        return EXPAND_ALL;
    }

    CollapseMode next_collapse_mode() const {
        if (all_terminal_closed()) return COLLAPSE_ALL;
        return COLLAPSE_TERMINAL;
    }
};

void AWT_graphic_tree::detect_group_state(AP_tree *at, AWT_graphic_tree_group_state *state, AP_tree *skip_this_son) {
    if (!at) return;
    if (at->is_leaf) {
        if (at->gb_node && GB_read_flag(at->gb_node)) state->marked_outside_groups++; // count marks
        return;                 // leafs never get grouped
    }

    if (at->is_named_group()) {
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

void AWT_graphic_tree::group_rest_tree(AP_tree *at, CollapseMode mode, int color_group) {
    if (at) {
        AP_tree *pa = at->get_father();
        if (pa) {
            group_tree(at->get_brother(), mode, color_group);
            group_rest_tree(pa, mode, color_group);
        }
    }
}

bool AWT_graphic_tree::group_tree(AP_tree *at, CollapseMode mode, int color_group) {
    /*! collapse/expand subtree according to mode and color_group
     * Run on father! (why?)
     * @return true if subtree shall expand
     */

    if (!at) return true;

    GB_transaction ta(tree_static->get_gb_main());

    bool expand_me = false;
    if (at->is_leaf) {
        if (mode & EXPAND_ALL) expand_me = true;
        if (at->gb_node) { // ignore zombies
            if (!expand_me && (mode & EXPAND_MARKED)) { // do not group marked
                if (GB_read_flag(at->gb_node)) expand_me = true;
            }
            if (!expand_me && (mode & EXPAND_COLOR)) { // do not group specified color_group
                int my_color_group = AW_find_color_group(at->gb_node, true);
                if (my_color_group == color_group) expand_me = true;
            }
        }
    }
    else {
        expand_me = group_tree(at->get_leftson (), mode, color_group);
        expand_me = group_tree(at->get_rightson(), mode, color_group) || expand_me;

        at->gr.grouped = 0;

        if (!expand_me) { // no son requests to be expanded
            if (at->is_named_group()) {
                at->gr.grouped = 1; // group me
                if (mode & COLLAPSE_TERMINAL) expand_me = true; // do not group non-terminal groups
            }
        }
        if (!at->father) get_root_node()->compute_tree();
    }
    return expand_me;
}

void AWT_graphic_tree::reorder_tree(TreeOrder mode) {
    AP_tree *at = get_root_node();
    if (at) {
        at->reorder_tree(mode);
        at->compute_tree();
    }
}

static void show_bootstrap_circle(AW_device *device, const char *bootstrap, double zoom_factor, double max_radius, double len, const Position& center, bool elipsoid, double elip_ysize, int filter) { // @@@ directly pass bootstrap value?
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

static void AWT_graphic_tree_root_changed(void *cd, AP_tree *old, AP_tree *newroot) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    if (agt->displayed_root == old || agt->displayed_root->is_inside(old)) {
        agt->displayed_root = newroot;
    }
}

static void AWT_graphic_tree_node_deleted(void *cd, AP_tree *del) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    if (agt->displayed_root == del) {
        agt->displayed_root = agt->get_root_node();
    }
    if (agt->get_root_node() == del) {
        agt->displayed_root = 0;
    }
}
void AWT_graphic_tree::toggle_group(AP_tree * at) {
    GB_ERROR error = NULL;

    if (at->is_named_group()) { // existing group
        const char *msg = GBS_global_string("What to do with group '%s'?", at->name);

        switch (aw_question(NULL, msg, "Rename,Destroy,Cancel")) {
            case 0: { // rename
                char *new_gname = aw_input("Rename group", "Change group name:", at->name);
                if (new_gname) {
                    freeset(at->name, new_gname);
                    error = GBT_write_string(at->gb_node, "group_name", new_gname);
                    exports.save = !error;
                }
                break;
            }

            case 1: // destroy
                at->gr.grouped = 0;
                at->name       = 0;
                error          = GB_delete(at->gb_node); // ODD: expecting this to also destroy linewidth, rot and spread - but it doesn't!
                at->gb_node    = 0;
                exports.save = !error; // ODD: even when commenting out this line info is not deleted
                break;

            case 2: // cancel
                break;
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
        if (gname && gname[0]) {
            GBDATA         *gb_tree  = tree_static->get_gb_tree();
            GBDATA         *gb_mainT = GB_get_root(gb_tree);
            GB_transaction  ta(gb_mainT);

            GBDATA *gb_node     = GB_create_container(gb_tree, "node");
            if (!gb_node) error = GB_await_error();

            if (at->gb_node) {                                     // already have existing node info (e.g. for linewidth)
                if (!error) error = GB_copy(gb_node, at->gb_node); // copy existing node and ..
                if (!error) error = GB_delete(at->gb_node);        // .. delete old one (to trigger invalidation of taxonomy-cache)
            }

            if (!error) error = GBT_write_int(gb_node, "id", 0); // force re-creation of node-id

            if (!error) {
                GBDATA *gb_name     = GB_search(gb_node, "group_name", GB_STRING);
                if (!gb_name) error = GB_await_error();
                else    error       = GBT_write_group_name(gb_name, gname);
            }
            exports.save = !error;
            if (!error) {
                at->gb_node = gb_node;
                at->name    = gname;
            }
            error = ta.close(error);
        }
    }
    else if (!at->gb_node) {
        td_assert(0); // please note here if this else-branch is ever reached (and if: how!)
        at->gb_node = GB_create_container(tree_static->get_gb_tree(), "node");

        if (!at->gb_node) error = GB_await_error();
        else {
            error = GBT_write_int(at->gb_node, "id", 0);
            exports.save = !error;
        }
    }

    return error;
}

class Dragged : public AWT_command_data {
    /*! Is dragged and can be dropped.
     * Knows how to indicate dragging.
     */
    AWT_graphic_exports& exports;

protected:
    AWT_graphic_exports& get_exports() { return exports; }

public:
    enum DragAction { DRAGGING, DROPPED };

    Dragged(AWT_graphic_exports& exports_)
        : exports(exports_)
    {}

    static bool valid_drag_device(AW_device *device) { return device->type() == AW_DEVICE_SCREEN; }

    virtual void draw_drag_indicator(AW_device *device, int drag_gc) const = 0;
    virtual void perform(DragAction action, const AW_clicked_element *target, const Position& mousepos) = 0;
    virtual void abort() = 0;

    void do_drag(const AW_clicked_element *drag_target, const Position& mousepos) {
        perform(DRAGGING, drag_target, mousepos);
    }
    void do_drop(const AW_clicked_element *drop_target, const Position& mousepos) {
        perform(DROPPED, drop_target, mousepos);
    }

    void hide_drag_indicator(AW_device *device, int drag_gc) const {
#ifndef ARB_GTK
        // hide by XOR-drawing only works in motif
        draw_drag_indicator(device, drag_gc);
#else
        exports.refresh = 1;
#endif
    }
};

bool AWT_graphic_tree::warn_inappropriate_mode(AWT_COMMAND_MODE mode) {
    if (mode == AWT_MODE_ROTATE || mode == AWT_MODE_SPREAD) {
        if (tree_sort != AP_TREE_RADIAL) {
            aw_message("Please select the radial tree display mode to use this command");
            return true;
        }
    }
    return false;
}


void AWT_graphic_tree::handle_key(AW_device *device, AWT_graphic_event& event) {
    //! handles AW_Event_Type = AW_Keyboard_Press.

    td_assert(event.type() == AW_Keyboard_Press);

    if (event.key_code() == AW_KEY_NONE) return;
    if (event.key_code() == AW_KEY_ASCII && event.key_char() == 0) return;

#if defined(DEBUG)
    printf("key_char=%i (=%c)\n", int(event.key_char()), event.key_char());
#endif // DEBUG

    // ------------------------
    //      drag&drop keys

    if (event.key_code() == AW_KEY_ESCAPE) {
        AWT_command_data *cmddata = get_command_data();
        if (cmddata) {
            Dragged *dragging = dynamic_cast<Dragged*>(cmddata);
            if (dragging) {
                dragging->hide_drag_indicator(device, drag_gc);
                dragging->abort(); // abort what ever we did
                store_command_data(NULL);
            }
        }
    }

    // ----------------------------------------
    //      commands independent of tree :

    bool handled = true;
    switch (event.key_char()) {
        case 9: {     // Ctrl-i = invert all
            GBT_mark_all(gb_main, 2);
            exports.structure_change = 1;
            break;
        }
        case 13: {     // Ctrl-m = mark/unmark all
            int mark   = GBT_first_marked_species(gb_main) == 0; // no species marked -> mark all
            GBT_mark_all(gb_main, mark);
            exports.structure_change = 1;
            break;
        }
        default: {
            handled = false;
            break;
        }
    }

    if (!handled) {
        // handle key events specific to pointed-to tree-element
        ClickedTarget pointed(this, event.best_click());

        if (pointed.species()) {
            handled = true;
            switch (event.key_char()) {
                case 'i':
                case 'm': {     // i/m = mark/unmark species
                    GB_write_flag(pointed.species(), 1-GB_read_flag(pointed.species()));
                    exports.structure_change = 1;
                    break;
                }
                case 'I': {     // I = invert all but current
                    int mark = GB_read_flag(pointed.species());
                    GBT_mark_all(gb_main, 2);
                    GB_write_flag(pointed.species(), mark);
                    exports.structure_change = 1;
                    break;
                }
                case 'M': {     // M = mark/unmark all but current
                    int mark = GB_read_flag(pointed.species());
                    GB_write_flag(pointed.species(), 0); // unmark current
                    GBT_mark_all(gb_main, GBT_first_marked_species(gb_main) == 0);
                    GB_write_flag(pointed.species(), mark); // restore mark of current
                    exports.structure_change = 1;
                    break;
                }
                default: handled = false; break;
            }
        }

        if (!handled && event.key_char() == '0') {
            // handle reset-key promised by
            // - KEYINFO_ABORT_AND_RESET (AWT_MODE_ROTATE, AWT_MODE_LENGTH, AWT_MODE_MULTIFURC, AWT_MODE_LINE, AWT_MODE_SPREAD)
            // - KEYINFO_RESET (AWT_MODE_LZOOM)

            if (event.cmd() == AWT_MODE_LZOOM) {
                displayed_root     = displayed_root->get_root_node();
                exports.zoom_reset = 1;
            }
            else if (pointed.is_ruler()) {
                GBDATA *gb_tree = tree_static->get_gb_tree();
                td_assert(gb_tree);

                switch (event.cmd()) {
                    case AWT_MODE_ROTATE: break; // ruler has no rotation
                    case AWT_MODE_SPREAD: break; // ruler has no spread
                    case AWT_MODE_LENGTH: {
                        GB_transaction ta(gb_tree);
                        GBDATA *gb_ruler_size = GB_searchOrCreate_float(gb_tree, RULER_SIZE, DEFAULT_RULER_LENGTH);
                        GB_write_float(gb_ruler_size, DEFAULT_RULER_LENGTH);
                        exports.structure_change = 1;
                        break;
                    }
                    case AWT_MODE_LINE: {
                        GB_transaction ta(gb_tree);
                        GBDATA *gb_ruler_width = GB_searchOrCreate_int(gb_tree, RULER_LINEWIDTH, DEFAULT_RULER_LINEWIDTH);
                        GB_write_int(gb_ruler_width, DEFAULT_RULER_LINEWIDTH);
                        exports.structure_change = 1;
                        break;
                    }
                    default: break;
                }
            }
            else if (pointed.node()) {
                if (warn_inappropriate_mode(event.cmd())) return;
                switch (event.cmd()) {
                    case AWT_MODE_ROTATE:
                        pointed.node()->reset_subtree_angles();
                        exports.save = 1;
                        break;

                    case AWT_MODE_LENGTH:
                        pointed.node()->set_branchlength_unrooted(0.0);
                        exports.save = 1;
                        break;

                    case AWT_MODE_MULTIFURC:
                        pointed.node()->multifurcate();
                        exports.save = 1;
                        break;

                    case AWT_MODE_LINE:
                        pointed.node()->reset_subtree_linewidths();
                        exports.save = 1;
                        break;

                    case AWT_MODE_SPREAD:
                        pointed.node()->reset_subtree_spreads();
                        exports.save = 1;
                        break;

                    default: break;
                }
            }
            return;
        }

        if (pointed.node()) {
            switch (event.key_char()) {
                case 'm': {     // m = mark/unmark (sub)tree
                    mark_species_in_tree(pointed.node(), !tree_has_marks(pointed.node()));
                    exports.structure_change = 1;
                    break;
                }
                case 'M': {     // M = mark/unmark all but (sub)tree
                    mark_species_in_rest_of_tree(pointed.node(), !rest_tree_has_marks(pointed.node()));
                    exports.structure_change = 1;
                    break;
                }

                case 'i': {     // i = invert (sub)tree
                    mark_species_in_tree(pointed.node(), 2);
                    exports.structure_change = 1;
                    break;
                }
                case 'I': {     // I = invert all but (sub)tree
                    mark_species_in_rest_of_tree(pointed.node(), 2);
                    exports.structure_change = 1;
                    break;
                }
                case 'c':
                case 'x': {
                    AWT_graphic_tree_group_state  state;
                    AP_tree                      *at = pointed.node();

                    detect_group_state(at, &state, 0);

                    if (!state.has_groups()) { // somewhere inside group
                      do_parent :
                        at  = at->get_father();
                        while (at) {
                            if (at->is_named_group()) break;
                            at = at->get_father();
                        }

                        if (at) {
                            state.clear();
                            detect_group_state(at, &state, 0);
                        }
                    }

                    if (at) {
                        CollapseMode next_group_mode;

                        if (event.key_char() == 'x') {  // expand
                            next_group_mode = state.next_expand_mode();
                        }
                        else { // collapse
                            if (state.all_closed()) goto do_parent;
                            next_group_mode = state.next_collapse_mode();
                        }

                        group_tree(at, next_group_mode, 0);
                        exports.save    = true;
                    }
                    break;
                }
                case 'C':
                case 'X': {
                    AP_tree *root_node = pointed.node();
                    while (root_node->father) root_node = root_node->get_father(); // search father

                    td_assert(root_node);

                    AWT_graphic_tree_group_state state;
                    detect_group_state(root_node, &state, pointed.node());

                    CollapseMode next_group_mode;
                    if (event.key_char() == 'X') {  // expand
                        next_group_mode = state.next_expand_mode();
                    }
                    else { // collapse
                        next_group_mode = state.next_collapse_mode();
                    }

                    group_rest_tree(pointed.node(), next_group_mode, 0);
                    exports.save = true;

                    break;
                }
            }
        }
    }
}

static bool command_on_GBDATA(GBDATA *gbd, const AWT_graphic_event& event, AD_map_viewer_cb map_viewer_cb) {
    // modes listed here are available in ALL tree-display-modes (i.e. as well in listmode)

    bool refresh = false;

    if (event.type() == AW_Mouse_Press && event.button() != AW_BUTTON_MIDDLE) {
        AD_MAP_VIEWER_TYPE selectType = ADMVT_NONE;

        switch (event.cmd()) {
            case AWT_MODE_MARK: // see also .@OTHER_MODE_MARK_HANDLER
                switch (event.button()) {
                    case AW_BUTTON_LEFT:
                        GB_write_flag(gbd, 1);
                        selectType = ADMVT_SELECT;
                        break;
                    case AW_BUTTON_RIGHT:
                        GB_write_flag(gbd, 0);
                        break;
                    default:
                        break;
                }
                refresh = true;
                break;

            case AWT_MODE_WWW:  selectType = ADMVT_WWW;    break;
            case AWT_MODE_INFO: selectType = ADMVT_INFO;   break;
            default:            selectType = ADMVT_SELECT; break;
        }

        if (selectType != ADMVT_NONE) {
            map_viewer_cb(gbd, selectType);
            refresh = true;
        }
    }

    return refresh;
}

class LineOrText {
    /*! Stores a copy of AW_clicked_line or AW_clicked_text.
     * Used as Drag&Drop source and target.
     */
    AW_clicked_line line;
    AW_clicked_text text;
    const AW_clicked_element *elem;

public:
    LineOrText(const AW_clicked_element& e) { set(e); }
    LineOrText(const LineOrText& other) { set(*other.element()); }
    DECLARE_ASSIGNMENT_OPERATOR(LineOrText);

    void set(const AW_clicked_line& l) { line = l; elem = &line; }
    void set(const AW_clicked_text& t) { text = t; elem = &text; }
    void set(const AW_clicked_element& e) {
        if (e.is_text()) set(dynamic_cast<const AW_clicked_text&>(e));
        else set(dynamic_cast<const AW_clicked_line&>(e));
    }
    const AW_clicked_element *element() const { return elem; }

    bool operator == (const LineOrText& other) const {
        return
            element()->is_text() == other.element()->is_text() &&
            line                 == other.line                 &&
            text                 == other.text;
    }
    bool operator != (const LineOrText& other) const { return !(*this == other); }
};

class DragNDrop : public Dragged {
    LineOrText Drag, Drop;

    virtual void perform_drop() = 0;

    void drag(const AW_clicked_element *drag_target)  {
        Drop = drag_target ? *drag_target : Drag;
    }
    void drop(const AW_clicked_element *drop_target) {
        drag(drop_target);
        perform_drop();
    }

    void perform(DragAction action, const AW_clicked_element *target, const Position&) OVERRIDE {
        switch (action) {
            case DRAGGING: drag(target); break;
            case DROPPED:  drop(target); break;
        }
    }

    void abort() OVERRIDE {
        perform(DROPPED, Drag.element(), Position()); // drop dragged element onto itself to abort
    }

protected:
    const AW_clicked_element *source_element() const { return Drag.element(); }
    const AW_clicked_element *dest_element() const { return Drop.element(); }

public:
    DragNDrop(const AW_clicked_element *dragFrom, AWT_graphic_exports& exports_)
        : Dragged(exports_),
          Drag(*dragFrom), Drop(Drag)
    {}

    void draw_drag_indicator(AW_device *device, int drag_gc) const {
        td_assert(valid_drag_device(device));
        source_element()->indicate_selected(device, drag_gc);
        if (Drag != Drop) dest_element()->indicate_selected(device, drag_gc);
        device->line(drag_gc, source_element()->get_connecting_line(*dest_element()));
    }
};

class BranchMover : public DragNDrop {
    AW_MouseButton button;

    void perform_drop() OVERRIDE {
        ClickedTarget source(source_element());
        ClickedTarget dest(dest_element());

        if (source.node() && dest.node() && source.node() != dest.node()) {
            GB_ERROR error = NULL;
            switch (button) {
                case AW_BUTTON_LEFT:
                    error = source.node()->cantMoveNextTo(dest.node());
                    if (!error) source.node()->moveNextTo(dest.node(), dest.get_rel_attach());
                    break;

                case AW_BUTTON_RIGHT:
                    error = source.node()->move_group_info(dest.node());
                    break;
                default:
                    td_assert(0);
                    break;
            }

            if (error) {
                aw_message(error);
            }
            else {
                get_exports().save = 1;
            }
        }
        else {
#if defined(DEBUG)
            if (!source.node()) printf("No source.node\n");
            if (!dest.node()) printf("No dest.node\n");
            if (dest.node() == source.node()) printf("source==dest\n");
#endif
        }
    }

public:
    BranchMover(const AW_clicked_element *dragFrom, AW_MouseButton button_, AWT_graphic_exports& exports_)
        : DragNDrop(dragFrom, exports_),
          button(button_)
    {}
};


class Scaler : public Dragged {
    Position mouse_start; // screen-coordinates
    Position last_drag_pos;
    double unscale;

    virtual void draw_scale_indicator(const AW::Position& drag_pos, AW_device *device, int drag_gc) const = 0;
    virtual void do_scale(const Position& drag_pos) = 0;

    void perform(DragAction action, const AW_clicked_element *, const Position& mousepos) OVERRIDE {
        switch (action) {
            case DRAGGING:
                last_drag_pos = mousepos;
                // fall-through (aka instantly apply drop-action while dragging)
            case DROPPED:
                do_scale(mousepos);
                break;
        }
    }
    void abort() OVERRIDE {
        perform(DROPPED, NULL, mouse_start); // drop exactly where dragging started
    }


protected:
    const Position& startpos() const { return mouse_start; }
    Vector scaling(const Position& current) const { return Vector(mouse_start, current)*unscale; } // returns world-coordinates

public:
    Scaler(const Position& start, double unscale_, AWT_graphic_exports& exports_)
        : Dragged(exports_),
          mouse_start(start),
          last_drag_pos(start),
          unscale(unscale_)
    {
        td_assert(!is_nan_or_inf(unscale));
    }

    void draw_drag_indicator(AW_device *device, int drag_gc) const { draw_scale_indicator(last_drag_pos, device, drag_gc); }
};

inline double discrete_value(double analog_value, int discretion_factor) {
    // discretion_factor:
    //     10 -> 1st digit behind dot
    //    100 -> 2nd ------- " ------
    //   1000 -> 3rd ------- " ------

    if (analog_value<0.0) return -discrete_value(-analog_value, discretion_factor);
    return int(analog_value*discretion_factor+0.5)/double(discretion_factor);
}

class DB_scalable {
    //! a DB entry scalable by dragging
    GBDATA   *gbd;
    GB_TYPES  type;
    double    min;
    double    max;
    int       discretion_factor;
    bool      inversed;

    static CONSTEXPR double INTSCALE = 100.0;

    void init() {
        min = -DBL_MAX;
        max =  DBL_MAX;

        discretion_factor = 0;
        inversed          = false;
    }

public:
    DB_scalable() : gbd(NULL), type(GB_NONE) { init(); }
    DB_scalable(GBDATA *gbd_) : gbd(gbd_), type(GB_read_type(gbd)) { init(); }

    GBDATA *data() { return gbd; }

    double read() {
        double res = 0.0;
        switch (type) {
            case GB_FLOAT: res = GB_read_float(gbd);        break;
            case GB_INT:   res = GB_read_int(gbd)/INTSCALE; break;
            default: break;
        }
        return inversed ? -res : res;
    }
    bool write(double val) {
        double old = read();

        if (inversed) val = -val;

        val = val<=min ? min : (val>=max ? max : val);
        val = discretion_factor ? discrete_value(val, discretion_factor) : val;

        switch (type) {
            case GB_FLOAT:
                GB_write_float(gbd, val);
                break;
            case GB_INT:
                GB_write_int(gbd, int(val*INTSCALE+0.5));
                break;
            default: break;
        }

        return old != read();
    }

    void set_discretion_factor(int df) { discretion_factor = df; }
    void set_min(double val) { min = (type == GB_INT) ? val*INTSCALE : val; }
    void set_max(double val) { max = (type == GB_INT) ? val*INTSCALE : val; }
    void inverse() { inversed = !inversed; }
};

class RulerScaler : public Scaler { // derived from Noncopyable
    Position    awar_start;
    DB_scalable x, y; // DB entries scaled by x/y movement

    GBDATA *gbdata() {
        GBDATA *gbd   = x.data();
        if (!gbd) gbd = y.data();
        td_assert(gbd);
        return gbd;
    }

    Position read_pos() { return Position(x.read(), y.read()); }
    bool write_pos(Position p) {
        bool xchanged = x.write(p.xpos());
        bool ychanged = y.write(p.ypos());
        return xchanged || ychanged;
    }

    void draw_scale_indicator(const AW::Position& , AW_device *, int ) const {}
    void do_scale(const Position& drag_pos) {
        GB_transaction ta(gbdata());
        if (write_pos(awar_start+scaling(drag_pos))) get_exports().refresh = 1;
    }
public:
    RulerScaler(const Position& start, double unscale_, const DB_scalable& xs, const DB_scalable& ys, AWT_graphic_exports& exports_)
        : Scaler(start, unscale_, exports_),
          x(xs),
          y(ys)
    {
        GB_transaction ta(gbdata());
        awar_start = read_pos();
    }
};

static void text_near_head(AW_device *device, int gc, const LineVector& line, const char *text) {
    // @@@ should keep a little distance between the line-head and the text (depending on line orientation)
    Position at = line.head();
    device->text(gc, text, at);
}

enum ScaleMode { SCALE_LENGTH, SCALE_LENGTH_PRESERVING, SCALE_SPREAD };

class BranchScaler : public Scaler { // derived from Noncopyable
    ScaleMode  mode;
    AP_tree   *node;

    float start_val;   // length or spread
    bool  zero_val_removed;

    LineVector branch;
    Position   attach; // point on 'branch' (next to click position)

    bool discrete; // @@@ replace me by (discretion_factor == 0);
    int  discretion_factor;

    bool allow_neg_val;

    float get_val() const {
        switch (mode) {
            case SCALE_LENGTH_PRESERVING:
            case SCALE_LENGTH: return node->get_branchlength_unrooted();
            case SCALE_SPREAD: return node->gr.spread;
        }
        td_assert(0);
        return 0.0;
    }
    void set_val(float val) {
        switch (mode) {
            case SCALE_LENGTH_PRESERVING: node->set_branchlength_preserving(val); break;
            case SCALE_LENGTH: node->set_branchlength_unrooted(val); break;
            case SCALE_SPREAD: node->gr.spread = val; break;
        }
    }

    void init_discretion_factor() {
        if (discrete) {
            discretion_factor = 10;
            if (start_val != 0) {
                while ((start_val*discretion_factor)<1) {
                    discretion_factor *= 10;
                }
            }
        }
    }

    Position get_dragged_attach(const AW::Position& drag_pos) const {
        // return dragged position of 'attach'
        Vector moved      = scaling(drag_pos);
        Vector attach2tip = branch.head()-attach;

        if (attach2tip.length()>0) {
            Vector   moveOnBranch = orthogonal_projection(moved, attach2tip);
            return attach+moveOnBranch;
        }
        return Position(); // no position
    }


    void draw_scale_indicator(const AW::Position& drag_pos, AW_device *device, int drag_gc) const {
        td_assert(valid_drag_device(device));
        Position attach_dragged = get_dragged_attach(drag_pos);
        if (attach_dragged.valid()) {
            Position   drag_world = device->rtransform(drag_pos);
            LineVector to_dragged(attach_dragged, drag_world);
            LineVector to_start(attach, -to_dragged.line_vector());

            device->set_line_attributes(drag_gc, 1, AW_SOLID);

            device->line(drag_gc, to_start);
            device->line(drag_gc, to_dragged);

            text_near_head(device, drag_gc, to_start,   GBS_global_string("old=%.3f", start_val));
            text_near_head(device, drag_gc, to_dragged, GBS_global_string("new=%.3f", get_val()));
        }

        device->set_line_attributes(drag_gc, 3, AW_SOLID);
        device->line(drag_gc, branch);
    }

    void do_scale(const Position& drag_pos) {
        double oldval = get_val();

        if (start_val == 0.0) { // can't scale
            if (!zero_val_removed) {
                switch (mode) {
                    case SCALE_LENGTH:
                    case SCALE_LENGTH_PRESERVING:
                        set_val(tree_defaults::LENGTH); // fake branchlength (can't scale zero-length branches)
                        aw_message("Cannot scale zero sized branches\nBranchlength has been set to 0.1\nNow you may scale the branch");
                        break;
                    case SCALE_SPREAD:
                        set_val(tree_defaults::SPREAD); // reset spread (can't scale unspreaded branches)
                        aw_message("Cannot spread unspreaded branches\nSpreading has been set to 1.0\nNow you may spread the branch"); // @@@ clumsy
                        break;
                }
                zero_val_removed = true;
            }
        }
        else {
            Position attach_dragged = get_dragged_attach(drag_pos);
            if (attach_dragged.valid()) {
                Vector to_attach(branch.start(), attach);
                Vector to_attach_dragged(branch.start(), attach_dragged);

                double tal = to_attach.length();
                double tdl = to_attach_dragged.length();

                if (tdl>0.0 && tal>0.0) {
                    bool   negate = are_antiparallel(to_attach, to_attach_dragged);
                    double scale  = tdl/tal * (negate ? -1 : 1);

                    float val = start_val * scale;
                    if (val<0.0) {
                        if (node->is_leaf || !allow_neg_val) {
                            val = 0.0; // do NOT accept negative values
                        }
                    }
                    if (discrete) {
                        val = discrete_value(val, discretion_factor);
                    }
                    set_val(NONAN(val));
                }
            }
        }

        if (oldval != get_val()) {
            get_exports().save = 1;
        }
    }

public:

    BranchScaler(ScaleMode mode_, AP_tree *node_, const LineVector& branch_, const Position& attach_, const Position& start, double unscale_, bool discrete_, bool allow_neg_values_, AWT_graphic_exports& exports_)
        : Scaler(start, unscale_, exports_),
          mode(mode_),
          node(node_),
          start_val(get_val()),
          zero_val_removed(false),
          branch(branch_),
          attach(attach_),
          discrete(discrete_),
          allow_neg_val(allow_neg_values_)
    {
        init_discretion_factor();
    }
};

class BranchLinewidthScaler : public Scaler, virtual Noncopyable {
    AP_tree *node;
    int      start_width;
    bool     wholeSubtree;

public:
    BranchLinewidthScaler(AP_tree *node_, const Position& start, bool wholeSubtree_, AWT_graphic_exports& exports_)
        : Scaler(start, 0.1, exports_), // 0.1 = > change linewidth dragpixel/10
          node(node_),
          start_width(node->get_linewidth()),
          wholeSubtree(wholeSubtree_)
    {}

    void draw_scale_indicator(const AW::Position& , AW_device *, int ) const OVERRIDE {}
    void do_scale(const Position& drag_pos) OVERRIDE {
        Vector moved = scaling(drag_pos);
        double ymove = -moved.y();
        int    old   = node->get_linewidth();

        int width = start_width + ymove;
        if (width<tree_defaults::LINEWIDTH) width = tree_defaults::LINEWIDTH;

        if (width != old) {
            if (wholeSubtree) {
                node->set_linewidth_recursive(width);
            }
            else {
                node->set_linewidth(width);
            }
            get_exports().save = 1;
        }
    }
};

class BranchRotator : public Dragged, virtual Noncopyable {
    AW_device  *device;
    AP_tree    *node;
    LineVector  clicked_branch;
    float       orig_angle;      // of node
    Position    hinge;
    Position    mousepos_world;

    void perform(DragAction, const AW_clicked_element *, const Position& mousepos) OVERRIDE {
        mousepos_world = device->rtransform(mousepos);

        double prev_angle = node->get_angle();

        Angle current(hinge, mousepos_world);
        Angle orig(clicked_branch.line_vector());
        Angle diff = current-orig;

        node->set_angle(orig_angle + diff.radian());

        if (node->get_angle() != prev_angle) get_exports().save = 1;
    }

    void abort() OVERRIDE {
        node->set_angle(orig_angle);
        get_exports().save    = 1;
        get_exports().refresh = 1;
    }

public:
    BranchRotator(AW_device *device_, AP_tree *node_, const LineVector& clicked_branch_, const Position& mousepos, AWT_graphic_exports& exports_)
        : Dragged(exports_),
          device(device_),
          node(node_),
          clicked_branch(clicked_branch_),
          orig_angle(node->get_angle()),
          hinge(clicked_branch.start()),
          mousepos_world(device->rtransform(mousepos))
    {
        td_assert(valid_drag_device(device));
    }

    void draw_drag_indicator(AW_device *IF_DEBUG(same_device), int drag_gc) const OVERRIDE {
        td_assert(valid_drag_device(same_device));
        td_assert(device == same_device);

        device->line(drag_gc, clicked_branch);
        device->line(drag_gc, LineVector(hinge, mousepos_world));
        device->circle(drag_gc, false, hinge, device->rtransform(Vector(5, 5)));
    }
};

static AWT_graphic_event::ClickPreference preferredForCommand(AWT_COMMAND_MODE mode) {
    // return preferred click target for tree-display
    // (Note: not made this function a member of AWT_graphic_event,
    //  since modes are still reused in other ARB applications,
    //  e.g. AWT_MODE_ROTATE in SECEDIT)

    switch (mode) {
        case AWT_MODE_ROTATE:
        case AWT_MODE_LENGTH:
        case AWT_MODE_MULTIFURC:
        case AWT_MODE_SPREAD:
            return AWT_graphic_event::PREFER_LINE;

        default:
            return AWT_graphic_event::PREFER_NEARER;
    }
}

void AWT_graphic_tree::handle_command(AW_device *device, AWT_graphic_event& event) {
    td_assert(event.button()!=AW_BUTTON_MIDDLE); // shall be handled by caller

    if (!tree_static) return;                      // no tree -> no commands

    if (event.type() == AW_Keyboard_Release) return;
    if (event.type() == AW_Keyboard_Press) return handle_key(device, event);

    // @@@ move code below into separate member function handle_mouse()
    if (event.button() == AW_BUTTON_NONE) return;
    td_assert(event.button() == AW_BUTTON_LEFT || event.button() == AW_BUTTON_RIGHT); // nothing else should come here

    ClickedTarget clicked(this, event.best_click(preferredForCommand(event.cmd())));
    if (clicked.species()) {
        if (command_on_GBDATA(clicked.species(), event, map_viewer_cb)) {
            exports.refresh = 1;
        }
        return;
    }

    if (!tree_static->get_root_node()) return; // no tree -> no commands
    
    GBDATA          *gb_tree  = tree_static->get_gb_tree();
    const Position&  mousepos = event.position();

    // -------------------------------------
    //      generic drag & drop handler
    {
        AWT_command_data *cmddata = get_command_data();
        if (cmddata) {
            Dragged *dragging = dynamic_cast<Dragged*>(cmddata);
            if (dragging) {
                dragging->hide_drag_indicator(device, drag_gc);
                if (event.type() == AW_Mouse_Press) {
                    // mouse pressed while dragging (e.g. press other button)
                    dragging->abort(); // abort what ever we did
                    store_command_data(NULL);
                }
                else {
                    switch (event.type()) {
                        case AW_Mouse_Drag:
                            dragging->do_drag(clicked.element(), mousepos);
                            dragging->draw_drag_indicator(device, drag_gc);
                            break;

                        case AW_Mouse_Release:
                            dragging->do_drop(clicked.element(), mousepos);
                            store_command_data(NULL);
                            break;
                        default:
                            break;
                    }
                }
                return;
            }
        }
    }

    if (event.type() == AW_Mouse_Press && clicked.is_ruler()) {
        DB_scalable xdata;
        DB_scalable ydata;
        double      unscale = device->get_unscale();

        switch (event.cmd()) {
            case AWT_MODE_LENGTH:
            case AWT_MODE_MULTIFURC: { // scale ruler
                xdata = GB_searchOrCreate_float(gb_tree, RULER_SIZE, DEFAULT_RULER_LENGTH);

                double rel  = clicked.get_rel_attach();
                if (tree_sort == AP_TREE_IRS) {
                    unscale /= (rel-1)*irs_tree_ruler_scale_factor; // ruler has opposite orientation in IRS mode
                }
                else {
                    unscale /= rel;
                }

                if (event.button() == AW_BUTTON_RIGHT) xdata.set_discretion_factor(10);
                xdata.set_min(0.01);
                break;
            }
            case AWT_MODE_LINE: // scale ruler linewidth
                ydata = GB_searchOrCreate_int(gb_tree, RULER_LINEWIDTH, DEFAULT_RULER_LINEWIDTH);
                ydata.set_min(0);
                ydata.inverse();
                break;

            default: { // move ruler or ruler text
                bool isText = clicked.is_text();
                xdata = GB_searchOrCreate_float(gb_tree, ruler_awar(isText ? "text_x" : "ruler_x"), 0.0);
                ydata = GB_searchOrCreate_float(gb_tree, ruler_awar(isText ? "text_y" : "ruler_y"), 0.0);
                break;
            }
        }
        store_command_data(new RulerScaler(mousepos, unscale, xdata, ydata, exports));
        return;
    }

    if (event.type() == AW_Mouse_Press && warn_inappropriate_mode(event.cmd())) return;

    switch (event.cmd()) {
        // -----------------------------
        //      two point commands:

        case AWT_MODE_MOVE:
            if (event.type() == AW_Mouse_Press && clicked.node() && clicked.node()->father) {
                BranchMover *mover = new BranchMover(clicked.element(), event.button(), exports);
                store_command_data(mover);
                mover->draw_drag_indicator(device, drag_gc);
            }
            break;

        case AWT_MODE_LENGTH:
        case AWT_MODE_MULTIFURC:
            if (event.type() == AW_Mouse_Press && clicked.node() && clicked.is_branch()) {
                bool allow_neg_branches = aw_root->awar(AWAR_EXPERT)->read_int();
                bool discrete_lengths   = event.button() == AW_BUTTON_RIGHT;

                const AW_clicked_line *cl = dynamic_cast<const AW_clicked_line*>(clicked.element());
                td_assert(cl);

                ScaleMode     mode   = event.cmd() == AWT_MODE_LENGTH ? SCALE_LENGTH : SCALE_LENGTH_PRESERVING;
                BranchScaler *scaler = new BranchScaler(mode, clicked.node(), cl->get_line(), clicked.element()->get_attach_point(), mousepos, device->get_unscale(), discrete_lengths, allow_neg_branches, exports);

                store_command_data(scaler);
                scaler->draw_drag_indicator(device, drag_gc);
            }
            break;

        case AWT_MODE_ROTATE:
            if (event.type() == AW_Mouse_Press && clicked.node() && clicked.is_branch()) {
                const AW_clicked_line *cl = dynamic_cast<const AW_clicked_line*>(clicked.element());
                td_assert(cl);
                BranchRotator *rotator = new BranchRotator(device, clicked.node(), cl->get_line(), mousepos, exports);
                store_command_data(rotator);
                rotator->draw_drag_indicator(device, drag_gc);
            }
            break;

        case AWT_MODE_LINE:
            if (event.type()==AW_Mouse_Press && clicked.node()) {
                BranchLinewidthScaler *widthScaler = new BranchLinewidthScaler(clicked.node(), mousepos, event.button() == AW_BUTTON_RIGHT, exports);
                store_command_data(widthScaler);
                widthScaler->draw_drag_indicator(device, drag_gc);
            }
            break;

        case AWT_MODE_SPREAD:
            if (event.type() == AW_Mouse_Press && clicked.node() && clicked.is_branch()) {
                const AW_clicked_line *cl = dynamic_cast<const AW_clicked_line*>(clicked.element());
                td_assert(cl);
                BranchScaler *spreader = new BranchScaler(SCALE_SPREAD, clicked.node(), cl->get_line(), clicked.element()->get_attach_point(), mousepos, device->get_unscale(), false, false, exports);
                store_command_data(spreader);
                spreader->draw_drag_indicator(device, drag_gc);
            }
            break;

        // -----------------------------
        //      one point commands:

        case AWT_MODE_LZOOM:
            if (event.type()==AW_Mouse_Press) {
                switch (event.button()) {
                    case AW_BUTTON_LEFT:
                        if (clicked.node()) {
                            displayed_root     = clicked.node();
                            exports.zoom_reset = 1;
                        }
                        break;
                    case AW_BUTTON_RIGHT:
                        if (displayed_root->father) {
                            displayed_root     = displayed_root->get_father();
                            exports.zoom_reset = 1;
                        }
                        break;

                    default: td_assert(0); break;
                }
            }
            break;

    act_like_group :
        case AWT_MODE_GROUP:
            if (event.type()==AW_Mouse_Press && clicked.node()) {
                switch (event.button()) {
                    case AW_BUTTON_LEFT: {
                        AP_tree *at = clicked.node();
                        if ((!at->gr.grouped) && (!at->name)) break; // not grouped and no name
                        if (at->is_leaf) break; // don't touch zombies

                        if (gb_tree) {
                            GB_ERROR error = this->create_group(at);
                            if (error) aw_message(error);
                        }
                        if (at->name) {
                            at->gr.grouped  ^= 1;
                            exports.save     = 1;
                        }
                        break;
                    }
                    case AW_BUTTON_RIGHT:
                        if (gb_tree) {
                            toggle_group(clicked.node());
                        }
                        break;
                    default: td_assert(0); break;
                }
            }
            break;

        case AWT_MODE_SETROOT:
            if (event.type() == AW_Mouse_Press) {
                switch (event.button()) {
                    case AW_BUTTON_LEFT:
                        if (clicked.node()) clicked.node()->set_root();
                        break;
                    case AW_BUTTON_RIGHT:
                        tree_static->find_innermost_edge().set_root();
                        break;
                    default: td_assert(0); break;
                }
                exports.save       = 1;
                exports.zoom_reset = 1;
            } 
            break;

        case AWT_MODE_SWAP:
            if (event.type()==AW_Mouse_Press && clicked.node()) {
                switch (event.button()) {
                    case AW_BUTTON_LEFT:  clicked.node()->swap_sons(); break;
                    case AW_BUTTON_RIGHT: clicked.node()->rotate_subtree();     break;
                    default: td_assert(0); break;
                }
                exports.save = 1;
            }
            break;

        case AWT_MODE_MARK: // see also .@OTHER_MODE_MARK_HANDLER
            if (event.type() == AW_Mouse_Press && clicked.node()) {
                GB_transaction ta(tree_static->get_gb_main());

                if (event.type() == AW_Mouse_Press) {
                    switch (event.button()) {
                        case AW_BUTTON_LEFT:  mark_species_in_tree(clicked.node(), 1); break;
                        case AW_BUTTON_RIGHT: mark_species_in_tree(clicked.node(), 0); break;
                        default: td_assert(0); break;
                    }
                }
                exports.refresh = 1;
                tree_static->update_timers(); // do not reload the tree
                update_structure();
            }
            break;

        case AWT_MODE_NONE:
        case AWT_MODE_SELECT:
            if (event.type()==AW_Mouse_Press && clicked.node()) {
                GB_transaction ta(tree_static->get_gb_main());
                exports.refresh = 1;        // No refresh needed !! AD_map_viewer will do the refresh (needed by arb_pars)
                map_viewer_cb(clicked.node()->gb_node, ADMVT_SELECT);

                if (event.button() == AW_BUTTON_LEFT) goto act_like_group; // now do the same like in AWT_MODE_GROUP
            }
            break;

        // now handle all modes which only act on tips (aka species) and
        // shall perform identically in tree- and list-modes

        case AWT_MODE_INFO:
        case AWT_MODE_WWW: { 
            if (clicked.node() && clicked.node()->gb_node) {
                if (command_on_GBDATA(clicked.node()->gb_node, event, map_viewer_cb)) {
                    exports.refresh = 1;
                }
            }
            break;
        }
        default:
            break;
    }
}

void AWT_graphic_tree::set_tree_type(AP_tree_display_type type, AWT_canvas *ntw) {
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
    : AWT_graphic(),
      line_filter          (AW_SCREEN|AW_CLICK|AW_CLICK_DROP|AW_PRINTER|AW_SIZE),
      vert_line_filter     (AW_SCREEN|AW_CLICK|AW_CLICK_DROP|AW_PRINTER),
      mark_filter          (AW_SCREEN|AW_PRINTER_EXT),
      group_bracket_filter (AW_SCREEN|AW_CLICK|AW_CLICK_DROP|AW_PRINTER|AW_SIZE_UNSCALED),
      bs_circle_filter     (AW_SCREEN|AW_PRINTER|AW_SIZE_UNSCALED),
      leaf_text_filter     (AW_SCREEN|AW_CLICK|AW_CLICK_DROP|AW_PRINTER|AW_SIZE_UNSCALED),
      group_text_filter    (AW_SCREEN|AW_CLICK|AW_CLICK_DROP|AW_PRINTER|AW_SIZE_UNSCALED),
      remark_text_filter   (AW_SCREEN|AW_CLICK|AW_CLICK_DROP|AW_PRINTER|AW_SIZE_UNSCALED),
      other_text_filter    (AW_SCREEN|AW_PRINTER|AW_SIZE_UNSCALED),
      ruler_filter         (AW_SCREEN|AW_CLICK|AW_PRINTER), // appropriate size-filter added manually in code
      root_filter          (AW_SCREEN|AW_PRINTER_EXT)

{
    td_assert(gb_main_);

    set_tree_type(AP_TREE_NORMAL, NULL);
    displayed_root   = 0;
    tree_proto       = 0;
    link_to_database = false;
    tree_static      = 0;
    baselinewidth    = 1;
    species_name     = 0;
    aw_root          = aw_root_;
    gb_main          = gb_main_;
    cmd_data         = NULL;
    nds_show_all     = true;
    map_viewer_cb    = map_viewer_cb_;
}

AWT_graphic_tree::~AWT_graphic_tree() {
    delete cmd_data;
    free(species_name);
    delete tree_proto;
    delete tree_static;
}

void AWT_graphic_tree::init(RootedTreeNodeFactory *nodeMaker_, AliView *aliview, AP_sequence *seq_prototype, bool link_to_database_, bool insert_delete_cbs) {
    tree_static = new AP_tree_root(aliview, nodeMaker_, seq_prototype, insert_delete_cbs);

    td_assert(!insert_delete_cbs || link_to_database); // inserting delete callbacks w/o linking to DB has no effect!
    link_to_database = link_to_database_;
}

void AWT_graphic_tree::unload() {
    delete tree_static->get_root_node();
    displayed_root = 0;
}

GB_ERROR AWT_graphic_tree::load(GBDATA *, const char *name, AW_CL /* cl_link_to_database */, AW_CL /* cl_insert_delete_cbs */) {
    GB_ERROR error = 0;

    if (!name) { // happens in error-case (called by AWT_graphic::postevent_handler to load previous state)
        if (tree_static) {
            name = tree_static->get_tree_name();
            td_assert(name);
        }
        else {
            error = "Please select a tree (name lost)";
        }
    }

    if (!error) {
        if (name[0] == 0 || strcmp(name, NO_TREE_SELECTED) == 0) {
            unload();
            zombies    = 0;
            duplicates = 0;
        }
        else {
            freenull(tree_static->gone_tree_name);
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
                displayed_root = get_root_node();

                get_root_node()->compute_tree();

                tree_static->set_root_changed_callback(AWT_graphic_tree_root_changed, this);
                tree_static->set_node_deleted_callback(AWT_graphic_tree_node_deleted, this);
            }
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
            td_assert(!tree_static->gone_tree_name);
            tree_static->gone_tree_name = strdup(tree_static->get_tree_name());

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
                    if (!error) tree_root->compute_tree();
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

        if (at->name && (disp_device->get_filter() & leaf_text_filter)) {
            // display text
            const char            *data       = make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at, tree_static->get_tree_name());
            const AW_font_limits&  charLimits = disp_device->get_font_limits(at->gr.gc, 'A');

            double   unscale  = disp_device->get_unscale();
            size_t   data_len = strlen(data);
            Position textPos  = Pen + 0.5*Vector((charLimits.width+NT_BOX_WIDTH)*unscale, scaled_font.ascent);
            disp_device->text(at->gr.gc, data, textPos, 0.0, leaf_text_filter, data_len);

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
        Position n0(s0);  n0.movex(at->gr.max_tree_depth);
        Position n1(s1);  n1.movex(at->gr.min_tree_depth);

        Position group[4] = { s0, s1, n1, n0 };

        set_line_attributes_for(at);
        disp_device->set_grey_level(at->gr.gc, grey_level);
        disp_device->filled_area(at->gr.gc, 4, group, line_filter);

        const AW_font_limits& charLimits  = disp_device->get_font_limits(at->gr.gc, 'A');
        double                text_ascent = charLimits.ascent * disp_device->get_unscale();

        Vector text_offset = 0.5 * Vector(text_ascent, text_ascent+box_height);

        limits.x_right = n0.xpos();

        if (at->gb_node && (disp_device->get_filter() & group_text_filter)) {
            const char *data     = make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at, tree_static->get_tree_name());
            size_t      data_len = strlen(data);

            Position textPos = n0+text_offset;
            disp_device->text(at->gr.gc, data, textPos, 0.0, group_text_filter, data_len);

            double textsize = disp_device->get_string_size(at->gr.gc, data, data_len) * disp_device->get_unscale();
            limits.x_right  = textPos.xpos()+textsize;
        }

        Position    countPos = s0+text_offset;
        const char *count    = GBS_global_string(" %u", at->gr.leaf_sum);
        disp_device->text(at->gr.gc, count, countPos, 0.0, group_text_filter);

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
            
                if (at->gb_node && (disp_device->get_filter() & group_text_filter)) {
                    const char *data     = make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at, tree_static->get_tree_name());
                    size_t      data_len = strlen(data);

                    LineVector worldBracket = disp_device->transform(bracket.right_edge());
                    LineVector clippedWorldBracket;
                    bool       visible      = disp_device->clip(worldBracket, clippedWorldBracket);
                    if (visible) {
                        LineVector clippedBracket = disp_device->rtransform(clippedWorldBracket);

                        Position textPos = clippedBracket.centroid()+Vector(half_text_ascent, half_text_ascent);
                        disp_device->text(at->gr.gc, data, textPos, 0.0, group_text_filter, data_len);

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
            if (son->get_remark()) {
                Position remarkPos(n);
                remarkPos.movey(-scaled_font.ascent*0.1);
                bool bootstrap_shown = AWT_show_branch_remark(disp_device, son->get_remark(), son->is_leaf, remarkPos, 1, remark_text_filter);
                if (show_circle && bootstrap_shown) {
                    show_bootstrap_circle(disp_device, son->get_remark(), circle_zoom_factor, circle_max_size, len, n, use_ellipse, scaled_branch_distance, bs_circle_filter);
                }
            }

            set_line_attributes_for(son);
            unsigned int gc = son->gr.gc;
            draw_branch_line(gc, s, n, line_filter);
            draw_branch_line(gc, attach, s, vert_line_filter);
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
    draw_branch_line(at->gr.gc, Position(x_root, y_root), Position(x_center, y_center), line_filter);

    if (at->is_leaf) {
        // draw mark box
        if (at->gb_node && GB_read_flag(at->gb_node)) {
            filled_box(at->gr.gc, Position(x_center, y_center), NT_BOX_WIDTH);
        }

        if (at->name && (disp_device->get_filter() & leaf_text_filter)) {
            if (at->hasName(species_name)) cursor = Position(x_center, y_center);
            scale_text_koordinaten(disp_device, at->gr.gc, x_center, y_center, tree_orientation, 0);
            const char *data =  make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at, tree_static->get_tree_name());
            disp_device->text(at->gr.gc, data,
                              (AW_pos)x_center, (AW_pos) y_center,
                              (AW_pos) .5 - .5 * cos(tree_orientation),
                              leaf_text_filter);
        }
        return;
    }

    if (at->gr.grouped) {
        l_min = at->gr.min_tree_depth;
        l_max = at->gr.max_tree_depth;

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

        if (at->gb_node && (disp_device->get_filter() & group_text_filter)) {
            w = tree_orientation + at->gr.right_angle;
            l_max = (l_max+l_min)*.5;
            x_center = x_center+l_max*cos(w);
            y_center = y_center+l_max*sin(w);
            scale_text_koordinaten(disp_device, at->gr.gc, x_center, y_center, w, 0);

            // insert text (e.g. name of group)
            const char *data = make_node_text_nds(this->gb_main, at->gb_node, NDS_OUTPUT_LEAFTEXT, at, tree_static->get_tree_name());
            disp_device->text(at->gr.gc, data,
                              (AW_pos)x_center, (AW_pos) y_center,
                              (AW_pos).5 - .5 * cos(tree_orientation),
                              group_text_filter);
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
        if (at->leftson->get_remark()) {
            AW_click_cd cdl(disp_device, (AW_CL)at->leftson);
            w = r*0.5*tree_spread + tree_orientation + at->gr.left_angle;
            z = at->leftlen * .5;
            Position center(x_center + z * cos(w), y_center + z * sin(w));
            show_bootstrap_circle(disp_device, at->leftson->get_remark(), circle_zoom_factor, circle_max_size, at->leftlen, center, false, 0, bs_circle_filter);
        }
        if (at->rightson->get_remark()) {
            AW_click_cd cdr(disp_device, (AW_CL)at->rightson);
            w = tree_orientation - l*0.5*tree_spread + at->gr.right_angle;
            z = at->rightlen * .5;
            Position center(x_center + z * cos(w), y_center + z * sin(w));
            show_bootstrap_circle(disp_device, at->rightson->get_remark(), circle_zoom_factor, circle_max_size, at->rightlen, center, false, 0, bs_circle_filter);
        }
    }
}

const char *AWT_graphic_tree::ruler_awar(const char *name) {
    // return "ruler/TREETYPE/name" (path to entry below tree)
    const char *tree_awar = 0;
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
            td_assert(0); // should not be called
            break;
    }

    static char awar_name[256];
    sprintf(awar_name, "ruler/%s/%s", tree_awar, name);
    return awar_name;
}

void AWT_graphic_tree::show_ruler(AW_device *device, int gc) {
    GBDATA *gb_tree = tree_static->get_gb_tree();
    if (!gb_tree) return; // no tree -> no ruler

    bool mode_has_ruler = ruler_awar(NULL);
    if (mode_has_ruler) {
        GB_transaction ta(gb_tree);

        float ruler_size = *GBT_readOrCreate_float(gb_tree, RULER_SIZE, DEFAULT_RULER_LENGTH);
        float ruler_y    = 0.0;

        const char *awar = ruler_awar("ruler_y");
        if (!GB_search(gb_tree, awar, GB_FIND)) {
            if (device->type() == AW_DEVICE_SIZE) {
                AW_world world;
                DOWNCAST(AW_device_size*, device)->get_size_information(&world);
                ruler_y = world.b * 1.3;
            }
        }

        double half_ruler_width = ruler_size*0.5;

        float ruler_add_y  = 0.0;
        float ruler_add_x  = 0.0;
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

        float ruler_x = 0.0;
        ruler_x       = ruler_add_x + *GBT_readOrCreate_float(gb_tree, ruler_awar("ruler_x"), ruler_x);

        td_assert(!is_nan_or_inf(ruler_x));

        float ruler_text_x = 0.0;
        ruler_text_x       = *GBT_readOrCreate_float(gb_tree, ruler_awar("text_x"), ruler_text_x);

        td_assert(!is_nan_or_inf(ruler_text_x));

        float ruler_text_y = 0.0;
        ruler_text_y       = *GBT_readOrCreate_float(gb_tree, ruler_awar("text_y"), ruler_text_y);

        td_assert(!is_nan_or_inf(ruler_text_y));

        int ruler_width = *GBT_readOrCreate_int(gb_tree, RULER_LINEWIDTH, DEFAULT_RULER_LINEWIDTH);

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
                      (AW_pos) 0, other_text_filter);

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

            disp_device->text(gc, col.text, x, y, 0.0, leaf_text_filter, col.len);
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

    if (!displayed_root && sort_is_tree_style(tree_sort)) { // if there is no tree, but display style needs tree
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
                show_dendrogram(displayed_root, pen, limits);
                list_tree_ruler_y = pen.ypos() + 2.0 * scaled_branch_distance;
                break;
            }
            case AP_TREE_RADIAL:
                empty_box(displayed_root->gr.gc, Origin, NT_ROOT_WIDTH);
                show_radial_tree(displayed_root, 0, 0, 2*M_PI, 0.0, 0, 0);
                break;

            case AP_TREE_IRS:
                show_irs_tree(displayed_root, scaled_branch_distance);
                break;

            case AP_LIST_NDS:       // this is the list all/marked species mode (no tree)
                show_nds_list(gb_main, true);
                break;

            case AP_LIST_SIMPLE:    // simple list of names (used at startup only)
                // don't see why we need to draw ANY tree at startup -> disabled
                // show_nds_list(gb_main, false);
                break;
        }
        if (are_distinct(Origin, cursor)) empty_box(AWT_GC_CURSOR, cursor, NT_SELECTED_WIDTH);
        if (sort_is_tree_style(tree_sort)) show_ruler(disp_device, AWT_GC_CURSOR);
    }

    if (cmd_data && Dragged::valid_drag_device(disp_device)) {
        Dragged *dragging = dynamic_cast<Dragged*>(cmd_data);
        if (dragging) {
            // if tree is redisplayed while dragging, redraw the drag indicator.
            // (happens in modes which modify the tree during drag, e.g. when scaling branches)
            dragging->draw_drag_indicator(disp_device, drag_gc);
        }
    }

    disp_device = NULL;
}

void AWT_graphic_tree::info(AW_device */*device*/, AW_pos /*x*/, AW_pos /*y*/, AW_clicked_line */*cl*/, AW_clicked_text */*ct*/) {
    aw_message("INFO MESSAGE");
}

AWT_graphic_tree *NT_generate_tree(AW_root *root, GBDATA *gb_main, AD_map_viewer_cb map_viewer_cb) {
    AWT_graphic_tree *apdt = new AWT_graphic_tree(root, gb_main, map_viewer_cb);
    apdt->init(new AP_TreeNodeFactory, new AliView(gb_main), NULL, true, false); // tree w/o sequence data
    return apdt;
}

void awt_create_dtree_awars(AW_root *aw_root, AW_default db) {
    aw_root->awar_int  (AWAR_DTREE_BASELINEWIDTH, 1)  ->set_minmax(1,    10);
    aw_root->awar_float(AWAR_DTREE_VERICAL_DIST,  1.0)->set_minmax(0.01, 30);
    aw_root->awar_int  (AWAR_DTREE_AUTO_JUMP,     1);

    aw_root->awar_int(AWAR_DTREE_SHOW_BRACKETS, 1);
    aw_root->awar_int(AWAR_DTREE_SHOW_CIRCLE,   0);
    aw_root->awar_int(AWAR_DTREE_USE_ELLIPSE,   1);

    aw_root->awar_float(AWAR_DTREE_CIRCLE_ZOOM,     1.0)->set_minmax(0.01, 20);
    aw_root->awar_float(AWAR_DTREE_CIRCLE_MAX_SIZE, 1.5)->set_minmax(0.01, 200);
    aw_root->awar_int  (AWAR_DTREE_GREY_LEVEL,      20) ->set_minmax(0,    100);
    
    aw_root->awar_int(AWAR_DTREE_RADIAL_ZOOM_TEXT, 0);
    aw_root->awar_int(AWAR_DTREE_RADIAL_XPAD,      150);
    aw_root->awar_int(AWAR_DTREE_DENDRO_ZOOM_TEXT, 0);
    aw_root->awar_int(AWAR_DTREE_DENDRO_XPAD,      300);

    aw_root->awar_int(AWAR_TREE_REFRESH, 0, db);
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
    0x977a0e, // AWT_GC_ZOMBIES
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

struct fake_AW_common : public AW_common {
    fake_AW_common()
        : AW_common(fcolors, dcolors, dcolors_count)
    {
        for (int gc = 0; gc < dcolors_count-AW_STD_COLOR_IDX_MAX; ++gc) { // gcs used in this example
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

    void test_print_tree(AW_device_print *print_device, AP_tree_display_type type, bool show_handles) {
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
            size_device.restart_tracking();
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

    agt.init(new AP_TreeNodeFactory, new AliView(gb_main), NULL, true, false);

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
            // for (AP_tree_display_type type = AP_TREE_NORMAL; type <= AP_LIST_SIMPLE; type = AP_tree_display_type(type+1)) {
            for (AP_tree_display_type type = AP_LIST_SIMPLE; type >= AP_TREE_NORMAL; type = AP_tree_display_type(type-1)) {
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
