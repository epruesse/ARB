// =============================================================== //
//                                                                 //
//   File      : ED4_terminal.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <ed4_extern.hxx>
#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_block.hxx"
#include "ed4_nds.hxx"
#include "ed4_ProteinViewer.hxx"

#include <aw_preset.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <awt_seq_colors.hxx>
#include <st_window.hxx>
#include <arbdb.h>

// -----------------------------------
//      static terminal properties

static ED4_objspec tree_terminal_spec(
    ED4_P_IS_TERMINAL,  // static props
    ED4_L_TREE,         // level
    ED4_L_NO_LEVEL,     // allowed children level
    ED4_L_NO_LEVEL,     // handled object
    ED4_L_NO_LEVEL      // restriction level
    );

static ED4_objspec bracket_terminal_spec(
    ED4_P_IS_TERMINAL,  // static props
    ED4_L_BRACKET,      // level
    ED4_L_NO_LEVEL,     // allowed children level
    ED4_L_NO_LEVEL,     // handled object
    ED4_L_NO_LEVEL      // restriction level
    );

static ED4_objspec species_name_terminal_spec(
    ED4_P_IS_TERMINAL,  // static props
    ED4_L_SPECIES_NAME, // level
    ED4_L_NO_LEVEL,     // allowed children level
    ED4_L_SPECIES,      // handled object
    ED4_L_NO_LEVEL      // restriction level
    );

static ED4_objspec sequence_info_terminal_spec(
    ED4_P_IS_TERMINAL,  // static props
    ED4_L_SEQUENCE_INFO, // level
    ED4_L_NO_LEVEL,     // allowed children level
    ED4_L_SEQUENCE,     // handled object
    ED4_L_NO_LEVEL      // restriction level
    );

static ED4_objspec sequence_terminal_spec(
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_SEQUENCE_STRING, // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL         // restriction level
    );

static ED4_objspec orf_terminal_spec(
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_ORF,             // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL         // restriction level
    );

static ED4_objspec pure_text_terminal_spec(
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_PURE_TEXT,       // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL         // restriction level
    );

static ED4_objspec spacer_terminal_spec(
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_SPACER,          // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL         // restriction level
    );

static ED4_objspec line_terminal_spec(
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_LINE,            // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL         // restriction level
    );

static ED4_objspec column_stat_terminal_spec(
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_COL_STAT,        // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL         // restriction level
    );


char *ED4_terminal::resolve_pointer_to_string_copy(int *str_len) const {
    int         len;
    const char *s = resolve_pointer_to_char_pntr(&len);
    char       *t = GB_strduplen(s, len); // space for zero byte is allocated by GB_strduplen

    if (str_len) *str_len = len;
    return t;
}

const char *ED4_terminal::resolve_pointer_to_char_pntr(int *str_len) const
{
    GB_TYPES    gb_type;
    const char *db_pointer = NULL;
    GBDATA     *gbd        = get_species_pointer();

    if (!gbd) {
        if (str_len) {
            if (id) *str_len = strlen(id);
            else *str_len    = 0;
        }
        return id; // if we don't have a link to the database we have to use our id
    }

    GB_push_transaction(GLOBAL_gb_main);

    const char *copy_of = 0; // if set, return a copy of this string

    gb_type = GB_read_type(gbd);
#if defined(WARN_TODO)
#warning temporary workaround - gb_type GB_INT and GB_FLOAT in the switch directive must not be displayed but ignored
#endif
    switch (gb_type)
    {
        case GB_STRING:
            db_pointer = GB_read_char_pntr(gbd);
            if (str_len) {
                *str_len = GB_read_string_count(gbd);
                e4_assert(*str_len == (int)strlen(db_pointer));
            }
            break;
        case GB_BITS:
            db_pointer = GB_read_bits_pntr(gbd, '.', '+');
            if (str_len) {
                *str_len = GB_read_bits_count(gbd);
                e4_assert(*str_len == (int)strlen(db_pointer));
            }
            break;
        case GB_BYTES:
            db_pointer = GB_read_bytes_pntr(gbd);
            if (str_len) {
                *str_len = GB_read_bytes_count(gbd);
                e4_assert(*str_len == (int)strlen(db_pointer));
            }
            break;
        case GB_DB:
            copy_of = "GB_DB";
            break;
        case GB_INT: // FIXME: temporary Workaround
            copy_of = "GB_INT";
            break;
        case GB_INTS:
            copy_of = "GB_INTS";
            break;
        case GB_FLOAT: // FIXME: temporary Workaround
            copy_of = "GB_FLOAT";
            break;
        case GB_FLOATS:
            copy_of = "GB_FLOATS";
            break;
        default:
            printf("Unknown data type - Please contact administrator\n");
            e4_assert(0);
            break;
    }

    if (copy_of) {
        e4_assert(db_pointer == 0);

        int len    = strlen(copy_of);
        db_pointer = GB_strduplen(copy_of, len);

        if (str_len) *str_len = len;
    }

    GB_pop_transaction(GLOBAL_gb_main);

    return db_pointer;
}

