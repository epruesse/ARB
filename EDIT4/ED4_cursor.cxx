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

#include <ed4_extern.hxx>

#include "ed4_class.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_tools.hxx"
#include "ed4_awars.hxx"
#include "ed4_ProteinViewer.hxx"

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

    int char_width;

    bool reverse;

    int point(int x, int y) {
        e4_assert(points<MAXPOINTS);
        if (reverse) {
            xpos[points] = char_width-x;
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
    lines      = 0;
    points     = 0;
    reverse    = reverse_cursor;
    char_width = character_width;

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

    cursor_shape->draw(ED4_ROOT->get_device(), int(x), int(y));

#if defined(DEBUG) && 0 
    printf("draw_cursor(%i, %i)\n", int(x), int(y));
#endif // DEBUG

    return ED4_R_OK;
}

ED4_returncode ED4_cursor::delete_cursor(AW_pos del_mark, ED4_base *target_terminal)
{
    AW_pos x, y;
    ED4_base *temp_parent;

    target_terminal->calc_world_coords( &x, &y );
    temp_parent = target_terminal->get_parent( ED4_L_SPECIES );
    if (!temp_parent || temp_parent->flag.hidden || !is_partly_visible()) {
        return ED4_R_BREAK;
    }
    x = del_mark;
    ED4_ROOT->world_to_win_coords(ED4_ROOT->get_aww(), &x, &y);

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

    AW_device *dev = ED4_ROOT->get_device();
    dev->push_clip_scale();

    int xmin, xmax, ymin, ymax;

    e4_assert(cursor_shape);
    cursor_shape->get_bounding_box(int(x), int(y), xmin, ymin, xmax, ymax);

    dev->clear_part(xmin, ymin, xmax-xmin+1, ymax-ymin+1, (AW_bitset)-1);
    
#if defined(DEBUG) && 0
    printf("delete_cursor(%i, %i)\n", int(x), int(y));
#endif // DEBUG

    dev->set_top_font_overlap(1);
    dev->set_bottom_font_overlap(1);
    dev->set_left_font_overlap(1);
    dev->set_right_font_overlap(1);

#define EXPAND_SIZE 0
    if (ED4_ROOT->get_device()->reduceClipBorders(ymin-EXPAND_SIZE, ymax+1+EXPAND_SIZE, xmin-EXPAND_SIZE, xmax+1+EXPAND_SIZE)) {
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
}
ED4_cursor::~ED4_cursor()
{
    delete cursor_shape;
}

ED4_window *ED4_cursor::window() const
{
    ED4_window *win;

    for (win=ED4_ROOT->first_window; win; win=win->next) {
        if (&win->cursor==this) {
            break;
        }
    }
    e4_assert(win);
    return win;
}

int ED4_cursor::get_sequence_pos() const
{
    ED4_remap *remap = ED4_ROOT->root_group_man->remap();
    size_t max_scrpos = remap->get_max_screen_pos();

    return remap->screen_to_sequence(size_t(screen_position)<=max_scrpos ? screen_position : max_scrpos);
}

bool ED4_species_manager::setCursorTo(ED4_cursor *cursor, int seq_pos, bool unfold_groups, ED4_CursorJumpType jump_type)
{
    ED4_group_manager *group_manager_to_unfold = is_in_folded_group();

    if (unfold_groups) {
        bool unfolded = false;

        while (group_manager_to_unfold && unfold_groups) {
            ED4_base *base = group_manager_to_unfold->search_spec_child_rek(ED4_L_BRACKET);
            if (!base) break;
            ED4_bracket_terminal *bracket = base->to_bracket_terminal();
            group_manager_to_unfold->unfold_group(bracket->id);
            unfolded = true;
            group_manager_to_unfold = is_in_folded_group();
        }

        if (unfolded) ED4_ROOT->refresh_all_windows(1);
    }

    if (!group_manager_to_unfold) { // species manager is visible (now)
        ED4_terminal *terminal = search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_terminal();
        if (terminal) {
            if (seq_pos == -1) seq_pos = cursor->get_sequence_pos();
            cursor->set_to_terminal(ED4_ROOT->get_aww(), terminal, seq_pos, jump_type);
            return true;
        }
    }

    return false;
}

static void jump_to_species(ED4_species_name_terminal *name_term, int seq_pos, bool unfold_groups, ED4_CursorJumpType jump_type)
{
    ED4_species_manager *species_manager = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();
    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
    bool jumped = false;

    if (species_manager) jumped = species_manager->setCursorTo(cursor, seq_pos, unfold_groups, jump_type);
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
            ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
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
            jump_to_species(name_term, -1, false, ED4_JUMP_KEEP_POSITION);
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
            jump_to_species(name_term, -1, true, ED4_JUMP_KEEP_POSITION);
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
            /* new AAseqTerminals should be created if it is in ProtView mode */
            if(ED4_ROOT->alignment_type == GB_AT_DNA) {
                PV_AddCorrespondingAAseqTerminals(name_term);
            }
            ED4_ROOT->main_manager->update_info.set_resize(1);
            ED4_ROOT->main_manager->resize_requested_by_parent();
            /* it should create new AA Sequence terminals if the protien viewer is enabled */
        }
        delete string;
    }
    if (name_term) {
        jump_to_species(name_term, -1, true, ED4_JUMP_KEEP_POSITION);
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
            /* new AAseqTerminals should be created if it is in ProtView mode */
            if (ED4_ROOT->alignment_type == GB_AT_DNA) {
                PV_AddAAseqTerminalsToLoadedSpecies();
            }
     
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
    ED4_window  *win = ED4_ROOT->get_ed4w();
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
        long ecoli_pos = ED4_ROOT->ecoli_ref->abs_2_rel(seq_pos);
        aw_root->awar(win->awar_path_for_Ecoli)->write_int(ecoli_pos+1);
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
    const char *helixNr = ED4_ROOT->helix->helixNr(seq_pos);
    aw_root->awar(win->awar_path_for_helixNr)->write_string(helixNr ? helixNr : "");
}

int ED4_cursor::get_screen_relative_pos() {
    ED4_coords *coords = &ED4_ROOT->get_ed4w()->coords;
    return cursor_abs_x - coords->window_left_clip_point;
}
void ED4_cursor::set_screen_relative_pos(AW_window *aww, int scroll_to_relpos) {
    int curr_rel_pos  = get_screen_relative_pos();
    int scroll_amount = curr_rel_pos-scroll_to_relpos;

    int length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);
    scroll_amount      = (scroll_amount/length_of_char)*length_of_char; // align to char-size

    if (scroll_amount != 0) {
        aww->set_horizontal_scrollbar_position(aww->slider_pos_horizontal + scroll_amount);
#if defined(DEBUG)
        printf("set_screen_relative_pos(%i) auto-scrolls %i\n", scroll_to_relpos, scroll_amount);
#endif // DEBUG
        ED4_horizontal_change_cb(aww, 0, 0);
    }
}


