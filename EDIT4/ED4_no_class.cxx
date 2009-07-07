#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <ad_config.h>

#include <aw_root.hxx>
#include <aw_keysym.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <AW_helix.hxx>
#include <awt.hxx>
#include <awt_item_sel_list.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_seq_colors.hxx>
#include <AW_rename.hxx>

#include <st_window.hxx>

#include <ed4_extern.hxx>

#include "ed4_awars.hxx"
#include "ed4_class.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_tools.hxx"
#include "ed4_nds.hxx"

//*************************************************
//* functions concerning no class   beginning *
//* Most functions are callback functions     *
  //*************************************************

void ED4_calc_terminal_extentions(){
    const AW_font_information *seq_font_info  = ED4_ROOT->get_device()->get_font_information( ED4_G_SEQUENCES, '=' );
    const AW_font_information *info_font_info = ED4_ROOT->get_device()->get_font_information( ED4_G_STANDARD, '.' );

    int info_char_width = info_font_info->max_letter.width;
    int seq_term_descent;

    if (ED4_ROOT->helix->is_enabled() || ED4_ROOT->protstruct) { // display helix ?
        ED4_ROOT->helix_spacing =
            seq_font_info->this_letter.ascent // the ascent of '='
            + ED4_ROOT->helix_add_spacing; // xtra user-defined spacing

        seq_term_descent = ED4_ROOT->helix_spacing;
    }
    else {
        ED4_ROOT->helix_spacing = 0;
        seq_term_descent  = seq_font_info->max_letter.descent;
    }

    // for wanted_seq_term_height ignore descent, because it additionally allocates 'ED4_ROOT->helix_spacing' space:
    int wanted_seq_term_height = seq_font_info->max_letter.ascent + seq_term_descent + ED4_ROOT->terminal_add_spacing;
    int wanted_seq_info_height = info_font_info->max_letter.height + ED4_ROOT->terminal_add_spacing;

    TERMINALHEIGHT = (wanted_seq_term_height>wanted_seq_info_height) ? wanted_seq_term_height : wanted_seq_info_height;

    {
        int maxchars;
        int maxbrackets;

        ED4_get_NDS_sizes(&maxchars, &maxbrackets);
        MAXSPECIESWIDTH =
            (maxchars+1)*info_char_width + // width defined in NDS window plus 1 char for marked-box
            maxbrackets*BRACKETWIDTH; // brackets defined in NDS window
    }
    MAXINFOWIDTH = CHARACTEROFFSET + info_char_width*ED4_ROOT->aw_root->awar(ED4_AWAR_NDS_INFO_WIDTH)->read_int() + 1;

    INFO_TERM_TEXT_YOFFSET = info_font_info->max_letter.ascent - 1;
    SEQ_TERM_TEXT_YOFFSET  = seq_font_info->max_letter.ascent - 1;

    if (INFO_TERM_TEXT_YOFFSET<SEQ_TERM_TEXT_YOFFSET) INFO_TERM_TEXT_YOFFSET = SEQ_TERM_TEXT_YOFFSET;

#if defined(DEBUG) && 0
    printf("seq_term_descent= %i\n", seq_term_descent);
    printf("TERMINALHEIGHT  = %i\n", TERMINALHEIGHT);
    printf("MAXSPECIESWIDTH = %i\n", MAXSPECIESWIDTH);
    printf("MAXINFOWIDTH    = %i\n", MAXINFOWIDTH);
    printf("INFO_TERM_TEXT_YOFFSET= %i\n", INFO_TERM_TEXT_YOFFSET);
    printf("SEQ_TERM_TEXT_YOFFSET= %i\n", SEQ_TERM_TEXT_YOFFSET);
#endif // DEBUG
}

void ED4_expose_recalculations() {
    ED4_ROOT->recalc_font_group();
    ED4_calc_terminal_extentions();

#if defined(DEBUG)
#warning below calculations have to be done at startup as well - maybe call expose_cb once after create_hierarchy.
#endif // DEBUG

    ED4_ROOT->ref_terminals.get_ref_sequence_info()->extension.size[HEIGHT] = TERMINALHEIGHT;
    ED4_ROOT->ref_terminals.get_ref_sequence()->extension.size[HEIGHT]      = TERMINALHEIGHT;
    ED4_ROOT->ref_terminals.get_ref_sequence_info()->extension.size[WIDTH]  = MAXINFOWIDTH;

    int screenwidth = ED4_ROOT->root_group_man->remap()->sequence_to_screen(MAXSEQUENCECHARACTERLENGTH);
    while (1) {
        ED4_ROOT->ref_terminals.get_ref_sequence()->extension.size[WIDTH] =
            ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES) *
            (screenwidth+3);

        ED4_terminal *top_middle_line_terminal = ED4_ROOT->main_manager->get_top_middle_line_terminal();

        ED4_ROOT->main_manager->get_top_middle_spacer_terminal()->extension.size[HEIGHT] = TERMINALHEIGHT - top_middle_line_terminal->extension.size[HEIGHT];

        ED4_ROOT->main_manager->route_down_hierarchy(NULL, NULL, &update_terminal_extension );
        ED4_ROOT->resize_all(); // may change mapping

        int new_screenwidth = ED4_ROOT->root_group_man->remap()->sequence_to_screen(MAXSEQUENCECHARACTERLENGTH);
        if (new_screenwidth == screenwidth) { // mapping did not change
            break;
        }
        screenwidth = new_screenwidth;
    }

    ED4_ROOT->get_ed4w()->update_scrolled_rectangle();
}

void ED4_expose_cb( AW_window *aww, AW_CL cd1, AW_CL cd2 )
{
    static bool dummy = 0;

    AWUSE(aww);
    AWUSE(cd1);
    AWUSE(cd2);

    ED4_ROOT->use_window(aww);

    GB_push_transaction(GLOBAL_gb_main);

    if (!dummy) {
        dummy = 1;
    }
    else {
        ED4_expose_recalculations(); // this case is needed every time, except the first
    }

    ED4_ROOT->get_ed4w()->update_window_coords();

    ED4_ROOT->get_device()->reset();
    ED4_ROOT->refresh_window(1);

    GB_pop_transaction(GLOBAL_gb_main);
}

void ED4_resize_cb( AW_window *aww, AW_CL cd1, AW_CL cd2 )
{
    AWUSE(aww);
    AWUSE(cd1);
    AWUSE(cd2);

    ED4_ROOT->use_window(aww);

    GB_push_transaction(GLOBAL_gb_main);

    ED4_ROOT->get_device()->reset();
    //  ED4_ROOT->deselect_all();

    ED4_ROOT->get_ed4w()->update_scrolled_rectangle();

    ED4_ROOT->get_ed4w()->slider_pos_horizontal= aww->slider_pos_horizontal;
    ED4_ROOT->get_ed4w()->slider_pos_vertical  = aww->slider_pos_vertical;

    GB_pop_transaction(GLOBAL_gb_main);
}

ED4_returncode call_edit( void **error, void **work_info_ptr, ED4_base *object )    // called after editing consensus to edit single sequences
{
    ED4_work_info *work_info = (ED4_work_info*)(*work_info_ptr);
    ED4_work_info new_work_info;
    ED4_base    *species_manager = NULL;
//     ED4_manager  *temp_parent;
    GB_TYPES    gb_type;

    if (((char **) error)[0] || !object->is_terminal())
        return ED4_R_BREAK;

    if (object->get_species_pointer()) {
        gb_type = GB_read_type(object->get_species_pointer());
    }
    else {
        gb_type = GB_NONE;
    }

    species_manager = object->get_parent( ED4_L_SPECIES );

    if (!species_manager || species_manager->flag.is_SAI) { // don't edit SAI's if editing the consensus
        return ED4_R_BREAK;
    }

    if ((object->dynamic_prop & ED4_P_CURSOR_ALLOWED) &&
        !species_manager->flag.is_consensus &&
        (object->dynamic_prop & ED4_P_ALIGNMENT_DATA)) // edit all aligned data - even if not consensus relevant
    {
        new_work_info.event            = work_info->event;
        new_work_info.char_position    = work_info->char_position;
        new_work_info.out_seq_position = work_info->out_seq_position;
        new_work_info.refresh_needed   = false;
        new_work_info.cursor_jump      = ED4_JUMP_KEEP_VISIBLE;
        new_work_info.out_string       = NULL;
        new_work_info.error            = NULL;
        new_work_info.mode             = work_info->mode;
        new_work_info.direction        = work_info->direction;
        new_work_info.cannot_handle    = false;
        new_work_info.is_sequence      = work_info->is_sequence;
        new_work_info.working_terminal = object->to_terminal();

        if (object->get_species_pointer()) {
            new_work_info.gb_data   = object->get_species_pointer();
            new_work_info.string    = NULL;
        }
        else {
            new_work_info.gb_data   = NULL;
            new_work_info.string    = object->id;
        }

        new_work_info.repeat_count = 1;

        ED4_ROOT->edit_string->init_edit(  );
        ED4_ROOT->edit_string->edit( &new_work_info );

//         if (ED4_ROOT->edit_string->old_seq)
//         {
//             temp_parent = species_manager->get_parent( ED4_L_MULTI_SPECIES )->to_manager();

//             ((ED4_terminal *)object)->actual_timestamp = GB_read_clock(gb_main);

//             char *seq = ED4_ROOT->edit_string->old_seq;
//             int seq_len = ED4_ROOT->edit_string->old_seq_len;

//             temp_parent->check_bases(seq, seq_len, species_manager);
//         }

        if (new_work_info.error) {
            ((char **) (error))[0] = new_work_info.error;
        }
        else if (new_work_info.out_string) {
            e4_assert(species_manager->flag.is_consensus);
            object->id = new_work_info.out_string;
        }

        if (new_work_info.refresh_needed) {
            object->set_refresh();
            object->parent->refresh_requested_by_child();

            if (object->is_sequence_terminal()) {
                ED4_sequence_terminal *seq_term = object->to_sequence_terminal();
                seq_term->results().searchAgain();
            }
        }

        if (move_cursor) {
            ED4_ROOT->get_ed4w()->cursor.jump_sequence_pos(ED4_ROOT->get_aww(), new_work_info.out_seq_position, ED4_JUMP_KEEP_VISIBLE);
            move_cursor = 0;
        }
    }

    return ED4_R_OK;
}