GB_ERROR ED4_terminal::write_sequence(const char *seq, int seq_len)
{
    GB_ERROR err = 0;
    GBDATA *gbd = get_species_pointer();
    e4_assert(gbd); // we must have a link to the database!

    GB_push_transaction(GLOBAL_gb_main);

    int   old_seq_len;
    char *old_seq = resolve_pointer_to_string_copy(&old_seq_len);

    bool allow_write_data = true;
    if (ED4_ROOT->aw_root->awar(ED4_AWAR_ANNOUNCE_CHECKSUM_CHANGES)->read_int()) {
        long old_checksum = GBS_checksum(old_seq, 1, "-.");
        long new_checksum = GBS_checksum(seq, 1, "-.");

        if (old_checksum != new_checksum) {
            if (aw_question(NULL, "Checksum changed!", "Allow, Reject") == 1) {
                allow_write_data = false;
            }

        }
    }

    if (allow_write_data) {
        GB_TYPES gb_type = GB_read_type(gbd);
        switch (gb_type) {
            case GB_STRING: {
                err = GB_write_string(gbd, seq);
                break;
            }
            case GB_BITS: {
                err = GB_write_bits(gbd, seq, seq_len, ".-");
                break;
            }
            default: {
                e4_assert(0);
                break;
            }
        }
    }

    GB_pop_transaction(GLOBAL_gb_main);

    if (!err && dynamic_prop&ED4_P_CONSENSUS_RELEVANT) {
        if (old_seq) {
            curr_timestamp = GB_read_clock(GLOBAL_gb_main);

            get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager()
                ->update_bases_and_rebuild_consensi(old_seq, old_seq_len, get_parent(ED4_L_SPECIES)->to_species_manager(), ED4_U_UP); // bases_check
        }
        else {
            aw_message("Couldn't read old sequence data");
        }

        request_refresh();
    }

    if (old_seq) free(old_seq);

    return err;
}


ED4_returncode ED4_terminal::remove_callbacks()                     // removes callbacks and gb_alignment
{
    if (get_species_pointer()) {
        set_species_pointer(0);
        tflag.deleted = 1; // @@@ why ?
        clr_property(ED4_P_CURSOR_ALLOWED);
        request_refresh();
    }
    return ED4_R_OK;
}

static ARB_ERROR ed4_remove_species_manager_callbacks(ED4_base *base) { // @@@ unused since [8286]
    if (base->is_species_manager()) {
        base->to_species_manager()->remove_all_callbacks();
    }
    return NULL;
}

inline void remove_from_consensus(ED4_manager *group_or_species_man) {
    GB_transaction ta(GLOBAL_gb_main);
    ED4_manager *parent_manager = group_or_species_man->parent;
    parent_manager->update_consensus(parent_manager, NULL, group_or_species_man);
    parent_manager->rebuild_consensi(parent_manager, ED4_U_UP);
}

ED4_returncode ED4_terminal::kill_object() {
    ED4_species_manager *species_manager = get_parent(ED4_L_SPECIES)->to_species_manager();

    if (species_manager->flag.is_consensus) { // kill whole group
        if (ED4_new_species_multi_species_manager()==species_manager->parent) {
            aw_message("This group has to exist - deleting it isn't allowed");
            return ED4_R_IMPOSSIBLE;
        }

        ED4_manager *group_manager = species_manager->get_parent(ED4_L_GROUP)->to_group_manager();
        remove_from_consensus(group_manager);
        group_manager->Delete();
    }
    else { // kill single species/SAI
        remove_from_consensus(species_manager);
        species_manager->Delete();
    }

    return ED4_R_OK;
}

ED4_base *ED4_terminal::get_competent_clicked_child(AW_pos /* x */, AW_pos /* y */, ED4_properties /* relevant_prop */)
{
    e4_assert(0);
    return NULL;
}

ED4_returncode   ED4_terminal::handle_move(ED4_move_info * /* moveinfo */)
{
    e4_assert(0);
    return ED4_R_IMPOSSIBLE;
}

ED4_returncode  ED4_terminal::move_requested_by_child(ED4_move_info * /* moveinfo */)
{
    e4_assert(0);
    return ED4_R_IMPOSSIBLE;
}


ED4_base *ED4_terminal::get_competent_child(AW_pos /* x */, AW_pos /* y */, ED4_properties /* relevant_prop */)
{
    e4_assert(0);
    return NULL;
}

ED4_returncode ED4_terminal::draw_drag_box(AW_pos x, AW_pos y, GB_CSTR text, int cursor_y)      // draws drag box of object at location abs_x, abs_y
{
    ED4_index      i;
    AW_pos         drag_x, drag_y;
    AW_pos         drag_line_x0[3], drag_line_y0[3];
    AW_pos         drag_line_x1[3], drag_line_y1[3];
    ED4_base      *drag_target = NULL;
    AW_pos         target_x, target_y;
    ED4_extension  location;

    if (cursor_y!=-1) {
        ED4_device_manager *device_manager = ED4_ROOT->get_device_manager();
        drag_x = 0;
        drag_y = (AW_pos)cursor_y; // cursor_y is already in world coordinates!
        location.position[X_POS] = drag_x;
        location.position[Y_POS] = drag_y;
        device_manager->children->search_target_species(&location, ED4_P_HORIZONTAL, &drag_target, ED4_L_NO_LEVEL);

        if (drag_target) {
            drag_target->calc_world_coords (&target_x, &target_y);
            current_ed4w()->world_to_win_coords(&target_x, &target_y);
#define ARROW_LENGTH 3
            drag_line_x0[0] = target_x + 5;                                     // horizontal
            drag_line_y0[0] = target_y + drag_target->extension.size[HEIGHT];
            drag_line_x1[0] = drag_line_x0[0] + 50;
            drag_line_y1[0] = target_y + drag_target->extension.size[HEIGHT];

            drag_line_x0[1] = drag_line_x0[0] -ARROW_LENGTH;                                // arrow
            drag_line_y0[1] = drag_line_y0[0] -ARROW_LENGTH;
            drag_line_x1[1] = drag_line_x0[0];
            drag_line_y1[1] = drag_line_y0[0];

            drag_line_x0[2] = drag_line_x0[0] -ARROW_LENGTH;                                // arrow
            drag_line_y0[2] = drag_line_y0[0] + ARROW_LENGTH;
            drag_line_x1[2] = drag_line_x0[0];
            drag_line_y1[2] = drag_line_y0[0];
#undef ARROW_LENGTH
            for (i = 0; i <= 2; i++) {
                current_device()->line(ED4_G_DRAG, drag_line_x0[i], drag_line_y0[i], drag_line_x1[i], drag_line_y1[i], AW_SCREEN);
            }
        }
    }

    if (text != NULL) {
        current_device()->text(ED4_G_DRAG, text, (x + 20), (y + INFO_TERM_TEXT_YOFFSET), 0, AW_SCREEN);
    }

    return (ED4_R_OK);
}

