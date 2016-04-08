// =============================================================== //
//                                                                 //
//   File      : ED4_no_class.cxx                                  //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <ed4_extern.hxx>

#include "ed4_awars.hxx"
#include "ed4_class.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_nds.hxx"
#include "ed4_list.hxx"
#include "ed4_seq_colors.hxx"

#include <ad_config.h>
#include <AW_helix.hxx>
#include <AW_rename.hxx>
#include <awt.hxx>
#include <item_sel_list.h>
#include <awt_sel_boxes.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <macros.hxx>
#include <arb_defs.h>
#include <iupac.h>

#include <cctype>
#include <limits.h>

#include <vector>
#include <awt_config_manager.hxx>
#include <consensus_config.h>
#include <awt_misc.hxx>

using namespace std;

void ED4_calc_terminal_extentions() {
    AW_device *device = ED4_ROOT->first_window->get_device(); // any device

    const AW_font_limits& seq_font_limits  = device->get_font_limits(ED4_G_SEQUENCES, 0);
    const AW_font_limits& seq_equal_limits = device->get_font_limits(ED4_G_SEQUENCES, '=');
    const AW_font_limits& info_font_limits = device->get_font_limits(ED4_G_STANDARD, 0);

    int info_char_width = info_font_limits.width;
    int seq_term_descent;

    if (ED4_ROOT->helix->is_enabled() || ED4_ROOT->protstruct) { // display helix ?
        ED4_ROOT->helix_spacing =
            seq_equal_limits.ascent // the ascent of '='
            + ED4_ROOT->helix_add_spacing; // xtra user-defined spacing

        seq_term_descent = ED4_ROOT->helix_spacing;
    }
    else {
        ED4_ROOT->helix_spacing = 0;
        seq_term_descent  = seq_font_limits.descent;
    }

    // for wanted_seq_term_height ignore descent, because it additionally allocates 'ED4_ROOT->helix_spacing' space:
    int wanted_seq_term_height = seq_font_limits.ascent + seq_term_descent + ED4_ROOT->terminal_add_spacing;
    int wanted_seq_info_height = info_font_limits.height + ED4_ROOT->terminal_add_spacing;

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

    INFO_TERM_TEXT_YOFFSET = info_font_limits.ascent - 1;
    SEQ_TERM_TEXT_YOFFSET  = seq_font_limits.ascent - 1;

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

static ARB_ERROR update_terminal_extension(ED4_base *this_object) {
    if (this_object->is_terminal()) {
        if (this_object->is_spacer_terminal()) {
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

    this_object->request_resize();

    return NULL;
}

void ED4_expose_recalculations() {
    ED4_ROOT->recalc_font_group();
    ED4_calc_terminal_extentions();

#if defined(WARN_TODO)
#warning below calculations have to be done at startup as well
#endif

    ED4_ROOT->ref_terminals.get_ref_sequence_info()->extension.size[HEIGHT] = TERMINALHEIGHT;
    ED4_ROOT->ref_terminals.get_ref_sequence()->extension.size[HEIGHT]      = TERMINALHEIGHT;
    ED4_ROOT->ref_terminals.get_ref_sequence_info()->extension.size[WIDTH]  = MAXINFOWIDTH;

    int screenwidth = ED4_ROOT->root_group_man->remap()->shown_sequence_to_screen(MAXSEQUENCECHARACTERLENGTH);
    while (1) {
        ED4_ROOT->ref_terminals.get_ref_sequence()->extension.size[WIDTH] =
            ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES) *
            (screenwidth+3);

        ED4_terminal *top_middle_line_terminal = ED4_ROOT->main_manager->get_top_middle_line_terminal();

        ED4_ROOT->main_manager->get_top_middle_spacer_terminal()->extension.size[HEIGHT] = TERMINALHEIGHT - top_middle_line_terminal->extension.size[HEIGHT];
        ED4_ROOT->main_manager->route_down_hierarchy(makeED4_route_cb(update_terminal_extension)).expect_no_error();

        ED4_ROOT->resize_all(); // may change mapping

        int new_screenwidth = ED4_ROOT->root_group_man->remap()->shown_sequence_to_screen(MAXSEQUENCECHARACTERLENGTH);
        if (new_screenwidth == screenwidth) { // mapping did not change
            break;
        }
        // @@@ request resize for all terminals ? 
        screenwidth = new_screenwidth;
    }
}

static ARB_ERROR call_edit(ED4_base *object, ED4_work_info *work_info) {
    // called after editing consensus to edit single sequences
    GB_ERROR error = NULL;

    if (object->is_species_seq_terminal()) {
        int expected_prop = ED4_P_CURSOR_ALLOWED|ED4_P_ALIGNMENT_DATA;

        if ((object->dynamic_prop & expected_prop) == expected_prop) {
            ED4_work_info new_work_info;

            new_work_info.event            = work_info->event;
            new_work_info.char_position    = work_info->char_position;
            new_work_info.out_seq_position = work_info->out_seq_position;
            new_work_info.refresh_needed   = false;
            new_work_info.cursor_jump      = ED4_JUMP_KEEP_VISIBLE;
            new_work_info.out_string       = NULL;
            new_work_info.mode             = work_info->mode;
            new_work_info.rightward        = work_info->rightward;
            new_work_info.cannot_handle    = false;
            new_work_info.is_sequence      = work_info->is_sequence;
            new_work_info.working_terminal = object->to_terminal();

            if (object->get_species_pointer()) {
                new_work_info.gb_data   = object->get_species_pointer();
                new_work_info.string    = NULL;
            }
            else {
                new_work_info.gb_data = NULL;
                new_work_info.string  = object->id; // @@@ looks obsolete (see [8402] for previous code)
                e4_assert(0); // assume we never come here
            }

            new_work_info.repeat_count = 1;

            ED4_ROOT->edit_string->init_edit();
            error = ED4_ROOT->edit_string->edit(&new_work_info);

            e4_assert(error || !new_work_info.out_string);  
                
            if (new_work_info.refresh_needed) {
                object->request_refresh();
                if (object->is_sequence_terminal()) {
                    ED4_sequence_terminal *seq_term = object->to_sequence_terminal();
                    seq_term->results().searchAgain();
                }
            }

            if (move_cursor) {
                current_cursor().jump_sequence_pos(new_work_info.out_seq_position, ED4_JUMP_KEEP_VISIBLE);
                move_cursor = 0;
            }
        }
    }
    return error;
}

static void executeKeystroke(AW_event *event, int repeatCount) {
    e4_assert(repeatCount>0);

    if (event->keycode!=AW_KEY_NONE) {
        ED4_cursor *cursor = &current_cursor();
        if (cursor->owner_of_cursor && !cursor->owner_of_cursor->flag.hidden) {
            if (event->keycode == AW_KEY_UP || event->keycode == AW_KEY_DOWN ||
                ((event->keymodifier & AW_KEYMODE_CONTROL) &&
                 (event->keycode == AW_KEY_HOME || event->keycode == AW_KEY_END)))
            {
                GB_transaction ta(GLOBAL_gb_main);
                while (repeatCount--) {
                    cursor->move_cursor(event);
                }
            }
            else {
                ED4_work_info *work_info = new ED4_work_info;
        
                work_info->cannot_handle    = false;
                work_info->event            = *event;
                work_info->char_position    = cursor->get_screen_pos();
                work_info->out_seq_position = cursor->get_sequence_pos();
                work_info->refresh_needed   = false;
                work_info->cursor_jump      = ED4_JUMP_KEEP_VISIBLE;
                work_info->out_string       = NULL;         // nur falls new malloc
                work_info->repeat_count     = repeatCount;

                ED4_terminal *terminal = cursor->owner_of_cursor;
                e4_assert(terminal->is_text_terminal());

                work_info->working_terminal = terminal;

                if (terminal->is_sequence_terminal()) {
                    work_info->mode        = awar_edit_mode;
                    work_info->rightward   = awar_edit_rightward;
                    work_info->is_sequence = 1;
                }
                else {
                    work_info->rightward   = true;
                    work_info->is_sequence = 0;

                    if (terminal->is_pure_text_terminal()) {
                        work_info->mode = awar_edit_mode;
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
                    e4_assert(terminal->is_consensus_terminal());
                    ED4_consensus_sequence_terminal *cterm = terminal->to_consensus_sequence_terminal();
                    work_info->string                      = cterm->temp_cons_seq;
                }

                ED4_Edit_String *edit_string = new ED4_Edit_String;
                ARB_ERROR        error       = NULL;

                GB_push_transaction(GLOBAL_gb_main);

                if (terminal->is_consensus_terminal()) {
                    ED4_consensus_sequence_terminal *cterm         = terminal->to_consensus_sequence_terminal();
                    ED4_group_manager               *group_manager = terminal->get_parent(ED4_L_GROUP)->to_group_manager();

                    e4_assert(cterm->temp_cons_seq == 0);
                    work_info->string = cterm->temp_cons_seq = group_manager->build_consensus_string();

                    error = edit_string->edit(work_info);

                    cursor->jump_sequence_pos(work_info->out_seq_position, ED4_JUMP_KEEP_VISIBLE);

                    work_info->string = 0;

                    if (work_info->cannot_handle) {
                        e4_assert(!error); // see ED4_Edit_String::edit()
                        move_cursor = 1;
                        if (!ED4_ROOT->edit_string) {
                            ED4_ROOT->edit_string = new ED4_Edit_String;
                        }
                        error = group_manager->route_down_hierarchy(makeED4_route_cb(call_edit, work_info));
                        group_manager->rebuild_consensi(group_manager, ED4_U_UP_DOWN);
                    }

                    freenull(cterm->temp_cons_seq);
                }
                else {
                    error = edit_string->edit(work_info);
                    cursor->jump_sequence_pos(work_info->out_seq_position, work_info->cursor_jump);
                }

                edit_string->finish_edit();

                if (error) work_info->refresh_needed = true;

                GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);

                if (work_info->refresh_needed) {
                    GB_transaction ta(GLOBAL_gb_main);

                    terminal->request_refresh();
                    if (terminal->is_sequence_terminal()) {
                        ED4_sequence_terminal *seq_term = terminal->to_sequence_terminal();
                        seq_term->results().searchAgain();
                    }
                }

                delete edit_string;
                delete work_info;
            }
        }
    }
}

void ED4_remote_event(AW_event *faked_event) { // keystrokes forwarded from SECEDIT
    ED4_MostRecentWinContext context;
    executeKeystroke(faked_event, 1);
}

static int get_max_slider_xpos() {
    const AW_screen_area& rect = current_device()->get_area_size();

    AW_pos x, y;
    ED4_base *horizontal_link = ED4_ROOT->scroll_links.link_for_hor_slider;
    horizontal_link->calc_world_coords(&x, &y);

    AW_pos max_xpos = horizontal_link->extension.size[WIDTH] // overall width of virtual scrolling area
        - (rect.r - x); // minus width of visible scroll-area (== relative width of horizontal scrollbar)

    if (max_xpos<0) max_xpos = 0; // happens when window-content is smaller than window (e.g. if (folded) alignment is narrow)
    return int(max_xpos+0.5);
}

static int get_max_slider_ypos() {
    const AW_screen_area& rect = current_device()->get_area_size(); 

    AW_pos x, y;
    ED4_base *vertical_link = ED4_ROOT->scroll_links.link_for_ver_slider;
    vertical_link->calc_world_coords(&x, &y);

    AW_pos max_ypos = vertical_link->extension.size[HEIGHT] // overall height of virtual scrolling area
        - (rect.b - y); // minus height of visible scroll-area (== relative height of vertical scrollbar)

    if (max_ypos<0) max_ypos = 0; // happens when window-content is smaller than window (e.g. if ARB_EDIT4 is not filled)
    return int(max_ypos+0.5);
}

static void ed4_scroll(AW_window *aww, int xdiff, int ydiff) {
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

    if (ydiff<0) { // scroll up
        if (new_ypos<0) new_ypos = 0;
    }
    else if (ydiff>0) { // scroll down
        int max_ypos = get_max_slider_ypos();
        if (max_ypos<0) max_ypos = 0;
        if (new_ypos>max_ypos) new_ypos = max_ypos;
    }

    if (new_xpos!=aww->slider_pos_horizontal) {
        aww->set_horizontal_scrollbar_position(new_xpos);
        ED4_horizontal_change_cb(aww);
    }

    if (new_ypos!=aww->slider_pos_vertical) {
        aww->set_vertical_scrollbar_position(new_ypos);
        ED4_vertical_change_cb(aww);
    }
}

void ED4_input_cb(AW_window *aww) {
    AW_event event;
    static AW_event lastEvent;
    static int repeatCount;

    ED4_LocalWinContext uses(aww);

    aww->get_event(&event);


#if defined(DEBUG) && 0
    printf("event.type=%i event.keycode=%i event.character='%c' event.keymodifier=%i\n", event.type, event.keycode, event.character, event.keymodifier);
#endif

    switch (event.type) {
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
                    executeKeystroke(&lastEvent, repeatCount);
                    lastEvent = event;
                    repeatCount = 1;
                }
            }

            if (repeatCount) {
#if defined(DARWIN) || 1
                // sth goes wrong with OSX -> always execute keystroke
                // Xfree 4.3 has problems as well, so repeat counting is disabled completely
                executeKeystroke(&lastEvent, repeatCount);
                repeatCount                       = 0;
#else
                AW_ProcessEventType nextEventType = ED4_ROOT->aw_root->peek_key_event(aww);

                if (nextEventType!=KEY_RELEASED) { // no key waiting => execute now
                    executeKeystroke(&lastEvent, repeatCount);
                    repeatCount = 0;
                }
#endif
            }
            break;
        }
        case AW_Keyboard_Release: {
            AW_ProcessEventType nextEventType = ED4_ROOT->aw_root->peek_key_event(aww);

            if (nextEventType!=KEY_PRESSED && repeatCount) { // no key follows => execute keystrokes (if any)
                executeKeystroke(&lastEvent, repeatCount);
                repeatCount = 0;
            }

            break;
        }
        default: {
            if (event.button == AW_WHEEL_UP || event.button == AW_WHEEL_DOWN) {
                if (event.type == AW_Mouse_Press) {
                    bool horizontal = event.keymodifier & AW_KEYMODE_ALT;
                    int  direction  = event.button == AW_WHEEL_UP ? -1 : 1;

                    int dx = horizontal ? direction*ED4_ROOT->font_group.get_max_width() : 0;
                    int dy = horizontal ? 0 : direction*ED4_ROOT->font_group.get_max_height();
                
                    ed4_scroll(aww, dx, dy);
                }
                return;
            }

            if (event.button == AW_BUTTON_MIDDLE) {
                if (event.type == AW_Mouse_Press) {
                    ED4_ROOT->scroll_picture.scroll = 1;
                    ED4_ROOT->scroll_picture.old_y = event.y;
                    ED4_ROOT->scroll_picture.old_x = event.x;
                    return;
                }
                if (event.type == AW_Mouse_Release) {
                    ED4_ROOT->scroll_picture.scroll = 0;
                    return;
                }
            }

#if defined(DEBUG) && 0
            if (event.button==AW_BUTTON_LEFT) {
                printf("[ED4_input_cb]  type=%i x=%i y=%i ", (int)event.type, (int)event.x, (int)event.y);
            }
#endif

            AW_pos win_x = event.x;
            AW_pos win_y = event.y;
            current_ed4w()->win_to_world_coords(&(win_x), &(win_y));
            event.x = (int) win_x;
            event.y = (int) win_y;

#if defined(DEBUG) && 0
            if (event.button==AW_BUTTON_LEFT) {
                printf("-> x=%i y=%i\n", (int)event.type, (int)event.x, (int)event.y);
            }
#endif

            GB_transaction ta(GLOBAL_gb_main);
            ED4_ROOT->main_manager->event_sent_by_parent(&event, aww);
            break;
        }
    }

    ED4_trigger_instant_refresh();
}