void ED4_cursor::jump_screen_pos(AW_window *aww, int screen_pos, ED4_CursorJumpType jump_type) {
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

#if defined(DEBUG) && 0
    printf("jump_screen_pos(%i)\n", screen_pos);
#endif // DEBUG

    ED4_terminal *term = owner_of_cursor->to_terminal();
    term->scroll_into_view(aww); // correct y-position of terminal

    int cursor_diff = screen_pos-screen_position;
    if (cursor_diff == 0) { // cursor position did not change
        if (jump_type == ED4_JUMP_KEEP_VISIBLE) return; // nothing special -> done
    }

    int terminal_pixel_length = aww->get_device(AW_MIDDLE_AREA)->get_string_size(ED4_G_SEQUENCES, 0, owner_of_cursor->to_text_terminal()->get_length());
    int length_of_char        = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);
    int abs_x_new             = cursor_abs_x+cursor_diff*length_of_char; // position in terminal

    if (abs_x_new > terminal_x+terminal_pixel_length+CHARACTEROFFSET || abs_x_new < terminal_x+CHARACTEROFFSET) {
        return; // don`t move out of terminal
    }

    ED4_coords *coords = &ED4_ROOT->get_ed4w()->coords;

    int screen_width  = coords->window_right_clip_point-coords->window_left_clip_point;
    int scroll_new_to = -1;     // if >0 -> scroll abs_x_new to this screen-relative position

    if (jump_type != ED4_JUMP_KEEP_VISIBLE) {
        int rel_x = cursor_abs_x-coords->window_left_clip_point;

        if (jump_type == ED4_JUMP_KEEP_POSITION) {
            bool was_in_screen = rel_x >= 0 && rel_x <= screen_width;
            if (!was_in_screen) {
                jump_type = ED4_JUMP_KEEP_VISIBLE; // // don't have useful relative position -> scroll
            }
        }

        switch (jump_type) {
            case ED4_JUMP_CENTERED:             scroll_new_to = screen_width/2;         break;
            case ED4_JUMP_KEEP_POSITION:        scroll_new_to = rel_x;                  break;
            case ED4_JUMP_KEEP_VISIBLE:         break; // handled below
            default: e4_assert(0); break;
        }
    }

    if (jump_type == ED4_JUMP_KEEP_VISIBLE) {
        int margin = length_of_char * ED4_ROOT->aw_root->awar(ED4_AWAR_SCROLL_MARGIN)->read_int();

        int right_margin = coords->window_right_clip_point - margin;
        int left_margin  = coords->window_left_clip_point + margin;

        if (left_margin >= right_margin)    scroll_new_to = screen_width/2; // margins too big -> center
        else if (abs_x_new > right_margin)  scroll_new_to = screen_width-margin;
        else if (abs_x_new < left_margin)   scroll_new_to = margin;
    }

    delete_cursor(cursor_abs_x, owner_of_cursor);

    int old_allowed_to_draw = allowed_to_draw;
    if (scroll_new_to >= 0) {   // scroll
        int rel_x_new     = abs_x_new-coords->window_left_clip_point;
        int scroll_amount = rel_x_new-scroll_new_to;

        if      (scroll_amount>0) scroll_amount += length_of_char/2;
        else if (scroll_amount<0) scroll_amount -= length_of_char/2;

        scroll_amount = (scroll_amount/length_of_char)*length_of_char; // align to char-size
        if (scroll_amount != 0) {
            aww->set_horizontal_scrollbar_position(aww->slider_pos_horizontal + scroll_amount);
#if defined(DEBUG) && 0
            printf("jump_screen_pos auto-scrolls %i\n", scroll_amount);
#endif // DEBUG
            allowed_to_draw = 0;
            ED4_horizontal_change_cb(aww, 0, 0);
        }
    }

    allowed_to_draw = 1;
    if (cursor_diff >= 0) {
        ShowCursor(cursor_diff*length_of_char, ED4_C_RIGHT, ABS(cursor_diff));
    }
    else {
        ShowCursor(cursor_diff*length_of_char, ED4_C_LEFT, ABS(cursor_diff));
    }
}