ED4_returncode  ED4_terminal::move_requested_by_parent(ED4_move_info *) { // handles a move request coming from parent
    e4_assert(0);
    return (ED4_R_IMPOSSIBLE);
}

void ED4_multi_species_manager::toggle_selected_species() {
    if (all_are_selected()) deselect_all_species();
    else select_all_species();

    ED4_correctBlocktypeAfterSelection();
}

#if defined(DEBUG) && 1
static inline void dumpEvent(const char *where, AW_event *event) {
    printf("%s: x=%i y=%i\n", where, event->x, event->y);
}
#else
#define dumpEvent(w, e)
#endif

ED4_returncode  ED4_terminal::event_sent_by_parent(AW_event *event, AW_window *aww)             // handles an input event coming from parent
{
    static ED4_species_name_terminal    *dragged_name_terminal = 0;
    static int              dragged_was_selected;
    static bool                 pressed_left_button = 0;
    static int              other_x, other_y; // coordinates of last event
    static bool                 right_button_started_on_sequence_term = 0;

    switch (event->type) {
        case AW_Mouse_Press: {
            switch (event->button) {
                case ED4_B_LEFT_BUTTON: {
                    pressed_left_button = 0;
                    if (is_species_name_terminal()) {
                        switch (ED4_ROOT->species_mode) {
                            case ED4_SM_KILL: {
                                if (tflag.selected) ED4_ROOT->remove_from_selected(this);
                                kill_object();
                                return ED4_R_BREAK;
                            }
                            case ED4_SM_MOVE: {
                                dragged_name_terminal = to_species_name_terminal();
                                pressed_left_button = 1;
                                other_x = event->x;
                                other_y = event->y;

                                if (!tflag.selected) {
                                    ED4_ROOT->add_to_selected(dragged_name_terminal);
                                    dragged_was_selected = 0;
                                    ED4_ROOT->main_manager->Show();
                                }
                                else {
                                    dragged_was_selected = 1;
                                }
                                break;
                            }
                            case ED4_SM_MARK: {
                                ED4_species_manager *species_man = get_parent(ED4_L_SPECIES)->to_species_manager();
                                GBDATA *gbd = species_man->get_species_pointer();

                                if (gbd) {
                                    GB_write_flag(gbd, !GB_read_flag(gbd));
                                    request_refresh();
                                    // ProtView: Refreshing orf terminals
                                    if (ED4_ROOT->alignment_type ==  GB_AT_DNA) {
                                        PV_RefreshWindow(aww->get_root());
                                    }
                                }
                                break;
                            }
                            default: {
                                e4_assert(0);
                                break;
                            }
                        }
                    }
                    else if (is_bracket_terminal()) { // fold/unfold group
                        if (dynamic_prop & ED4_P_IS_FOLDED) {
                            to_bracket_terminal()->unfold();
                        }
                        else {
                            to_bracket_terminal()->fold();
                        }
                    }
                    else {
                        if (dynamic_prop & ED4_P_CURSOR_ALLOWED) {
                            ED4_no_dangerous_modes();
                            current_cursor().show_clicked_cursor(event->x, this);
                        }
                    }
                    break;
                }
                case ED4_B_RIGHT_BUTTON: {
                    right_button_started_on_sequence_term = 0;
                    if (is_bracket_terminal()) { // right click on bracket terminal
                        ED4_base *group = get_parent(ED4_L_GROUP);
                        if (group) {
                            ED4_multi_species_manager *multi_man = group->to_group_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
                            multi_man->toggle_selected_species();
                        }
                    }
                    else if (is_species_name_terminal()) {
                        ED4_species_manager *species_man = get_parent(ED4_L_SPECIES)->to_species_manager();

                        if (parent->flag.is_consensus) { // click on consensus-name
                            ED4_multi_species_manager *multi_man = parent->get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
                            multi_man->toggle_selected_species();
                            // return (ED4_R_BREAK);
                        }
                        else if (species_man->flag.is_SAI) {
                            ; // don't mark SAIs
                        }
                        else { // click on species name
                            if (!tflag.selected) { // select if not selected
                                if (ED4_ROOT->add_to_selected(this) == ED4_R_OK) ED4_correctBlocktypeAfterSelection();
                            }
                            else { // deselect if already selected
                                ED4_ROOT->remove_from_selected(this);
                                ED4_correctBlocktypeAfterSelection();
                                // return (ED4_R_BREAK);   // in case we were called by event_to selected() @@@ why? 
                            }
                        }
                    }
                    else if (is_sequence_terminal()) {
                        dumpEvent("Press", event);

                        ED4_no_dangerous_modes();
                        ED4_setColumnblockCorner(event, to_sequence_terminal()); // mark columnblock
                        right_button_started_on_sequence_term = 1;
                    }
                    break;
                }
                case ED4_B_MIDDLE_BUTTON: {
                    break;
                }
            }
            break;
        }
        case AW_Mouse_Drag: {
            switch (event->button) {
                case ED4_B_LEFT_BUTTON: {
                    if (dragged_name_terminal) {
                        ED4_selection_entry *sel_info = dragged_name_terminal->selection_info;

                        if (pressed_left_button) {
                            AW_pos world_x, world_y;

                            dragged_name_terminal->calc_world_coords(&world_x, &world_y);
                            current_ed4w()->world_to_win_coords(&world_x, &world_y);

                            sel_info->drag_old_x = world_x;
                            sel_info->drag_old_y = world_y;
                            sel_info->drag_off_x = world_x-other_x;
                            sel_info->drag_off_y = world_y-other_y;
                            sel_info->old_event_y = -1;

                            pressed_left_button = 0;
                        }

                        GB_CSTR text = dragged_name_terminal->get_displayed_text();

                        if (dragged_name_terminal->tflag.dragged) {
                            dragged_name_terminal->draw_drag_box(sel_info->drag_old_x, sel_info->drag_old_y, text, sel_info->old_event_y);
                        }

                        AW_pos new_x = sel_info->drag_off_x + event->x;
                        AW_pos new_y = sel_info->drag_off_y + event->y;

                        dragged_name_terminal->draw_drag_box(new_x, new_y, text, event->y); // @@@ event->y ist falsch, falls vertikal gescrollt ist!

                        sel_info->drag_old_x = new_x;
                        sel_info->drag_old_y = new_y;
                        sel_info->old_event_y = event->y;

                        dragged_name_terminal->tflag.dragged = 1;
                    }

                    break;
                }
                case ED4_B_RIGHT_BUTTON: {
                    if (is_sequence_terminal() && right_button_started_on_sequence_term) {
                        ED4_no_dangerous_modes();
                        ED4_setColumnblockCorner(event, to_sequence_terminal()); // mark columnblock
                    }
                    break;
                }
            }
            break;
        }
        case AW_Mouse_Release: {
            switch (event->button) {
                case ED4_B_LEFT_BUTTON: {
                    if (dragged_name_terminal) {
                        if (dragged_name_terminal->tflag.dragged) {
                            {
                                char                *db_pointer = dragged_name_terminal->resolve_pointer_to_string_copy();
                                ED4_selection_entry *sel_info   = dragged_name_terminal->selection_info;

                                dragged_name_terminal->draw_drag_box(sel_info->drag_old_x, sel_info->drag_old_y, db_pointer, sel_info->old_event_y);
                                dragged_name_terminal->tflag.dragged = 0;

                                free(db_pointer);
                            }
                            {
                                ED4_move_info mi;

                                mi.object = dragged_name_terminal;
                                mi.end_x = event->x;
                                mi.end_y = event->y;
                                mi.mode = ED4_M_FREE;
                                mi.preferred_parent = ED4_L_NO_LEVEL;

                                dragged_name_terminal->parent->move_requested_by_child(&mi);
                            }
                            {
                                ED4_device_manager *device_manager = ED4_ROOT->get_device_manager();

                                for (int i=0; i<device_manager->children->members(); i++) { // when moving species numbers have to be recalculated
                                    ED4_base *member = device_manager->children->member(i);

                                    if (member->is_area_manager()) {
                                        member->to_area_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->update_requested_by_child();
                                    }
                                }
                            }
                        }
                        if (!dragged_was_selected) {
                            ED4_ROOT->remove_from_selected(dragged_name_terminal);
                        }

                        pressed_left_button = 0;
                        dragged_name_terminal = 0;
                    }
                    break;
                }

                case ED4_B_RIGHT_BUTTON: {
                    if (right_button_started_on_sequence_term && is_sequence_terminal()) {
                        dumpEvent("Relea", event);
                        ED4_no_dangerous_modes();
                        ED4_setColumnblockCorner(event, to_sequence_terminal()); // mark columnblock
                    }
                    break;
                }
            }
            break;
        }
        default:
            e4_assert(0);
            break;
    }
    return (ED4_R_OK);
}

