#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <aw_keysym.hxx>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_awars.hxx>
#include <aw_window.hxx>
#include <AW_helix.hxx>

#include "ed4_class.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_tools.hxx"
#include "ed4_awars.hxx"
#include "ed4_secedit.hxx"

/* --------------------------------------------------------------------------------
   CursorShape
   -------------------------------------------------------------------------------- */

#define MAXLINES  30
#define MAXPOINTS (2*MAXLINES)

class ED4_CursorShape {
    int points;
    int xpos[MAXPOINTS];
    int ypos[MAXPOINTS];

    int lines;
    int start[MAXLINES];
    int end[MAXLINES];

    bool reverse;

    int point(int x, int y) {
        e4_assert(points<MAXPOINTS);
        if (reverse) {
            xpos[points] = -x-1;
        }
        else {
            xpos[points] = x;
        }
        ypos[points] = y;
        return points++;
    }
    int line(int p1, int p2) {
        e4_assert(p1>=0 && p1<points);
        e4_assert(p2>=0 && p2<points);

        e4_assert(lines<MAXLINES);
        start[lines] = p1;
        end[lines] = p2;

        return lines++;
    }
    int horizontal_line(int x, int y, int half_width) {
        return line(point(x-half_width, y), point(x+half_width, y));
    }

public:

    ED4_CursorShape(ED4_CursorType type, /*int x, int y, */int term_height, int character_width, bool reverse_cursor);
    ~ED4_CursorShape() {}

    void draw(AW_device *device, int x, int y) const {
        e4_assert(lines);
        for (int l=0; l<lines; l++) {
            int p1 = start[l];
            int p2 = end[l];
            device->line(ED4_G_CURSOR, xpos[p1]+x, ypos[p1]+y, xpos[p2]+x, ypos[p2]+y, 1, 0, 0);
        }
        device->flush();
    }

    void get_bounding_box(int x, int y, int &xmin, int &ymin, int &xmax, int &ymax) const {
        e4_assert(points);
        xmin = ymin = INT_MAX;
        xmax = ymax = INT_MIN;
        for (int p=0; p<points; p++) {
            if (xpos[p]<xmin) 		{ xmin = xpos[p]; }
            else if (xpos[p]>xmax) 	{ xmax = xpos[p]; }
            if (ypos[p]<ymin) 		{ ymin = ypos[p]; }
            else if (ypos[p]>ymax) 	{ ymax = ypos[p]; }
        }

        xmin += x; xmax +=x;
        ymin += y; ymax +=y;
    }


};

ED4_CursorShape::ED4_CursorShape(ED4_CursorType typ, /*int x, int y, */int term_height, int character_width, bool reverse_cursor)
{
    lines    = 0;
    points   = 0;
    reverse  = reverse_cursor;

    int x = 0;
    int y = 0;

    switch (typ) {
#define UPPER_OFFSET	(-1)
#define LOWER_OFFSET 	(term_height-1)
#define CWIDTH 		(character_width)

        case ED4_RIGHT_ORIENTED_CURSOR_THIN:
        case ED4_RIGHT_ORIENTED_CURSOR: {
            // --CWIDTH--
            //
            // 0--------1 		UPPER_OFFSET (from top)
            // |3-------4
            // ||
            // ||
            // ||
            // ||
            // ||
            // ||
            // 25			LOWER_OFFSET (from top)
            //
            //
            //  x

            if (typ == ED4_RIGHT_ORIENTED_CURSOR) { // non-thin version
                int p0 = point(x-1, y+UPPER_OFFSET);

                line(p0, point(x+CWIDTH-1, y+UPPER_OFFSET)); // 0->1
                line(p0, point(x-1, y+LOWER_OFFSET)); // 0->2
            }

            int p3 = point(x, y+UPPER_OFFSET+1);
                
            line(p3, point(x+CWIDTH-1, y+UPPER_OFFSET+1)); // 3->4
            line(p3, point(x, y+LOWER_OFFSET)); // 3->5

            break;

#undef UPPER_OFFSET
#undef LOWER_OFFSET
#undef CWIDTH

        }
        case ED4_TRADITIONAL_CURSOR_CONNECTED:
        case ED4_TRADITIONAL_CURSOR_BOTTOM:
        case ED4_TRADITIONAL_CURSOR: {
            int small_version = (term_height <= 10) ? 1 : 0;


#define UPPER_OFFSET	0
#define LOWER_OFFSET 	(term_height-1)
#define CWIDTH 		3

            //  -----2*CWIDTH----
            //
            //	0---------------1 	UPPER_OFFSET (from top)
            //	   4---------5
            //	      8---9
            //         C/C
            //
            //	       D/D 
            //	      A---B
            //	   6---------7
            //	2---------------3	LOWER_OFFSET (from top)

            bool draw_upper = typ != ED4_TRADITIONAL_CURSOR_BOTTOM;

            if (draw_upper) horizontal_line(x, y+UPPER_OFFSET, CWIDTH-small_version); // 0/1
            horizontal_line(x, y+LOWER_OFFSET, CWIDTH-small_version); // 2/3

            if (draw_upper) horizontal_line(x, y+UPPER_OFFSET+1, (CWIDTH*2)/3-small_version); // 4/5
            horizontal_line(x, y+LOWER_OFFSET-1, (CWIDTH*2)/3-small_version); // 6/7

            if (!small_version) {
                if (draw_upper) horizontal_line(x, y+UPPER_OFFSET+2, CWIDTH/3); // 8/9
                horizontal_line(x, y+LOWER_OFFSET-2, CWIDTH/3); // A/B
            }

            int pu = point(x, y+UPPER_OFFSET+3-small_version);
            int pl = point(x, y+LOWER_OFFSET-3+small_version);

            if (draw_upper) line(pu,pu);    // C/C
            line(pl,pl);    // D/D

            if (typ == ED4_TRADITIONAL_CURSOR_CONNECTED) {
                int pu2 = point(x, y+UPPER_OFFSET+5-small_version);
                int pl2 = point(x, y+LOWER_OFFSET-5+small_version);
                line(pu2, pl2); 
            }

            break;

#undef UPPER_OFFSET
#undef LOWER_OFFSET
#undef CWIDTH

        }
        case ED4_FUCKING_BIG_CURSOR: {

#define OUTER_HEIGHT	(term_height/4)
#define INNER_HEIGHT	(term_height-1)
#define CWIDTH	    	12
#define STEPS 		6
#define THICKNESS	2

#if (2*STEPS+THICKNESS > MAXLINES)
#error Bad definitions!
#endif

            //		2		3 		|
            //		  \	      /			| OUTER_HEIGHT
            //		     \     /			|
            //			0  		|       |
            //			|		| INNER_HEIGHT
            //			|		|
            //			1		|
            //		      /    \                    --
            //		   /	      \                 --
            //		4		5
            //
            //          -----------------
            //                2*WIDTH

            int s;
            for (s=0; s<STEPS; s++) {
                int size = ((STEPS-s)*CWIDTH)/STEPS;

                horizontal_line(x, y-OUTER_HEIGHT+s, 			size);
                horizontal_line(x, y+INNER_HEIGHT+OUTER_HEIGHT-s, 	size);
            }

            int y_upper = ypos[s-4];
            int y_lower = ypos[s-2];

            int t;
            for (t=0; t<THICKNESS; t++) {
                int xp = x-(THICKNESS/2)+t+1;
                line(point(xp, y_upper), point(xp, y_lower));
            }

            break;

#undef OUTER_HEIGHT
#undef INNER_HEIGHT
#undef CWIDTH
#undef STEPS
#undef THICKNESS

        }
        default: {
            e4_assert(0);
            break;
        }
    }
}

