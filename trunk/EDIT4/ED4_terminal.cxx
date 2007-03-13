#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
// #include <malloc.h>
#include <assert.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_keysym.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>

#include <awt_seq_colors.hxx>
#include <st_window.hxx>

#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_block.hxx"
#include "ed4_nds.hxx"
#include "ed4_ProteinViewer.hxx"

//*******************************************
//* Terminal static properties  beginning   *
//*******************************************


ED4_object_specification tree_terminal_spec =                       // variables which determine static default properties of predefined (sub-)classes
{
    ED4_P_IS_TERMINAL,  // static props
    ED4_L_TREE,         // level
    ED4_L_NO_LEVEL,     // allowed children level
    ED4_L_NO_LEVEL,     // handled object
    ED4_L_NO_LEVEL,     // restriction level
    0                   // justification value --no meaning for a terminal
};

ED4_object_specification bracket_terminal_spec =
{
    ED4_P_IS_TERMINAL,  // static props
    ED4_L_BRACKET,      // level
    ED4_L_NO_LEVEL,     // allowed children level
    ED4_L_NO_LEVEL,     // handled object
    ED4_L_NO_LEVEL,     // restriction level
    0                   // justification value --no meaning for a terminal
};

ED4_object_specification species_name_terminal_spec =
{
    ED4_P_IS_TERMINAL,  // static props
    ED4_L_SPECIES_NAME, // level
    ED4_L_NO_LEVEL,     // allowed children level
    ED4_L_SPECIES,      // handled object
    ED4_L_NO_LEVEL,     // restriction level
    0                   // justification value --no meaning for a terminal
};

ED4_object_specification sequence_info_terminal_spec =
{
    ED4_P_IS_TERMINAL,  // static props
    ED4_L_SEQUENCE_INFO,// level
    ED4_L_NO_LEVEL,     // allowed children level
    ED4_L_SEQUENCE,     // handled object
    ED4_L_NO_LEVEL,     // restriction level
    0                   // justification value --no meaning for a terminal
};

ED4_object_specification sequence_terminal_spec =
{
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_SEQUENCE_STRING, // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL,        // restriction level
    0                      // justification value --no meaning for a terminal
};

ED4_object_specification pure_text_terminal_spec =
{
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_PURE_TEXT,       // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL,        // restriction level
    0                      // justification value --no meaning for a terminal
};

ED4_object_specification spacer_terminal_spec =
{
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_SPACER,          // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL,        // restriction level
    0                      // justification value --no meaning for a terminal
};

ED4_object_specification line_terminal_spec =
{
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_LINE,            // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL,        // restriction level
    0                      // justification value --no meaning for a terminal
};

ED4_object_specification column_stat_terminal_spec =
{
    ED4_P_IS_TERMINAL,     // static props
    ED4_L_COL_STAT,        // level
    ED4_L_NO_LEVEL,        // allowed children level
    ED4_L_NO_LEVEL,        // handled object
    ED4_L_NO_LEVEL,        // restriction level
    0                      // justification value --no meaning for a terminal
};

//*******************************************
//* Terminal static properties      end *
//*******************************************




//*****************************************
//* ED4_terminal Methods    beginning *
//*****************************************


char *ED4_terminal::resolve_pointer_to_string_copy(int *str_len) const {
    int         len;
    const char *s = resolve_pointer_to_char_pntr(&len);
    char       *t = GB_strduplen(s, len); // space for zero byte is allocated by GB_strduplen

    if (str_len) *str_len = len;
    return t;
}

const char *ED4_terminal::resolve_pointer_to_char_pntr(int *str_len) const
{
    GB_TYPES  gb_type;
    char     *db_pointer = NULL;
    GBDATA   *gbd        = get_species_pointer();

    if (!gbd) {
        if (str_len) {
            if (id) *str_len = strlen(id);
            else *str_len    = 0;
        }
        return id; // if we don't have a link to the database we have to use our id
    }

    GB_push_transaction(gb_main);

    const char *copy_of = 0; // if set, return a copy of this string

    gb_type = GB_read_type(gbd);
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
        case GB_INTS:
            copy_of = "GB_INTS";
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

    GB_pop_transaction( gb_main );

    return db_pointer;
}