void ED4_terminal::update_requested_children() {}

bool ED4_terminal::calc_bounding_box() {
    // calculates the smallest rectangle containing the object.
    // requests refresh and returns true if bounding box has changed.

    bool bb_changed = false;

    if (width_link) {
        if (extension.size[WIDTH] != width_link->extension.size[WIDTH]) {   // all bounding Boxes have the same size !!!
            extension.size[WIDTH] = width_link->extension.size[WIDTH];
            bb_changed = true;
        }
    }

    if (height_link) {
        if (extension.size[HEIGHT] != height_link->extension.size[HEIGHT]) {
            extension.size[HEIGHT] = height_link->extension.size[HEIGHT];
            bb_changed = true;
        }
    }


    if (bb_changed) {
        request_resize_of_linked();
        request_refresh();
    }
    return bb_changed;
}

void ED4_terminal::resize_requested_children() {
    if (update_info.resize) { // likes to resize?
        if (calc_bounding_box()) { 
            request_resize();
        }
    }
}


void ED4_terminal::request_refresh(int clear) {
    update_info.set_refresh(1);
    update_info.set_clear_at_refresh(clear);
    if (parent) parent->refresh_requested_by_child();
}


ED4_base* ED4_terminal::search_ID(const char *temp_id)
{
    if (id && strcmp(temp_id, id) == 0) return (this);
    return (NULL);
}


int ED4_terminal::adjust_clipping_rectangle() {
    // set scrolling area in AW_MIDDLE_AREA
    AW_pos x, y;
    calc_world_coords(&x, &y);
    current_ed4w()->world_to_win_coords(&x, &y);

    return current_device()->reduceClipBorders(int(y), int(y+extension.size[HEIGHT]-1), int(x), int(x+extension.size[WIDTH]-1));
}