void ED4_vertical_change_cb(AW_window *aww) {
    ED4_LocalWinContext uses(aww);

    GB_push_transaction(GLOBAL_gb_main);

    ED4_window *win = current_ed4w();
    int old_slider_pos = win->slider_pos_vertical;

    { // correct slider_pos if necessary
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

void ED4_horizontal_change_cb(AW_window *aww) {
    ED4_LocalWinContext uses(aww);

    GB_push_transaction(GLOBAL_gb_main);

    ED4_window *win = current_ed4w();
    int old_slider_pos = win->slider_pos_horizontal;

    { // correct slider_pos if necessary
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

void ED4_scrollbar_change_cb(AW_window *aww) {
    ED4_LocalWinContext uses(aww);

    GB_push_transaction(GLOBAL_gb_main);

    ED4_window *win = current_ed4w();

    int old_hslider_pos = win->slider_pos_horizontal;
    int old_vslider_pos = win->slider_pos_vertical;

    {
        // correct slider_pos if necessary
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

    win->scroll_rectangle(-slider_hdiff, -slider_vdiff);

    win->slider_pos_vertical   = aww->slider_pos_vertical;
    win->slider_pos_horizontal = aww->slider_pos_horizontal;

    GB_pop_transaction(GLOBAL_gb_main);
    win->update_window_coords();
}

void ED4_motion_cb(AW_window *aww) {
    AW_event event;

    ED4_LocalWinContext uses(aww);

    aww->get_event(&event);

    if (event.type == AW_Mouse_Drag && event.button == AW_BUTTON_MIDDLE) {
        if (ED4_ROOT->scroll_picture.scroll) {
            int xdiff = ED4_ROOT->scroll_picture.old_x - event.x;
            int ydiff = ED4_ROOT->scroll_picture.old_y - event.y;

            ed4_scroll(aww, xdiff, ydiff);

            ED4_ROOT->scroll_picture.old_x = event.x;
            ED4_ROOT->scroll_picture.old_y = event.y;
        }
    }
    else {

#if defined(DEBUG) && 0
        if (event.button==AW_BUTTON_LEFT) {
            printf("[ED4_motion_cb] type=%i x=%i y=%i ", (int)event.type, (int)event.x, (int)event.y);
        }
#endif

        AW_pos win_x = event.x;
        AW_pos win_y = event.y;
        current_ed4w()->win_to_world_coords(&win_x, &win_y);
        event.x = (int) win_x;
        event.y = (int) win_y;

#if defined(DEBUG) && 0
        if (event.button==AW_BUTTON_LEFT) {
            printf("-> x=%i y=%i\n", (int)event.type, (int)event.x, (int)event.y);
        }
#endif

        GB_transaction ta(GLOBAL_gb_main);
        ED4_ROOT->main_manager->event_sent_by_parent(&event, aww);
    }
}

void ED4_remote_set_cursor_cb(AW_root *awr) {
    AW_awar *awar = awr->awar(AWAR_SET_CURSOR_POSITION);
    long     pos  = awar->read_int();

    if (pos != -1) {
        ED4_MostRecentWinContext context;
        ED4_cursor *cursor = &current_cursor();
        cursor->jump_sequence_pos(pos, ED4_JUMP_CENTERED);
        awar->write_int(-1);
    }
}

void ED4_jump_to_cursor_position(AW_window *aww, const char *awar_name, PositionType posType) {
    ED4_LocalWinContext  uses(aww);
    ED4_cursor          *cursor = &current_cursor();
    GB_ERROR             error  = 0;

    long pos = aww->get_root()->awar(awar_name)->read_int();

    if (pos>0) pos = bio2info(pos);
    else if (pos<0) { // jump negative (count from back)
        int last_pos = -1; // [1..]

        switch (posType) {
            case ED4_POS_SEQUENCE: {
                last_pos = MAXSEQUENCECHARACTERLENGTH;
                break;
            }
            case ED4_POS_ECOLI: {
                BI_ecoli_ref *ecoli = ED4_ROOT->ecoli_ref;
                if (ecoli->gotData()) {
                    last_pos = ecoli->abs_2_rel(INT_MAX);
                }
                else {
                    last_pos = 0; // doesnt matter (error below)
                }
                break;
            }
            case ED4_POS_BASE: {
                last_pos = cursor->sequence2base_position(INT_MAX);
                break;
            }
        }

        e4_assert(last_pos != -1);
        pos = bio2info(last_pos+1+pos);
    }

    switch (posType) {
        case ED4_POS_SEQUENCE: {
            e4_assert(strcmp(awar_name, current_ed4w()->awar_path_for_cursor)==0);
            break;
        }
        case ED4_POS_ECOLI: {
            e4_assert(strcmp(awar_name, current_ed4w()->awar_path_for_Ecoli)==0);

            BI_ecoli_ref *ecoli = ED4_ROOT->ecoli_ref;
            if (ecoli->gotData()) pos = ecoli->rel_2_abs(pos);
            else error = "No ecoli reference";
            break;
        }
        case ED4_POS_BASE: {
            e4_assert(strcmp(awar_name, current_ed4w()->awar_path_for_basePos)==0);
            pos = cursor->base2sequence_position(pos); 
            break;
        }
    }

    // now position is absolute [0..N-1]

    // limit to screen
    {
        ED4_remap *remap = ED4_ROOT->root_group_man->remap();
        long       max   = remap->screen_to_sequence(remap->get_max_screen_pos());

        if (pos > max) pos  = max;
        else if (pos<0) pos = 0;
    }

    if (error) {
        aw_message(error);
    }
    else {
        cursor->jump_sequence_pos(pos, ED4_JUMP_CENTERED);
    }
}

void ED4_set_helixnr(AW_window *aww, const char *awar_name) {
    ED4_LocalWinContext uses(aww);
    ED4_cursor *cursor = &current_cursor();

    if (cursor->owner_of_cursor) {
        AW_root  *root     = aww->get_root();
        char     *helix_nr = root->awar(awar_name)->read_string();
        BI_helix *helix    = ED4_ROOT->helix;

        if (helix->has_entries()) {
            long pos = helix->first_position(helix_nr);

            if (pos == -1) {
                aw_message(GBS_global_string("No helix '%s' found", helix_nr));
            }
            else {
                cursor->jump_sequence_pos(pos, ED4_JUMP_CENTERED);
            }
        }
        else {
            aw_message("Got no helix information");
        }
        free(helix_nr);
    }
}

void ED4_set_iupac(AW_window *aww, const char *awar_name) {
    ED4_LocalWinContext uses(aww);
    ED4_cursor *cursor = &current_cursor();

    if (cursor->owner_of_cursor) {
        if (cursor->in_consensus_terminal()) {
            aw_message("You cannot change the consensus");
        }
        else {
            int   len;
            char *seq     = cursor->owner_of_cursor->resolve_pointer_to_string_copy(&len);
            int   seq_pos = cursor->get_sequence_pos();

            e4_assert(seq);

            if (seq_pos<len) {
                char *iupac    = ED4_ROOT->aw_root->awar(awar_name)->read_string();
                char  new_char = iupac::encode(iupac, ED4_ROOT->alignment_type);

                seq[seq_pos] = new_char;
                cursor->owner_of_cursor->write_sequence(seq, len);

                free(iupac);
            }

            free(seq);
        }
    }
}

void ED4_exit() {
    ED4_ROOT->aw_root->unlink_awars_from_DB(GLOBAL_gb_main);

    ED4_window *ed4w = ED4_ROOT->first_window;

    while (ed4w) {
        ed4w->aww->hide();
        ed4w->cursor.prepare_shutdown(); // removes any callbacks
        ed4w = ed4w->next;
    }

    while (ED4_ROOT->first_window)
        ED4_ROOT->first_window->delete_window(ED4_ROOT->first_window);


    shutdown_macro_recording(ED4_ROOT->aw_root);

    delete ED4_ROOT;

    GBDATA *gb_main = GLOBAL_gb_main;
    GLOBAL_gb_main  = NULL;
#if defined(DEBUG)
    AWT_browser_forget_db(gb_main);
#endif // DEBUG
    GB_close(gb_main);

    ::exit(EXIT_SUCCESS);
}

void ED4_quit_editor(AW_window *aww) {
    ED4_LocalWinContext uses(aww); // @@@ dont use context here

    if (ED4_ROOT->first_window == current_ed4w()) { // quit button has been pressed in first window
        ED4_exit();
    }
    // case : in another window close has been pressed
    current_aww()->hide();
    current_ed4w()->is_hidden = true;
}

static int timer_calls           = 0;
static int timer_calls_triggered = 0;

static unsigned ED4_timer(AW_root *) {
    timer_calls++;

#if defined(TRACE_REFRESH)
    fprintf(stderr, "ED4_timer\n"); fflush(stderr);
#endif
    // get all changes from server
    GB_begin_transaction(GLOBAL_gb_main);
    GB_tell_server_dont_wait(GLOBAL_gb_main);
    GB_commit_transaction(GLOBAL_gb_main);

    ED4_ROOT->refresh_all_windows(0);

    if (timer_calls == timer_calls_triggered) {
        timer_calls_triggered++;
        return 2000; // trigger callback after 2s
    }
    return 0; // do not trigger callback
}

void ED4_trigger_instant_refresh() {
#if defined(TRACE_REFRESH)
    fprintf(stderr, "ED4_trigger_instant_refresh\n"); fflush(stderr);
#endif
    timer_calls_triggered++;
    ED4_ROOT->aw_root->add_timed_callback(1, makeTimedCallback(ED4_timer)); // trigger instant callback
}
void ED4_request_full_refresh() {
    ED4_ROOT->main_manager->request_refresh();
}
void ED4_request_full_instant_refresh() {
    ED4_request_full_refresh();
    ED4_trigger_instant_refresh();
}

void ED4_request_relayout() {
    ED4_expose_recalculations();
    ED4_ROOT->main_manager->request_resize();
    ED4_trigger_instant_refresh();
}

#define SIGNIFICANT_FIELD_CHARS 30 // length used to compare field contents (in createGroupFromSelected)

static void createGroupFromSelected(GB_CSTR group_name, GB_CSTR field_name, GB_CSTR field_content) {
    // creates a new group named group_name
    // if field_name==0 -> all selected species & subgroups are moved to this new group
    // if field_name!=0 -> all selected species containing field_content in field field_name are moved to this new group

    ED4_group_manager *new_group_manager = NULL;
    ED4_ROOT->main_manager->create_group(&new_group_manager, group_name);

    {
        ED4_multi_species_manager *multi_species_manager = ED4_ROOT->top_area_man->get_multi_species_manager();

        new_group_manager->extension.position[Y_POS] = 2;
        ED4_base::touch_world_cache();
        multi_species_manager->children->append_member(new_group_manager);
        new_group_manager->parent = (ED4_manager *) multi_species_manager;
    }

    ED4_multi_species_manager *new_multi_species_manager = new_group_manager->get_multi_species_manager();
    bool lookingForNoContent = field_content==0 || field_content[0]==0;

    ED4_selected_elem *list_elem = ED4_ROOT->selected_objects->head();
    while (list_elem) {
        ED4_base *object = list_elem->elem()->object;
        object = object->get_parent(ED4_L_SPECIES);

        bool move_object = true;
        if (object->is_consensus_manager()) {
            object = object->get_parent(ED4_L_GROUP);
            if (field_name) move_object = false; // don't move groups if moving by field_name
        }
        else {
            e4_assert(object->is_species_manager());
            if (field_name) {
                GBDATA *gb_species = object->get_species_pointer();
                GBDATA *gb_field = GB_search(gb_species, field_name, GB_FIND);

                move_object = lookingForNoContent;
                if (gb_field) { // field was found
                    char *found_content = GB_read_as_string(gb_field);
                    if (found_content) {
                        move_object = strncmp(found_content, field_content, SIGNIFICANT_FIELD_CHARS)==0;
                        free(found_content);
                    }
                }
            }
        }

        if (move_object) {
            ED4_base *base = object->get_parent(ED4_L_MULTI_SPECIES);
            if (base && base->is_multi_species_manager()) {
                ED4_multi_species_manager *old_multi = base->to_multi_species_manager();
                old_multi->invalidate_species_counters();
            }
            
            object->parent->children->remove_member(object);
            new_multi_species_manager->children->append_member(object);

            object->parent = (ED4_manager *)new_multi_species_manager;
            object->set_width();
        }

        list_elem = list_elem->next();
    }

    new_group_manager->create_consensus(new_group_manager, NULL);
    new_multi_species_manager->invalidate_species_counters();
    
    {
        ED4_bracket_terminal *bracket = new_group_manager->get_defined_level(ED4_L_BRACKET)->to_bracket_terminal();
        if (bracket) bracket->fold();
    }

    new_multi_species_manager->resize_requested_by_child();
}

static void group_species(int use_field, AW_window *use_as_main_window) {
    GB_ERROR error = 0;
    GB_push_transaction(GLOBAL_gb_main);

    ED4_LocalWinContext uses(use_as_main_window);

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
        char   *field_name   = ED4_ROOT->aw_root->awar(AWAR_FIELD_CHOSEN)->read_string();
        char   *doneContents = strdup(";");
        size_t  doneLen      = 1;

        int tryAgain     = 1;
        int foundField   = 0;
        int foundSpecies = 0;

        while (tryAgain && !error) {
            tryAgain = 0;
            ED4_selected_elem *list_elem = ED4_ROOT->selected_objects->head();
            while (list_elem && !error) {
                ED4_base *object = list_elem->elem()->object;
                object = object->get_parent(ED4_L_SPECIES);
                if (!object->is_consensus_manager()) {
                    GBDATA *gb_species = object->get_species_pointer();
                    GBDATA *gb_field   = NULL;

                    if (gb_species) {
                        foundSpecies = 1;
                        gb_field     = GB_search(gb_species, field_name, GB_FIND);
                    }

                    if (gb_field) {
                        char *field_content = GB_read_as_string(gb_field);
                        if (field_content) {
                            size_t field_content_len = strlen(field_content);

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

                                int   newlen  = doneLen + field_content_len + 1;
                                char *newDone = (char*)malloc(newlen+1);

                                GBS_global_string_to_buffer(newDone, newlen+1, "%s%s;", doneContents, field_content);
                                freeset(doneContents, newDone);
                                doneLen = newlen;
                            }
                            free(field_content);
                        }
                        else {
                            error = "Incompatible field type";
                        }
                    }
                    else {
                        if (GB_have_error()) error = GB_await_error();
                    }
                }
                list_elem = list_elem->next();
            }
        }

        if (!foundSpecies) {
            e4_assert(!error);
            error = "Please select some species in order to insert them into new groups";
        }
        else if (!foundField) {
            error = GBS_global_string("Field not found: '%s'%s", field_name, error ? GBS_global_string(" (Reason: %s)", error) : "");
        }

        free(doneContents);
        free(field_name);
    }

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
}

static void group_species_by_field_content(AW_window*, AW_window *use_as_main_window, AW_window *window_to_hide) {
    group_species(1, use_as_main_window);
    window_to_hide->hide();
}

static AW_window *create_group_species_by_field_window(AW_root *aw_root, AW_window *use_as_main_window) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, "CREATE_GROUP_USING_FIELD_CONTENT", "Create groups using field");
    aws->auto_space(10, 10);

    aws->button_length(10);
    aws->at_newline();

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("group_by_field.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at_newline();
    aws->label("Use content of field");
    create_itemfield_selection_button(aws, FieldSelDef(AWAR_FIELD_CHOSEN, GLOBAL_gb_main, SPECIES_get_selector(), FIELD_FILTER_STRING_READABLE), NULL);

    aws->at_newline();
    aws->callback(makeWindowCallback(group_species_by_field_content, use_as_main_window, static_cast<AW_window*>(aws)));
    aws->create_autosize_button("USE_FIELD", "Group selected species by content", "");

    return aws;
}

void group_species_cb(AW_window *aww, bool use_fields) {
    if (!use_fields) {
        group_species(0, aww);
    }
    else {
        static AW_window *ask_field_window;

        if (!ask_field_window) ask_field_window = create_group_species_by_field_window(ED4_ROOT->aw_root, aww);
        ask_field_window->activate();
    }
}

static GB_ERROR ED4_load_new_config(char *name) {
    GB_ERROR error;
    GBT_config cfg(GLOBAL_gb_main, name, error);
    if (cfg.exists()) {
        ED4_ROOT->main_manager->clear_whole_background();
        ED4_calc_terminal_extentions();

        max_seq_terminal_length = 0;

        ED4_init_notFoundMessage();

        if (ED4_ROOT->selected_objects->size() > 0) {
            ED4_ROOT->deselect_all();
        }

        ED4_ROOT->remove_all_callbacks();

        ED4_ROOT->scroll_picture.scroll = 0;
        ED4_ROOT->scroll_picture.old_x  = 0;
        ED4_ROOT->scroll_picture.old_y  = 0;

        ED4_ROOT->ref_terminals.clear();

        for (ED4_window *window = ED4_ROOT->first_window; window; window=window->next) {
            window->cursor.init();
            window->aww->set_horizontal_scrollbar_position (0);
            window->aww->set_vertical_scrollbar_position (0);
        }

        ED4_ROOT->scroll_links.link_for_hor_slider = NULL;
        ED4_ROOT->scroll_links.link_for_ver_slider = NULL;
        ED4_ROOT->middle_area_man                  = NULL;
        ED4_ROOT->top_area_man                     = NULL;

        delete ED4_ROOT->main_manager;
        ED4_ROOT->main_manager = NULL;
        delete ED4_ROOT->ecoli_ref;

        ED4_ROOT->first_window->reset_all_for_new_config();
        ED4_ROOT->create_hierarchy(cfg.get_definition(GBT_config::MIDDLE_AREA), cfg.get_definition(GBT_config::TOP_AREA));
    }

    return error;
}

static ED4_EDITMODE ED4_get_edit_mode(AW_root *root) {
    if (!root->awar(AWAR_EDIT_MODE)->read_int()) return AD_ALIGN;
    return root->awar(AWAR_INSERT_MODE)->read_int() ? AD_INSERT : AD_REPLACE;
}

void ed4_changesecurity(AW_root *root) {
    ED4_EDITMODE  mode      = ED4_get_edit_mode(root);
    const char   *awar_name = 0;

    switch (mode) {
        case AD_ALIGN:
            awar_name = AWAR_EDIT_SECURITY_LEVEL_ALIGN;
            break;
        default:
            awar_name = AWAR_EDIT_SECURITY_LEVEL_CHANGE;
    }

    ED4_ROOT->aw_root->awar(AWAR_EDIT_SECURITY_LEVEL)->map(awar_name);

    long level = ED4_ROOT->aw_root->awar(awar_name)->read_int();
    GB_change_my_security(GLOBAL_gb_main, level);
}

void ed4_change_edit_mode(AW_root *root) {
    awar_edit_mode = ED4_get_edit_mode(root);
    ed4_changesecurity(root);
}

ARB_ERROR rebuild_consensus(ED4_base *object) {
    if (object->is_consensus_manager()) {
        ED4_species_manager *spec_man = object->to_species_manager();
        spec_man->do_callbacks();

        ED4_base *sequence_data_terminal = object->search_spec_child_rek(ED4_L_SEQUENCE_STRING);
        sequence_data_terminal->request_refresh();
    }
    return NULL;
}

void ED4_new_editor_window(AW_window *aww) {
    ED4_LocalWinContext uses(aww);

    AW_device  *device;
    ED4_window *new_window = 0;

    if (ED4_ROOT->generate_window(&device, &new_window) != ED4_R_BREAK) {
        ED4_LocalWinContext now_uses(new_window);

        new_window->set_scrolled_rectangle(ED4_ROOT->scroll_links.link_for_hor_slider,
                                           ED4_ROOT->scroll_links.link_for_ver_slider,
                                           ED4_ROOT->scroll_links.link_for_hor_slider,
                                           ED4_ROOT->scroll_links.link_for_ver_slider);

        new_window->aww->show();
        new_window->update_scrolled_rectangle();
    }
}



static void ED4_start_editor_on_configuration(AW_window *aww) {
    aww->hide();

    GB_ERROR error;
    {
        char *name = aww->get_root()->awar(AWAR_EDIT_CONFIGURATION)->read_string();
        error      = ED4_load_new_config(name);
        free(name);
    }

    if (error) {
        aw_message(error);
        aww->show(); // show old window
    }
}

struct cursorpos {
    ED4_cursor& cursor;
    int screen_rel;
    int seq;

    cursorpos(ED4_window *win)
        : cursor(win->cursor),
          screen_rel(cursor.get_screen_relative_pos()),
          seq(cursor.get_sequence_pos())
    {}
    cursorpos(const cursorpos& other)
        : cursor(other.cursor),
          screen_rel(other.screen_rel),
          seq(other.seq)
    {}
    DECLARE_ASSIGNMENT_OPERATOR(cursorpos);
};


void ED4_compression_changed_cb(AW_root *awr) {
    ED4_remap_mode mode    = (ED4_remap_mode)awr->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->read_int();
    int            percent = awr->awar(ED4_AWAR_COMPRESS_SEQUENCE_PERCENT)->read_int();
    GB_transaction ta(GLOBAL_gb_main);

    if (ED4_ROOT->root_group_man) {
        vector<cursorpos> pos;

        for (ED4_window *win = ED4_ROOT->first_window; win; win = win->next) {
            pos.push_back(cursorpos(win));
        }

        ED4_ROOT->root_group_man->remap()->set_mode(mode, percent);
        ED4_expose_recalculations();

        for (vector<cursorpos>::const_iterator i = pos.begin(); i != pos.end(); ++i) {
            ED4_cursor&  cursor = const_cast<ED4_cursor&>(i->cursor);
            ED4_window  *win    = cursor.window();

            win->update_scrolled_rectangle(); // @@@ needed ? 

            cursor.jump_sequence_pos(i->seq, ED4_JUMP_KEEP_POSITION);
            cursor.set_screen_relative_pos(i->screen_rel);
        }

        ED4_request_full_instant_refresh();
    }
}

void ED4_compression_toggle_changed_cb(AW_root *root, bool hideChanged) {
    int gaps = root->awar(ED4_AWAR_COMPRESS_SEQUENCE_GAPS)->read_int();
    int hide = root->awar(ED4_AWAR_COMPRESS_SEQUENCE_HIDE)->read_int();

    ED4_remap_mode mode = ED4_remap_mode(root->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->read_int()); // @@@ mode is overwritten below

    if (hideChanged) {
        // ED4_AWAR_COMPRESS_SEQUENCE_HIDE changed
        if (hide!=0 && gaps!=2) {
            root->awar(ED4_AWAR_COMPRESS_SEQUENCE_GAPS)->write_int(2);
            return;
        }
    }
    else {
        // ED4_AWAR_COMPRESS_SEQUENCE_GAPS changed
        if (gaps!=2 && hide!=0) {
            root->awar(ED4_AWAR_COMPRESS_SEQUENCE_HIDE)->write_int(0);
            return;
        }
    }

    mode = ED4_RM_NONE;
    switch (gaps) {
        case 0: mode = ED4_RM_NONE; break;
        case 1: mode = ED4_RM_DYNAMIC_GAPS; break;
        case 2: {
            switch (hide) {
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

static AWT_config_mapping_def editor_options_config_mapping[] = {
    { ED4_AWAR_COMPRESS_SEQUENCE_GAPS,    "compressgaps" },
    { ED4_AWAR_COMPRESS_SEQUENCE_HIDE,    "hidenucs" },
    { ED4_AWAR_COMPRESS_SEQUENCE_PERCENT, "hidepercent" },
    { AWAR_EDIT_HELIX_SPACING,            "helixspacing" },
    { AWAR_EDIT_TERMINAL_SPACING,         "terminalspacing" },
    { ED4_AWAR_SCROLL_SPEED_X,            "scrollspeedx" },
    { ED4_AWAR_SCROLL_SPEED_Y,            "scrollspeedy" },
    { ED4_AWAR_SCROLL_MARGIN,             "scrollmargin" },
    { ED4_AWAR_GAP_CHARS,                 "gapchars" },
    { ED4_AWAR_DIGITS_AS_REPEAT,          "digitsasrepeat" },
    { ED4_AWAR_FAST_CURSOR_JUMP,          "fastcursorjump" },
    { ED4_AWAR_ANNOUNCE_CHECKSUM_CHANGES, "announcechecksumchanges" },

    { 0, 0 }
};

AW_window *ED4_create_editor_options_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "EDIT4_PROPS", "EDIT4 Options");
    aws->load_xfig("edit4/options.fig");

    aws->auto_space(5, 5);

    const int SCALEDCOLUMNS = 4;
    const int SCALERLEN     = 200;

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("e4_options.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

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
    aws->create_input_field_with_scaler(ED4_AWAR_COMPRESS_SEQUENCE_PERCENT, SCALEDCOLUMNS, SCALERLEN, AW_SCALER_LINEAR);

    //  --------------
    //      Layout

    aws->at("seq_helix"); aws->create_input_field_with_scaler(AWAR_EDIT_HELIX_SPACING,    SCALEDCOLUMNS, SCALERLEN, AW_SCALER_EXP_CENTER);
    aws->at("seq_seq");   aws->create_input_field_with_scaler(AWAR_EDIT_TERMINAL_SPACING, SCALEDCOLUMNS, SCALERLEN, AW_SCALER_EXP_CENTER);

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
    aws->create_toggle(ED4_AWAR_DIGITS_AS_REPEAT);

    aws->at("fast");
    aws->create_toggle(ED4_AWAR_FAST_CURSOR_JUMP);

    aws->at("checksum");
    aws->create_toggle(ED4_AWAR_ANNOUNCE_CHECKSUM_CHANGES);

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "options", editor_options_config_mapping);

    return aws;
}

static AWT_config_mapping_def consensus_config_mapping[] = {
    { ED4_AWAR_CONSENSUS_COUNTGAPS,   CONSENSUS_CONFIG_COUNTGAPS },
    { ED4_AWAR_CONSENSUS_GAPBOUND,    CONSENSUS_CONFIG_GAPBOUND },
    { ED4_AWAR_CONSENSUS_GROUP,       CONSENSUS_CONFIG_GROUP },
    { ED4_AWAR_CONSENSUS_CONSIDBOUND, CONSENSUS_CONFIG_CONSIDBOUND },
    { ED4_AWAR_CONSENSUS_UPPER,       CONSENSUS_CONFIG_UPPER },
    { ED4_AWAR_CONSENSUS_LOWER,       CONSENSUS_CONFIG_LOWER },

    { 0, 0 }
};

AW_window *ED4_create_consensus_definition_window(AW_root *root) {
    // keep in sync with ../NTREE/AP_consensus.cxx@AP_create_con_expert_window

    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "EDIT4_CONSENSUS_DEFm", "EDIT4 Consensus Definition");
        aws->load_xfig("edit4/consensus.fig");

        aws->auto_space(5, 5);

        const int SCALEDCOLUMNS = 3;
        const int SCALERSIZE    = 150;

        // top part of window:
        aws->button_length(9);

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("e4_consensus.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        // center part of window (same as in NTREE):
        aws->at("countgaps");
        aws->create_toggle_field(ED4_AWAR_CONSENSUS_COUNTGAPS);
        aws->insert_toggle("on", "1", 1);
        aws->insert_default_toggle("off", "0", 0);
        aws->update_toggle_field();

        aws->at("gapbound");
        aws->create_input_field_with_scaler(ED4_AWAR_CONSENSUS_GAPBOUND, SCALEDCOLUMNS, SCALERSIZE, AW_SCALER_LINEAR);

        aws->at("group");
        aws->create_toggle_field(ED4_AWAR_CONSENSUS_GROUP);
        aws->insert_toggle("on", "1", 1);
        aws->insert_default_toggle("off", "0", 0);
        aws->update_toggle_field();

        aws->at("considbound");
        aws->create_input_field_with_scaler(ED4_AWAR_CONSENSUS_CONSIDBOUND, SCALEDCOLUMNS, SCALERSIZE, AW_SCALER_LINEAR);

        aws->at("showgroups");
        aws->callback(AWT_create_IUPAC_info_window);
        aws->create_autosize_button("SHOW_IUPAC", "Show IUPAC groups", "S");

        aws->at("upper");
        aws->create_input_field_with_scaler(ED4_AWAR_CONSENSUS_UPPER, SCALEDCOLUMNS, SCALERSIZE, AW_SCALER_LINEAR);

        aws->at("lower");
        aws->create_input_field_with_scaler(ED4_AWAR_CONSENSUS_LOWER, SCALEDCOLUMNS, SCALERSIZE, AW_SCALER_LINEAR);

        // bottom part of window:
        aws->at("show");
        aws->label("Display consensus?");
        aws->create_toggle(ED4_AWAR_CONSENSUS_SHOW);

        aws->at("config");
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, CONSENSUS_CONFIG_ID, consensus_config_mapping);
    }

    return aws;
}

static void consensus_upper_lower_changed_cb(AW_root *awr, bool upper_changed) {
    AW_awar *awar_lower = awr->awar(ED4_AWAR_CONSENSUS_LOWER);
    AW_awar *awar_upper = awr->awar(ED4_AWAR_CONSENSUS_UPPER);

    int lower = awar_lower->read_int();
    int upper = awar_upper->read_int();

    if (upper<lower) {
        if (upper_changed) awar_lower->write_int(upper);
        else               awar_upper->write_int(lower);
    }
    ED4_consensus_definition_changed(awr);
}

void ED4_create_consensus_awars(AW_root *aw_root) {
    GB_transaction ta(GLOBAL_gb_main);

    aw_root->awar_int(ED4_AWAR_CONSENSUS_COUNTGAPS,   1) ->add_callback(ED4_consensus_definition_changed);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_GROUP,       1) ->add_callback(ED4_consensus_definition_changed);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_GAPBOUND,    60)->set_minmax(0, 100)->add_callback(ED4_consensus_definition_changed);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_CONSIDBOUND, 30)->set_minmax(0, 100)->add_callback(ED4_consensus_definition_changed);
    aw_root->awar_int(ED4_AWAR_CONSENSUS_UPPER,       95)->set_minmax(0, 100)->add_callback(makeRootCallback(consensus_upper_lower_changed_cb, true));
    aw_root->awar_int(ED4_AWAR_CONSENSUS_LOWER,       70)->set_minmax(0, 100)->add_callback(makeRootCallback(consensus_upper_lower_changed_cb, false));

    AW_awar *cons_show = aw_root->awar_int(ED4_AWAR_CONSENSUS_SHOW, 1);

    cons_show->write_int(1);
    cons_show->add_callback(ED4_consensus_display_changed);
}