static void executeKeystroke(AW_window *aww, AW_event *event, int repeatCount) {
    ED4_work_info *work_info = new ED4_work_info;
    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;

    if (event->keycode==AW_KEY_NONE) return;

    e4_assert(repeatCount>0);

    if (!cursor->owner_of_cursor || cursor->owner_of_cursor->flag.hidden) {
        return;
    }

    if (event->keycode == AW_KEY_UP || event->keycode == AW_KEY_DOWN ||
        ((event->keymodifier & AW_KEYMODE_CONTROL) &&
         (event->keycode == AW_KEY_HOME || event->keycode == AW_KEY_END)))
    {
        GB_transaction dummy(GLOBAL_gb_main);
        while (repeatCount--) {
            cursor->move_cursor(event);
        }
        return;
    }

    work_info->cannot_handle    = false;
    work_info->event            = *event;
    work_info->char_position    = cursor->get_screen_pos();
    work_info->out_seq_position = cursor->get_sequence_pos();
    work_info->refresh_needed   = false;
    work_info->cursor_jump      = ED4_JUMP_KEEP_VISIBLE;
    work_info->out_string       = NULL; // nur falls new malloc
    work_info->error            = NULL;
    work_info->repeat_count     = repeatCount;

    ED4_terminal *terminal = cursor->owner_of_cursor->to_terminal();
    e4_assert(terminal->is_text_terminal());

    work_info->working_terminal = terminal;

    if (terminal->is_sequence_terminal()) {
        work_info->mode        = awar_edit_modus;
        work_info->direction   = awar_edit_direction;
        work_info->is_sequence = 1;
    }
    else {
        work_info->direction   = 1;
        work_info->is_sequence = 0;

        if (terminal->is_pure_text_terminal()) {
            work_info->mode = awar_edit_modus;  // AD_INSERT; // why is AD_INSERT forced here ?
        }
        else if (terminal->is_columnStat_terminal()) {
            work_info->mode = AD_NOWRITE;
        }
        else {
            e4_assert(0);
        }
    }

    work_info->string  = NULL;
    work_info->gb_data = NULL;

    if (terminal->get_species_pointer()) {
        work_info->gb_data = terminal->get_species_pointer();
    }
    else if (terminal->is_columnStat_terminal()) {
        work_info->gb_data = terminal->to_columnStat_terminal()->corresponding_sequence_terminal()->get_species_pointer();
    }
    else {
        work_info->string = terminal->id;
    }

    ED4_Edit_String     *edit_string     = new ED4_Edit_String;
    ED4_species_manager *species_manager = terminal->get_parent(ED4_L_SPECIES)->to_species_manager();
    char                *error           = NULL;

    GB_push_transaction(GLOBAL_gb_main);

    if (species_manager->flag.is_consensus) {
        ED4_group_manager *group_manager = terminal->get_parent(ED4_L_GROUP)->to_group_manager();

        work_info->string = terminal->id = group_manager->table().build_consensus_string();

        edit_string->edit(work_info);
        cursor->jump_sequence_pos(aww, work_info->out_seq_position, ED4_JUMP_KEEP_VISIBLE);

        work_info->string = 0;

        if (work_info->cannot_handle) {
            // group_manager = terminal->get_parent( ED4_L_GROUP )->to_group_manager();
            move_cursor = 1;
            if (!ED4_ROOT->edit_string) {
                ED4_ROOT->edit_string = new ED4_Edit_String();
            }
            group_manager->route_down_hierarchy((void **) &error, (void **) &work_info, call_edit);
            group_manager->rebuild_consensi( group_manager, ED4_U_UP_DOWN);
        }

        delete terminal->id;
        terminal->id = 0;
    }
    else {
        edit_string->edit( work_info );

        // ED4_ROOT->main_manager->Show(); // original version
        ED4_ROOT->main_manager->Show(1, 0); // temporary fix for worst-refresh problems
        cursor->jump_sequence_pos(aww, work_info->out_seq_position, work_info->cursor_jump);
    }

    edit_string->finish_edit();

    if (work_info->error || (error && *error == 1)) {
        // hier evtl. nochmal route_down_hierarchy aufrufen ?!!
        aw_message(work_info->error);
        GB_abort_transaction(GLOBAL_gb_main);
        ED4_ROOT->refresh_all_windows(0);
    }
    else {
        GB_pop_transaction( GLOBAL_gb_main );

        if (work_info->refresh_needed) {
            GB_transaction dummy(GLOBAL_gb_main);

            terminal->set_refresh();
            terminal->parent->refresh_requested_by_child();
            if (terminal->is_sequence_terminal()) {
                ED4_sequence_terminal *seq_term = terminal->to_sequence_terminal();
                seq_term->results().searchAgain();
            }
            ED4_ROOT->refresh_all_windows(0);
        }
    }

    delete edit_string;
    delete work_info;
}

void ED4_remote_event(AW_event *faked_event) { // keystrokes forwarded from SECEDIT
    executeKeystroke(ED4_ROOT->get_aww(), faked_event, 1);
}

void ED4_input_cb(AW_window *aww,AW_CL /*cd1*/, AW_CL /*cd2*/)
{
    AW_event event;
    static AW_event lastEvent;
    static int repeatCount;

    ED4_ROOT->use_window(aww);

    aww->get_event(&event);

#if defined(DEBUG) && 0
    printf("event.type=%i event.keycode=%i event.character='%c' event.keymodifier=%i\n", event.type, event.keycode, event.character, event.keymodifier);
#endif

    switch(event.type) {
        case AW_Keyboard_Press: {
            if (repeatCount==0) { // first key event?
                lastEvent = event;
                repeatCount = 1;
            }
            else {
                if (lastEvent.keycode==event.keycode &&
                    lastEvent.character==event.character &&
                    lastEvent.keymodifier==event.keymodifier) { // same key as last?
                    repeatCount++;
                }
                else { // other key => execute now
                    executeKeystroke(aww, &lastEvent, repeatCount);
                    lastEvent = event;
                    repeatCount = 1;
                }
            }

            if (repeatCount) {
#if defined(DARWIN) || 1
                // sth goes wrong with OSX -> always execute keystroke
                // Xfree 4.3 has problems as well, so repeat counting is disabled completely
                executeKeystroke(aww, &lastEvent, repeatCount);
                repeatCount                       = 0;
#else
                AW_ProcessEventType nextEventType = ED4_ROOT->aw_root->peek_key_event(aww);

                if (nextEventType!=KEY_RELEASED) { // no key waiting => execute now
                    executeKeystroke(aww, &lastEvent, repeatCount);
                    repeatCount = 0;
                }
#endif
            }
            break;
        }
        case AW_Keyboard_Release: {
            AW_ProcessEventType nextEventType = ED4_ROOT->aw_root->peek_key_event(aww);

            if (nextEventType!=KEY_PRESSED && repeatCount) { // no key follows => execute keystrokes (if any)
                executeKeystroke(aww, &lastEvent, repeatCount);
                repeatCount = 0;
            }

            break;
        }
        default: {
            if (event.type == AW_Mouse_Release && event.button == ED4_B_MIDDLE_BUTTON) {
                ED4_ROOT->scroll_picture.scroll = 0;
                return;
            }
            else if (event.type == AW_Mouse_Press && event.button == ED4_B_MIDDLE_BUTTON) {
                ED4_ROOT->scroll_picture.scroll = 1;
                ED4_ROOT->scroll_picture.old_y = event.y;
                ED4_ROOT->scroll_picture.old_x = event.x;
                return;
            }

#if defined(DEBUG) && 0
            if (event.button==ED4_B_LEFT_BUTTON) {
                printf("[ED4_input_cb]  type=%i x=%i y=%i ", (int)event.type, (int)event.x, (int)event.y);
            }
#endif

            AW_pos win_x = event.x;
            AW_pos win_y = event.y;
            ED4_ROOT->win_to_world_coords( aww, &(win_x), &(win_y) );
            event.x = (int) win_x;
            event.y = (int) win_y;

#if defined(DEBUG) && 0
            if (event.button==ED4_B_LEFT_BUTTON) {
                printf("-> x=%i y=%i\n", (int)event.type, (int)event.x, (int)event.y);
            }
#endif

            GB_transaction dummy(GLOBAL_gb_main);
            ED4_ROOT->main_manager->event_sent_by_parent( &event, aww );
            break;
        }
    }
}

static int get_max_slider_xpos() {
    AW_rectangle rect;
    ED4_ROOT->get_device()->get_area_size(&rect); // screensize

    AW_pos x, y;
    ED4_base *horizontal_link = ED4_ROOT->scroll_links.link_for_hor_slider;
    horizontal_link->calc_world_coords(&x, &y);

    AW_pos max_xpos = horizontal_link->extension.size[WIDTH] // overall width of virtual scrolling area
        - (rect.r - x); // minus width of visible scroll-area (== relative width of horizontal scrollbar)

    if (max_xpos<0) max_xpos = 0; // happens when window-content is smaller then window (e.g. if ARB_EDIT4 is not filled)
    return int(max_xpos+0.5);
}

static int get_max_slider_ypos() {
    AW_rectangle rect;
    ED4_ROOT->get_device()->get_area_size(&rect); // screensize

    AW_pos x, y;
    ED4_base *vertical_link = ED4_ROOT->scroll_links.link_for_ver_slider;
    vertical_link->calc_world_coords(&x, &y);

    AW_pos max_ypos = vertical_link->extension.size[HEIGHT] // overall width of virtual scrolling area
        - (rect.b - y); // minus width of visible scroll-area (== relative width of horizontal scrollbar)

    if (max_ypos<0) max_ypos = 0; // happens when window-content is smaller then window (e.g. if ARB_EDIT4 is not filled)
    return int(max_ypos+0.5);
}

void ED4_vertical_change_cb( AW_window *aww, AW_CL cd1, AW_CL cd2 )
{
    AWUSE(cd1);
    AWUSE(cd2);

    ED4_ROOT->use_window(aww);

    GB_push_transaction(GLOBAL_gb_main);

    ED4_window *win = ED4_ROOT->get_ed4w();
    int old_slider_pos = win->slider_pos_vertical;

    { // correct slider_pos if neccessary
        int max_slider_ypos = get_max_slider_ypos();

        if (aww->slider_pos_vertical>max_slider_ypos) aww->set_vertical_scrollbar_position(max_slider_ypos);
        if (aww->slider_pos_vertical<0)               aww->set_vertical_scrollbar_position(0);
    }

    int slider_diff = aww->slider_pos_vertical - old_slider_pos;

    win->coords.window_upper_clip_point += slider_diff;
    win->coords.window_lower_clip_point += slider_diff;

    win->scroll_rectangle(0, -slider_diff);
    win->slider_pos_vertical = aww->slider_pos_vertical;

    GB_pop_transaction(GLOBAL_gb_main);
    win->update_window_coords();
}