/* --------------------------------------------------------------------------------
   ED4_cursor
   -------------------------------------------------------------------------------- */

ED4_returncode ED4_cursor::draw_cursor(AW_pos x, AW_pos y)
{
    if (cursor_shape) {
        delete cursor_shape;
        cursor_shape = 0;
    }

    cursor_shape = new ED4_CursorShape(ctype,
                                       SEQ_TERM_TEXT_YOFFSET+2, 
                                       ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES),
                                       !awar_edit_direction);

    cursor_shape->draw(ED4_ROOT->temp_device, int(x), int(y));

    return ED4_R_OK;
}

ED4_returncode ED4_cursor::delete_cursor(AW_pos del_mark, ED4_base *target_terminal)
{
    AW_pos x, y;
    ED4_base *temp_parent;

    e4_assert(is_partly_visible());
    target_terminal->calc_world_coords( &x, &y );
    temp_parent = target_terminal->get_parent( ED4_L_SPECIES );
    if (!temp_parent || temp_parent->flag.hidden) {
        return ED4_R_BREAK;
    }
    x = del_mark;
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x, &y );

    // refresh own terminal + terminal above + terminal below

    target_terminal->set_refresh();
    target_terminal->parent->refresh_requested_by_child();

    {
        ED4_terminal *term = target_terminal->to_terminal();
        int backward = 1;

        while (1) {
            int done = 0;

            term = backward ? term->get_prev_terminal() : term->get_next_terminal();
            if (!term) {
                done = 1;
            }
            else if ((term->is_sequence_terminal()) && !term->is_in_folded_group()) {
                term->set_refresh();
                term->parent->refresh_requested_by_child();
                done = 1;
            }

            if (done) {
                if (!backward) {
                    break;
                }
                backward = 0;
                term = target_terminal->to_terminal();
            }
        }

        term = target_terminal->to_terminal();
        while(1) {
            term = term->get_next_terminal();
            if (!term) {
                break;
            }

        }
    }

    // clear rectangle where cursor is displayed

    AW_device *dev = ED4_ROOT->temp_device;
    dev->push_clip_scale();

    int xmin, xmax, ymin, ymax;

    e4_assert(cursor_shape);
    cursor_shape->get_bounding_box(int(x), int(y), xmin, ymin, xmax, ymax);

    dev->clear_part(xmin, ymin, xmax-xmin+1, ymax-ymin+1, (AW_bitset)-1);

    dev->set_top_font_overlap(1);
    dev->set_bottom_font_overlap(1);
    dev->set_left_font_overlap(1);
    dev->set_right_font_overlap(1);

#define EXPAND_SIZE 0
    if (ED4_ROOT->temp_device->reduceClipBorders(ymin-EXPAND_SIZE, ymax+1+EXPAND_SIZE, xmin-EXPAND_SIZE, xmax+1+EXPAND_SIZE)) {
        // refresh terminal
        int old_allowed_to_draw = allowed_to_draw;
        allowed_to_draw = 0;
        ED4_ROOT->refresh_window(1);
        allowed_to_draw = old_allowed_to_draw;
    }
    dev->pop_clip_scale();

    return ( ED4_R_OK );
}


ED4_cursor::ED4_cursor()
{
    init();
    allowed_to_draw = 1;
    cursor_shape    = 0;

    ctype = (ED4_CursorType)(ED4_ROOT->aw_root->awar(ED4_AWAR_CURSOR_TYPE)->read_int()%ED4_CURSOR_TYPES);
}
void ED4_cursor::init() // used by ED4_terminal-d-tor
{
    owner_of_cursor   = NULL;
    cursor_abs_x      = 0;
    screen_position   = 0;
    last_seq_position = 0;
}
ED4_cursor::~ED4_cursor()
{
    delete cursor_shape;
}

ED4_window *ED4_cursor::window() const
{
    ED4_window *window;

    for (window=ED4_ROOT->first_window; window; window=window->next) {
        if (&window->cursor==this) {
            break;
        }
    }
    e4_assert(window);
    return window;
}

int ED4_cursor::get_sequence_pos() const
{
    ED4_remap *remap = ED4_ROOT->root_group_man->remap();
    size_t max_scrpos = remap->get_max_screen_pos();

    return remap->screen_to_sequence(size_t(screen_position)<=max_scrpos ? screen_position : max_scrpos);
}

void ED4_cursor::set_to_sequence_position(int seq_pos)
{
    if (owner_of_cursor) {
        jump_centered_cursor_to_seq_pos(window()->aww, seq_pos);
    }
}

int ED4_species_manager::setCursorTo(ED4_cursor *cursor, int position, int unfold_groups)
{
    ED4_group_manager *group_manager_to_unfold = is_in_folded_group();
    int unfolded = 0;

    while (group_manager_to_unfold && unfold_groups) {
        ED4_base *base = group_manager_to_unfold->search_spec_child_rek(ED4_L_BRACKET);
        if (!base) break;
        ED4_bracket_terminal *bracket = base->to_bracket_terminal();
        group_manager_to_unfold->unfold_group(bracket->id);
        unfolded = 1;
        group_manager_to_unfold = is_in_folded_group();
    }

    if (unfolded) {
        //	ED4_ROOT->deselect_all();
        ED4_ROOT->refresh_all_windows(1);
    }

    if (!group_manager_to_unfold) {
        ED4_terminal *terminal = search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_terminal();
        if (terminal) {
            int seq_pos = position==-1 ? cursor->get_sequence_pos() : position;
            cursor->set_to_terminal(ED4_ROOT->temp_aww, terminal, seq_pos);

            //	    e4_assert(cursor->is_visible());
            return 1;
        }
    }

    return 0;
}

static void jump_to_species(ED4_species_name_terminal *name_term, int position, int unfold_groups)
{
    ED4_species_manager *species_manager = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();
    ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;
    int jumped = 0;

    if (species_manager) jumped = species_manager->setCursorTo(cursor, position, unfold_groups);
    if (!jumped) cursor->HideCursor();
}

static bool ignore_selected_species_changes_cb = false;
static bool ignore_selected_SAI_changes_cb     = false;