void ED4_reloadConfiguration(AW_window *aww) {
    ED4_start_editor_on_configuration(aww);
}

AW_window *ED4_create_loadConfiguration_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "LOAD_CONFIGURATION", "Load existing configuration");
        aws->load_xfig("edit4/load_config.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("species_configs_saveload.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("confs");
        awt_create_CONFIG_selection_list(GLOBAL_gb_main, aws, AWAR_EDIT_CONFIGURATION, false);

        aws->at("go");
        aws->callback(ED4_start_editor_on_configuration);
        aws->create_button("LOAD", "LOAD");

        aws->window_fit();
    }
    return aws;
}

void ED4_saveConfiguration(AW_window *aww, bool hide_aww) {
    if (hide_aww) aww->hide();

    char *name = aww->get_root()->awar(AWAR_EDIT_CONFIGURATION)->read_string();
    ED4_ROOT->database->save_current_config(name);
    free(name);
}

AW_window *ED4_create_saveConfigurationAs_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "SAVE_CONFIGURATION", "Save current configuration as");
        aws->load_xfig("edit4/save_config.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE");

        aws->at("help");
        aws->callback(makeHelpCallback("species_configs_saveload.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("save");
        aws->create_input_field(AWAR_EDIT_CONFIGURATION);

        aws->at("confs");
        awt_create_CONFIG_selection_list(GLOBAL_gb_main, aws, AWAR_EDIT_CONFIGURATION, false);

        aws->at("go");
        aws->callback(makeWindowCallback(ED4_saveConfiguration, true));
        aws->create_button("SAVE", "SAVE");
    }
    return aws;
}