ED4_terminal::ED4_terminal(const ED4_objspec& spec_, GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_base(spec_, temp_id, x, y, width, height, temp_parent)
{
    memset((char*)&tflag, 0, sizeof(tflag));
    selection_info   = 0;
    curr_timestamp = 0;
}


ED4_terminal::~ED4_terminal() {
    delete selection_info;
    for (ED4_window *window = ED4_ROOT->first_window; window; window=window->next) {
        ED4_cursor& cursor = window->cursor;
        if (this == cursor.owner_of_cursor) {
            cursor.init();
        }
    }
}

ED4_tree_terminal::ED4_tree_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_terminal(tree_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
}

ED4_returncode ED4_tree_terminal::Show(int IF_ASSERTION_USED(refresh_all), int is_cleared)
{
    e4_assert(update_info.refresh || refresh_all);
    current_device()->push_clip_scale();
    if (adjust_clipping_rectangle()) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
        }
        draw();
    }
    current_device()->pop_clip_scale();

    return (ED4_R_OK);
}



ED4_returncode ED4_tree_terminal::draw() {
    AW_pos  x, y;
    AW_pos  text_x, text_y;
    char   *db_pointer;

    calc_world_coords(&x, &y);
    current_ed4w()->world_to_win_coords(&x, &y);

    text_x = x + CHARACTEROFFSET;                           // don't change
    text_y = y + SEQ_TERM_TEXT_YOFFSET;

    db_pointer = resolve_pointer_to_string_copy();
    current_device()->text(ED4_G_STANDARD, db_pointer, text_x, text_y, 0, AW_SCREEN);
    free(db_pointer);

    return (ED4_R_OK);
}

ED4_bracket_terminal::ED4_bracket_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_terminal(bracket_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
}

ED4_returncode ED4_bracket_terminal::Show(int IF_ASSERTION_USED(refresh_all), int is_cleared)
{
    e4_assert(update_info.refresh || refresh_all);
    current_device()->push_clip_scale();
    if (adjust_clipping_rectangle()) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
        }
        draw();
    }
    current_device()->pop_clip_scale();

    return ED4_R_OK;
}


ED4_returncode ED4_bracket_terminal::draw() {
    ED4_index   i;
    AW_pos      x, y,
        width  = extension.size[WIDTH] - 1,
        height = extension.size[HEIGHT] - 1,
        margin = 0;
    AW_pos      line_x0[3], line_y0[3];
    AW_pos      line_x1[3], line_y1[3];
    AW_pos      arrow_x0[6], arrow_y0[6];
    AW_pos      arrow_x1[6], arrow_y1[6];


    calc_world_coords(&x, &y);
    current_ed4w()->world_to_win_coords(&x, &y);

    line_x0[0] = x + margin + 2;
    line_y0[0] = y + margin + 2;
    line_x1[0] = x + width - margin + 2;
    line_y1[0] = y + margin + 2;

    line_x0[1] = x + width - margin + 2;
    line_y0[1] = y + height - margin - 2;
    line_x1[1] = x + margin + 2;
    line_y1[1] = y + height - margin - 2;

    line_x0[2] = x + margin + 2;
    line_y0[2] = y + height - margin - 2;
    line_x1[2] = x + margin + 2;
    line_y1[2] = y + margin + 2;

    ED4_group_manager *group_man = get_parent(ED4_L_GROUP)->to_group_manager();
    ED4_multi_species_manager *multi_man = group_man->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
    if (multi_man->get_no_of_selected_species()) {  // if multi_species_manager contains selected species
        current_device()->box(ED4_G_SELECTED, true, x, y, extension.size[WIDTH], extension.size[HEIGHT]);
    }

    if (dynamic_prop & ED4_P_IS_FOLDED) { // paint triangle for folded group
        arrow_x0[0] = x + margin + 4;
        arrow_y0[0] = y + margin + 4 + 0;
        arrow_x1[0] = x + margin + 4;
        arrow_y1[0] = y + margin + 4 + 10;

        arrow_x0[1] = x + margin + 5;
        arrow_y0[1] = y + margin + 4 + 1;
        arrow_x1[1] = x + margin + 5;
        arrow_y1[1] = y + margin + 4 + 9;

        arrow_x0[2] = x + margin + 6;
        arrow_y0[2] = y + margin + 4 + 2;
        arrow_x1[2] = x + margin + 6;
        arrow_y1[2] = y + margin + 4 + 8;

        arrow_x0[3] = x + margin + 7;
        arrow_y0[3] = y + margin + 4 + 3;
        arrow_x1[3] = x + margin + 7;
        arrow_y1[3] = y + margin + 4 + 7;

        arrow_x0[4] = x + margin + 8;
        arrow_y0[4] = y + margin + 4 + 4;
        arrow_x1[4] = x + margin + 8;
        arrow_y1[4] = y + margin + 4 + 6;

        arrow_x0[5] = x + margin + 9;
        arrow_y0[5] = y + margin + 4 + 5;
        arrow_x1[5] = x + margin + 9;
        arrow_y1[5] = y + margin + 4 + 5;

        for (i = 0; i < 6; i++) {
            current_device()->line(ED4_G_STANDARD, arrow_x0[i], arrow_y0[i], arrow_x1[i], arrow_y1[i], AW_SCREEN);
        }
    }
    else {
        arrow_x0[0] = x + margin + 4 + 0;
        arrow_y0[0] = y + margin + 4;
        arrow_x1[0] = x + margin + 4 + 4;
        arrow_y1[0] = y + margin + 4;

        arrow_x0[1] = x + margin + 4 + 0;
        arrow_y0[1] = y + margin + 5;
        arrow_x1[1] = x + margin + 4 + 4;
        arrow_y1[1] = y + margin + 5;

        arrow_x0[2] = x + margin + 4 + 1;
        arrow_y0[2] = y + margin + 6;
        arrow_x1[2] = x + margin + 4 + 3;
        arrow_y1[2] = y + margin + 6;

        arrow_x0[3] = x + margin + 4 + 1;
        arrow_y0[3] = y + margin + 7;
        arrow_x1[3] = x + margin + 4 + 3;
        arrow_y1[3] = y + margin + 7;

        arrow_x0[4] = x + margin + 4 + 2;
        arrow_y0[4] = y + margin + 8;
        arrow_x1[4] = x + margin + 4 + 2;
        arrow_y1[4] = y + margin + 8;

        arrow_x0[5] = x + margin + 4 + 2;
        arrow_y0[5] = y + margin + 9;
        arrow_x1[5] = x + margin + 4 + 2;
        arrow_y1[5] = y + margin + 9;

        for (i = 0; i < 6; i++) {
            current_device()->line(ED4_G_STANDARD, arrow_x0[i], arrow_y0[i], arrow_x1[i], arrow_y1[i], AW_SCREEN);
        }
    }

    for (i = 0; i <= 2; i++) {
        current_device()->line(ED4_G_STANDARD, line_x0[i], line_y0[i], line_x1[i], line_y1[i], AW_SCREEN);
    }

    return (ED4_R_OK);
}