ED4_ERROR *ED4_terminal::write_sequence(const char *seq, int seq_len)
{
    ED4_ERROR *err = 0;
    GBDATA *gbd = get_species_pointer();
    e4_assert(gbd); // we must have a link to the database!

    GB_push_transaction(gb_main);

    int   old_seq_len;
    char *old_seq = resolve_pointer_to_string_copy(&old_seq_len);

    bool allow_write_data = true;
    if (ED4_ROOT->aw_root->awar(ED4_AWAR_ANNOUNCE_CHECKSUM_CHANGES)->read_int()) {
        long old_checksum = GBS_checksum(old_seq, 1, "-.");
        long new_checksum = GBS_checksum(seq, 1, "-.");

        if (old_checksum != new_checksum) {
            if (aw_message("Checksum changed!", "Allow, Reject") == 1) {
                allow_write_data = false;
                // GB_write_string(info->gb_data, old_seq);
            }

        }
    }

    if (allow_write_data) {
        GB_TYPES gb_type = GB_read_type(gbd);
        switch(gb_type) {
            case GB_STRING: {
                //             old_seq = GB_read_char_pntr(gbd);
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

    GB_pop_transaction(gb_main);

    if (!err && dynamic_prop&ED4_P_CONSENSUS_RELEVANT) {
        if (old_seq) {
            actual_timestamp = GB_read_clock(gb_main);

            get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager()
                ->check_bases_and_rebuild_consensi(old_seq, old_seq_len, get_parent(ED4_L_SPECIES)->to_species_manager(), ED4_U_UP); // bases_check
        }
        else {
            aw_message("Couldn't read old sequence data");
        }

        set_refresh();
        parent->refresh_requested_by_child();
    }

    if (old_seq) free(old_seq);

    return err;
}


ED4_returncode ED4_terminal::remove_callbacks()                     //removes callbacks and gb_alignment
{
    if (get_species_pointer()) {
        set_species_pointer(0);
        flag.deleted = 1;
        dynamic_prop = (ED4_properties) (dynamic_prop & ~ED4_P_CURSOR_ALLOWED);

        set_refresh();
        parent->refresh_requested_by_child();
    }
    return ED4_R_OK;
}

static ED4_returncode ed4_remove_species_manager_callbacks(void **, void **, ED4_base *base) {
    if (base->is_species_manager()) {
        base->to_species_manager()->remove_all_callbacks();
    }
    return ED4_R_OK;
}

ED4_returncode ED4_terminal::kill_object()
{
    ED4_species_manager *species_manager = get_parent( ED4_L_SPECIES )->to_species_manager();
    ED4_manager         *parent_manager  = species_manager->parent;
    ED4_group_manager   *group_manager   = 0;

    if (species_manager->flag.is_consensus)                         // whole group has to be killed
    {
        if (parent_manager->is_multi_species_manager() && ED4_new_species_multi_species_manager()==parent_manager) {
            aw_message("This group has to exist - deleting it isn't allowed");
            return ED4_R_IMPOSSIBLE;
        }

        parent_manager = species_manager;
        while (!parent_manager->is_group_manager()) {
            parent_manager = parent_manager->parent;
        }
        group_manager = parent_manager->to_group_manager();

        parent_manager = group_manager->parent;
        parent_manager->children->delete_member( group_manager );

        GB_push_transaction ( gb_main );
        parent_manager->update_consensus( parent_manager ,NULL , group_manager );
        parent_manager->rebuild_consensi( parent_manager , ED4_U_UP );
        GB_pop_transaction ( gb_main );
        species_manager = NULL;
    }
    else
    {
        parent_manager->children->delete_member( species_manager );
        GB_push_transaction (gb_main );
        parent_manager->update_consensus( parent_manager , NULL, species_manager );
        parent_manager->rebuild_consensi( species_manager, ED4_U_UP );
        GB_pop_transaction (gb_main);
    }

    ED4_device_manager *device_manager = ED4_ROOT->main_manager
        ->get_defined_level(ED4_L_GROUP)->to_group_manager()
        ->get_defined_level(ED4_L_DEVICE)->to_device_manager();

    for (int i=0; i<device_manager->children->members(); i++) { // when killing species numbers have to be recalculated
        ED4_base *member = device_manager->children->member(i);

        if (member->is_area_manager()) {
            member->to_area_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->generate_id_for_groups();
        }
    }

    if (species_manager) {
        species_manager->parent = NULL;
        species_manager->remove_all_callbacks();
        delete species_manager;
    }

    if (group_manager) {
        group_manager->parent = NULL;
        group_manager->route_down_hierarchy(0, 0, ed4_remove_species_manager_callbacks);
        delete group_manager;
    }

    return ED4_R_OK;
}

ED4_base *ED4_terminal::get_competent_clicked_child( AW_pos /*x*/, AW_pos /*y*/, ED4_properties /*relevant_prop*/)
{
    e4_assert(0);
    return NULL;
}

ED4_returncode   ED4_terminal::handle_move(ED4_move_info */*moveinfo*/)
{
    e4_assert(0);
    return ED4_R_IMPOSSIBLE;
}

ED4_returncode  ED4_terminal::move_requested_by_child(ED4_move_info */*moveinfo*/)
{
    e4_assert(0);
    return ED4_R_IMPOSSIBLE;
}


ED4_base *ED4_terminal::get_competent_child( AW_pos /*x*/, AW_pos /*y*/, ED4_properties /*relevant_prop*/)
{
    e4_assert(0);
    return NULL;
}

ED4_returncode ED4_terminal::resize_requested_by_child(void)
{
    e4_assert(0);
    return ED4_R_IMPOSSIBLE;
}

ED4_returncode ED4_terminal::draw_drag_box( AW_pos x, AW_pos y, GB_CSTR text, int cursor_y )    // draws drag box of object at location abs_x, abs_y
{
    ED4_index   i;
    AW_pos          width, height, drag_x, drag_y;
    AW_pos      drag_line_x0[3], drag_line_y0[3];
    AW_pos      drag_line_x1[3], drag_line_y1[3];
    ED4_base    *drag_target = NULL;
    AW_pos      target_x, target_y;
    ED4_extension   location;

    width = extension.size[WIDTH] - 1;
    height = extension.size[HEIGHT] - 1;

    if (cursor_y!=-1)
    {
        ED4_device_manager *device_manager = ED4_ROOT->main_manager->search_spec_child_rek(ED4_L_DEVICE)->to_device_manager();
        drag_x = 0;
        drag_y = (AW_pos)cursor_y; // cursor_y is already in world coordinates!
        //  ED4_ROOT->win_to_world_coords( ED4_ROOT->temp_aww, &drag_x, &drag_y );
        location.position[X_POS] = drag_x;
        location.position[Y_POS] = drag_y;
        //  ED4_base::touch_world_cache();
        device_manager->children->search_target_species( &location, ED4_P_HORIZONTAL, &drag_target, ED4_L_NO_LEVEL );

        if (drag_target && !is_sequence_info_terminal()) {
            drag_target->calc_world_coords ( &target_x, &target_y );
            ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &target_x, &target_y );
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
            drag_line_y0[2] = drag_line_y0[0] +ARROW_LENGTH;
            drag_line_x1[2] = drag_line_x0[0] ;
            drag_line_y1[2] = drag_line_y0[0];
#undef ARROW_LENGTH
            for( i = 0; i <= 2; i++ ) {
                ED4_ROOT->temp_device->line( ED4_G_DRAG, drag_line_x0[i], drag_line_y0[i], drag_line_x1[i], drag_line_y1[i], 1, 0, 0);
            }
        }
    }

    if ( text != NULL ) {
        ED4_ROOT->temp_device->text( ED4_G_DRAG, text, (x + 20), (y + INFO_TERM_TEXT_YOFFSET), 0, 1, 0, 0);
    }

    return ( ED4_R_OK );
}

ED4_returncode  ED4_terminal::move_requested_by_parent(ED4_move_info *) { // handles a move request coming from parent
    e4_assert(0);
    return ( ED4_R_IMPOSSIBLE );
}

#if defined(DEBUG) && 1
static inline void dumpEvent(const char *where, AW_event *event) {
    printf("%s: x=%i y=%i\n", where, event->x, event->y);
}
#else
#define dumpEvent(w,e)
#endif

ED4_returncode  ED4_terminal::event_sent_by_parent( AW_event *event, AW_window *aww )           // handles an input event coming from parent
{
    static ED4_species_name_terminal    *dragged_name_terminal = 0;
    static int              dragged_was_selected;
    static bool                 pressed_left_button = 0;
    static int              other_x, other_y; // coordinates of last event
    static bool                 right_button_started_on_sequence_term = 0;

    switch ( event->type ) {
        case AW_Mouse_Press: {
            switch ( event->button ) {
                case ED4_B_LEFT_BUTTON: {
                    pressed_left_button = 0;
                    if (is_species_name_terminal()) {
                        switch (ED4_ROOT->species_mode) {
                            case ED4_SM_KILL: {
                                if (flag.selected) {
                                    ED4_ROOT->remove_from_selected(this);
                                }
                                kill_object();
                                ED4_ROOT->refresh_all_windows(0);
                                return ED4_R_BREAK;
                            }
                            case ED4_SM_MOVE: {
                                dragged_name_terminal = to_species_name_terminal();
                                pressed_left_button = 1;
                                other_x = event->x;
                                other_y = event->y;

                                if (!flag.selected) {
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
                                    set_refresh();
                                    parent->refresh_requested_by_child();
                                    //ProtView: Refreshing AA_sequence terminals
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
                            ED4_ROOT->main_manager->unfold_group(id);
                        }
                        else {
                            ED4_ROOT->main_manager->fold_group(id);
                        }
                        ED4_ROOT->refresh_all_windows(1);
                    }
                    else  {
                        if (dynamic_prop & ED4_P_CURSOR_ALLOWED) {
                            ED4_no_dangerous_modes();
                            ED4_ROOT->temp_ed4w->cursor.show_clicked_cursor(event->x, this);
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
                            multi_man->deselect_all_species();
                            ED4_correctBlocktypeAfterSelection();
                            ED4_ROOT->refresh_all_windows(0);
                        }
                    }
                    else if (is_species_name_terminal()) {
                        ED4_species_manager *species_man = get_parent(ED4_L_SPECIES)->to_species_manager();

                        if (parent->flag.is_consensus) { // click on consensus-name
                            ED4_multi_species_manager *multi_man = parent->get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
                            int species = multi_man->get_no_of_species();
                            int selected = multi_man->get_no_of_selected_species();

                            if (selected==species) { // all species selected => deselect all
                                multi_man->deselect_all_species();
                            }
                            else { // otherwise => select all
                                multi_man->select_all_species();
                            }

                            ED4_correctBlocktypeAfterSelection();
                            ED4_ROOT->refresh_all_windows(0);
                            return ( ED4_R_BREAK );
                        }
                        else if (species_man->flag.is_SAI) {
                            ; // don't mark SAIs
                        }
                        else { // click on species name
                            if (!flag.selected) { // select if not selected
                                if ( ED4_ROOT->add_to_selected( this ) == ED4_R_OK ) {
                                    ED4_correctBlocktypeAfterSelection();
                                    ED4_ROOT->refresh_all_windows(0);
                                }
                            }
                            else { // deselect if already selected
                                ED4_ROOT->remove_from_selected( this );
                                ED4_correctBlocktypeAfterSelection();
                                ED4_ROOT->refresh_all_windows(0);
                                return ( ED4_R_BREAK ); // in case we were called by event_to selected()
                            }
                        }
                    }
                    else if (is_sequence_terminal()) {
                        dumpEvent("Press", event);

                        ED4_no_dangerous_modes();
                        ED4_setColumnblockCorner(event, to_sequence_terminal()); // mark columnblock
                        right_button_started_on_sequence_term = 1;
                        ED4_ROOT->refresh_all_windows(0);
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
            switch ( event->button ) {
                case ED4_B_LEFT_BUTTON: {
                    if (dragged_name_terminal) {
                        ED4_selection_entry *sel_info = dragged_name_terminal->selection_info;

                        if (pressed_left_button) {
                            AW_pos world_x, world_y;

                            dragged_name_terminal->calc_world_coords(&world_x, &world_y);
                            ED4_ROOT->world_to_win_coords(aww, &world_x, &world_y);

                            sel_info->drag_old_x = world_x;
                            sel_info->drag_old_y = world_y;
                            sel_info->drag_off_x = world_x-other_x;
                            sel_info->drag_off_y = world_y-other_y;
                            sel_info->old_event_y = -1;

                            pressed_left_button = 0;
                        }

                        //          ED4_species_manager *species_man = dragged_name_terminal->get_parent(ED4_L_SPECIES)->to_species_manager();
                        //          char *text = ED4_get_NDS_text(species_man);
                        GB_CSTR text = dragged_name_terminal->get_displayed_text();

                        if (dragged_name_terminal->flag.dragged) {
                            dragged_name_terminal->draw_drag_box(sel_info->drag_old_x, sel_info->drag_old_y, text, sel_info->old_event_y);
                        }

                        AW_pos new_x = sel_info->drag_off_x + event->x;
                        AW_pos new_y = sel_info->drag_off_y + event->y;

                        dragged_name_terminal->draw_drag_box(new_x, new_y, text, event->y); // @@@ event->y ist falsch, falls vertikal gescrollt ist!

                        sel_info->drag_old_x = new_x;
                        sel_info->drag_old_y = new_y;
                        sel_info->old_event_y = event->y;

                        dragged_name_terminal->flag.dragged = 1;
                    }

                    break;
                }
                case ED4_B_RIGHT_BUTTON: {
                    if (is_sequence_terminal() && right_button_started_on_sequence_term) {
                        ED4_no_dangerous_modes();
                        ED4_setColumnblockCorner(event, to_sequence_terminal()); // mark columnblock
                        ED4_ROOT->refresh_all_windows(0);
                    }
                    break;
                }
            }
            break;
        }
        case AW_Mouse_Release: {
            switch ( event->button ) {
                case ED4_B_LEFT_BUTTON: {
                    if (dragged_name_terminal) {
                        if (dragged_name_terminal->flag.dragged) {
                            {
                                char                *db_pointer = dragged_name_terminal->resolve_pointer_to_string_copy();
                                ED4_selection_entry *sel_info   = dragged_name_terminal->selection_info;

                                dragged_name_terminal->draw_drag_box(sel_info->drag_old_x, sel_info->drag_old_y, db_pointer, sel_info->old_event_y);
                                dragged_name_terminal->flag.dragged = 0;

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
                                ED4_device_manager *device_manager = ED4_ROOT->main_manager
                                    ->get_defined_level(ED4_L_GROUP)->to_group_manager()
                                    ->get_defined_level(ED4_L_DEVICE)->to_device_manager();

                                int i;
                                for (i=0; i<device_manager->children->members(); i++) { // when moving species numbers have to be recalculated
                                    ED4_base *member = device_manager->children->member(i);

                                    if (member->is_area_manager()) {
                                        member->to_area_manager()->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->generate_id_for_groups();
                                    }
                                }
                            }
                        }
                        if (!dragged_was_selected) {
                            ED4_ROOT->remove_from_selected(dragged_name_terminal);
                        }

                        ED4_expose_cb(ED4_ROOT->temp_aww, 0, 0);
                        //          ED4_ROOT->refresh_all_windows(0);

                        pressed_left_button = 0;
                        dragged_name_terminal = 0;
                    }
                    break;
                }

                case ED4_B_RIGHT_BUTTON: {
                    if (is_sequence_terminal() && right_button_started_on_sequence_term) {
                        dumpEvent("Relea", event);
                        ED4_no_dangerous_modes();
                        ED4_setColumnblockCorner(event, to_sequence_terminal()); // mark columnblock
                        ED4_ROOT->refresh_all_windows(0);
                    }
                    break;
                }
            }
            break;
        }
        default:
            assert(0);
            break;
    }
    return ( ED4_R_OK );
}


ED4_returncode  ED4_terminal::calc_size_requested_by_parent( void )
{
    return ED4_R_OK;
}

short ED4_terminal::calc_bounding_box( void )
{
    short            bb_changed = 0;
    ED4_list_elem   *current_list_elem;
    ED4_base        *object;

    if (width_link) {
        if ( extension.size[WIDTH] != width_link->extension.size[WIDTH] ) { //all bounding Boxes have the same size !!!
            extension.size[WIDTH] = width_link->extension.size[WIDTH];
            bb_changed = 1;
        }
    }

    if (height_link) {
        if ( extension.size[HEIGHT] != height_link->extension.size[HEIGHT] ) {
            extension.size[HEIGHT] = height_link->extension.size[HEIGHT];
            bb_changed = 1;
        }
    }


    if ( bb_changed ) {
        //  printf("calc_bounding_box changed by terminal\n");
        //  dump();

        current_list_elem = linked_objects.first();
        while ( current_list_elem ) {
            object = ( ED4_base *) current_list_elem->elem();

            if ( object != NULL ) {
                object->link_changed( this );
            }

            current_list_elem = current_list_elem->next();
        }

        clear_background();

        set_refresh();
        parent->refresh_requested_by_child();
    }

    return ( bb_changed );
}

ED4_returncode ED4_terminal::Show(int /*refresh_all*/, int /*is_cleared*/)
{
    e4_assert(0);
    return ED4_R_IMPOSSIBLE;
}


ED4_returncode ED4_terminal::draw(int /*only_text*/)
{
    e4_assert(0);
    return ED4_R_IMPOSSIBLE;
}


ED4_returncode ED4_terminal::resize_requested_by_parent( void )
{
    // #ifndef NDEBUG
    //     ED4_base *base = (ED4_base*)this;
    //     e4_assert(!base->flag.hidden);
    // #endif

    if (update_info.resize) { // likes to resize?
        if (calc_bounding_box()) { // size changed?
            parent->resize_requested_by_child(); // tell parent!
        }
    }

    return ED4_R_OK;
}


ED4_returncode ED4_terminal::set_refresh(int clear)                 // sets refresh flag of current object
{
    update_info.set_refresh(1);
    update_info.set_clear_at_refresh(clear);
    return ( ED4_R_OK );
}


ED4_base* ED4_terminal::search_ID(const char *temp_id )
{
    if ( id && strcmp( temp_id, id ) == 0 ) return (this);
    return ( NULL );
}


int ED4_terminal::adjust_clipping_rectangle( void )                  //set scrolling area in AW_MIDDLE_AREA
{
    AW_pos              x, y; //, width, height;
    AW_rectangle        area_size;

    ED4_ROOT->temp_device->get_area_size( &area_size );

    calc_world_coords( &x, &y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );

    return ED4_ROOT->temp_device->reduceClipBorders(int(y), int(y+extension.size[HEIGHT]-1), int(x), int(x+extension.size[WIDTH]-1));
}



ED4_terminal::ED4_terminal(GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent ) :
    ED4_base( temp_id, x, y, width, height, temp_parent )
{
    memset((char*)&flag, 0, sizeof(flag));
    //    selected  = 0;
    //    dragged   = 0;
    //    deleted   = 0;
    selection_info = 0;
    actual_timestamp = 0;
}


ED4_terminal::~ED4_terminal()
{
    if (selection_info) {
        delete selection_info;
    }
    ED4_cursor& cursor = ED4_ROOT->temp_ed4w->cursor;
    if (this == cursor.owner_of_cursor) {
        cursor.init();
    }
}

//***********************************
//* ED4_terminal Methods    end *
//***********************************



//*********************************************************
//* Terminal constructors and destructor    beginning *
//*********************************************************

ED4_tree_terminal::ED4_tree_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &(tree_terminal_spec);
}

ED4_returncode ED4_tree_terminal::Show(int IF_DEBUG(refresh_all), int is_cleared)
{
    e4_assert(update_info.refresh || refresh_all);
    ED4_ROOT->temp_device->push_clip_scale();
    if (adjust_clipping_rectangle()) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
        }
        draw();
    }
    ED4_ROOT->temp_device->pop_clip_scale();

    return ( ED4_R_OK );
}



ED4_returncode ED4_tree_terminal::draw( int /*only_text*/ )                 // draws bounding box of object
{
    AW_pos  x, y;
    AW_pos  text_x, text_y;
    char   *db_pointer;

    calc_world_coords( &x, &y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );

    text_x = x + CHARACTEROFFSET;                           // don't change
    text_y = y + SEQ_TERM_TEXT_YOFFSET;

    db_pointer = resolve_pointer_to_string_copy();
    ED4_ROOT->temp_device->text( ED4_G_STANDARD, db_pointer, text_x, text_y, 0, 1, 0, 0);
    free(db_pointer);

    return ( ED4_R_OK );
}

ED4_tree_terminal::~ED4_tree_terminal()
{
}

ED4_bracket_terminal::ED4_bracket_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &(bracket_terminal_spec);
}

ED4_returncode ED4_bracket_terminal::Show(int IF_DEBUG(refresh_all), int is_cleared)
{
    e4_assert(update_info.refresh || refresh_all);
    ED4_ROOT->temp_device->push_clip_scale();
    if (adjust_clipping_rectangle()) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
        }
        draw();
    }
    ED4_ROOT->temp_device->pop_clip_scale();

    return ED4_R_OK;
}


ED4_returncode ED4_bracket_terminal::draw( int /*only_text*/ )                  // draws bounding box of object
{
    ED4_index   i;
    AW_pos      x, y,
        width  = extension.size[WIDTH] - 1,
        height = extension.size[HEIGHT] - 1,
        margin = 0;
    AW_pos      line_x0[3], line_y0[3];
    AW_pos      line_x1[3], line_y1[3];
    AW_pos      arrow_x0[6], arrow_y0[6];
    AW_pos      arrow_x1[6], arrow_y1[6];


    calc_world_coords( &x, &y );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );

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
        ED4_ROOT->temp_device->box(ED4_G_SELECTED, x, y, extension.size[WIDTH], extension.size[HEIGHT], (AW_bitset)-1, 0, 0);
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

        for( i = 0; i < 6; i++ ) {
            ED4_ROOT->temp_device->line( ED4_G_STANDARD, arrow_x0[i], arrow_y0[i], arrow_x1[i], arrow_y1[i], 1, 0, 0 );
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

        for( i = 0; i < 6; i++ ) {
            ED4_ROOT->temp_device->line( ED4_G_STANDARD, arrow_x0[i], arrow_y0[i], arrow_x1[i], arrow_y1[i], 1, 0, 0 );
        }
    }

    for( i = 0; i <= 2; i++ ) {
        ED4_ROOT->temp_device->line( ED4_G_STANDARD, line_x0[i], line_y0[i], line_x1[i], line_y1[i], 1, 0, 0 );
    }

    return ( ED4_R_OK );
}