static char *filter_loadable_SAIs(GBDATA *gb_sai) {
    GBDATA *gb_ali = GB_search(gb_sai, ED4_ROOT->alignment_name, GB_FIND);
    if (gb_ali) {
        GBDATA *gb_data = GB_search(gb_ali, "data", GB_FIND);
        if (gb_data) {
            const char *sai_name = GBT_get_name(gb_sai);
            if (!ED4_find_SAI_name_terminal(sai_name)) { // if not loaded yet
                return strdup(sai_name);
            }
        }
    }
    return NULL;
}

AW_window *ED4_create_loadSAI_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "LOAD_SAI", "Load additional SAI");
        aws->load_xfig("edit4/load_sai.fig");

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE");

        aws->at("help");
        aws->callback(makeHelpCallback("e4_get_species.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("sai");
        awt_create_SAI_selection_list(GLOBAL_gb_main, aws, AWAR_SAI_NAME, false, makeSaiSelectionlistFilterCallback(filter_loadable_SAIs));
        ED4_ROOT->loadable_SAIs = LSAI_UPTODATE;

        aws->at("go");
        aws->callback(ED4_get_and_jump_to_selected_SAI);
        aws->create_button("LOAD", "LOAD");
    }
    return aws;
}

static GB_ERROR createDataFromConsensus(GBDATA *gb_species, ED4_group_manager *group_man) {
    GB_ERROR error = 0;

    int   len;
    char *consensus = group_man->build_consensus_string(&len);

    char *equal_to = ED4_ROOT->aw_root->awar(ED4_AWAR_CREATE_FROM_CONS_REPL_EQUAL)->read_string();
    char *point_to = ED4_ROOT->aw_root->awar(ED4_AWAR_CREATE_FROM_CONS_REPL_POINT)->read_string();
    int   allUpper = ED4_ROOT->aw_root->awar(ED4_AWAR_CREATE_FROM_CONS_ALL_UPPER)->read_int();

    for (int p=0; p<len; p++) {
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
        for (int p=0; p<len; p++) {
            if (!ED4_is_gap_character(consensus[p])) break;
            consensus[p] = '.';
        }
        for (int p=len-1; p>=0; p--) {
            if (!ED4_is_gap_character(consensus[p])) break;
            consensus[p] = '.';
        }
    }

    GB_CSTR ali = GBT_get_default_alignment(GLOBAL_gb_main);
    GBDATA *gb_ali = GB_search(gb_species, ali, GB_DB);
    if (gb_ali) {
        GBDATA *gb_data = GB_search(gb_ali, "data", GB_STRING);
        error = GB_write_pntr(gb_data, consensus, len+1, len);
    }
    else {
        error = GB_export_errorf("Can't find alignment '%s'", ali);
    }
    free(consensus);
    return error;
}