void ED4_cursor::jump_sequence_pos(AW_window *aww, int seq_pos, ED4_CursorJumpType jump_type) {
    int screen_pos = ED4_ROOT->root_group_man->remap()->sequence_to_screen_clipped(seq_pos);
    jump_screen_pos(aww, screen_pos, jump_type);
}

void ED4_cursor::jump_base_pos(AW_window *aww, int base_pos, ED4_CursorJumpType jump_type) {
    int seq_pos = base2sequence_position(base_pos);
    jump_sequence_pos(aww, seq_pos, jump_type);
}

static bool has_gap_or_base_at(ED4_base *terminal, bool test_for_base, int seq_pos) {
    bool test_succeeded = false;

    if (terminal->is_sequence_terminal()) {
        ED4_sequence_terminal *seqTerm = terminal->to_sequence_terminal();
        int len;
        char *seq = seqTerm->resolve_pointer_to_string_copy(&len);
        if (seq) {
            test_succeeded = len>seq_pos && bool(ADPP_IS_ALIGN_CHARACTER(seq[seq_pos]))!=test_for_base;
        }
        free(seq);
    }

#if defined(DEBUG) && 0
    printf("test_for_base=%i test_succeeded=%i\n", int(test_for_base), int(test_succeeded));
#endif // DEBUG
    return test_succeeded;
}