void ED4_select_named_sequence_terminal(const char *name, int mode) {
    // mode == 1 -> only select species
    // mode == 2 -> only select SAI

    ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(name);
    if (name_term) {
        // lookup current name term
        ED4_species_name_terminal *cursor_name_term = 0;
        {
            ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;
            if (cursor) {
                ED4_sequence_terminal *cursor_seq_term = 0;
                
                ED4_terminal *cursor_term = cursor->owner_of_cursor->to_text_terminal();;
                if (cursor_term->is_sequence_terminal()) {
                    cursor_seq_term = cursor_term->to_sequence_terminal();
                }
                else {          // user clicked into a non-sequence text terminal
                    // search for corresponding sequence terminal
                    ED4_multi_sequence_manager *seq_man = cursor_term->get_parent(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
                    if (seq_man) {
                        cursor_seq_term = seq_man->search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_sequence_terminal();
                    }
                }

                if (cursor_seq_term) {
                    cursor_name_term = cursor_seq_term->corresponding_species_name_terminal();
                }
            }
        }
        if (name_term!=cursor_name_term) { // do not change if already there!
#if defined(DEBUG) && 1
            printf("Jumping to species/SAI '%s'\n", name);
#endif
            jump_to_species(name_term, -1, 0);
        }
        else {
#if defined(DEBUG) && 1
            printf("Change ignored because same name term!\n");
#endif
        }
    }
}

void ED4_selected_SAI_changed_cb(AW_root */*aw_root*/)
{
    ED4_update_global_cursor_awars_allowed = false;

    if (!ignore_selected_SAI_changes_cb) {
        char *name = GBT_read_string(gb_main,AWAR_SAI_NAME);

        if (name && name[0]) {
#if defined(DEBUG)
            printf("Selected SAI is '%s'\n", name);
#endif // DEBUG

            ED4_select_named_sequence_terminal(name, 2);
            free(name);
        }
    }
    ignore_selected_SAI_changes_cb = false;
    ED4_update_global_cursor_awars_allowed = true;
}

void ED4_selected_species_changed_cb(AW_root */*aw_root*/)
{
    ED4_update_global_cursor_awars_allowed = false;

    if (!ignore_selected_species_changes_cb) {
        char *name = GBT_read_string(gb_main,AWAR_SPECIES_NAME);
        if (name && name[0]) {
#if defined(DEBUG) && 1
            printf("Selected species is '%s'\n", name);
#endif
            ED4_select_named_sequence_terminal(name, 1);
            free(name);
        }
    }
    else {
#if defined(DEBUG) && 1
        printf("Change ignored because ignore_selected_species_changes_cb!\n");
#endif
    }
    ignore_selected_species_changes_cb = false;
    ED4_update_global_cursor_awars_allowed = true;
}

void ED4_jump_to_current_species(AW_window */*aw*/, AW_CL)
{
    char *name = GBT_read_string(gb_main, AWAR_SPECIES_NAME);
    if (name && name[0]) {
        GB_transaction dummy(gb_main);
#if defined(DEBUG) && 1
        printf("Jump to selected species (%s)\n", name);
#endif
        ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(name);

        if (name_term) {
            jump_to_species(name_term, -1, 1);
        }
        else {
            aw_message(GBS_global_string("Species '%s' is not loaded - use GET to load it.", name));
        }
    }
    else {
        aw_message("Please select a species");
    }
}

static int multi_species_man_consensus_id_starts_with(ED4_base *base, AW_CL cl_start) { // TRUE if consensus id starts with 'start'
    ED4_multi_species_manager *ms_man = base->to_multi_species_manager();
    char *start = (char*)cl_start;
    ED4_base *consensus = ms_man->search_spec_child_rek(ED4_L_SPECIES);

    if (consensus) {
        ED4_base *consensus_name = consensus->search_spec_child_rek(ED4_L_SPECIES_NAME);

        if (consensus_name) {
            if (strncmp(consensus_name->id, start, strlen(start))==0) {
                ED4_multi_species_manager *cons_ms_man = consensus_name->get_parent(ED4_L_MULTI_SPECIES)->to_multi_species_manager();
                return cons_ms_man==ms_man;
            }
        }
    }

    return 0;
}

ED4_multi_species_manager *ED4_new_species_multi_species_manager(void) { // returns manager into which new species should be inserted
    ED4_base *manager = ED4_ROOT->root_group_man->find_first_that(ED4_L_MULTI_SPECIES, multi_species_man_consensus_id_starts_with, (AW_CL)"More Sequences");
    return manager ? manager->to_multi_species_manager() : 0;
}

void ED4_get_and_jump_to_species(GB_CSTR species_name)
{
    e4_assert(species_name && species_name[0]);

    GB_transaction dummy(gb_main);
    ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(species_name);
    int loaded = 0;

    if (!name_term) { // insert new species
        ED4_multi_species_manager *insert_into_manager = ED4_new_species_multi_species_manager();
        ED4_group_manager         *group_man           = insert_into_manager->get_parent(ED4_L_GROUP)->to_group_manager();
        char                      *string              = (char*)GB_calloc(strlen(species_name)+3, sizeof(*string));
        int                        index               = 0;
        ED4_index                  y                   = 0;
        ED4_index                  lot                 = 0;

        all_found         = 0;
        not_found_message = GBS_stropen(1000);
        GBS_strcat(not_found_message,"Species not found: ");

        sprintf(string, "-L%s", species_name);
        ED4_ROOT->database->fill_species(insert_into_manager,
                                         ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence(),
                                         string, &index, &y, 0, &lot, insert_into_manager->calc_group_depth());
        loaded = 1;

        {
            char *out_message = GBS_strclose(not_found_message);
            not_found_message = 0;
            if (all_found != 0) aw_message(out_message);
            free(out_message);
        }

        {
            GBDATA *gb_species = GBT_find_species(gb_main, species_name);
            GBDATA *gbd = GBT_read_sequence(gb_species, ED4_ROOT->alignment_name);

            if (gbd) {
                char *data = GB_read_string(gbd);
                int len = GB_read_string_count(gbd);
                group_man->table().add(data, len);
            }
        }

        name_term = ED4_find_species_name_terminal(species_name);
        if (name_term) {
            ED4_ROOT->main_manager->update_info.set_resize(1);
            ED4_ROOT->main_manager->resize_requested_by_parent();
        }
        delete string;
    }
    if (name_term) {
        jump_to_species(name_term, -1, 1);
        if (!loaded) {
            aw_message(GBS_global_string("'%s' is already loaded.", species_name));
        }
    }
    else {
        aw_message(GBS_global_string("Cannot get species '%s'", species_name));
    }
}

void ED4_get_and_jump_to_actual(AW_window *, AW_CL)
{
    char *name = GBT_read_string(gb_main, AWAR_SPECIES_NAME);
    if (name && name[0]) {
        ED4_get_and_jump_to_species(name);
        ED4_ROOT->refresh_all_windows(0);
    }
    else {
        aw_message("Please select a species");
    }
}

void ED4_get_and_jump_to_actual_from_menu(AW_window *aw, AW_CL cl, AW_CL) {
    ED4_get_and_jump_to_actual(aw, cl);
    ED4_ROOT->refresh_all_windows(0);
}

void ED4_get_marked_from_menu(AW_window *, AW_CL, AW_CL) {
#define BUFFERSIZE 1000
    GB_transaction dummy(gb_main);
    int marked = GBT_count_marked_species(gb_main);

    if (marked) {
        GBDATA *gb_species = GBT_first_marked_species(gb_main);
        int count = 0;
        char *buffer = new char[BUFFERSIZE+1];
        char *bp = buffer;
        //	int inbuf = 0; // # of names in buffer
        ED4_multi_species_manager *insert_into_manager = ED4_new_species_multi_species_manager();
        ED4_group_manager *group_man = insert_into_manager->get_parent(ED4_L_GROUP)->to_group_manager();
        int group_depth = insert_into_manager->calc_group_depth();
        int index = 0;
        ED4_index y = 0;
        ED4_index lot = 0;
        int inserted = 0;
        char *default_alignment = GBT_get_default_alignment(gb_main);

        aw_openstatus("ARB_EDIT4");
        aw_status("Loading species...");

        all_found         = 0;
        not_found_message = GBS_stropen(1000);
        GBS_strcat(not_found_message,"Species not found: ");

        while (gb_species) {
            count++;
            GB_status(double(count)/double(marked));

            char *name = GBT_read_name(gb_species);
            ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(name);

            if (!name_term) { // species not found -> insert
                {
                    int namelen = strlen(name);
                    int buffree = BUFFERSIZE-int(bp-buffer);

                    if ((namelen+2)>buffree) {
                        *bp++ = 0;
                        ED4_ROOT->database->fill_species(insert_into_manager,
                                                         ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence(),
                                                         buffer, &index, &y, 0, &lot, group_depth);
                        bp = buffer;
                        index = 0;
                    }

                    *bp++ = 1;
                    *bp++ = 'L';
                    memcpy(bp, name, namelen);
                    bp += namelen;
                }

                {
                    GBDATA *gbd = GBT_read_sequence(gb_species, default_alignment);

                    if (gbd) {
                        char *data = GB_read_string(gbd);
                        int len = GB_read_string_count(gbd);
                        group_man->table().add(data, len);
                    }
                }

                inserted++;
            }
            gb_species = GBT_next_marked_species(gb_species);
        }

        if (bp>buffer) {
            *bp++ = 0;
            ED4_ROOT->database->fill_species(insert_into_manager,
                                             ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence(),
                                             buffer, &index, &y, 0, &lot, group_depth);
        }

        aw_closestatus();
        aw_message(GBS_global_string("Loaded %i of %i marked species.", inserted, marked));

        {
            char *out_message = GBS_strclose(not_found_message);
            not_found_message = 0;
            if (all_found != 0) aw_message(out_message);
            free(out_message);
        }

        if (inserted) {
            ED4_ROOT->main_manager->update_info.set_resize(1);
            ED4_ROOT->main_manager->resize_requested_by_parent();
        }

        delete buffer;
        delete default_alignment;
    }
    else {
        aw_message("No species marked.");
    }

#undef BUFFERSIZE

    ED4_ROOT->refresh_all_windows(0);
}

ED4_returncode ED4_cursor::HideCursor()
{
    if (owner_of_cursor) {
        if (is_partly_visible()) {
            delete_cursor(cursor_abs_x, owner_of_cursor);
        }
        owner_of_cursor = 0;
    }

    return ED4_R_OK;
}

void ED4_cursor::changeType(ED4_CursorType typ)
{
    if (owner_of_cursor) {
        ED4_base *old_owner_of_cursor = owner_of_cursor;

        HideCursor();
        ctype = typ;
        ED4_ROOT->aw_root->awar(ED4_AWAR_CURSOR_TYPE)->write_int(int(ctype));
        owner_of_cursor = old_owner_of_cursor;
        ShowCursor(0, ED4_C_NONE, 0);
    }
    else {
        ctype = typ;
    }
}

int ED4_update_global_cursor_awars_allowed = true;

void ED4_cursor::updateAwars()
{
    AW_root	*aw_root = ED4_ROOT->aw_root;
    ED4_window  *win = ED4_ROOT->temp_ed4w;
    int 	seq_pos = get_sequence_pos();

    if (ED4_update_global_cursor_awars_allowed) {
        if (owner_of_cursor) {
            ED4_terminal *cursor_terminal = owner_of_cursor->to_terminal();
            char         *species_name    = cursor_terminal->get_name_of_species();

            if (species_name) {
                ED4_manager *cursor_manager = cursor_terminal->parent->parent->to_manager();

                if (cursor_manager->parent->flag.is_SAI) {
                    static char *last_set_SAI = 0;
                    if (!last_set_SAI || strcmp(last_set_SAI, species_name) != 0) {
                        free(last_set_SAI);
                        last_set_SAI = strdup(species_name);

                        ignore_selected_SAI_changes_cb = true;
                        GBT_write_string(gb_main, AWAR_SAI_NAME, species_name);
                        ignore_selected_SAI_changes_cb = false;
                    }
                }
                else {
                    static char *last_set_species = 0;
                    if (!last_set_species || strcmp(last_set_species, species_name) != 0) {
                        free(last_set_species);
                        last_set_species = strdup(species_name);

                        ignore_selected_species_changes_cb = true;
                        GBT_write_string(gb_main, AWAR_SPECIES_NAME, species_name);
                        ignore_selected_species_changes_cb = false;
                    }
                }
            }
        }
    }

    // update awars for cursor position:

    aw_root->awar(win->awar_path_for_cursor)->write_int(seq_pos+1);
    if (ED4_update_global_cursor_awars_allowed) {
        aw_root->awar(AWAR_CURSOR_POSITION)->write_int(seq_pos+1);	// this supports last cursor position for all applications
    }

    // look if we have a commented search result under the cursor:

    if (owner_of_cursor && owner_of_cursor->is_sequence_terminal()) {
        ED4_sequence_terminal *seq_term = owner_of_cursor->to_sequence_terminal();
        ED4_SearchResults &results = seq_term->results();
        ED4_SearchPosition *pos = results.get_shown_at(seq_pos);

        if (pos) {
            GB_CSTR comment = pos->get_comment();

            if (comment) {
                char *buffer = GB_give_buffer(strlen(comment)+30);

                sprintf(buffer, "%s: %s", ED4_SearchPositionTypeId[pos->get_whatsFound()], comment);
                aw_message(buffer);
            }
        }
    }

    // update awar for ecoli position:

    {
        long lseq_pos = seq_pos+1;
        long ecoli_pos;
        long dummy;

        ED4_ROOT->ecoli_ref->abs_2_rel(lseq_pos, ecoli_pos, dummy);
        aw_root->awar(win->awar_path_for_Ecoli)->write_int(ecoli_pos);
    }

    // update awar for base position:

    int base_pos = base_position.get_base_position(owner_of_cursor, seq_pos+1);
    aw_root->awar(win->awar_path_for_basePos)->write_int(base_pos); // update awar for base position

    // update awar for IUPAC:

    char iupac[5];
    strcpy(iupac, IUPAC_EMPTY);

    if (owner_of_cursor) {
        ED4_species_manager *species_manager = owner_of_cursor->get_parent(ED4_L_SPECIES)->to_species_manager();
        int                  len;
        char                *seq;

        if (species_manager->flag.is_consensus) {
            ED4_group_manager *group_manager = owner_of_cursor->get_parent(ED4_L_GROUP)->to_group_manager();

            seq = group_manager->table().build_consensus_string(seq_pos, seq_pos, 0);
            len = seq_pos+1; // fake
        }
        else {
            seq = owner_of_cursor->resolve_pointer_to_string_copy(&len);
        }

        e4_assert(seq);

        if (seq_pos<len) {
            char base = seq[seq_pos];
            const char *i = ED4_decode_iupac(base, ED4_ROOT->alignment_type);

            strcpy(iupac, i);
        }

        free(seq);
    }

    aw_root->awar(win->awar_path_for_IUPAC)->write_string(iupac);

    // update awar for helix#:

    {
        AW_helix *helix = ED4_ROOT->helix;
        int helixnr = 0;

        if (size_t(seq_pos)<helix->size && helix->entries[seq_pos].pair_type) {
            helixnr = atoi(helix->entries[seq_pos].helix_nr);
        }
        aw_root->awar(win->awar_path_for_helixNr)->write_int(helixnr);
    }
}

void ED4_cursor::jump_cursor(AW_window *aww, int new_cursor_screen_pos, bool center_cursor, bool fix_left_border)
{
    int old_allowed_to_draw = allowed_to_draw;

    if (!owner_of_cursor) {
        aw_message("First you have to place the cursor");
        return;
    }

    ED4_base *temp_parent = owner_of_cursor;
    while (temp_parent->parent) {
        temp_parent = temp_parent->parent;
        if (temp_parent->flag.hidden) return; // don't move cursor if terminal is flag.hidden
    }

    AW_pos terminal_x, terminal_y;
    owner_of_cursor->calc_world_coords(&terminal_x, &terminal_y);

    if (!is_partly_visible()) {
        ED4_terminal *term = owner_of_cursor->to_terminal();

        allowed_to_draw = 0;
        int set = term->setCursorTo(this, get_sequence_pos(), 0);
        allowed_to_draw = old_allowed_to_draw;
        if (!set || !is_partly_visible()) return;
    }

    int cursor_diff = new_cursor_screen_pos-screen_position;
    if ((cursor_diff==0 && !center_cursor) || new_cursor_screen_pos<0) return; // already at new position

    int terminal_pixel_length = aww->get_device(AW_MIDDLE_AREA)->get_string_size(ED4_G_SEQUENCES, 0, owner_of_cursor->to_text_terminal()->get_length());
    // int length_of_char = ED4_ROOT->font_info[ED4_G_SEQUENCES].get_width();
    int length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);
    int x_pos_cursor_new = cursor_abs_x+cursor_diff*length_of_char;

    if (x_pos_cursor_new > terminal_x+terminal_pixel_length+CHARACTEROFFSET ||
        x_pos_cursor_new < terminal_x+CHARACTEROFFSET) {
        return; // don`t move out of string
    }

    ED4_coords *coords = &ED4_ROOT->temp_ed4w->coords;
    int margin;

    if (center_cursor) {
        margin = (coords->window_right_clip_point - coords->window_left_clip_point + 1 - 2) / 2;
    }
    else {
        margin = length_of_char * MARGIN;
    }

    bool inside_right_margin = x_pos_cursor_new<=coords->window_right_clip_point && x_pos_cursor_new > coords->window_right_clip_point - margin; // we are in the right margin
    bool inside_left_margin  = x_pos_cursor_new> coords->window_left_clip_point  && x_pos_cursor_new < coords->window_left_clip_point + margin; // we are in the left margin
    bool scroll_horizontal = x_pos_cursor_new<coords->window_left_clip_point+margin || x_pos_cursor_new>coords->window_right_clip_point-margin; // we have to scroll

    if (cursor_diff >= 0) { // move cursor to the right
        if (!scroll_horizontal || inside_left_margin) { // move cursor without scrolling or enter margin
            delete_cursor( cursor_abs_x , owner_of_cursor);
            allowed_to_draw = 1;
            ShowCursor( cursor_diff*length_of_char, ED4_C_RIGHT, ABS(cursor_diff));
            allowed_to_draw = old_allowed_to_draw;
        }
        else  { // we have to scroll to the right
            int diff_cursor_margin = (ED4_ROOT->temp_ed4w->coords.window_right_clip_point - margin) - cursor_abs_x; // difference between old cursor position & right margin

            delete_cursor(cursor_abs_x, owner_of_cursor);
            cursor_abs_x += cursor_diff*length_of_char;
            aww->set_horizontal_scrollbar_position(aww->slider_pos_horizontal + length_of_char*cursor_diff - diff_cursor_margin);

            allowed_to_draw = 0;
            ED4_horizontal_change_cb(aww, 0, 0);
            allowed_to_draw = old_allowed_to_draw;

            ShowCursor(0, ED4_C_RIGHT, ABS( cursor_diff ));
        }
    }
    else if (cursor_diff < 0) // move cursor to the left
    {
        if (!scroll_horizontal || inside_right_margin) // move cursor without scrolling or enter margin
        {
            delete_cursor(cursor_abs_x, owner_of_cursor);
            allowed_to_draw = 1;
            ShowCursor( cursor_diff * length_of_char, ED4_C_LEFT, ABS( cursor_diff ) );
            allowed_to_draw = old_allowed_to_draw;
        }
        else // we have to scroll to the left
        {
            int diff_cursor_margin = cursor_abs_x - (ED4_ROOT->temp_ed4w->coords.window_left_clip_point + margin);

            delete_cursor(cursor_abs_x, owner_of_cursor);
            cursor_abs_x -= ABS(cursor_diff) * length_of_char;
            if ( aww->slider_pos_horizontal - length_of_char*ABS(cursor_diff) < 0) {
                aww->slider_pos_horizontal = 0;
            }
            else {
                aww->set_horizontal_scrollbar_position(aww->slider_pos_horizontal - length_of_char*ABS(cursor_diff) + diff_cursor_margin);
            }

            allowed_to_draw = 0;
            ED4_horizontal_change_cb(aww, 0, 0);
            allowed_to_draw = old_allowed_to_draw;

            ShowCursor(0, ED4_C_LEFT, ABS( cursor_diff ));
        }
    }

    last_seq_position = -1;
}