void ED4_horizontal_change_cb( AW_window *aww, AW_CL cd1, AW_CL cd2 )
{
    AWUSE(cd1);
    AWUSE(cd2);

    ED4_ROOT->use_window(aww);

    GB_push_transaction(GLOBAL_gb_main);

    ED4_window *win = ED4_ROOT->get_ed4w();
    int old_slider_pos = win->slider_pos_horizontal;

    { // correct slider_pos if neccessary
        int max_slider_xpos = get_max_slider_xpos();

        if (aww->slider_pos_horizontal>max_slider_xpos) aww->set_horizontal_scrollbar_position(max_slider_xpos);
        if (aww->slider_pos_horizontal<0)               aww->set_horizontal_scrollbar_position(0);
    }

    int slider_diff = aww->slider_pos_horizontal - old_slider_pos;

    win->coords.window_left_clip_point  += slider_diff;
    win->coords.window_right_clip_point += slider_diff;

    win->scroll_rectangle(-slider_diff, 0);
    win->slider_pos_horizontal = aww->slider_pos_horizontal;

    GB_pop_transaction(GLOBAL_gb_main);
    win->update_window_coords();
}

void ED4_scrollbar_change_cb(AW_window *aww, AW_CL cd1, AW_CL cd2)
{
    AWUSE(cd1);
    AWUSE(cd2);

    ED4_ROOT->use_window(aww);

    GB_push_transaction(GLOBAL_gb_main);

    ED4_window *win = ED4_ROOT->get_ed4w();

    int old_hslider_pos = win->slider_pos_horizontal;
    int old_vslider_pos = win->slider_pos_vertical;

    { // correct slider_pos if neccessary
        int max_slider_xpos = get_max_slider_xpos();
        int max_slider_ypos = get_max_slider_ypos();

        if (aww->slider_pos_horizontal>max_slider_xpos) aww->set_horizontal_scrollbar_position(max_slider_xpos);
        if (aww->slider_pos_horizontal<0)               aww->set_horizontal_scrollbar_position(0);

        if (aww->slider_pos_vertical>max_slider_ypos) aww->set_vertical_scrollbar_position(max_slider_ypos);
        if (aww->slider_pos_vertical<0)               aww->set_vertical_scrollbar_position(0);
    }

    int slider_hdiff = aww->slider_pos_horizontal - old_hslider_pos;
    int slider_vdiff = aww->slider_pos_vertical   - old_vslider_pos;

    ED4_coords *coords = &win->coords;
    coords->window_left_clip_point  += slider_hdiff;
    coords->window_right_clip_point += slider_hdiff;
    coords->window_upper_clip_point += slider_vdiff;
    coords->window_lower_clip_point += slider_vdiff;

    win->scroll_rectangle( -slider_hdiff, -slider_vdiff);

    win->slider_pos_vertical   = aww->slider_pos_vertical;
    win->slider_pos_horizontal = aww->slider_pos_horizontal;

    GB_pop_transaction(GLOBAL_gb_main);
    win->update_window_coords();
    //    paintMarksOfSelectedObjects();
}

void ED4_motion_cb( AW_window *aww, AW_CL cd1, AW_CL cd2 )
{
    AWUSE(cd1);
    AWUSE(cd2);
    AW_event event;

    ED4_ROOT->use_window(aww);

    aww->get_event(&event);

    if (event.type == AW_Mouse_Drag && event.button == ED4_B_MIDDLE_BUTTON) {
        if (ED4_ROOT->scroll_picture.scroll) {
            int xdiff    = ED4_ROOT->scroll_picture.old_x - event.x;
            int ydiff    = ED4_ROOT->scroll_picture.old_y - event.y;
            int new_xpos = aww->slider_pos_horizontal + (xdiff*ED4_ROOT->aw_root->awar(ED4_AWAR_SCROLL_SPEED_X)->read_int())/10;
            int new_ypos = aww->slider_pos_vertical   + (ydiff*ED4_ROOT->aw_root->awar(ED4_AWAR_SCROLL_SPEED_Y)->read_int())/10;

            if (xdiff<0) { // scroll left
                if (new_xpos<0) new_xpos = 0;
            }
            else if (xdiff>0) { // scroll right
                int max_xpos = get_max_slider_xpos();
                if (max_xpos<0) max_xpos = 0;
                if (new_xpos>max_xpos) new_xpos = max_xpos;
            }

            if (ydiff<0) { // scroll left
                if (new_ypos<0) new_ypos = 0;
            }
            else if (ydiff>0) { // scroll right
                int max_ypos = get_max_slider_ypos();
                if (max_ypos<0) max_ypos = 0;
                if (new_ypos>max_ypos) new_ypos = max_ypos;
            }

            if (new_xpos!=aww->slider_pos_horizontal) {
                aww->set_horizontal_scrollbar_position(new_xpos);
                ED4_horizontal_change_cb(aww, cd1, cd2 );
                ED4_ROOT->scroll_picture.old_x = event.x;
            }

            if (new_ypos!=aww->slider_pos_vertical) {
                aww->set_vertical_scrollbar_position(new_ypos);
                ED4_vertical_change_cb(aww, cd1, cd2 );
                ED4_ROOT->scroll_picture.old_y = event.y;
            }
        }
    }
    else {

#if defined(DEBUG) && 0
        if (event.button==ED4_B_LEFT_BUTTON) {
            printf("[ED4_motion_cb] type=%i x=%i y=%i ", (int)event.type, (int)event.x, (int)event.y);
        }
#endif

        AW_pos win_x = event.x;
        AW_pos win_y = event.y;
        ED4_ROOT->win_to_world_coords( aww, &win_x, &win_y);
        event.x = (int) win_x;
        event.y = (int) win_y;

#if defined(DEBUG) && 0
        if (event.button==ED4_B_LEFT_BUTTON) {
            printf("-> x=%i y=%i\n", (int)event.type, (int)event.x, (int)event.y);
        }
#endif

        GB_transaction dummy(GLOBAL_gb_main);
        ED4_ROOT->main_manager->event_sent_by_parent( &event, aww );
    }
}

void ED4_remote_set_cursor_cb(AW_root *awr, AW_CL /*cd1*/, AW_CL /*cd2*/)
{
    AW_awar *awar = awr->awar(AWAR_SET_CURSOR_POSITION);
    long     pos  = awar->read_int();

    if (pos != -1) {
        ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
        cursor->jump_sequence_pos(ED4_ROOT->get_aww(), pos, ED4_JUMP_CENTERED);
        awar->write_int(-1);
    }
}

void ED4_jump_to_cursor_position(AW_window *aww, char *awar_name, bool /*callback_flag*/)           // callback function
{
    ED4_ROOT->use_window(aww);

    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
    GB_ERROR    error  = 0;

    long pos = aww->get_root()->awar(awar_name)->read_int();
    if (pos>0) pos--;                               // user->real position (userpos is 1..n)

    ED4_remap *remap  = ED4_ROOT->root_group_man->remap();
    long       max    = remap->screen_to_sequence(remap->get_max_screen_pos());
    if(pos > max) pos = max;

    if (strcmp(awar_name, ED4_ROOT->get_ed4w()->awar_path_for_Ecoli)==0) { // callback from ecoli
        BI_ecoli_ref *ecoli = ED4_ROOT->ecoli_ref;

        if (ecoli->gotData()) pos = ecoli->rel_2_abs(pos);
        else error = "No ecoli reference";
    }
    else if (strcmp(awar_name, ED4_ROOT->get_ed4w()->awar_path_for_basePos)==0) { // callback from basePos
        pos = cursor->base2sequence_position(pos);
    }

    if (error) {
        aw_message(error);
    }
    else {
        cursor->jump_sequence_pos(aww, pos, ED4_JUMP_CENTERED);
    }
}

void ED4_set_helixnr(AW_window *aww, char *awar_name, bool /*callback_flag*/)
{
    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;

    if (cursor->owner_of_cursor) {
        AW_root    *root     = aww->get_root();
        const char *helix_nr = root->awar(awar_name)->read_string();
        BI_helix   *helix    = ED4_ROOT->helix;

        if (helix->has_entries()) {
            long pos = helix->first_position(helix_nr);

            if (pos == -1) {
                aw_message(GBS_global_string("No helix '%s' found", helix_nr));
            }
            else {
                cursor->jump_sequence_pos(aww, pos, ED4_JUMP_CENTERED);
            }
        }
        else {
            aw_message("Got no helix information");
        }
    }
}

void ED4_set_iupac(AW_window */*aww*/, char *awar_name, bool /*callback_flag*/)
{
    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;

    if (cursor->owner_of_cursor) {
        ED4_species_manager *species_manager =  cursor->owner_of_cursor->get_parent(ED4_L_SPECIES)->to_species_manager();

        if (species_manager->flag.is_consensus) {
            aw_message("You cannot change the consensus");
            return;
        }
        int len;
        char *seq = cursor->owner_of_cursor->resolve_pointer_to_string_copy(&len);
        int seq_pos = cursor->get_sequence_pos();

        e4_assert(seq);

        if (seq_pos<len) {
            const char *iupac = ED4_ROOT->aw_root->awar(awar_name)->read_string();
            char new_char = ED4_encode_iupac(iupac, ED4_ROOT->alignment_type);

            seq[seq_pos] = new_char;
            cursor->owner_of_cursor->write_sequence(seq, len);
        }

        free(seq);
    }
}

void ED4_gc_is_modified(AW_window *aww,AW_CL cd1, AW_CL cd2)                        // callback if gc is modified
{
    ED4_ROOT->use_window(aww);

    ED4_resize_cb(aww,cd1,cd2);
    ED4_expose_cb(aww,cd1,cd2);
}

void ED4_exit() {
    ED4_ROOT->aw_root->unlink_awars_from_DB(GLOBAL_gb_main);

    ED4_window *ed4w = ED4_ROOT->first_window;

    while (ed4w) {
        ed4w->aww->hide();
        ed4w->cursor.invalidate_base_position();
        ed4w = ed4w->next;
    }

    delete ED4_ROOT->main_manager;

    while (ED4_ROOT->first_window)
        ED4_ROOT->first_window->delete_window(ED4_ROOT->first_window);

    GBDATA *gb_main = GLOBAL_gb_main;
    GLOBAL_gb_main  = NULL;
#if defined(DEBUG)
    AWT_browser_forget_db(gb_main);
#endif // DEBUG
    GB_close(gb_main);

    ::exit(0);
}

void ED4_quit_editor(AW_window *aww, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    ED4_ROOT->use_window(aww);

    if (ED4_ROOT->first_window == ED4_ROOT->get_ed4w()) { // quit button has been pressed in first window
        ED4_exit();
    }
    // case : in another window close has been pressed
    ED4_ROOT->get_aww()->hide();
    ED4_ROOT->get_ed4w()->is_hidden = 1;
}