static bool has_base_at(ED4_base *terminal, int seq_pos) { return has_gap_or_base_at(terminal, true,  seq_pos); }
static bool has_gap_at(ED4_base *terminal, int seq_pos) { return has_gap_or_base_at(terminal, false, seq_pos); }


ED4_returncode ED4_cursor::move_cursor(AW_event *event) {
    // move cursor up down
    ED4_cursor_move dir     = ED4_C_NONE;
    ED4_returncode result   = ED4_R_OK;
    bool            endHome = false;

    switch (event->keycode) {
        case AW_KEY_UP:   dir = ED4_C_UP;   break;
        case AW_KEY_DOWN: dir = ED4_C_DOWN; break;
        case AW_KEY_HOME: dir = ED4_C_UP;   endHome = true; break;
        case AW_KEY_END:  dir = ED4_C_DOWN; endHome = true; break;
        default: e4_assert(0); break; // illegal call of move_cursor()
    }

    if (dir != ED4_C_NONE) {
        // don't move cursor if terminal is hidden
        {
            ED4_base *temp_parent = owner_of_cursor->parent;
            while (temp_parent->parent && result == ED4_R_OK) {
                if (temp_parent->flag.hidden) { result = ED4_R_IMPOSSIBLE; }
                temp_parent = temp_parent->parent;
            }
        }

        if (result == ED4_R_OK) {
            AW_pos     x_dummy, y_world;
            AW_window *aww      = ED4_ROOT->get_aww();

            owner_of_cursor->calc_world_coords(&x_dummy, &y_world);

            int       seq_pos         = get_sequence_pos();
            ED4_base *target_terminal = 0;
            
            if (event->keymodifier & AW_KEYMODE_CONTROL) {
                bool has_base = has_base_at(owner_of_cursor, seq_pos);

                // stay in current area
                ED4_manager *start_at_manager = 0;
                if (owner_of_cursor->has_parent(ED4_ROOT->top_area_man)) {
                    start_at_manager = ED4_ROOT->top_area_man;
                }
                else {
                    start_at_manager = ED4_ROOT->middle_area_man;
                }

                if (!endHome) { // not End or Home
                    target_terminal = get_upper_lower_cursor_pos(start_at_manager, dir, y_world, false, has_base ? has_gap_at : has_base_at, seq_pos);
                    if (target_terminal && !has_base && ED4_ROOT->aw_root->awar(ED4_AWAR_FAST_CURSOR_JUMP)->read_int())  { // if jump_over group
                        target_terminal->calc_world_coords(&x_dummy, &y_world);
                        target_terminal = get_upper_lower_cursor_pos(start_at_manager, dir, y_world, false, has_gap_at, seq_pos);
                    }
                }

                if (target_terminal == 0) {
                    // either already in last group (or no space behind last group) -> jump to end (or start)
                    target_terminal = get_upper_lower_cursor_pos(start_at_manager,
                                                                 dir == ED4_C_UP ? ED4_C_DOWN : ED4_C_UP, // use opposite movement direction
                                                                 dir == ED4_C_UP ? 0 : INT_MAX, false, // search for top-/bottom-most terminal
                                                                 0, seq_pos); 
                }
            }
            else {
                e4_assert(!endHome); // END and HOME w/o Ctrl should not call move_cursor() 
                
                bool isScreen = false;
                if (dir == ED4_C_DOWN) {
                    ED4_ROOT->world_to_win_coords(aww, &x_dummy, &y_world); // special handling to move cursor from top to bottom area
                    isScreen = true;
                }
                target_terminal = get_upper_lower_cursor_pos(ED4_ROOT->main_manager, dir, y_world, isScreen, 0, seq_pos);
            }

            if (target_terminal) {
                set_to_terminal(aww, target_terminal->to_terminal(), seq_pos, ED4_JUMP_KEEP_VISIBLE);
            }
        }
    }

    return result;
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

    ED4_ROOT->world_to_win_coords(ED4_ROOT->get_aww(), &x_help, &y_help);

    if (allowed_to_draw) draw_cursor(x_help, y_help);
#if defined(DEBUG) && 0
    else printf("Skip draw_cursor (allowed_to_draw=false)\n");
#endif // DEBUG

    cursor_abs_x += offset_x;

    return ( ED4_R_OK );
}