void ED4_cursor::jump_left_right_cursor(AW_window *aww, int new_cursor_screen_pos) {
    jump_cursor(aww, new_cursor_screen_pos, false, false);
}

void ED4_cursor::jump_centered_cursor(AW_window *aww, int new_cursor_screen_pos) {
    jump_cursor(aww, new_cursor_screen_pos, true, false);
}

void ED4_cursor::jump_left_right_cursor_to_seq_pos(AW_window *aww, int new_cursor_seq_pos) {
    int screen_pos = ED4_ROOT->root_group_man->remap()->sequence_to_screen_clipped(new_cursor_seq_pos);

    jump_left_right_cursor(aww,screen_pos);
    last_seq_position = new_cursor_seq_pos;
}

void ED4_cursor::jump_centered_cursor_to_seq_pos(AW_window *aww, int new_cursor_seq_pos) {
    int screen_pos = ED4_ROOT->root_group_man->remap()->sequence_to_screen_clipped(new_cursor_seq_pos);

    if (!awar_edit_direction) { // reverse editing (5'<-3')
        screen_pos++;
        new_cursor_seq_pos = -1; // uninitialized
    }

    jump_centered_cursor(aww, screen_pos);
    last_seq_position = new_cursor_seq_pos;
}

static int current_position = -1;