ED4_species_name_terminal::ED4_species_name_terminal(GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_text_terminal(species_name_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
}

#define MAXNAMELEN MAXSPECIESWIDTH
#define BUFFERSIZE (MAXNAMELEN<<1)
GB_CSTR ED4_species_name_terminal::get_displayed_text() const
{
    static char *real_name;
    static int allocatedSize;

    if (!real_name || allocatedSize<(BUFFERSIZE+1)) {
        free(real_name);
        allocatedSize = BUFFERSIZE+1;
        real_name = (char*)malloc(allocatedSize);
    }
    memset(real_name, 0, allocatedSize);

    if (parent->flag.is_consensus) {
        char *db_pointer = resolve_pointer_to_string_copy();
        char *bracket = strchr(db_pointer, '(');

        if (bracket) {
            int bracket_len = strlen(bracket);
            int name_len = bracket-db_pointer;

            if (bracket[-1]==' ') {
                name_len--;
            }

            if ((name_len+1+bracket_len)<=MAXNAMELEN) {
                strcpy(real_name, db_pointer);
            }
            else {
                int short_name_len = MAXNAMELEN-bracket_len-1;

                memcpy(real_name, db_pointer, short_name_len);
                real_name[short_name_len] = ' ';
                strcpy(real_name+short_name_len+1, bracket);
            }
        }
        else {
            strncpy(real_name, db_pointer, BUFFERSIZE);
        }

        free(db_pointer);
    }
    else if (parent->parent->parent->flag.is_SAI) { // whether species_manager has is_SAI flag
        char *db_pointer = resolve_pointer_to_string_copy();

        strcpy(real_name, "SAI: ");
        if (strcmp(db_pointer, "ECOLI")==0) {
            const char *name_for_ecoli = ED4_ROOT->aw_root->awar(ED4_AWAR_NDS_ECOLI_NAME)->read_char_pntr();
            if (name_for_ecoli[0]==0) name_for_ecoli = db_pointer;
            strncpy(real_name+5, name_for_ecoli, BUFFERSIZE-5);
        }
        else {
            strncpy(real_name+5, db_pointer, BUFFERSIZE-5);
        }
        free(db_pointer);
    }
    else { // normal species
        ED4_species_manager *species_man = get_parent(ED4_L_SPECIES)->to_species_manager();
        char *result = ED4_get_NDS_text(species_man);

        strncpy(real_name, result, BUFFERSIZE);
        free(result);
    }

    return real_name;
}
#undef MAXNAMELEN
#undef BUFFERSIZE


ED4_sequence_info_terminal::ED4_sequence_info_terminal(const char *temp_id, /* GBDATA *gbd, */ AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_text_terminal(sequence_info_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
}

ED4_consensus_sequence_terminal::ED4_consensus_sequence_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_sequence_terminal(temp_id, x, y, width, height, temp_parent)
{
    species_name = NULL;
}

ED4_abstract_sequence_terminal::ED4_abstract_sequence_terminal(const ED4_objspec& spec_, const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_text_terminal(spec_, temp_id, x, y, width, height, temp_parent)
{
    species_name = NULL;
}

ED4_abstract_sequence_terminal::~ED4_abstract_sequence_terminal() {
    free(species_name);
}

ED4_orf_terminal::ED4_orf_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_abstract_sequence_terminal(orf_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
    aaSequence   = 0;
    aaSeqLen     = 0;
    aaColor      = 0;
    aaStartPos   = 0;
    aaStrandType = 0;
}

GB_alignment_type ED4_orf_terminal::GetAliType()
{
    return (GB_alignment_type) GB_AT_AA;
}

ED4_orf_terminal::~ED4_orf_terminal()
{
    free(aaSequence);
    free(aaColor);
}

ED4_sequence_terminal::ED4_sequence_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_abstract_sequence_terminal(sequence_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
    st_ml_node = NULL;
}

GB_alignment_type ED4_sequence_terminal::GetAliType()
{
    return ED4_ROOT->alignment_type;
}

ED4_pure_text_terminal::ED4_pure_text_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_text_terminal(pure_text_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
}

ED4_returncode ED4_spacer_terminal::Show(int /* refresh_all */, int is_cleared) // a spacer terminal doesn't show anything - it's just a dummy terminal
{
    if (update_info.clear_at_refresh && !is_cleared) {
        clear_background();
    }
    draw();
    return ED4_R_OK;
}


ED4_returncode ED4_spacer_terminal::draw() {
#if defined(DEBUG) && 0
    clear_background(ED4_G_FIRST_COLOR_GROUP); // draw colored spacers to make them visible
#else
    clear_background(0);
#endif // DEBUG
    return (ED4_R_OK);
}

ED4_spacer_terminal::ED4_spacer_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_terminal(spacer_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
}

ED4_returncode ED4_line_terminal::draw() {
    AW_pos x1, y1;
    calc_world_coords(&x1, &y1);
    current_ed4w()->world_to_win_coords(&x1, &y1);

    AW_pos x2 = x1+extension.size[WIDTH]-1;
    AW_pos y2 = y1+extension.size[HEIGHT]-1;

    AW_device *device = current_device();

    device->line(ED4_G_STANDARD, x1, y1, x2, y1);
#if defined(DEBUG)
    device->box(ED4_G_MARKED, true, x1, y1+1, x2-x1+1, y2-y1-1);
#else
    device->clear_part(x1, y1+1, x2-x1+1, y2-y1-1, AW_ALL_DEVICES);
#endif // DEBUG
    device->line(ED4_G_STANDARD, x1, y2, x2, y2);

    return ED4_R_OK;
}

ED4_returncode ED4_line_terminal::Show(int /* refresh_all */, int is_cleared)
{
    if (update_info.clear_at_refresh && !is_cleared) {
        clear_background();
    }
    draw();
    return ED4_R_OK;
}


ED4_line_terminal::ED4_line_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent)
    : ED4_terminal(line_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
}

// --------------------------------------------------------------------------------
//  ED4_columnStat_terminal
// --------------------------------------------------------------------------------

ED4_returncode ED4_columnStat_terminal::Show(int IF_ASSERTION_USED(refresh_all), int is_cleared)
{
    e4_assert(update_info.refresh || refresh_all);
    current_device()->push_clip_scale();
    if (adjust_clipping_rectangle()) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
        }
        draw();
    }
    current_device()->pop_clip_scale();

    return ED4_R_OK;
}