bool ED4_cursor::is_partly_visible() const {
    e4_assert(owner_of_cursor);
    e4_assert(cursor_shape); // cursor is not drawn, cannot test visibility

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

void ED4_terminal::scroll_into_view(AW_window *aww) { // scroll y-position only
    AW_pos termw_x, termw_y;
    calc_world_coords(&termw_x, &termw_y);

    ED4_coords *coords  = &ED4_ROOT->get_ed4w()->coords;

    int term_height = int(extension.size[1]);
    int win_ysize   = coords->window_lower_clip_point - coords->window_upper_clip_point + 1;

    bool scroll = false;
    int slider_pos_y;

    AW_pos termw_y_upper = termw_y - term_height; // upper border of terminal

    if (termw_y_upper > coords->top_area_height) { // dont scroll if terminal is in top area (always visible)
        if (termw_y_upper < coords->window_upper_clip_point) {
#if defined(DEBUG) && 0
            printf("termw_y(%i)-term_height(%i) < window_upper_clip_point(%i)\n",
                   int(termw_y), term_height, int(coords->window_upper_clip_point));
#endif // DEBUG
            slider_pos_y = int(termw_y - coords->top_area_height - term_height);
            scroll       = true;
        }
        else if (termw_y > coords->window_lower_clip_point) {
#if defined(DEBUG) && 0
            printf("termw_y(%i) > window_lower_clip_point(%i)\n",
                   int(termw_y), int(coords->window_lower_clip_point));
#endif // DEBUG
            slider_pos_y = int(termw_y - coords->top_area_height - win_ysize);
            scroll       = true;
        }
    }
    
#if defined(DEBUG) && 0
    if (!scroll) {
        printf("No scroll needed (termw_y=%i termw_y_upper=%i term_height=%i window_upper_clip_point=%i window_lower_clip_point=%i)\n",
               int(termw_y), int(termw_y_upper), term_height,
               int(coords->window_upper_clip_point), int(coords->window_lower_clip_point));
    }
#endif // DEBUG

    if (scroll) {
        int pic_ysize         = int(aww->get_scrolled_picture_height());
        int slider_pos_yrange = pic_ysize - win_ysize;

        if (slider_pos_y>slider_pos_yrange) slider_pos_y = slider_pos_yrange;
        if (slider_pos_y<0) slider_pos_y                 = 0;
        
        aww->set_vertical_scrollbar_position(slider_pos_y);
        ED4_scrollbar_change_cb(aww, 0, 0);
    }
}

void ED4_cursor::set_to_terminal(AW_window *aww, ED4_terminal *terminal, int seq_pos, ED4_CursorJumpType jump_type)
{
    if (seq_pos == -1) seq_pos = get_sequence_pos();
    
    if (owner_of_cursor == terminal) {
        jump_sequence_pos(aww, seq_pos, jump_type);
    }
    else {
        if (owner_of_cursor) {
            if (get_sequence_pos() != seq_pos) {
                jump_sequence_pos(aww, seq_pos, jump_type); // position to wanted column -- scrolls horizontally
            }
        }

        int scr_pos = ED4_ROOT->root_group_man->remap()->sequence_to_screen_clipped(seq_pos);
        show_cursor_at(terminal, scr_pos);

        if (!is_partly_visible()) {
#if defined(DEBUG) && 0
            printf("Cursor not visible in set_to_terminal (was drawn outside screen)\n");
#endif // DEBUG
            jump_sequence_pos(aww, seq_pos, jump_type);
        }
    }

    GB_transaction ta(gb_main);
    updateAwars();
}

ED4_returncode ED4_cursor::show_cursor_at(ED4_terminal *target_terminal, ED4_index scr_pos)
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

    target_terminal->scroll_into_view(ED4_ROOT->get_aww());
    
    AW_pos termw_x, termw_y;
    target_terminal->calc_world_coords( &termw_x, &termw_y );

    screen_position = scr_pos;

    int length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);

    AW_pos world_x = termw_x + length_of_char*screen_position + CHARACTEROFFSET;
    AW_pos world_y = termw_y;

    AW_pos win_x = world_x;
    AW_pos win_y = world_y;
    ED4_ROOT->world_to_win_coords(ED4_ROOT->get_aww(), &win_x, &win_y);

    cursor_abs_x = (long int)world_x;
    owner_of_cursor = target_terminal;

    draw_cursor(win_x, win_y);

    GB_transaction gb_dummy(gb_main);
    updateAwars();

    return ED4_R_OK;
}