static bool terminal_has_gap_or_base_at_current_position(ED4_base *terminal, bool test_for_base) {
    bool test_succeeded = false;

    if (terminal->is_sequence_terminal()) {
        ED4_sequence_terminal *seqTerm = terminal->to_sequence_terminal();
        int len;
        char *seq = seqTerm->resolve_pointer_to_string_copy(&len);
        if (seq) {
            test_succeeded =
                len>current_position &&
                bool(ADPP_IS_ALIGN_CHARACTER(seq[current_position])) != test_for_base;
        }
        free(seq);
    }

#if defined(DEBUG) && 0
    printf("test_for_base=%i test_succeeded=%i\n", int(test_for_base), int(test_succeeded));
#endif // DEBUG
    return test_succeeded;
}

static bool terminal_has_base_at_current_position(ED4_base *terminal) { return terminal_has_gap_or_base_at_current_position(terminal, 1); }
static bool terminal_has_gap_at_current_position(ED4_base *terminal) { return terminal_has_gap_or_base_at_current_position(terminal, 0); }

ED4_returncode ED4_cursor::move_cursor( AW_event *event )	/* up down */
{
    AW_pos x, y, x_screen, y_screen, target_x, target_y, scroll_diff;
    ED4_base *temp_parent, *target_terminal = NULL;
    ED4_AREA_LEVEL area_level;

    temp_parent = owner_of_cursor->parent;
    while (temp_parent->parent) {
        temp_parent = temp_parent->parent;

        if (temp_parent->flag.hidden) {
            return ED4_R_IMPOSSIBLE; // don't move cursor if terminal is flag.hidden
        }
    }

    owner_of_cursor->calc_world_coords( &x, &y );
    x_screen = cursor_abs_x;
    y_screen = y;
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x_screen, &y_screen );
    area_level = owner_of_cursor->get_area_level();
    
    if (!is_partly_visible()) return ED4_R_BREAK; // @@@ maybe a bad idea

    switch (event->keycode) {
        case AW_KEY_UP: {
            if (event->keymodifier & AW_KEY_CONTROL) {
                current_position = last_seq_position;
                bool has_base = terminal_has_base_at_current_position(owner_of_cursor);

                get_upper_lower_cursor_pos( ED4_ROOT->main_manager, &target_terminal, ED4_C_UP, y,
                                            has_base ? terminal_has_gap_at_current_position : terminal_has_base_at_current_position);
                if (!has_base && ED4_ROOT->aw_root->awar(ED4_AWAR_FAST_CURSOR_JUMP)->read_int())  { // if jump_over group
                    get_upper_lower_cursor_pos( ED4_ROOT->main_manager, &target_terminal, ED4_C_DOWN, y_screen, terminal_has_gap_at_current_position);
                }
            }
            else {
                get_upper_lower_cursor_pos( ED4_ROOT->main_manager, &target_terminal, ED4_C_UP, y, 0);
            }

            if (target_terminal) {
                target_terminal->calc_world_coords( &target_x, &target_y );
                scroll_diff = target_y - y;

                if (! (target_terminal->is_visible( 0, target_y, ED4_D_VERTICAL )) && ED4_ROOT->temp_aww->slider_pos_vertical > 0) {
                    if (ED4_ROOT->temp_aww->slider_pos_vertical + scroll_diff < 0) {
                        ED4_ROOT->temp_aww->set_vertical_scrollbar_position (0);
                    }
                    else {
                        ED4_ROOT->temp_aww->set_vertical_scrollbar_position ((int)(ED4_ROOT->temp_aww->slider_pos_vertical + scroll_diff));
                    }
                    ED4_vertical_change_cb( ED4_ROOT->temp_aww, 0, 0 );
                    DRAW = 0;
                }
                show_cursor_at(target_terminal, screen_position);
            }
            break;
        }

        case AW_KEY_DOWN: {
            if (event->keymodifier & AW_KEY_CONTROL) {
                current_position = last_seq_position;
                bool has_base = terminal_has_base_at_current_position(owner_of_cursor);

                get_upper_lower_cursor_pos( ED4_ROOT->main_manager, &target_terminal, ED4_C_DOWN, y_screen,
                                            has_base ? terminal_has_gap_at_current_position : terminal_has_base_at_current_position);
                if (!has_base && ED4_ROOT->aw_root->awar(ED4_AWAR_FAST_CURSOR_JUMP)->read_int())  { // if jump_over group
                    get_upper_lower_cursor_pos( ED4_ROOT->main_manager, &target_terminal, ED4_C_DOWN, y_screen, terminal_has_gap_at_current_position);
                }
            }
            else {
                get_upper_lower_cursor_pos( ED4_ROOT->main_manager, &target_terminal, ED4_C_DOWN, y_screen, 0);
            }

            if (target_terminal) {
                target_terminal->calc_world_coords( &target_x, &target_y );
                scroll_diff = target_y - y;

                if (! target_terminal->is_visible( 0, target_y, ED4_D_VERTICAL_ALL ) && area_level == ED4_A_MIDDLE_AREA) {
                    ED4_ROOT->temp_aww->set_vertical_scrollbar_position ((int)(ED4_ROOT->temp_aww->slider_pos_vertical + scroll_diff));
                    ED4_vertical_change_cb( ED4_ROOT->temp_aww, 0, 0 );
                    DRAW = 0;
                }

                show_cursor_at(target_terminal, screen_position);
            }
            break;
        }

        default: {
            break;
        }
    }

    return ED4_R_OK;
}