ED4_bracket_terminal::~ED4_bracket_terminal()
{
}

ED4_species_name_terminal::ED4_species_name_terminal( GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent ) :
    ED4_text_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &(species_name_terminal_spec);
}


ED4_species_name_terminal::~ED4_species_name_terminal()
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
            char *name_for_ecoli = ED4_ROOT->aw_root->awar(ED4_AWAR_NDS_ECOLI_NAME)->read_string();
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


ED4_sequence_info_terminal::ED4_sequence_info_terminal(const char *temp_id, /*GBDATA *gbd,*/ AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_text_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &(sequence_info_terminal_spec);
    // gbdata = gbd;
}


ED4_sequence_info_terminal::~ED4_sequence_info_terminal()
{
}

ED4_consensus_sequence_terminal::ED4_consensus_sequence_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_sequence_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &(sequence_terminal_spec);
    species_name = NULL;
}

ED4_consensus_sequence_terminal::~ED4_consensus_sequence_terminal()
{
}

ED4_sequence_terminal_basic::ED4_sequence_terminal_basic(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_text_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &(sequence_terminal_spec);
    species_name = NULL;
}

ED4_sequence_terminal_basic::~ED4_sequence_terminal_basic()
{
}

ED4_AA_sequence_terminal::ED4_AA_sequence_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_sequence_terminal_basic( temp_id, x, y, width, height, temp_parent )
{
    spec = &(sequence_terminal_spec);
    aaSequence   = 0;
    aaStartPos   = 0;
    aaStrandType = 0;
}