ED4_returncode ED4_cursor::show_clicked_cursor(AW_pos click_xpos, ED4_terminal *target_terminal)
{
    AW_pos termw_x, termw_y;
    target_terminal->calc_world_coords( &termw_x, &termw_y );

    ED4_index scr_pos = ED4_ROOT->pixel2pos(click_xpos - termw_x);
    return show_cursor_at(target_terminal, scr_pos);
}

ED4_terminal *ED4_cursor::get_upper_lower_cursor_pos(ED4_manager *starting_point, ED4_cursor_move cursor_move, AW_pos current_y, bool isScreen, ED4_TerminalTest terminal_is_appropriate, int seq_pos)
{
    // current_y is y-position of terminal at which search starts.
    // It may be in world or screen coordinates (isScreen marks which is used)
    // This is needed to move the cursor from top- to middle-area w/o scrolling middle-area to top-position.

    ED4_terminal *result = 0;

    for (int i=0; i<starting_point->children->members(); i++) {
        ED4_base *member = starting_point->children->member(i);

        AW_pos x, y;
        member->calc_world_coords( &x, &y );

        switch (cursor_move) {
            case ED4_C_UP:
                e4_assert(!isScreen); // use screen coordinates for ED4_C_DOWN only!
                
                if (y < current_y) {
                    if ((member->is_manager()) && !member->flag.hidden) {
                        ED4_terminal *result_in_manager = get_upper_lower_cursor_pos(member->to_manager(), cursor_move, current_y, isScreen, terminal_is_appropriate, seq_pos);
                        if (result_in_manager) result   = result_in_manager;
                    }

                    if ((member->dynamic_prop & ED4_P_CURSOR_ALLOWED)) {
                        if (terminal_is_appropriate==0 || terminal_is_appropriate(member, seq_pos)) {
                            result = member->to_terminal(); // overwrite (i.e. take last matching terminal)
                        }
                    }
                }
                break;
                
            case ED4_C_DOWN:
                if (!result) { // don't overwrite (i.e. take first matching terminal)
                    if ((member->is_manager()) && !member->flag.hidden) {
                        result = get_upper_lower_cursor_pos(member->to_manager(), cursor_move, current_y, isScreen, terminal_is_appropriate, seq_pos);
                    }

                    if (isScreen) ED4_ROOT->world_to_win_coords(ED4_ROOT->get_aww(), &x, &y); // if current_y is screen, convert x/y to screen-coordinates as well
                    
                    if ((member->dynamic_prop & ED4_P_CURSOR_ALLOWED) && y > current_y) {
                        ED4_multi_species_manager *marea_man = NULL; // probably multi_species_manager of middle_area, otherwise just a dummy
                        ED4_AREA_LEVEL level                 = member->get_area_level(&marea_man);

                        if (level != ED4_A_MIDDLE_AREA) {
                            if (terminal_is_appropriate==0 || terminal_is_appropriate(member, seq_pos)) {
                                result = member->to_terminal();
                            }
                        }
                        else if (level == ED4_A_MIDDLE_AREA) {
                            AW_pos y_area = marea_man->parent->extension.position[Y_POS];
                            if (y > y_area) {
                                member = starting_point->children->member(i);
                                if (terminal_is_appropriate==0 || terminal_is_appropriate(member, seq_pos)) {
                                    result = member->to_terminal();
                                }
                            }
                        }
                    }
                }
                break;
                
            default:
                e4_assert(0);
                break;
        }
    }
    
    return result;
}