void ED4_cursor::set_abs_x()
{
    AW_pos x, y;
    owner_of_cursor->calc_world_coords( &x, &y );
    cursor_abs_x = int(get_sequence_pos()*ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES) + CHARACTEROFFSET + x);
}


ED4_returncode ED4_cursor::ShowCursor(ED4_index offset_x, ED4_cursor_move move, int move_pos)
{
    AW_pos x=0, y=0, x_help = 0, y_help;

    owner_of_cursor->calc_world_coords( &x, &y );

    switch (move)
    {
        case ED4_C_RIGHT:
            screen_position += move_pos;
            break;
        case ED4_C_LEFT:
            screen_position -= move_pos;
            break;
        case ED4_C_NONE:
            break;
        default:
            e4_assert(0);
            break;
    }

    updateAwars();

    x_help = cursor_abs_x + offset_x;
    y_help = y;

    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &x_help, &y_help );

    if (allowed_to_draw) draw_cursor(x_help, y_help);

    cursor_abs_x += offset_x;

    return ( ED4_R_OK );
}


bool ED4_cursor::is_partly_visible() const {
    e4_assert(owner_of_cursor);

    AW_pos x,y;
    owner_of_cursor->calc_world_coords( &x, &y );
    
    int x1, y1, x2, y2;
    cursor_shape->get_bounding_box(cursor_abs_x, int(y), x1, y1, x2, y2);

    bool visible = false;

    switch (owner_of_cursor->get_area_level(0)) {
        case ED4_A_TOP_AREA:
            visible = 
                owner_of_cursor->is_visible(x1, 0, ED4_D_HORIZONTAL) ||
                owner_of_cursor->is_visible(x2, 0, ED4_D_HORIZONTAL);
            break;
        case ED4_A_MIDDLE_AREA:
            visible = owner_of_cursor->is_visible(x1, y1, x2, y2, ED4_D_ALL_DIRECTION);
            break;
        default:
            break;
    }

    return visible;
}