GB_alignment_type ED4_AA_sequence_terminal::GetAliType()
{
    return (GB_alignment_type) GB_AT_AA;
}

ED4_AA_sequence_terminal::~ED4_AA_sequence_terminal()
{
    delete aaSequence;
}

ED4_sequence_terminal::ED4_sequence_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_sequence_terminal_basic( temp_id, x, y, width, height, temp_parent )
{
    spec = &(sequence_terminal_spec);
    //    species_name = NULL;
    st_ml_node = NULL;
}

GB_alignment_type ED4_sequence_terminal::GetAliType()
{
    return ED4_ROOT->alignment_type;
}

ED4_sequence_terminal::~ED4_sequence_terminal()
{
}

ED4_pure_text_terminal::ED4_pure_text_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_text_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &pure_text_terminal_spec;
}

ED4_pure_text_terminal::~ED4_pure_text_terminal()
{
}


ED4_returncode ED4_spacer_terminal::Show(int /*refresh_all*/, int is_cleared) // a spacer terminal doesn't show anything - it's just a dummy terminal
{
    if (update_info.clear_at_refresh && !is_cleared) {
        clear_background();
    }
    draw();
    return ED4_R_OK;
}


ED4_returncode ED4_spacer_terminal::draw( int /*only_text*/ )                   // draws bounding box of object
{
#if defined(DEBUG) && 0
    clear_background(ED4_G_FIRST_COLOR_GROUP); // draw colored spacers to make them visible
#else
    clear_background(0);
#endif // DEBUG
    return ( ED4_R_OK );
}