void ED4_load_data( AW_window *aww, AW_CL cd1, AW_CL cd2 )
{
    ED4_ROOT->use_window(aww);

    AWUSE(aww);
    AWUSE(cd1);
    AWUSE(cd2);
}

void ED4_save_data( AW_window *aww, AW_CL cd1, AW_CL cd2 )
{
    ED4_ROOT->use_window(aww);

    AWUSE(aww);
    AWUSE(cd1);
    AWUSE(cd2);
}

void ED4_timer_refresh()
{
    GB_begin_transaction(GLOBAL_gb_main);                  // for callbacks from database
    //    ED4_ROOT->refresh_all_windows(0);
    GB_tell_server_dont_wait(GLOBAL_gb_main);
    GB_commit_transaction(GLOBAL_gb_main);
}

void ED4_timer(AW_root *, AW_CL cd1, AW_CL cd2 )
{
    //    last_used_timestamp = GB_read_clock(gb_main);
    ED4_timer_refresh();
    ED4_ROOT->aw_root->add_timed_callback(200,ED4_timer,cd1,cd2);
}

void ED4_refresh_window( AW_window *aww, AW_CL cd_called_from_menu, AW_CL /*cd2*/ )
{
    GB_transaction dummy(GLOBAL_gb_main);

    if (int(cd_called_from_menu)) {
        ED4_ROOT->use_window(aww);
    }

    ED4_main_manager *mainman = ED4_ROOT->main_manager;
    if (mainman) { // avoids a crash durin startup
        if (mainman->update_info.delete_requested) {
            mainman->delete_requested_childs();
        }

        //    ED4_ROOT->deselect_all();
        mainman->update_info.set_clear_at_refresh(1);
        mainman->Show(1);
    }
}

void ED4_set_reference_species( AW_window *aww, AW_CL disable, AW_CL cd2 ){
    GB_transaction dummy(GLOBAL_gb_main);

    if (disable) {
        ED4_ROOT->reference->init();
    }
    else {
        ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;

        if (cursor->owner_of_cursor) {
            ED4_terminal *terminal = cursor->owner_of_cursor->to_terminal();
            ED4_manager *manager = terminal->parent->parent->to_manager();

            if (manager->flag.is_consensus) {
                ED4_char_table *table = &terminal->get_parent(ED4_L_GROUP)->to_group_manager()->table();
                char *consensus = table->build_consensus_string();

                ED4_ROOT->reference->init("CONSENSUS", consensus, table->size());
            }
            else if (manager->parent->flag.is_SAI) {
                char *name = GBT_read_string(GLOBAL_gb_main,AWAR_SPECIES_NAME);
                int   datalen;
                char *data = terminal->resolve_pointer_to_string_copy(&datalen);

                ED4_ROOT->reference->init(name, data, datalen);

                free(data);
                free(name);
            }
            else {
                char *name = GBT_read_string(GLOBAL_gb_main,AWAR_SPECIES_NAME);

                ED4_ROOT->reference->init(name, ED4_ROOT->alignment_name);
                delete name;
            }
        }
        else {
            aw_message("First you have to place your cursor");
        }
    }

    ED4_refresh_window(aww,0,cd2);
}

static void show_detailed_column_stats_activated(AW_window *aww) {
    ED4_ROOT->column_stat_initialized = 1;
    ED4_show_detailed_column_stats(aww, 0, 0);
}

double ED4_columnStat_terminal::threshold = -1;
int ED4_columnStat_terminal::threshold_is_set() {
    return threshold>=0 && threshold<=100;
}
void ED4_columnStat_terminal::set_threshold(double aThreshold) {
    threshold = aThreshold;
    e4_assert(threshold_is_set());
}

void ED4_set_col_stat_threshold(AW_window */*aww*/, AW_CL cl_do_refresh, AW_CL) {
    double default_threshold = 90.0;
    if (ED4_columnStat_terminal::threshold_is_set()) {
        default_threshold = ED4_columnStat_terminal::get_threshold();
    }
    char default_input[40];
    sprintf(default_input, "%6.2f", default_threshold);

    char *input = aw_input("Please insert threshold value for marking:", default_input);
    if (input) {
        double input_threshold = atof(input);

        if (input_threshold<0 || input_threshold>100) {
            aw_message("Illegal threshold value (allowed: 0..100)");
        }
        else {
            ED4_columnStat_terminal::set_threshold(input_threshold);
            if (int(cl_do_refresh)) {
                ED4_ROOT->refresh_all_windows(1);
            }
        }
        free(input);
    }
}

void ED4_show_detailed_column_stats(AW_window *aww, AW_CL, AW_CL)
{
    while (!ED4_columnStat_terminal::threshold_is_set()) {
        ED4_set_col_stat_threshold(aww, 0, 0);
    }

    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
    if (!cursor->owner_of_cursor) {
        aw_message("First you have to place your cursor");
        return;
    }

    ED4_terminal *cursor_terminal = cursor->owner_of_cursor->to_terminal();
    ED4_manager *cursor_manager = cursor_terminal->parent->parent->to_manager();
    if (!cursor_terminal->is_sequence_terminal() || cursor_manager->flag.is_consensus || cursor_manager->parent->flag.is_SAI) {
        aw_message("Display of column-statistic-details is only possible for species!");
        return;
    }

    ED4_sequence_terminal *seq_term = cursor_terminal->to_sequence_terminal();
    if (!seq_term->st_ml_node && !(seq_term->st_ml_node=st_ml_convert_species_name_to_node(ED4_ROOT->st_ml, seq_term->species_name))) {
        if (ED4_ROOT->column_stat_initialized) {
            aw_message("Cannot display column statistics for this species (internal error?)");
            return;
        }
        AW_window *aww_st = st_create_main_window(ED4_ROOT->aw_root,ED4_ROOT->st_ml,(AW_CB0)show_detailed_column_stats_activated,(AW_window *)aww);
        aww_st->show();
        return;
    }

    ED4_multi_sequence_manager *multi_seq_man = cursor_manager->to_multi_sequence_manager();
    char buffer[35];
    int count = 1;
    sprintf( buffer, "Sequence_Manager.%ld.%d", ED4_counter, count++);

    ED4_sequence_manager *new_seq_man = new ED4_sequence_manager(buffer, 0, 0, 0, 0, multi_seq_man);
    new_seq_man->set_properties(ED4_P_MOVABLE);
    multi_seq_man->children->append_member(new_seq_man);

    int pixel_length = max_seq_terminal_length;
#if defined(DEBUG) && 1
    printf("max_seq_terminal_length=%li\n", max_seq_terminal_length);
#endif

    AW_pos font_height = ED4_ROOT->font_group.get_height(ED4_G_SEQUENCES);
    AW_pos columnStatHeight = ceil((COLUMN_STAT_ROWS+0.5/* reserve a bit more space*/)*COLUMN_STAT_ROW_HEIGHT(font_height));
    ED4_columnStat_terminal *ref_colStat_terminal = ED4_ROOT->ref_terminals.get_ref_column_stat();
    ref_colStat_terminal->extension.size[HEIGHT] = columnStatHeight;
    ref_colStat_terminal->extension.size[WIDTH] = pixel_length;

    ED4_sequence_info_terminal *ref_colStat_info_terminal = ED4_ROOT->ref_terminals.get_ref_column_stat_info();
    ED4_sequence_info_terminal *new_colStat_info_term = new ED4_sequence_info_terminal("CStat", /*0,*/ 0, 0, SEQUENCEINFOSIZE, columnStatHeight, new_seq_man);
    new_colStat_info_term->set_properties( (ED4_properties) (ED4_P_SELECTABLE | ED4_P_DRAGABLE | ED4_P_IS_HANDLE) );
    new_colStat_info_term->set_links(ref_colStat_info_terminal, ref_colStat_terminal);
    new_seq_man->children->append_member(new_colStat_info_term);

    sprintf( buffer, "Column_Statistic_Terminal.%ld.%d", ED4_counter, count++);
    ED4_columnStat_terminal *new_colStat_term = new ED4_columnStat_terminal(buffer, SEQUENCEINFOSIZE, 0, 0, columnStatHeight, new_seq_man);
    new_colStat_term->set_properties(ED4_P_CURSOR_ALLOWED);
    new_colStat_term->set_links(ref_colStat_terminal, ref_colStat_terminal);
    //    new_colStat_term->extension.size[WIDTH] = pixel_length;
    new_seq_man->children->append_member(new_colStat_term);

    ED4_counter++;

    new_seq_man->resize_requested_by_child();
    ED4_ROOT->refresh_all_windows(0);
}

ED4_returncode update_terminal_extension( void **/*arg1*/, void **/*arg2*/, ED4_base *this_object )
{
    if (this_object->is_terminal()){
        if (this_object->is_spacer_terminal()){
            if (this_object->parent->is_device_manager()) { // the rest is managed by reference links
                ;
                //      this_object->extension.size[HEIGHT] = TERMINALHEIGHT / 2;   // @@@ Zeilenabstand verringern hier?
            }
        }
        else if (this_object->is_species_name_terminal()) {
            this_object->extension.size[WIDTH] = MAXSPECIESWIDTH - BRACKETWIDTH * this_object->calc_group_depth();
        }
        else if (this_object->is_sequence_info_terminal()) {
            this_object->extension.size[WIDTH] = MAXINFOWIDTH;
        }
        else if (this_object->is_line_terminal()) { // thought for line terminals which are direct children of the device manager
            this_object->extension.size[WIDTH] =
                TREETERMINALSIZE + MAXSPECIESWIDTH +
                ED4_ROOT->ref_terminals.get_ref_sequence_info()->extension.size[WIDTH] +
                ED4_ROOT->ref_terminals.get_ref_sequence()->extension.size[WIDTH];
        }
    }

    if (this_object->parent) {
        this_object->parent->resize_requested_by_child();
    }

    return ED4_R_OK;
}

#define SIGNIFICANT_FIELD_CHARS 30 // length used to compare field contents (in createGroupFromSelected)