// --------------------------------------------------------------------------------

struct SpeciesMergeList {
    GBDATA *species;
    char   *species_name;

    SpeciesMergeList *next;
};

static ARB_ERROR add_species_to_merge_list(ED4_base *base, SpeciesMergeList **smlp, GBDATA *gb_species_data) {
    GB_ERROR error = NULL;

    if (base->is_species_name_terminal()) {
        ED4_species_name_terminal *name_term = base->to_species_name_terminal();

        if (!name_term->inside_consensus_manager()) {
            char   *species_name    = name_term->resolve_pointer_to_string_copy();
            GBDATA *gb_species      = GBT_find_species_rel_species_data(gb_species_data, species_name);

            if (gb_species) {
                SpeciesMergeList *sml = new SpeciesMergeList;

                sml->species      = gb_species;
                sml->species_name = strdup(species_name);
                sml->next         = *smlp;
                *smlp             = sml;
            }
            else {
                error = GB_append_exportedError(GBS_global_string("can't find species '%s'", species_name));
            }

            free(species_name);
        }
    }
    return error;
}
static int SpeciesMergeListLength(SpeciesMergeList *sml)
{
    int length = 0;

    while (sml) {
        length++;
        sml = sml->next;
    }

    return length;
}
static void freeSpeciesMergeList(SpeciesMergeList *sml) {
    while (sml) {
        free(sml->species_name);
        freeset(sml, sml->next);
    }
}