ED4_spacer_terminal::ED4_spacer_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &(spacer_terminal_spec);
}


ED4_spacer_terminal::~ED4_spacer_terminal()
{
}

ED4_returncode ED4_line_terminal::draw( int /*only_text*/ )     // draws bounding box of object
{
    AW_pos x1, y1;
    calc_world_coords( &x1, &y1 );
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x1, &y1 );

    AW_pos x2 = x1+extension.size[WIDTH]-1;
    AW_pos y2 = y1+extension.size[HEIGHT]-1;

    AW_device *device = ED4_ROOT->temp_device;

    device->line(ED4_G_STANDARD, x1, y1, x2, y1, -1, 0, 0);
#if defined(DEBUG)
    device->box(ED4_G_MARKED, x1, y1+1, x2-x1+1, y2-y1-1, -1, 0, 0);
#else
    device->clear_part(x1, y1+1, x2-x1+1, y2-y1-1, -1);
#endif // DEBUG
    device->line(ED4_G_STANDARD, x1, y2, x2, y2, -1, 0, 0);
    
    return ED4_R_OK;
}

ED4_returncode ED4_line_terminal::Show(int /*refresh_all*/, int is_cleared)
{
    if (update_info.clear_at_refresh && !is_cleared) {
        clear_background();
    }
    draw();
    return ED4_R_OK;
}