inline char stat2display(int val, bool is_upper_digit) {
    if (val<0) {
        e4_assert(val==-1); // -1 indicates that no statistic is existing for that column
        return '?';
    }
    if (val==20) return ' '; // value if ACGT and - are distributed equally
    if (val==100) return '^'; // we have only 2 characters to display likelihood (100 cannot be displayed)

    e4_assert(val>=0 && val<100);

    return '0' + (is_upper_digit ? (val/10) : (val%10));
}

inline int find_significant_positions(int sig, int like_A, int like_C, int like_G, int like_TU, int *sumPtr) {
    // result == 0      -> no base-char has a significant likelihood (>=sig)
    // result == 1, 2, 4, 8     -> A, C, G, T/U has a significant likelihood
    // else             -> the sum two of the likelihoods >= sig (bit-or-ed as in line above)

    int like[4];
    like[0] = like_A;
    like[1] = like_C;
    like[2] = like_G;
    like[3] = like_TU;

    int bestSum = 0;
    int bestResult = 0;

    int b, c;
    for (b=0; b<4; b++) {
        int sum = like[b];
        if (sum>=sig && sum>=bestSum) {
            bestSum = sum;
            bestResult = 1<<b;
        }
    }

    if (!bestResult) {
        for (b=0; b<4; b++) {
            for (c=b+1; c<4; c++) {
                int sum = like[b]+like[c];
                if (sum>=sig && sum>=bestSum) {
                    bestSum = sum;
                    bestResult = (1<<b)|(1<<c);
                }
            }
        }
    }

    if (bestResult) {
        if (sumPtr) *sumPtr = bestSum;
        return bestResult;
    }

    return 0;
}

#define PROBE_MATCH_TARGET_STRING_LENGTH 32

GB_CSTR ED4_columnStat_terminal::build_probe_match_string(PosRange range) const {
    static char            result[PROBE_MATCH_TARGET_STRING_LENGTH+1]; // see create_probe_match_window() for length
    int                    max_insert   = PROBE_MATCH_TARGET_STRING_LENGTH;
    char                  *r            = result;
    int                    significance = int(get_threshold());
    ED4_sequence_terminal *seq_term     = corresponding_sequence_terminal()->to_sequence_terminal();
    char                  *seq          = seq_term->resolve_pointer_to_string_copy();

    for (int pos=range.start(); pos<=range.end(); pos++) {
        int found = find_significant_positions(significance, likelihood[0][pos], likelihood[1][pos], likelihood[2][pos], likelihood[3][pos], 0);

        if (found || strchr("-=.", seq[pos])==0) {
            // '?' is inserted at every base position without significant bases (not at gaps!)

            *r++ = "NACMGRSVUWYHKDBN"[found]; // this creates a string containing the full IUPAC code

            if (max_insert--==0) {
                r--;
                break;
            }
        }
    }
    r[0] = 0;
    free(seq);

    if (max_insert<0) {
        aw_message(GBS_global_string("Max. %i bases allowed in Probe Match", PROBE_MATCH_TARGET_STRING_LENGTH));
        result[0] = 0;
    }
    return result;
}
#undef PROBE_MATCH_TARGET_STRING_LENGTH