static void createGroupFromSelected(GB_CSTR group_name, GB_CSTR field_name, GB_CSTR field_content)
    // creates a new group named group_name
    // if field_name==0 -> all selected species & subgroups are moved to this new group
    // if field_name!=0 -> all selected species containing field_content in field field_name are moved to this new group
{
    ED4_group_manager *group_manager = NULL;
    ED4_ROOT->main_manager->create_group(&group_manager, group_name);
    ED4_manager *top_area = ED4_ROOT->main_manager->search_spec_child_rek(ED4_L_AREA)->to_manager();
    ED4_multi_species_manager *multi_species_manager = top_area->search_spec_child_rek( ED4_L_MULTI_SPECIES )->to_multi_species_manager();

    group_manager->extension.position[Y_POS] = 2;
    ED4_base::touch_world_cache();
    multi_species_manager->children->append_member( group_manager );
    group_manager->parent = (ED4_manager *) multi_species_manager;

    multi_species_manager = group_manager->get_defined_level( ED4_L_MULTI_SPECIES )->to_multi_species_manager();

    ED4_list_elem *list_elem = ED4_ROOT->selected_objects.first();
    while (list_elem) {
        ED4_base *object = ((ED4_selection_entry *) list_elem->elem())->object;
        object = object->get_parent( ED4_L_SPECIES );
        int move_object = 1;

        if (object->flag.is_consensus) {
            object = object->get_parent( ED4_L_GROUP );
            if (field_name) move_object = 0; // don't move groups if moving by field_name
        }
        else {
            e4_assert(object->is_species_manager());
            if (field_name) {
                GBDATA *gb_species = object->get_species_pointer();
                GBDATA *gb_field = GB_search(gb_species, field_name, GB_FIND);

                if (gb_field) { // field was found
                    int type = gb_field->flags.type;
                    if (type==GB_STRING) {
                        char *found_content = GB_read_as_string(gb_field);
                        move_object = strncmp(found_content, field_content, SIGNIFICANT_FIELD_CHARS)==0;
                        free(found_content);
                    }
                    else {
                        e4_assert(0); // field has to be string field
                    }
                }
                else { // field was NOT found
                    move_object = field_content==0 || field_content[0]==0; // move object if we search for no content
                }
            }
        }

        if (move_object) {
            object->parent->children->delete_member(object);

            ED4_base *base = object->get_parent(ED4_L_MULTI_SPECIES);
            if (base && base->is_multi_species_manager()) {
                ED4_multi_species_manager *old_multi = base->to_multi_species_manager();
                old_multi->invalidate_species_counters();
            }
            multi_species_manager->children->append_member( object );

            object->parent = ( ED4_manager * )multi_species_manager;
            object->set_width();
        }

        list_elem = list_elem->next();
    }

    group_manager->create_consensus(group_manager);
    group_manager->get_defined_level(ED4_L_MULTI_SPECIES)->to_multi_species_manager()->count_all_children_and_set_group_id();

    {
        ED4_bracket_terminal *bracket = group_manager->get_defined_level(ED4_L_BRACKET)->to_bracket_terminal();

        if (bracket) {
            group_manager->fold_group(bracket->id);
        }
    }

    multi_species_manager->resize_requested_by_child();

    ED4_ROOT->get_ed4w()->update_scrolled_rectangle();
    ED4_ROOT->get_device()->reset();
    ED4_ROOT->refresh_all_windows(1);
}

static void group_species(int use_field, AW_window *use_as_main_window) {
    GB_ERROR error = 0;
    GB_push_transaction(GLOBAL_gb_main);

    ED4_ROOT->use_window(use_as_main_window);

    if (!use_field) {
        char *group_name = aw_input("Enter name for new group:");

        if (group_name) {
            if (strlen(group_name)>GB_GROUP_NAME_MAX) {
                group_name[GB_GROUP_NAME_MAX] = 0;
                aw_message("Truncated too long group name");
            }
            createGroupFromSelected(group_name, 0, 0);
            free(group_name);
        }
    }
    else {
        const char *field_name   = ED4_ROOT->aw_root->awar(AWAR_FIELD_CHOSEN)->read_string();
        char       *doneContents = strdup(";");
        size_t      doneLen      = 1;

        int tryAgain     = 1;
        int foundField   = 0;
        int foundSpecies = 0;

        while (tryAgain && !error) {
            tryAgain = 0;
            ED4_list_elem *list_elem = ED4_ROOT->selected_objects.first();
            while (list_elem && !error) {
                ED4_base *object = ((ED4_selection_entry *) list_elem->elem())->object;
                object = object->get_parent( ED4_L_SPECIES );
                if (!object->flag.is_consensus) {
                    GBDATA *gb_species = object->get_species_pointer();
                    GBDATA *gb_field = GB_search(gb_species, field_name, GB_FIND);

                    if (gb_species) {
                        foundSpecies = 1;
                    }

                    if (gb_field) {
                        int type = gb_field->flags.type;

                        if (type==GB_STRING) {
                            char   *field_content     = GB_read_as_string(gb_field);
                            size_t  field_content_len = strlen(field_content);

                            foundField = 1;
                            if (field_content_len>SIGNIFICANT_FIELD_CHARS) {
                                field_content[SIGNIFICANT_FIELD_CHARS] = 0;
                                field_content_len                      = SIGNIFICANT_FIELD_CHARS;
                            }

                            char with_semi[SIGNIFICANT_FIELD_CHARS+2+1];
                            sprintf(with_semi, ";%s;", field_content);

                            if (strstr(doneContents, with_semi)==0) { // field_content was not used yet
                                createGroupFromSelected(field_content, field_name, field_content);
                                tryAgain = 1;

                                int newlen = doneLen + field_content_len + 1;
                                char *newDone = (char*)malloc(newlen+1);
                                GBS_global_string_to_buffer(newDone, newlen, "%s%s;", doneContents, field_content);
                                freeset(doneContents, newDone);
                                doneLen = newlen;
                            }
                            free(field_content);
                        }
                        else {
                            error = "You have to use a string type field";
                        }
                    }
                }
                list_elem = list_elem->next();
            }
        }

        if (!foundSpecies) {
            error = "Please select some species in order to insert them into new groups";
        }
        else if (!foundField) {
            error = GB_export_errorf("Field not found: '%s'", field_name);
        }
    }

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
}

static void group_species2_cb(AW_window*, AW_CL cl_use_as_main_window, AW_CL cl_window_to_hide) {
    group_species(1, (AW_window*)cl_use_as_main_window);
    ((AW_window*)cl_window_to_hide)->hide();
}

static AW_window *create_group_species_by_field_window(AW_root *aw_root, AW_window *use_as_main_window) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "CREATE_GROUP_USING_FIELD", "Create group using field");
    aws->load_xfig("edit4/choose_field.fig");

    aws->button_length(20);
    aws->at("doit");
    aws->callback(group_species2_cb, (AW_CL)use_as_main_window, (AW_CL)aws);
    aws->create_button("USE_FIELD","Use selected field","");

    aws->button_length(10);
    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    awt_create_selection_list_on_scandb(GLOBAL_gb_main, (AW_window*)aws, AWAR_FIELD_CHOSEN, -1, "source",0, &AWT_species_selector, 20, 10);

    return aws;
}

void group_species_cb(AW_window *aww, AW_CL cl_use_fields, AW_CL) {

    int use_fields = int(cl_use_fields);

    if (!use_fields) {
        group_species(0, aww);
    }
    else {
        static AW_window *ask_field_window;

        if (!ask_field_window) ask_field_window = create_group_species_by_field_window(ED4_ROOT->aw_root, aww);
        ask_field_window->activate();
    }
}

void ED4_load_new_config(char *string)
{
    char *config_data_top    = NULL;
    char *config_data_middle = NULL;

    ED4_window     *window;
    GB_transaction  dummy(GLOBAL_gb_main);


    ED4_ROOT->main_manager->clear_whole_background();
    ED4_calc_terminal_extentions();

    all_found = 0;
    species_read = 0;
    max_seq_terminal_length = 0;
    not_found_message = GBS_stropen(1000);
    GBS_strcat(not_found_message,"Some species not found:\n");


    if ( ED4_ROOT->selected_objects.no_of_entries() > 0 ) {
        ED4_ROOT->deselect_all();
    }

    ED4_ROOT->scroll_picture.scroll         = 0;
    ED4_ROOT->scroll_picture.old_x          = 0;
    ED4_ROOT->scroll_picture.old_y          = 0;

    ED4_ROOT->ref_terminals.clear();

    for (window=ED4_ROOT->first_window; window; window=window->next) {
        window->cursor.init();
        window->aww->set_horizontal_scrollbar_position ( 0 );
        window->aww->set_vertical_scrollbar_position ( 0 );
    }

    ED4_ROOT->scroll_links.link_for_hor_slider  = NULL;
    ED4_ROOT->scroll_links.link_for_ver_slider  = NULL;
    ED4_ROOT->middle_area_man           = NULL;
    ED4_ROOT->top_area_man              = NULL;

    delete ED4_ROOT->main_manager;
    ED4_ROOT->main_manager              = NULL;
    delete ED4_ROOT->ecoli_ref;
    {
        GB_push_transaction(GLOBAL_gb_main);
        GBDATA *gb_configuration = GBT_find_configuration(GLOBAL_gb_main, string);
        GBDATA *gb_middle_area = GB_search(gb_configuration, "middle_area", GB_FIND);
        GBDATA *gb_top_area = GB_search(gb_configuration, "top_area", GB_FIND);
        config_data_middle = GB_read_as_string(gb_middle_area);
        config_data_top = GB_read_as_string(gb_top_area);
        GB_pop_transaction(GLOBAL_gb_main);
    }

    ED4_ROOT->first_window->reset_all_for_new_config();

    ED4_ROOT->create_hierarchy( config_data_middle, config_data_top );
    ED4_ROOT->refresh_all_windows(1);

    free(config_data_middle);
    free(config_data_top);
}

static long ED4_get_edit_modus(AW_root *root)
{
    if (!root->awar(AWAR_EDIT_MODE)->read_int()) {
        return AD_ALIGN;
    }
    return root->awar(AWAR_INSERT_MODE)->read_int() ? AD_INSERT : AD_REPLACE;
}

void ed4_changesecurity(AW_root *root, AW_CL /*cd1*/)
{
    long mode = ED4_get_edit_modus(root);
    long level;
    const char *awar_name = 0;

    switch (mode){
        case AD_ALIGN:
            awar_name = AWAR_EDIT_SECURITY_LEVEL_ALIGN;
            break;
        default:
            awar_name = AWAR_EDIT_SECURITY_LEVEL_CHANGE;
    }

    ED4_ROOT->aw_root->awar(AWAR_EDIT_SECURITY_LEVEL)->map(awar_name);
    level = ED4_ROOT->aw_root->awar(awar_name)->read_int();
    GB_change_my_security (GLOBAL_gb_main, level, "");
}

void ed4_change_edit_mode(AW_root *root, AW_CL cd1)
{
    awar_edit_modus = (ad_edit_modus)ED4_get_edit_modus(root);
    ed4_changesecurity(root, cd1);
}