/* --------------------------------------------------------------------------------
   ED4_base_position
   -------------------------------------------------------------------------------- */

ED4_base_position::ED4_base_position()
    : calced4base(0)
    , seq_pos(0)
    , count(0)
{
}

ED4_base_position::~ED4_base_position() {
    invalidate();
    delete [] seq_pos;
}

static void ed4_bp_sequence_changed_cb(ED4_species_manager *, AW_CL cl_base_pos) {
    ED4_base_position *base_pos = (ED4_base_position*)cl_base_pos;
    base_pos->invalidate();
}

void ED4_base_position::invalidate() {
    if (calced4base) {
        ED4_species_manager *species_manager = calced4base->get_parent(ED4_L_SPECIES)->to_species_manager();
        species_manager->remove_sequence_changed_cb(ed4_bp_sequence_changed_cb, (AW_CL)this);

        calced4base = 0;
    }
}

void ED4_base_position::calc4base(const ED4_base *base)
{
    e4_assert(base);

    ED4_species_manager *species_manager = base->get_parent(ED4_L_SPECIES)->to_species_manager();
    int                  len;
    char                *seq;

    if (calced4base) {
        ED4_species_manager *prev_species_manager = calced4base->get_parent(ED4_L_SPECIES)->to_species_manager();
        prev_species_manager->remove_sequence_changed_cb(ed4_bp_sequence_changed_cb, (AW_CL)this);
    }

    species_manager->add_sequence_changed_cb(ed4_bp_sequence_changed_cb, (AW_CL)this);

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
int ED4_base_position::get_base_position(const ED4_base *base, int sequence_position)
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

            if (sequence_position<seq_pos[m]) h = m;
            else                              l = m+1;
        }
    }

    return l;
}
int ED4_base_position::get_sequence_position(const ED4_base *base, int base_pos)
{
    if (!base) return 0;
    if (base!=calced4base) calc4base(base);
    return base_pos<count ? seq_pos[base_pos] : seq_pos[count-1]+1;
}

int ED4_get_base_position(const ED4_sequence_terminal *seq_term, int seq_position) {
    static ED4_base_position *base_pos = 0;
    
    if (!base_pos) base_pos = new ED4_base_position;
    return base_pos->get_base_position(seq_term, seq_position);
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
    ED4_ROOT->use_window(aww);
    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
    if (!cursor->owner_of_cursor) {
        aw_message("First you have to place the cursor.");
        return;
    }

    new CursorPos(cursor->owner_of_cursor->to_terminal(), cursor->get_sequence_pos());
}

void ED4_restore_curpos(AW_window *aww, AW_CL /*cd1*/, AW_CL /*cd2*/)
{
    GB_transaction dummy(gb_main);
    ED4_ROOT->use_window(aww);
    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;

    CursorPos *pos = CursorPos::get_head();
    if (!pos) {
        aw_message("No cursor position stored.");
        return;
    }

    pos->get_terminal()->setCursorTo(cursor, pos->get_seq_pos(), true, ED4_JUMP_KEEP_VISIBLE);
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
    GB_transaction  dummy(gb_main);
    ED4_ROOT->use_window(aww);
    ED4_cursor     *cursor = &ED4_ROOT->get_ed4w()->cursor;

    if (!cursor->owner_of_cursor) {
        aw_message("First you have to place the cursor.");
        return;
    }

    int           seq_pos  = cursor->get_sequence_pos();
    AW_helix     *helix    = ED4_ROOT->helix;
    BI_PAIR_TYPE  pairType = helix->pairtype(seq_pos);

    if (pairType != HELIX_NONE) {
        int pairing_pos = helix->opposite_position(seq_pos);
        cursor->jump_sequence_pos(aww, pairing_pos, ED4_JUMP_KEEP_POSITION);
    }
    else {
        aw_message("Not at helix position");
    }
}

void ED4_change_cursor(AW_window */*aww*/, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    ED4_cursor *cursor = &ED4_ROOT->get_ed4w()->cursor;
    ED4_CursorType typ = cursor->getType();

    cursor->changeType((ED4_CursorType)((typ+1)%ED4_CURSOR_TYPES));
}