ED4_returncode ED4_cursor::set_to_terminal(AW_window *aww, ED4_terminal *terminal, int seq_pos)
{
    if (owner_of_cursor) {
        if (is_partly_visible()) {
            delete_cursor(cursor_abs_x, owner_of_cursor);
        }

        owner_of_cursor->set_refresh(); // we have to refresh old owner of cursor
        owner_of_cursor->parent->refresh_requested_by_child();
        owner_of_cursor = NULL;
        ED4_ROOT->refresh_window(0);
        DRAW = 1;
    }

    AW_pos termw_x, termw_y;
    terminal->calc_world_coords(&termw_x, &termw_y);

    screen_position = ED4_ROOT->root_group_man->remap()->sequence_to_screen_clipped(seq_pos);
    last_seq_position = seq_pos;
    owner_of_cursor = terminal;

    //    AW_device *device = aww->get_device(AW_MIDDLE_AREA);
    // int length_of_char = ED4_ROOT->font_info[ED4_G_SEQUENCES].get_width();
    int length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);
    AW_pos seq_relx = AW_pos(screen_position*length_of_char + CHARACTEROFFSET);	// position relative to start of terminal

    AW_pos world_x = seq_relx+termw_x;		// world position of cursor
    AW_pos world_y = termw_y;

    ED4_coords *coords = &ED4_ROOT->temp_ed4w->coords;
    int win_x_size = coords->window_right_clip_point - coords->window_left_clip_point + 1;	// size of scroll window
    int win_y_size = coords->window_lower_clip_point - coords->window_upper_clip_point + 1;

    ED4_folding_line *foldLine = ED4_ROOT->temp_ed4w->vertical_fl;
    int left_area_width = 0;

    if (foldLine) {
        left_area_width = int(foldLine->world_pos[X_POS]);
    }

    int win_left = int(world_x - left_area_width - win_x_size/2); 		if (win_left<0) win_left = 0;	// position
    int win_top = int(world_y - coords->top_area_height - win_y_size/2);	if (win_top<0) win_top = 0;

    int pic_xsize = int(aww->get_scrolled_picture_width());		// size of scrolled picture = size of sliderrange
    int pic_ysize = int(aww->get_scrolled_picture_height());

    int slider_xsize = win_x_size;
    int slider_ysize = win_y_size;

    int slider_pos_xrange = pic_xsize - slider_xsize;	// allowed positions for slider
    int slider_pos_yrange = pic_ysize - slider_ysize;

    int slider_pos_x = win_left;	// new slider positions
    int slider_pos_y = win_top;

    if (slider_pos_x>slider_pos_xrange) slider_pos_x = slider_pos_xrange;
    if (slider_pos_y>slider_pos_yrange) slider_pos_y = slider_pos_yrange;

    if (slider_pos_x<0) slider_pos_x = 0;
    if (slider_pos_y<0) slider_pos_y = 0;

    aww->set_horizontal_scrollbar_position(slider_pos_x);
    aww->set_vertical_scrollbar_position(slider_pos_y);
    ED4_scrollbar_change_cb(aww, 0, 0);

    AW_pos win_x = world_x;	// recalc coords cause window scrolled!
    AW_pos win_y = world_y;
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &win_x, &win_y );

    cursor_abs_x = (long int)world_x;
    owner_of_cursor = terminal;

    draw_cursor(win_x, win_y);

    GB_transaction gb_dummy(gb_main);
    updateAwars();

    return ED4_R_OK;
}

ED4_returncode ED4_cursor::show_cursor_at( ED4_base *target_terminal, ED4_index scr_pos)
{
    if (owner_of_cursor) {
        if (is_partly_visible()) {
            delete_cursor(cursor_abs_x, owner_of_cursor);
        }

        owner_of_cursor->set_refresh(); // we have to refresh old owner of cursor
        owner_of_cursor->parent->refresh_requested_by_child();
        owner_of_cursor = NULL;
        ED4_ROOT->refresh_window(0);
        DRAW = 1;
    }

    AW_pos termw_x, termw_y;
    target_terminal->calc_world_coords( &termw_x, &termw_y );

    screen_position   = scr_pos;
    last_seq_position = ED4_ROOT->root_group_man->remap()->screen_to_sequence(screen_position);

    int length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);
    
    AW_pos world_x = termw_x + length_of_char*screen_position + CHARACTEROFFSET;
    AW_pos world_y = termw_y;

    AW_pos win_x = world_x;
    AW_pos win_y = world_y;
    ED4_ROOT->world_to_win_coords( ED4_ROOT->temp_aww, &win_x, &win_y );
    
    cursor_abs_x = (long int)world_x;
    owner_of_cursor = target_terminal;

    draw_cursor(win_x, win_y);

    GB_transaction gb_dummy(gb_main);
    updateAwars();

    return ED4_R_OK;
}

ED4_returncode ED4_cursor::show_clicked_cursor(AW_pos click_xpos, ED4_base *target_terminal)
{
    AW_pos termw_x, termw_y;
    target_terminal->calc_world_coords( &termw_x, &termw_y );

    ED4_index scr_pos = ED4_ROOT->pixel2pos(click_xpos - termw_x);

    if (!awar_edit_direction) scr_pos++; // reverse editing -> set behind clicked base

    return show_cursor_at(target_terminal, scr_pos);
}