// --------------------------------------------------------------------------------

inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data) {
    return GBT_find_species_rel_species_data(gb_species_data, short_name)==0;
}


static void create_new_species(AW_window *, SpeciesCreationMode creation_mode) {
    char      *new_species_full_name = ED4_ROOT->aw_root->awar(ED4_AWAR_SPECIES_TO_CREATE)->read_string(); // this contains the full_name now!
    ARB_ERROR  error                 = 0;

    e4_assert(creation_mode>=0 && creation_mode<=2);

    if (!new_species_full_name || new_species_full_name[0]==0) {
        error = "Please enter a full_name for the new species";
    }
    else {
        ED4_MostRecentWinContext context;

        error = GB_begin_transaction(GLOBAL_gb_main);

        GBDATA *gb_species_data  = GBT_get_species_data(GLOBAL_gb_main);
        char   *new_species_name = 0;
        char   *acc              = 0;
        char   *addid            = 0;

        enum e_dataSource { MERGE_FIELDS, COPY_FIELDS } dataSource = (enum e_dataSource)ED4_ROOT->aw_root ->awar(ED4_AWAR_CREATE_FROM_CONS_DATA_SOURCE)->read_int();
        enum { NOWHERE, ON_SPECIES, ON_CONSENSUS } where_we_are = NOWHERE;
        ED4_terminal *cursor_terminal = 0;

        if (!error) {
            if (creation_mode==CREATE_FROM_CONSENSUS || creation_mode==COPY_SPECIES) {
                ED4_cursor *cursor = &current_cursor();

                if (cursor->owner_of_cursor) {
                    cursor_terminal = cursor->owner_of_cursor;
                    where_we_are    = cursor_terminal->is_consensus_terminal() ? ON_CONSENSUS : ON_SPECIES;
                }
            }

            if (creation_mode==COPY_SPECIES || (creation_mode==CREATE_FROM_CONSENSUS && dataSource==COPY_FIELDS)) {
                if (where_we_are==ON_SPECIES) {
                    ED4_species_name_terminal *spec_name   = cursor_terminal->to_sequence_terminal()->corresponding_species_name_terminal();
                    const char                *source_name = spec_name->resolve_pointer_to_char_pntr();
                    GBDATA                    *gb_source   = GBT_find_species_rel_species_data(gb_species_data, source_name);

                    if (!gb_source) error = GBS_global_string("No such species: '%s'", source_name);
                    else {
                        GBDATA *gb_acc  = GB_search(gb_source, "acc", GB_FIND);
                        if (gb_acc) acc = GB_read_string(gb_acc); // if has accession

                        const char *add_field = AW_get_nameserver_addid(GLOBAL_gb_main);
                        GBDATA     *gb_addid  = add_field[0] ? GB_search(gb_source, add_field, GB_FIND) : 0;
                        if (gb_addid) addid   = GB_read_as_string(gb_addid);
                    }
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
                error = AWTC_generate_one_name(GLOBAL_gb_main, new_species_full_name, acc, addid, new_species_name);
                if (!error) {   // name was created
                    if (!nameIsUnique(new_species_name, gb_species_data)) {
                        if (!existingNames) existingNames = new UniqueNameDetector(gb_species_data);
                        freeset(new_species_name, AWTC_makeUniqueShortName(new_species_name, *existingNames));
                        if (!new_species_name) error = GB_await_error();
                    }
                }
            }

            if (error) {        // try to make a random name
                const char *msg = GBS_global_string("%s\nGenerating a random name instead.", error.deliver());
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
                        SpeciesMergeList  *sml       = 0;  // list of species in group

                        error = group_man->route_down_hierarchy(makeED4_route_cb(add_species_to_merge_list, &sml, gb_species_data));
                        if (!error && !sml) {
                            error = "Please choose a none empty group!";
                        }

                        GBDATA *gb_new_species = 0;
                        if (!error) {
                            GBDATA *gb_source = sml->species;
                            gb_new_species = GB_create_container(gb_species_data, "species");
                            error = GB_copy(gb_new_species, gb_source); // copy first found species to create a new species
                        }
                        if (!error) error = GBT_write_string(gb_new_species, "name", new_species_name); // insert new 'name'
                        if (!error) error = GBT_write_string(gb_new_species, "full_name", new_species_full_name); // insert new 'full_name'
                        if (!error) error = createDataFromConsensus(gb_new_species, group_man); // insert consensus as 'data'

                        if (!error) {
                            char             *doneFields = strdup(";name;full_name;"); // all fields which are already merged
                            int               doneLen    = strlen(doneFields);
                            SpeciesMergeList *sl         = sml;
                            int               sl_length  = SpeciesMergeListLength(sml);
                            int              *fieldStat  = new int[sl_length];         // 0 = not used yet ; -1 = don't has field ; 1..n = field content, same number means same content

                            arb_progress progress("Merging fields", sl_length);

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

                                        GB_TYPES type = GB_read_type(gb_field);
                                        if (type==GB_STRING) { // we only merge string fields
                                            int i;
                                            int doneSpecies = 0;
                                            int nextStat = 1;

                                            for (i=0; i<sl_length; i++) { // clear field status
                                                fieldStat[i] = 0;
                                            }

                                            while (doneSpecies<sl_length) { // since all species in list were handled
                                                SpeciesMergeList *sl2 = sml;
                                                i = 0;

                                                while (sl2) {
                                                    if (fieldStat[i]==0) {
                                                        gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                                        if (gb_field) {
                                                            char *content = GB_read_as_string(gb_field);
                                                            SpeciesMergeList *sl3 = sl2->next;

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
                                                    SpeciesMergeList *sl2 = sml;

                                                    while (sl2) {
                                                        gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                                        if (gb_field) {
                                                            new_content = GB_read_as_string(gb_field);
                                                            new_content_len = strlen(new_content); // @@@ new_content_len never used
                                                            break;
                                                        }
                                                        sl2 = sl2->next;
                                                    }
                                                }
                                                else { // different field contents
                                                    int currStat;
                                                    for (currStat=1; currStat<nextStat; currStat++) {
                                                        int names_len = 1; // open bracket
                                                        SpeciesMergeList *sl2 = sml;
                                                        i = 0;
                                                        char *content = 0;

                                                        while (sl2) {
                                                            if (fieldStat[i]==currStat) {
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
                                                            if (fieldStat[i]==currStat) {
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
                                progress.inc_and_check_user_abort(error);
                            }
                            free(doneFields);
                            delete [] fieldStat;
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
                        GBDATA *gb_new_species = GB_create_container(gb_species_data, "species");
                        error                  = GB_copy(gb_new_species, gb_source);
                        if (!error) error      = GBT_write_string(gb_new_species, "name", new_species_name);
                        if (!error) error      = GBT_write_string(gb_new_species, "full_name", new_species_full_name); // insert new 'full_name'
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

            error = GB_end_transaction(GLOBAL_gb_main, error);
            if (!error) ED4_get_and_jump_to_species(new_species_name);
        }
        else {
            GB_abort_transaction(GLOBAL_gb_main);
        }

        free(addid);
        free(acc);
        free(new_species_name);
    }

    aw_message_if(error);
    free(new_species_full_name);
}

AW_window *ED4_create_new_seq_window(AW_root *root, SpeciesCreationMode creation_mode) {
    e4_assert(valid(creation_mode));

    AW_window_simple *aws = new AW_window_simple;
    switch (creation_mode) {
        case CREATE_NEW_SPECIES:    aws->init(root, "create_species",                "Create species");                break;
        case CREATE_FROM_CONSENSUS: aws->init(root, "create_species_from_consensus", "Create species from consensus"); break;
        case COPY_SPECIES:          aws->init(root, "copy_species",                  "Copy current species");          break;
    }
    
    if (creation_mode==CREATE_FROM_CONSENSUS) {
        aws->load_xfig("edit4/create_seq_fc.fig");
    }
    else {
        aws->load_xfig("edit4/create_seq.fig");
    }

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the FULL_NAME\nof the new species");

    aws->at("input");
    aws->create_input_field(ED4_AWAR_SPECIES_TO_CREATE, 30);

    aws->at("ok");
    aws->callback(makeWindowCallback(create_new_species, creation_mode));
    aws->create_button("GO", "GO", "g");

    if (creation_mode==CREATE_FROM_CONSENSUS) {
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
        aws->label("Other fields");
        aws->create_option_menu(ED4_AWAR_CREATE_FROM_CONS_DATA_SOURCE, true);
        aws->insert_default_option("Merge from all in group", "", 0);
        aws->insert_option("Copy from current species", "", 1);
        aws->update_option_menu();
    }

    return aws;
}