ED4_returncode rebuild_consensus(void **/*arg1*/, void **/*arg2*/, ED4_base *object)                // arg1 and arg2 are NULL-values !!!
{
    if ( object->flag.is_consensus) {
        ED4_species_manager *spec_man = object->to_species_manager();
        spec_man->do_callbacks();

        ED4_base *sequence_data_terminal = object->search_spec_child_rek( ED4_L_SEQUENCE_STRING );

        sequence_data_terminal->set_refresh();
        sequence_data_terminal->parent->refresh_requested_by_child();
    }
    return ED4_R_OK;
}

void ED4_new_editor_window( AW_window *aww, AW_CL /*cd1*/, AW_CL /*cd2*/ )
{
    ED4_ROOT->use_window(aww);

    AW_device  *device;
    ED4_window *new_window;
    if (ED4_ROOT->generate_window( &device, &new_window) == ED4_R_BREAK) // don't open more then five windows
        return;

    ED4_ROOT->use_window(new_window);

    new_window->set_scrolled_rectangle( 0, 0, 0, 0, ED4_ROOT->scroll_links.link_for_hor_slider,
                                        ED4_ROOT->scroll_links.link_for_ver_slider,
                                        ED4_ROOT->scroll_links.link_for_hor_slider,
                                        ED4_ROOT->scroll_links.link_for_ver_slider );

    new_window->aww->show();
    new_window->update_scrolled_rectangle();

    ED4_ROOT->use_window(aww);
}



void ED4_start_editor_on_configuration(AW_window *aww){
    aww->hide();
    char *cn = aww->get_root()->awar(AWAR_EDIT_CONFIGURATION)->read_string();

    ED4_load_new_config(cn);
    free(cn);
}

void ED4_compression_changed_cb(AW_root *awr){
    ED4_remap_mode mode = (ED4_remap_mode)awr->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->read_int();
    int percent = awr->awar(ED4_AWAR_COMPRESS_SEQUENCE_PERCENT)->read_int();
    GB_transaction transaction_var(GLOBAL_gb_main);

    if (ED4_ROOT->root_group_man) {
        ED4_cursor& cursor  = ED4_ROOT->get_ed4w()->cursor;
        int         rel_pos = cursor.get_screen_relative_pos();
        int         seq_pos = cursor.get_sequence_pos();

        AW_window *aww    = ED4_ROOT->get_aww();
        AW_device *device = ED4_ROOT->get_device();

        ED4_remap *remap = ED4_ROOT->root_group_man->remap();
        remap->set_mode(mode, percent);

        device->push_clip_scale();
        device->set_clipall();     // draw nothing

        ED4_expose_recalculations();

        cursor.jump_sequence_pos(aww, seq_pos, ED4_JUMP_KEEP_POSITION);
        cursor.set_screen_relative_pos(aww, rel_pos);

        ED4_expose_cb(aww, 0, 0); // does pop_clip_scale + refresh
    }
}

void ED4_compression_toggle_changed_cb(AW_root *root, AW_CL cd1, AW_CL /*cd2*/)
{
    //    static int cst_change_denied;

    int gaps = root->awar(ED4_AWAR_COMPRESS_SEQUENCE_GAPS)->read_int();
    int hide = root->awar(ED4_AWAR_COMPRESS_SEQUENCE_HIDE)->read_int();
    ED4_remap_mode mode = ED4_remap_mode(root->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->read_int());

    switch(int(cd1)) {
        case 0: { // ED4_AWAR_COMPRESS_SEQUENCE_GAPS changed
            if (gaps!=2 && hide!=0) {
                root->awar(ED4_AWAR_COMPRESS_SEQUENCE_HIDE)->write_int(0);
                return;
            }
            break;
        }
        case 1: { // ED4_AWAR_COMPRESS_SEQUENCE_HIDE changed
            if (hide!=0 && gaps!=2) {
                root->awar(ED4_AWAR_COMPRESS_SEQUENCE_GAPS)->write_int(2);
                return;
            }
            break;
        }
        default: {
            e4_assert(0);
            break;
        }
    }

    mode = ED4_RM_NONE;
    switch (gaps) {
        case 0: mode = ED4_RM_NONE; break;
        case 1: mode = ED4_RM_DYNAMIC_GAPS; break;
        case 2: {
            switch(hide) {
                case 0: mode = ED4_RM_MAX_ALIGN; break;
                case 1: mode = ED4_RM_SHOW_ABOVE; break;
                default: e4_assert(0); break;
            }
            break;
        }
        default: e4_assert(0); break;
    }

    root->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->write_int(int(mode));
}

//  -------------------------------------------------------------------
//      AW_window *ED4_create_level_1_options_window(AW_root *root)
//  -------------------------------------------------------------------
AW_window *ED4_create_level_1_options_window(AW_root *root){
    AW_window_simple *aws = new AW_window_simple;

    aws->init( root, "EDIT4_PROPS","EDIT4 Options");
    aws->load_xfig("edit4/options.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"e4_options.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    //  -----------------------------------
    //      Online Sequence Compression

    aws->at("gaps");
    aws->create_toggle_field(ED4_AWAR_COMPRESS_SEQUENCE_GAPS);
    aws->insert_default_toggle("Show all gaps", "A", 0);
    aws->insert_toggle("Show some gaps", "S", 1);
    aws->insert_toggle("Hide all gaps", "H", 2);
    aws->update_toggle_field();

    aws->at("hide");
    aws->create_toggle_field(ED4_AWAR_COMPRESS_SEQUENCE_HIDE);
    aws->insert_default_toggle("Hide no Nucleotides", "0", 0);
    aws->insert_toggle("Hide columns with less than...", "1", 1);
    aws->update_toggle_field();

    aws->at("percent");
    aws->create_input_field(ED4_AWAR_COMPRESS_SEQUENCE_PERCENT);

    //  --------------
    //      Layout

    aws->at("seq_helix");
    aws->create_input_field(AWAR_EDIT_HELIX_SPACING);

    aws->at("seq_seq");
    aws->create_input_field(AWAR_EDIT_TERMINAL_SPACING);

    //  --------------------
    //      Scroll-Speed

    aws->at("scroll_x");
    aws->create_input_field(ED4_AWAR_SCROLL_SPEED_X);

    aws->at("scroll_y");
    aws->create_input_field(ED4_AWAR_SCROLL_SPEED_Y);

    aws->at("margin");
    aws->create_input_field(ED4_AWAR_SCROLL_MARGIN);

    //  ---------------
    //      Editing

    aws->at("gapchars");
    aws->create_input_field(ED4_AWAR_GAP_CHARS);

    aws->at("repeat");
    aws->label("Use digits to repeat edit commands?");
    aws->create_toggle(ED4_AWAR_DIGITS_AS_REPEAT);

    aws->at("fast");
    aws->label("Should Ctrl-Cursor jump over next group?");
    aws->create_toggle(ED4_AWAR_FAST_CURSOR_JUMP);

    aws->at("checksum");
    aws->label("Announce all checksum changes\ncaused by editing commands.");
    aws->create_toggle(ED4_AWAR_ANNOUNCE_CHECKSUM_CHANGES);

    return aws;
}

/* Open window to show IUPAC tables */
static AW_window * CON_showgroupswin_cb( AW_root *aw_root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "SHOW_IUPAC", "Show IUPAC");
    aws->load_xfig("consensus/groups.fig");
    aws->button_length( 7 );

    aws->at("ok");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","O");

    return aws;
}


AW_window *ED4_create_consensus_definition_window(AW_root *root) {
    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "EDIT4_CONSENSUS", "EDIT4 Consensus Definition");
        aws->load_xfig("edit4/consensus.fig");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE","C");

        aws->callback( AW_POPUP_HELP,(AW_CL)"e4_consensus.hlp");
        aws->at("help");
        aws->create_button("HELP","HELP","H");

        aws->button_length(10);
        aws->at("showgroups");aws->callback(AW_POPUP,(AW_CL)CON_showgroupswin_cb,0);
        aws->create_button("SHOW_IUPAC", "show\nIUPAC...","s");

        aws->at("countgaps");
        aws->create_toggle_field(ED4_AWAR_CONSENSUS_COUNTGAPS);
        aws->insert_toggle("on","1", 1);
        aws->insert_default_toggle("off","0", 0);
        aws->update_toggle_field();

        aws->at("gapbound");
        aws->create_input_field(ED4_AWAR_CONSENSUS_GAPBOUND, 4);

        aws->at("group");
        aws->create_toggle_field(ED4_AWAR_CONSENSUS_GROUP);
        aws->insert_toggle("on", "1", 1);
        aws->insert_default_toggle("off", "0", 0);
        aws->update_toggle_field();

        aws->at("considbound");
        aws->create_input_field(ED4_AWAR_CONSENSUS_CONSIDBOUND, 4);

        aws->at("upper");
        aws->create_input_field(ED4_AWAR_CONSENSUS_UPPER, 4);

        aws->at("lower");
        aws->create_input_field(ED4_AWAR_CONSENSUS_LOWER, 4);

        aws->at("show");
        aws->label("Display consensus?");
        aws->create_toggle(ED4_AWAR_CONSENSUS_SHOW);
    }

    return aws;
}

void ED4_create_consensus_awars(AW_root *aw_root)
{
    GB_transaction dummy(GLOBAL_gb_main);

    aw_root->awar_int(ED4_AWAR_CONSENSUS_COUNTGAPS, 1)->add_callback(ED4_consensus_definition_changed, 0, 0);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_GAPBOUND, 60)->add_callback(ED4_consensus_definition_changed, 0, 0);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_GROUP, 1)->add_callback(ED4_consensus_definition_changed, 0, 0);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_CONSIDBOUND, 30)->add_callback(ED4_consensus_definition_changed, 0, 0);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_UPPER, 95)->add_callback(ED4_consensus_definition_changed, 0, 0);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_LOWER, 70)->add_callback(ED4_consensus_definition_changed, 0, 0);

    AW_awar *cons_show = aw_root->awar_int(ED4_AWAR_CONSENSUS_SHOW, 1);

    cons_show->write_int(1);
    cons_show->add_callback(ED4_consensus_display_changed, 0, 0);
}

void ED4_restart_editor(AW_window *aww, AW_CL, AW_CL)
{
    ED4_start_editor_on_configuration(aww);
}