ED4_returncode ED4_columnStat_terminal::draw() {
    if (!update_likelihood()) {
        aw_message("Can't calculate likelihood.");
        return ED4_R_IMPOSSIBLE;
    }

    AW_pos x, y;
    calc_world_coords(&x, &y);
    current_ed4w()->world_to_win_coords(&x, &y);

    AW_pos term_height = extension.size[HEIGHT];
    AW_pos font_height = ED4_ROOT->font_group.get_height(ED4_G_SEQUENCES);
    AW_pos font_width  = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);

    AW_pos text_x = x + CHARACTEROFFSET;
    AW_pos text_y = y + term_height - font_height;

    AW_device             *device   = current_device();
    ED4_sequence_terminal *seq_term = corresponding_sequence_terminal();
    const ED4_remap       *rm       = ED4_ROOT->root_group_man->remap();
    
    PosRange index_range = rm->clip_screen_range(seq_term->calc_update_interval());
    {
        int max_seq_len = seq_term->get_length();
        int max_seq_pos = rm->sequence_to_screen_clipped(max_seq_len);

        index_range = ExplicitRange(index_range, max_seq_pos);
        if (index_range.is_empty()) return ED4_R_OK;
    }

    const int left  = index_range.start();
    const int right = index_range.end();

    char *sbuffer = new char[right+2];  // used to build displayed terminal content  (values = '0'-'9')
    memset(sbuffer, ' ', right+1);
    sbuffer[right+1] = 0; // eos

    AW_pos y2;
    int r;

    // Set background:
    {
        int significance = int(get_threshold());
        // normal columns are colored in ED4_G_STANDARD
        // all columns with one (or sum of two) probability above 'significance' are back-colored in ED4_G_CBACK_0
        // the responsible probabilities (one or two) are back-colored in ED4_G_CBACK_1..ED4_G_CBACK_9

        int color     = ED4_G_STANDARD;
        int old_color = ED4_G_STANDARD;

        AW_pos x2     = text_x + font_width*left + 1;
        AW_pos old_x2 = x2;

        PosRange selection;
        bool     is_selected = ED4_get_selected_range(seq_term, selection);

        for (int i=left; i<=right; i++, x2+=font_width) { // colorize significant columns in ALL rows
            int p = rm->screen_to_sequence(i);
            int found = find_significant_positions(significance, likelihood[0][p], likelihood[1][p], likelihood[2][p], likelihood[3][p], 0);

            if (found)  color = ED4_G_CBACK_0;
            else    color = ED4_G_STANDARD;

            if (is_selected && selection.contains(p)) {
                color = ED4_G_SELECTED;
            }

            if (color!=old_color) {
                if (x2>old_x2 && old_color!=ED4_G_STANDARD) {
                    device->box(old_color, true, old_x2, y, x2-old_x2, term_height);
                }
                old_color = color;
                old_x2 = x2;
            }
        }
        if (x2>old_x2 && old_color!=ED4_G_STANDARD) {
            device->box(old_color, true, old_x2, y, x2-old_x2, term_height);
        }

        color = ED4_G_STANDARD;
        x2 = text_x + font_width*left + 1;

        for (int i=left; i<=right; i++, x2+=font_width) { // colorize significant columns in SINGLE rows
            int p = rm->screen_to_sequence(i);
            int sum;
            int found = find_significant_positions(significance, likelihood[0][p], likelihood[1][p], likelihood[2][p], likelihood[3][p], &sum);

            if (found && significance<100) {
                e4_assert(sum>=significance && sum<=100);
                color = ED4_G_CBACK_1+((sum-significance)*(ED4_G_CBACK_9-ED4_G_CBACK_1))/(100-significance);
                e4_assert(color>=ED4_G_CBACK_1 && color<=ED4_G_CBACK_9);
            }
            else {
                color = ED4_G_STANDARD;
            }

            if (color!=ED4_G_STANDARD) {
                int bit;

                for (r=3, y2=text_y+1, bit=1<<3;
                     r>=0;
                     r--, y2-=COLUMN_STAT_ROW_HEIGHT(font_height), bit>>=1)
                {
                    if (found&bit) {
                        device->box(color, true, x2, y2-2*font_height+1, font_width, 2*font_height);
                    }
                }
            }
        }
    }

    // Draw text:
    for (r=3, y2=text_y;
         r>=0;
         r--, y2-=COLUMN_STAT_ROW_HEIGHT(font_height)) { // 4 rows (one for T/U, G, C and A)

        int gc = ED4_ROOT->sequence_colors->char_2_gc[(unsigned char)"ACGU"[r]];
        int i;
        for (i=left; i<=right; i++) {
            int p = rm->screen_to_sequence(i);
            int val = likelihood[r][p];
            sbuffer[i] = stat2display(val, 0); // calc lower digit
        }
        device->text(gc, sbuffer, text_x+font_width*0.2, y2, 0, AW_SCREEN, right); // draw lower-significant digit (shifted a bit to the right)

        for (i=left; i<=right; i++) {
            int p = rm->screen_to_sequence(i);
            int val = likelihood[r][p];
            sbuffer[i] = stat2display(val, 1); // calc upper digit
        }
        device->text(gc, sbuffer, text_x, y2-font_height, 0, AW_SCREEN, right); // draw higher-significant digit
    }

    delete [] sbuffer;
    return ED4_R_OK;
}

int ED4_columnStat_terminal::update_likelihood() {
    ED4_sequence_terminal *seq_term = corresponding_sequence_terminal();

    return STAT_update_ml_likelihood(ED4_ROOT->st_ml, likelihood, latest_update, 0, seq_term->st_ml_node);
}

ED4_columnStat_terminal::ED4_columnStat_terminal(GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_text_terminal(column_stat_terminal_spec, temp_id, x, y, width, height, temp_parent)
{
    for (int i=0; i<4; i++) likelihood[i] = 0;
    latest_update = 0;
}

ED4_columnStat_terminal::~ED4_columnStat_terminal()
{
    for (int i=0; i<4; i++) free(likelihood[i]);
}

// --------------------------------------------------------------------------------
//  ED4_reference_terminals
// --------------------------------------------------------------------------------

void ED4_reference_terminals::clear()
{
    delete ref_sequence_info;
    delete ref_sequence;
    delete ref_column_stat;
    delete ref_column_stat_info;
    null();
}

void ED4_reference_terminals::init(ED4_sequence_info_terminal *ref_sequence_info_,
                                   ED4_sequence_terminal *ref_sequence_,
                                   ED4_sequence_info_terminal *ref_column_stat_info_,
                                   ED4_columnStat_terminal *ref_column_stat_)
{
    clear();
    ref_sequence_info    = ref_sequence_info_;
    ref_sequence     = ref_sequence_;
    ref_column_stat_info = ref_column_stat_info_;
    ref_column_stat      = ref_column_stat_;
}