ED4_line_terminal::ED4_line_terminal(const char *temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent )
    : ED4_terminal( temp_id, x, y, width, height, temp_parent )
{
    spec = &(line_terminal_spec);
}

ED4_line_terminal::~ED4_line_terminal()
{
}

// --------------------------------------------------------------------------------
//  ED4_columnStat_terminal
// --------------------------------------------------------------------------------

ED4_returncode ED4_columnStat_terminal::Show(int IF_DEBUG(refresh_all), int is_cleared)
{
    e4_assert(update_info.refresh || refresh_all);
    ED4_ROOT->temp_device->push_clip_scale();
    if (adjust_clipping_rectangle()) {
        if (update_info.clear_at_refresh && !is_cleared) {
            clear_background();
        }
        draw();
    }
    ED4_ROOT->temp_device->pop_clip_scale();

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

GB_CSTR ED4_columnStat_terminal::build_probe_match_string(int start_pos, int end_pos) const {
    static char            result[PROBE_MATCH_TARGET_STRING_LENGTH+1]; // see create_probe_match_window() for length
    int                    max_insert   = PROBE_MATCH_TARGET_STRING_LENGTH;
    char                  *r            = result;
    int                    significance = int(get_threshold());
    ED4_sequence_terminal *seq_term     = corresponding_sequence_terminal()->to_sequence_terminal();
    char                  *seq          = seq_term->resolve_pointer_to_string_copy();

    for (int pos=start_pos; pos<=end_pos; pos++) {
        int found = find_significant_positions(significance, likelihood[0][pos], likelihood[1][pos], likelihood[2][pos], likelihood[3][pos], 0);

        if (found || strchr("-=.", seq[pos])==0) {
            // '?' is inserted at every base position without significant bases (not at gaps!)

            *r++ = "NACMGRSVUWYHKDBN"[found]; // this creates a string containing the full IUPAC code
            //*r++ = "NACNGNNNUNNNNNNN"[found]; // this creates a string containing only ACGU + N

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

ED4_returncode ED4_columnStat_terminal::draw(int /*only_text*/)
{
#warning test drawing of ED4_columnStat_terminal    
    if (!update_likelihood()) {
        aw_message("Can't calculate likelihood.", "OK");
        return ED4_R_IMPOSSIBLE;
    }

    AW_pos x, y;
    calc_world_coords(&x, &y);
    ED4_ROOT->world_to_win_coords(ED4_ROOT->temp_aww, &x, &y);

    AW_pos term_height = extension.size[HEIGHT];
    AW_pos font_height = ED4_ROOT->font_group.get_height(ED4_G_SEQUENCES);
    AW_pos font_width  = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);

    AW_pos text_x = x + CHARACTEROFFSET;
    AW_pos text_y = y + term_height - (font_height /*+ ED4_ROOT->font_info[ED4_G_SEQUENCES].get_descent()*/ );

    AW_device *device = ED4_ROOT->temp_device;
    ED4_sequence_terminal *seq_term = corresponding_sequence_terminal();
    const ED4_remap *rm = ED4_ROOT->root_group_man->remap();
    long left,right;
    seq_term->calc_update_intervall(&left, &right );
    rm->clip_screen_range(&left, &right);

    {
        int max_seq_len = seq_term->get_length();
        int max_seq_pos = rm->sequence_to_screen_clipped(max_seq_len);

        if (right>max_seq_len) right = max_seq_pos;
        if (left>right) return ED4_R_OK;
    }

    //int seq_start = rm->screen_to_sequence(left); // real start of sequence
    char *sbuffer = new char[right+1];  // used to build displayed terminal content  (values = '0'-'9')
    memset(sbuffer, ' ', right);
    sbuffer[right] = 0; // eos

    AW_pos y2;
    int r;

    // Set background:
    {
        int significance = int(get_threshold());
        // normal columns are colored in ED4_G_STANDARD
        // all columns with one (or sum of two) probability above 'significance' are back-colored in ED4_G_CBACK_0
        // the responsible probabilities (one or two) are back-colored in ED4_G_CBACK_1..ED4_G_CBACK_9

        int color = ED4_G_STANDARD;
        int old_color = ED4_G_STANDARD;
        AW_pos x2 = text_x + font_width*left + 1;
        AW_pos old_x2 = x2;
        int i;
        int selection_col1, selection_col2;
        int is_selected = ED4_get_selected_range(seq_term, &selection_col1, &selection_col2);

        for (i=left; i<=right; i++,x2+=font_width) { // colorize significant columns in ALL rows
            int p = rm->screen_to_sequence(i);
            int found = find_significant_positions(significance, likelihood[0][p], likelihood[1][p], likelihood[2][p], likelihood[3][p], 0);

            if (found)  color = ED4_G_CBACK_0;
            else    color = ED4_G_STANDARD;

            if (is_selected && p>=selection_col1 && p<=selection_col2) {
                color = ED4_G_SELECTED;
            }

            if (color!=old_color) {
                if (x2>old_x2 && old_color!=ED4_G_STANDARD) {
                    device->box(old_color, old_x2, y, x2-old_x2, term_height, -1, 0, 0);
                }
                old_color = color;
                old_x2 = x2;
            }
        }
        if (x2>old_x2 && old_color!=ED4_G_STANDARD) {
            device->box(old_color, old_x2, y, x2-old_x2, term_height, -1, 0, 0);
        }

        color = ED4_G_STANDARD;
        x2 = text_x + font_width*left + 1;

        for (i=left; i<=right; i++,x2+=font_width) { // colorize significant columns in SINGLE rows
            int p = rm->screen_to_sequence(i);
            int sum;
            int found = find_significant_positions(significance, likelihood[0][p], likelihood[1][p], likelihood[2][p], likelihood[3][p], &sum);

            if (found) {
                e4_assert(sum>=significance && sum<=100);
                color = ED4_G_CBACK_1+((sum-significance)*(ED4_G_CBACK_9-ED4_G_CBACK_1))/(100-significance);
                e4_assert(color>=ED4_G_CBACK_1 && color<=ED4_G_CBACK_9);
            }
            else {
                color = ED4_G_STANDARD;
            }

            if (color!=ED4_G_STANDARD) {
                int bit;

                for (r=3,y2=text_y+1,bit=1<<3;
                     r>=0;
                     r--,y2-=COLUMN_STAT_ROW_HEIGHT(font_height),bit>>=1)
                {
                    if (found&bit) {
                        device->box(color, x2, y2-2*font_height+1, font_width, 2*font_height, -1, 0, 0);
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
        device->text(gc, sbuffer, text_x+font_width*0.2, y2, 0, 1, 0, 0, right); // draw lower-significant digit (shifted a bit to the right)

        for (i=left; i<=right; i++) {
            int p = rm->screen_to_sequence(i);
            int val = likelihood[r][p];
            sbuffer[i] = stat2display(val, 1); // calc upper digit
        }
        device->text(gc, sbuffer, text_x, y2-font_height, 0, 1, 0, 0, right); // draw higher-significant digit
    }

    free(sbuffer);
    return ED4_R_OK;
}

int ED4_columnStat_terminal::update_likelihood() {
    ED4_sequence_terminal *seq_term = corresponding_sequence_terminal();

    return st_ml_update_ml_likelihood(ED4_ROOT->st_ml, likelihood, &latest_update, 0, seq_term->st_ml_node);
}

ED4_columnStat_terminal::ED4_columnStat_terminal(GB_CSTR temp_id, AW_pos x, AW_pos y, AW_pos width, AW_pos height, ED4_manager *temp_parent) :
    ED4_text_terminal(temp_id, x, y, width, height, temp_parent)
{
    spec = &(column_stat_terminal_spec);
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