AW_window *ED4_start_editor_on_old_configuration(AW_root *awr)
{
    static AW_window_simple *aws = 0;

    if (aws) return (AW_window *)aws;
    aws = new AW_window_simple;
    aws->init( awr, "LOAD_OLD_CONFIGURATION", "SELECT A CONFIGURATION");
    aws->at(10,10);
    aws->auto_space(0,0);
    awt_create_selection_list_on_configurations(GLOBAL_gb_main,(AW_window *)aws,AWAR_EDIT_CONFIGURATION);
    aws->at_newline();

    aws->callback((AW_CB0)ED4_start_editor_on_configuration);
    aws->create_button("LOAD","LOAD");

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->window_fit();
    return (AW_window *)aws;
}

void ED4_save_configuration(AW_window *aww,AW_CL close_flag){
    char *cn = aww->get_root()->awar(AWAR_EDIT_CONFIGURATION)->read_string();
    if (close_flag) aww->hide();

    ED4_ROOT->database->generate_config_string( cn );

    free(cn);
}

AW_window *ED4_save_configuration_as_open_window(AW_root *awr){
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;
    aws = new AW_window_simple;
    aws->init( awr, "SAVE_CONFIGURATION", "SAVE A CONFIGURATION");
    aws->load_xfig("edit4/save_config.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"configuration.hlp");
    aws->create_button("HELP","HELP");

    aws->at("save");
    aws->create_input_field(AWAR_EDIT_CONFIGURATION);

    aws->at("confs");
    awt_create_selection_list_on_configurations(GLOBAL_gb_main,aws,AWAR_EDIT_CONFIGURATION);

    aws->at("go");
    aws->callback(ED4_save_configuration, 1);
    aws->create_button("SAVE","SAVE");

    return aws;
}

static GB_ERROR createDataFromConsensus(GBDATA *gb_species, ED4_group_manager *group_man)
{
    GB_ERROR error = 0;
    ED4_char_table *table = &group_man->table();
    char *consensus = table->build_consensus_string();
    int len = table->size();
    int p;
    char *equal_to = ED4_ROOT->aw_root->awar(ED4_AWAR_CREATE_FROM_CONS_REPL_EQUAL)->read_string();
    char *point_to = ED4_ROOT->aw_root->awar(ED4_AWAR_CREATE_FROM_CONS_REPL_POINT)->read_string();
    int allUpper = ED4_ROOT->aw_root->awar(ED4_AWAR_CREATE_FROM_CONS_ALL_UPPER)->read_int();

    for (p=0; p<len; p++) {
        switch (consensus[p]) {
            case '=': consensus[p] = equal_to[0]; break;
            case '.': consensus[p] = point_to[0]; break;
            default: {
                if (allUpper) {
                    consensus[p] = toupper(consensus[p]);
                }
                break;
            }
        }
    }

    if (ED4_ROOT->aw_root->awar(ED4_AWAR_CREATE_FROM_CONS_CREATE_POINTS)) { // points at start & end of sequence?
        for (p=0; p<len; p++) {
            if (!ADPP_IS_ALIGN_CHARACTER(consensus[p])) break;
            consensus[p] = '.';
        }
        for (p=len-1; p>=0; p--) {
            if (!ADPP_IS_ALIGN_CHARACTER(consensus[p])) break;
            consensus[p] = '.';
        }
    }

    GB_CSTR ali = GBT_get_default_alignment(GLOBAL_gb_main);
    GBDATA *gb_ali = GB_search(gb_species, ali, GB_DB);
    if (gb_ali) {
        GBDATA *gb_data = GB_search(gb_ali, "data", GB_STRING);
        error = GB_write_pntr(gb_data, consensus, len, len);
    }
    else {
        error = GB_export_errorf("Can't find alignment '%s'", ali);
    }
    free(consensus);
    return error;
}

// --------------------------------------------------------------------------------

struct S_SpeciesMergeList {
    GBDATA *species;
    char *species_name;
    struct S_SpeciesMergeList *next;

};
typedef struct S_SpeciesMergeList *SpeciesMergeList;

static ED4_returncode add_species_to_merge_list(void **SpeciesMergeListPtr, void **GBDATAPtr, ED4_base *base)
{
    if (base->is_species_name_terminal()) {
        ED4_species_name_terminal *name_term       = base->to_species_name_terminal();
        char                      *species_name    = name_term->resolve_pointer_to_string_copy();
        GBDATA                    *gb_species_data = (GBDATA*)GBDATAPtr;
        GBDATA                    *gb_species      = GBT_find_species_rel_species_data(gb_species_data, species_name);

        if (gb_species) {
            SpeciesMergeList *smlp = (SpeciesMergeList*)SpeciesMergeListPtr;
            SpeciesMergeList sml = new S_SpeciesMergeList;
            sml->species = gb_species;
            sml->species_name = strdup(species_name);
            sml->next = *smlp;
            *smlp = sml;
        }

        free(species_name);
    }
    return ED4_R_OK;
}
static int SpeciesMergeListLength(SpeciesMergeList sml)
{
    int length = 0;

    while (sml) {
        length++;
        sml = sml->next;
    }

    return length;
}
static void freeSpeciesMergeList(SpeciesMergeList sml) {
    while (sml) {
        free(sml->species_name);
        freeset(sml, sml->next);
    }
}

// --------------------------------------------------------------------------------

inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data) {
    return GBT_find_species_rel_species_data(gb_species_data, short_name)==0;
}