ED4_returncode  ED4_cursor::get_upper_lower_cursor_pos( ED4_manager *starting_point, ED4_base **terminal, ED4_cursor_move cursor_move, AW_pos actual_y, bool (*terminal_is_appropriate)(ED4_base *terminal) )
{
    AW_pos                     x, y, y_area;
    int                        i;
    ED4_AREA_LEVEL             level;
    ED4_multi_species_manager *middle_area_mult_spec = NULL;

    for (i=0; i < starting_point->children->members(); i++) {
        ED4_base *member = starting_point->children->member(i);

        member->calc_world_coords( &x, &y );

        switch (cursor_move) {
            case ED4_C_UP: {
                if (y < actual_y) {
                    if ((member->is_manager()) && !member->flag.hidden) {
                        get_upper_lower_cursor_pos( member->to_manager(), terminal, cursor_move, actual_y, terminal_is_appropriate);
                    }

                    if ((member->dynamic_prop & ED4_P_CURSOR_ALLOWED)) {
                        if (terminal_is_appropriate==0 || terminal_is_appropriate(member)) {
                            *terminal = member;
                        }
                    }
                }
                break;
            }
            case ED4_C_DOWN: {
                if (!(*terminal)) {
                    if ((member->is_manager()) && !member->flag.hidden) {
                        get_upper_lower_cursor_pos(member->to_manager() ,terminal, cursor_move, actual_y, terminal_is_appropriate);
                    }
                    ED4_ROOT->world_to_win_coords(ED4_ROOT->temp_aww, &x, &y);
                    if ((member->dynamic_prop & ED4_P_CURSOR_ALLOWED) && y > actual_y) {
                        level = member->get_area_level(&middle_area_mult_spec);		//probably multi_species_manager of middle_area, otherwise just a dummy
                        if (level != ED4_A_MIDDLE_AREA) {
                            if (terminal_is_appropriate==0 || terminal_is_appropriate(member)) {
                                *terminal = member;
                            }
                        }
                        else if (level == ED4_A_MIDDLE_AREA) {
                            y_area = middle_area_mult_spec->parent->extension.position[Y_POS];
                            if (y > y_area) {
                                member = starting_point->children->member(i);
                                if (terminal_is_appropriate==0 || terminal_is_appropriate(member)) {
                                    *terminal = member;
                                }
                            }
                        }
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
    return ED4_R_OK;
}

/* --------------------------------------------------------------------------------
   ED4_base_position
   -------------------------------------------------------------------------------- */

void ED4_base_position::calc4base(ED4_base *base)
{
    e4_assert(base);

    ED4_species_manager *species_manager = base->get_parent(ED4_L_SPECIES)->to_species_manager();
    int                  len;
    char                *seq;

    if (species_manager->flag.is_consensus) {
        ED4_group_manager *group_manager = base->get_parent(ED4_L_GROUP)->to_group_manager();

        seq = group_manager->table().build_consensus_string();
        len = strlen(seq);
    }
    else {
        seq = base->resolve_pointer_to_string_copy(&len);
    }

    e4_assert(seq);

    delete [] seq_pos;
    calced4base = base;

    {
        int *pos = new int[len];
        int p;

        count = 0;
        for (p=0; p<len; p++) {
            if (!ADPP_IS_ALIGN_CHARACTER(seq[p])) {
                pos[count++] = p;
            }
        }

        if (count) {
            seq_pos = new int[count];
            for (p=0; p<count; p++) {
                seq_pos[p] = pos[p];
            }
        }
        else {
            seq_pos = 0;
        }

        delete[] pos;
    }

    free(seq);
}
int ED4_base_position::get_base_position(ED4_base *base, int sequence_position)
{
    if (!base) return 0;
    if (base!=calced4base) calc4base(base);

    if (count==0) return 0;
    if (sequence_position>seq_pos[count-1]) return count;

    int h = count-1,
        l = 0;

    while (1)
    {
        int m = (l+h)/2;

        if (seq_pos[m]==sequence_position) {
            return m;
        }
        else {
            if (l==h) break;

            if (sequence_position<seq_pos[m]) 	h = m;
            else				l = m+1;
        }
    }

    return l;
}
int ED4_base_position::get_sequence_position(ED4_base *base, int base_position)
{
    if (!base) return 0;
    if (base!=calced4base) calc4base(base);
    return base_position<count ? seq_pos[base_position] : seq_pos[count-1]+1;
}

/* --------------------------------------------------------------------------------
             Store/Restore Cursorpositions
   -------------------------------------------------------------------------------- */

class CursorPos
{
    ED4_terminal *terminal;
    int seq_pos;

    CursorPos *next;

    static CursorPos *head;

public:

    static void clear()
        {
            while (head) {
                CursorPos *p = head->next;

                delete head;
                head = p;
            }
        }
    static CursorPos *get_head() { return head; }

    CursorPos(ED4_terminal *t, int p)
        {
            terminal = t;
            seq_pos = p;
            next = head;
            head = this;
        }
    ~CursorPos() {}

    ED4_terminal *get_terminal() const { return terminal; }
    int get_seq_pos() const { return seq_pos; }

    void moveToEnd()
        {
            e4_assert(this==head);

            if (next) {
                CursorPos *p = head = next;

                while (p->next) {
                    p = p->next;
                }

                p->next = this;
                this->next = 0;
            }
        }
};

CursorPos *CursorPos::head = 0;

void ED4_store_curpos(AW_window *aww, AW_CL /*cd1*/, AW_CL /*cd2*/)
{
    GB_transaction dummy(gb_main);
    ED4_ROOT->temp_aww 	= aww;
    ED4_ROOT->temp_device = aww->get_device(AW_MIDDLE_AREA);
    ED4_ROOT->temp_ed4w = ED4_ROOT->first_window->get_matching_ed4w( aww );
    ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;
    if (!cursor->owner_of_cursor) {
        aw_message("First you have to place the cursor.");
        return;
    }

    new CursorPos(cursor->owner_of_cursor->to_terminal(), cursor->get_sequence_pos());
}

void ED4_restore_curpos(AW_window *aww, AW_CL /*cd1*/, AW_CL /*cd2*/)
{
    GB_transaction dummy(gb_main);
    ED4_ROOT->temp_aww 	= aww;
    ED4_ROOT->temp_device = aww->get_device(AW_MIDDLE_AREA);
    ED4_ROOT->temp_ed4w = ED4_ROOT->first_window->get_matching_ed4w( aww );
    ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;

    CursorPos *pos = CursorPos::get_head();
    if (!pos) {
        aw_message("No cursor position stored.");
        return;
    }

    pos->get_terminal()->setCursorTo(cursor, pos->get_seq_pos(), 1);
    pos->moveToEnd();
}

void ED4_clear_stored_curpos(AW_window */*aww*/, AW_CL /*cd1*/, AW_CL /*cd2*/)
{
    CursorPos::clear();
}

/* --------------------------------------------------------------------------------
   Other stuff
   -------------------------------------------------------------------------------- */

void ED4_helix_jump_opposite(AW_window *aww, AW_CL /*cd1*/, AW_CL /*cd2*/)
{
    GB_transaction dummy(gb_main);
    ED4_ROOT->temp_aww 	= aww;
    ED4_ROOT->temp_device = aww->get_device(AW_MIDDLE_AREA);
    ED4_ROOT->temp_ed4w = ED4_ROOT->first_window->get_matching_ed4w( aww );
    ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;

    if (!cursor->owner_of_cursor) {
        aw_message("First you have to place the cursor.");
        return;
    }

    int seq_pos = cursor->get_sequence_pos();
    AW_helix *helix = ED4_ROOT->helix;

    if (size_t(seq_pos)<helix->size && helix->entries[seq_pos].pair_type) {
        int pairing_pos = helix->entries[seq_pos].pair_pos;
        e4_assert(helix->entries[pairing_pos].pair_pos==seq_pos);
        cursor->jump_centered_cursor_to_seq_pos(aww, pairing_pos);
        return;
    }

    aw_message("No helix position");
}

void ED4_change_cursor(AW_window */*aww*/, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    ED4_cursor *cursor = &ED4_ROOT->temp_ed4w->cursor;
    ED4_CursorType typ = cursor->getType();

    cursor->changeType((ED4_CursorType)((typ+1)%ED4_CURSOR_TYPES));
}