static void create_new_species(AW_window */*aww*/, AW_CL cl_creation_mode)
    // creation_mode ==     0 -> create new species
    //                      1 -> create new species from group konsensus
    //                      2 -> copy current species
{
    enum e_creation_mode { CREATE_NEW_SPECIES, CREATE_FROM_CONSENSUS, COPY_SPECIES } creation_mode = (enum e_creation_mode)(cl_creation_mode);
    GB_CSTR new_species_full_name = ED4_ROOT->aw_root->awar(ED4_AWAR_SPECIES_TO_CREATE)->read_string(); // this contains the full_name now!
    GB_CSTR error = 0;

    e4_assert(creation_mode>=0 && creation_mode<=2);

    if (!new_species_full_name || new_species_full_name[0]==0) {
        error = GB_export_error("Please enter a full_name for the new species");
    }
    else {
        error                    = GB_begin_transaction(GLOBAL_gb_main);
        GBDATA *gb_species_data  = GB_search(GLOBAL_gb_main, "species_data",  GB_CREATE_CONTAINER);
        char   *new_species_name = 0;
        char   *acc              = 0;
        char   *addid            = 0;

        enum e_dataSource { MERGE_FIELDS, COPY_FIELDS } dataSource = (enum e_dataSource)ED4_ROOT->aw_root ->awar(ED4_AWAR_CREATE_FROM_CONS_DATA_SOURCE)->read_int();
        enum { NOWHERE, ON_SPECIES, ON_KONSENSUS } where_we_are = NOWHERE;
        ED4_terminal *cursor_terminal = 0;

        if (!error) {
            if (creation_mode==CREATE_FROM_CONSENSUS || creation_mode==COPY_SPECIES) {
                ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;

                if (cursor->owner_of_cursor) {
                    cursor_terminal = cursor->owner_of_cursor->to_terminal();

                    if (cursor_terminal->parent->parent->flag.is_consensus) {
                        where_we_are = ON_KONSENSUS;
                    }
                    else {
                        where_we_are = ON_SPECIES;
                    }
                }
            }

            if (creation_mode==COPY_SPECIES || (creation_mode==CREATE_FROM_CONSENSUS && dataSource==COPY_FIELDS)) {
                if (where_we_are==ON_SPECIES) {
                    ED4_species_name_terminal *spec_name   = cursor_terminal->to_sequence_terminal()->corresponding_species_name_terminal();
                    const char                *source_name = spec_name->resolve_pointer_to_char_pntr();
                    GBDATA                    *gb_source   = GBT_find_species_rel_species_data(gb_species_data, source_name);

                    GBDATA *gb_acc  = GB_search(gb_source, "acc", GB_FIND);
                    if (gb_acc) acc = GB_read_string(gb_acc); // if has accession

                    const char *add_field = AW_get_nameserver_addid(GLOBAL_gb_main);
                    GBDATA     *gb_addid  = add_field[0] ? GB_search(gb_source, add_field, GB_FIND) : 0;
                    if (gb_addid) addid   = GB_read_as_string(gb_addid);
                }
                else {
                    error = "Please place cursor on a species";
                }
            }
        }

        if (!error) {
            UniqueNameDetector *existingNames = 0;

            if (creation_mode==0) {
                error = "It's no good idea to create the short-name for a new species using the nameserver! (has no acc yet)";
            }
            else {
                error = AWTC_generate_one_name(GLOBAL_gb_main, new_species_full_name, acc, addid, new_species_name, true, true);
                if (!error) {   // name was created
                    if (!nameIsUnique(new_species_name, gb_species_data)) {
                        if (!existingNames) existingNames = new UniqueNameDetector(gb_species_data);
                        freeset(new_species_name, AWTC_makeUniqueShortName(new_species_name, *existingNames));
                        if (!new_species_name) error = GB_await_error();
                    }
                }
            }

            if (error) {        // try to make a random name
                const char *msg = GBS_global_string("%s\nGenerating a random name instead.", error);
                aw_message(msg);
                error           = 0;

                if (!existingNames) existingNames = new UniqueNameDetector(gb_species_data);
                new_species_name = AWTC_generate_random_name(*existingNames);

                if (!new_species_name) {
                    error = GBS_global_string("Failed to create a new name for '%s'", new_species_full_name);
                }
            }

            if (existingNames) delete existingNames;
        }

        if (!error) {
            GBDATA *gb_new_species = 0;

            if (!error) {
                if (creation_mode==CREATE_NEW_SPECIES) {
                    GBDATA *gb_created_species = GBT_find_or_create_species(GLOBAL_gb_main, new_species_name);
                    if (!gb_created_species) {
                        error = GBS_global_string("Failed to create new species '%s'", new_species_name);
                    }
                    else {
                        GB_CSTR  ali    = GBT_get_default_alignment(GLOBAL_gb_main);
                        GBDATA  *gb_ali = GB_search(gb_created_species, ali, GB_DB);

                        if (gb_ali) error = GBT_write_string(gb_ali, "data", ".......");
                        else error        = GBS_global_string("Can't create alignment '%s' (Reason: %s)", ali, GB_await_error());
                    }
                    if (!error) error = GBT_write_string(gb_created_species, "full_name", new_species_full_name);
                }
                else if (creation_mode==CREATE_FROM_CONSENSUS && dataSource==MERGE_FIELDS)
                { // create from consensus (merge fields from all species in container)
                    if (where_we_are==NOWHERE) {
                        error = "Please place cursor on any sequence/consensus of group";
                    }
                    else {
                        ED4_group_manager *group_man = cursor_terminal->get_parent(ED4_L_GROUP)->to_group_manager();
                        SpeciesMergeList sml = 0; // list of species in group

                        group_man->route_down_hierarchy((void**)&sml, (void**)gb_species_data, add_species_to_merge_list);
                        if (sml==0) {
                            error = "Please choose a none empty group!";
                        }

                        if (!error) {
                            GBDATA *gb_source = sml->species;
                            gb_new_species = GB_create_container(gb_species_data, "species");
                            error = GB_copy(gb_new_species, gb_source); // copy first found species to create a new species
                        }
                        if (!error) error = GBT_write_string(gb_new_species, "name", new_species_name); // insert new 'name'
                        if (!error) error = GBT_write_string(gb_new_species, "full_name", new_species_full_name); // insert new 'full_name'
                        if (!error) error = createDataFromConsensus(gb_new_species, group_man); // insert consensus as 'data'

                        if (!error) {
                            char *doneFields = strdup(";name;"); // all fields which are already merged
                            int doneLen = strlen(doneFields);
                            SpeciesMergeList sl = sml;
                            int sl_length = SpeciesMergeListLength(sml);
                            int *fieldStat = new int[sl_length]; // 0 = not used yet ; -1 = don't has field ;
                            // 1..n = field content (same number means same content)

                            while (sl && !error) { // with all species do..
                                char *newFields = GB_get_subfields(sl->species);
                                char *fieldStart = newFields; // points to ; before next field

                                while (fieldStart[1] && !error) { // with all subfields of the species do..
                                    char *fieldEnd = strchr(fieldStart+1, ';');

                                    e4_assert(fieldEnd);
                                    char behind = fieldEnd[1];
                                    fieldEnd[1] = 0;

                                    if (strstr(doneFields, fieldStart)==0) { // field is not merged yet
                                        char *fieldName = fieldStart+1;
                                        int fieldLen = int(fieldEnd-fieldName);

                                        e4_assert(fieldEnd[0]==';');
                                        fieldEnd[0] = 0;

                                        GBDATA *gb_field = GB_search(sl->species, fieldName, GB_FIND);
                                        e4_assert(gb_field); // field has to exist, cause it was found before
                                        int type = gb_field->flags.type; //GB_TYPE(gb_field);
                                        if (type==GB_STRING) { // we only merge string fields
                                            int i;
                                            int doneSpecies = 0;
                                            int nextStat = 1;

                                            for (i=0; i<sl_length; i++) { // clear field status
                                                fieldStat[i] = 0;
                                            }

                                            while (doneSpecies<sl_length) { // since all species in list were handled
                                                SpeciesMergeList sl2 = sml;
                                                i = 0;

                                                while (sl2) {
                                                    if (fieldStat[i]==0) {
                                                        gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                                        if (gb_field) {
                                                            char *content = GB_read_as_string(gb_field);
                                                            SpeciesMergeList sl3 = sl2->next;

                                                            fieldStat[i] = nextStat;
                                                            doneSpecies++;
                                                            int j = i+1;
                                                            while (sl3) {
                                                                if (fieldStat[j]==0) {
                                                                    gb_field = GB_search(sl3->species, fieldName, GB_FIND);
                                                                    if (gb_field) {
                                                                        char *content2 = GB_read_as_string(gb_field);

                                                                        if (strcmp(content, content2)==0) { // if contents are the same, they get the same status
                                                                            fieldStat[j] = nextStat;
                                                                            doneSpecies++;
                                                                        }
                                                                        free(content2);
                                                                    }
                                                                    else {
                                                                        fieldStat[j] = -1;
                                                                        doneSpecies++;
                                                                    }
                                                                }
                                                                sl3 = sl3->next;
                                                                j++;
                                                            }

                                                            free(content);
                                                            nextStat++;
                                                        }
                                                        else {
                                                            fieldStat[i] = -1; // field does not exist here
                                                            doneSpecies++;
                                                        }
                                                    }
                                                    sl2 = sl2->next;
                                                    i++;
                                                }
                                            }

                                            e4_assert(nextStat!=1); // this would mean that none of the species contained the field

                                            {
                                                char *new_content = 0;
                                                int new_content_len = 0;

                                                if (nextStat==2) { // all species contain same field content or do not have the field
                                                    SpeciesMergeList sl2 = sml;

                                                    while (sl2) {
                                                        gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                                        if (gb_field) {
                                                            new_content = GB_read_as_string(gb_field);
                                                            new_content_len = strlen(new_content);
                                                            break;
                                                        }
                                                        sl2 = sl2->next;
                                                    }
                                                }
                                                else { // different field contents
                                                    int actualStat;
                                                    for (actualStat=1; actualStat<nextStat; actualStat++) {
                                                        int names_len = 1; // open bracket
                                                        SpeciesMergeList sl2 = sml;
                                                        i = 0;
                                                        char *content = 0;

                                                        while (sl2) {
                                                            if (fieldStat[i]==actualStat) {
                                                                names_len += strlen(sl2->species_name)+1;
                                                                if (!content) {
                                                                    gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                                                    e4_assert(gb_field);
                                                                    content = GB_read_as_string(gb_field);
                                                                }
                                                            }
                                                            sl2 = sl2->next;
                                                            i++;
                                                        }

                                                        e4_assert(content);
                                                        int add_len = names_len+1+strlen(content);
                                                        char *whole = (char*)malloc(new_content_len+1+add_len+1);
                                                        e4_assert(whole);
                                                        char *add = new_content ? whole+sprintf(whole, "%s ", new_content) : whole;
                                                        sl2 = sml;
                                                        i = 0;
                                                        int first = 1;
                                                        while (sl2) {
                                                            if (fieldStat[i]==actualStat) {
                                                                add += sprintf(add, "%c%s", first ? '{' : ';', sl2->species_name);
                                                                first = 0;
                                                            }
                                                            sl2 = sl2->next;
                                                            i++;
                                                        }
                                                        add += sprintf(add, "} %s", content);

                                                        free(content);

                                                        freeset(new_content, whole);
                                                        new_content_len = strlen(new_content);
                                                    }
                                                }

                                                if (new_content) {
                                                    error = GBT_write_string(gb_new_species, fieldName, new_content);
                                                    free(new_content);
                                                }
                                            }
                                        }

                                        // mark field as done:
                                        char *new_doneFields = (char*)malloc(doneLen+fieldLen+1+1);
                                        sprintf(new_doneFields, "%s%s;", doneFields, fieldName);
                                        doneLen += fieldLen+1;
                                        freeset(doneFields, new_doneFields);

                                        fieldEnd[0] = ';';
                                    }

                                    fieldEnd[1] = behind;
                                    fieldStart = fieldEnd;
                                }
                                free(newFields);
                                sl = sl->next;
                            }
                            free(doneFields);
                            free(fieldStat);
                        }
                        freeSpeciesMergeList(sml); sml = 0;
                    }
                }
                else { // copy species or create from consensus (copy fields from one species)
                    e4_assert(where_we_are==ON_SPECIES);

                    ED4_species_name_terminal *spec_name   = cursor_terminal->to_sequence_terminal()->corresponding_species_name_terminal();
                    const char                *source_name = spec_name->resolve_pointer_to_char_pntr();
                    GBDATA                    *gb_source   = GBT_find_species_rel_species_data(gb_species_data, source_name);

                    if (gb_source) {
                        gb_new_species    = GB_create_container(gb_species_data, "species");
                        error             = GB_copy(gb_new_species, gb_source);
                        if (!error) error = GBT_write_string(gb_new_species, "name", new_species_name);
                        if (!error) error = GBT_write_string(gb_new_species, "full_name", new_species_full_name); // insert new 'full_name'
                        if (!error && creation_mode==CREATE_FROM_CONSENSUS) {
                            ED4_group_manager *group_man = cursor_terminal->get_parent(ED4_L_GROUP)->to_group_manager();
                            error = createDataFromConsensus(gb_new_species, group_man);
                        }
                    }
                    else {
                        error = GBS_global_string("Can't find species '%s'", source_name);
                    }
                }
            }

            if (error) {
                GB_abort_transaction(GLOBAL_gb_main);
            }
            else {
                GB_pop_transaction(GLOBAL_gb_main);

                ED4_get_and_jump_to_species(new_species_name);
                ED4_ROOT->refresh_all_windows(1);
            }
        }

        free(addid);
        free(acc);
        free(new_species_name);
    }

    if (error) aw_popup_ok(error);
}

AW_window *ED4_create_new_seq_window(AW_root *root, AW_CL cl_creation_mode)
    // creation_mode ==     0 -> create new species
    //                      1 -> create new species from group konsensus
    //                      2 -> copy current species
{
    AW_window_simple *aws = new AW_window_simple;
    int creation_mode = int(cl_creation_mode);
    switch (creation_mode) {
        case 0:
            aws->init( root, "create_species","Create species");
            break;
        case 1:
            aws->init( root, "create_species_from_consensus","Create species from consensus");
            break;
        case 2:
            aws->init( root, "copy_species","Copy current species");
            break;
        default:
            e4_assert(0);
            return 0;
    }
    if (creation_mode==1) { // create from consensus
        aws->load_xfig("edit4/create_seq_fc.fig");
    }
    else {
        aws->load_xfig("edit4/create_seq.fig");
    }

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the FULL_NAME\nof the new species");

    aws->at("input");
    aws->create_input_field(ED4_AWAR_SPECIES_TO_CREATE, 30);

    aws->at("ok");
    aws->callback(create_new_species, cl_creation_mode);
    aws->create_button("GO","GO","g");

    if (creation_mode==1) { // create from consensus
        aws->at("replace_equal");
        aws->label("Replace '=' by ");
        aws->create_input_field(ED4_AWAR_CREATE_FROM_CONS_REPL_EQUAL, 1);

        aws->at("replace_point");
        aws->label("Replace '.' by ");
        aws->create_input_field(ED4_AWAR_CREATE_FROM_CONS_REPL_POINT, 1);

        aws->at("replace_start_end");
        aws->label("Create ... at ends of sequence?");
        aws->create_toggle(ED4_AWAR_CREATE_FROM_CONS_CREATE_POINTS);

        aws->at("upper");
        aws->label("Convert all chars to upper?");
        aws->create_toggle(ED4_AWAR_CREATE_FROM_CONS_ALL_UPPER);

        aws->at("data");
        aws->create_option_menu(ED4_AWAR_CREATE_FROM_CONS_DATA_SOURCE, "Other fields", "");
        aws->insert_default_option("Merge from all in group", "", 0);
        aws->insert_option("Copy from current species", "", 1);
        aws->update_option_menu();
    }

    return (AW_window *)aws;
}

