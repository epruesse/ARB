#include <cctype>
#include <climits>

#include <arbdbt.h>
#include <aw_awars.hxx>
#include <AW_helix.hxx>
#include <BI_basepos.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>

#include <ed4_extern.hxx>

#include "ed4_class.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_tools.hxx"
#include "ed4_awars.hxx"
#include "ed4_ProteinViewer.hxx"

#include <arb_strbuf.h>
#include <arb_defs.h>

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

    ED4_CursorShape(ED4_CursorType type, int term_height, int character_width, bool reverse_cursor);
    ~ED4_CursorShape() {}

    void draw(AW_device *device, int x, int y) const {
        e4_assert(lines);
        for (int l=0; l<lines; l++) {
            int p1 = start[l];
            int p2 = end[l];
            device->line(ED4_G_CURSOR, xpos[p1]+x, ypos[p1]+y, xpos[p2]+x, ypos[p2]+y, AW_SCREEN);
        }
        device->flush();
    }

    void get_bounding_box(int x, int y, int &xmin, int &ymin, int &xmax, int &ymax) const {
        e4_assert(points);
        xmin = ymin = INT_MAX;
        xmax = ymax = INT_MIN;
        for (int p=0; p<points; p++) {
            if (xpos[p]<xmin)      { xmin = xpos[p]; }
            else if (xpos[p]>xmax) { xmax = xpos[p]; }
            if (ypos[p]<ymin)      { ymin = ypos[p]; }
            else if (ypos[p]>ymax) { ymax = ypos[p]; }
        }

        xmin += x; xmax += x;
        ymin += y; ymax += y;
    }


};

ED4_CursorShape::ED4_CursorShape(ED4_CursorType typ, /* int x, int y, */ int term_height, int character_width, bool reverse_cursor)
{
    lines      = 0;
    points     = 0;
    reverse    = reverse_cursor;
    char_width = character_width;

    int x = 0;
    int y = 0;

    switch (typ) {
#define UPPER_OFFSET    (-1)
#define LOWER_OFFSET    (term_height-1)
#define CWIDTH          (character_width)

        case ED4_RIGHT_ORIENTED_CURSOR_THIN:
        case ED4_RIGHT_ORIENTED_CURSOR: {
            // --CWIDTH--
            //
            // 0--------1               UPPER_OFFSET (from top)
            // |3-------4
            // ||
            // ||
            // ||
            // ||
            // ||
            // ||
            // 25                       LOWER_OFFSET (from top)
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


#define UPPER_OFFSET    0
#define LOWER_OFFSET    (term_height-1)
#define CWIDTH          3

            //  -----2*CWIDTH----
            //
            //  0---------------1       UPPER_OFFSET (from top)
            //     4---------5
            //        8---9
            //         C/C
            //
            //         D/D
            //        A---B
            //     6---------7
            //  2---------------3       LOWER_OFFSET (from top)

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

            if (draw_upper) line(pu, pu);   // C/C
            line(pl, pl);   // D/D

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

#define OUTER_HEIGHT    (term_height/4)
#define INNER_HEIGHT    (term_height-1)
#define CWIDTH          12
#define STEPS           6
#define THICKNESS       2

#if (2*STEPS+THICKNESS > MAXLINES)
#error Bad definitions!
#endif

            //          2               3               |
            //            \           /                 | OUTER_HEIGHT
            //               \     /                    |
            //                  0               |       |
            //                  |               | INNER_HEIGHT
            //                  |               |
            //                  1               |
            //                /    \                    --
            //             /          \                 --
            //          4               5
            //
            //          -----------------
            //                2*WIDTH

            int s;
            for (s=0; s<STEPS; s++) {
                int size = ((STEPS-s)*CWIDTH)/STEPS;

                horizontal_line(x, y-OUTER_HEIGHT+s,                    size);
                horizontal_line(x, y+INNER_HEIGHT+OUTER_HEIGHT-s,       size);
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

ED4_returncode ED4_cursor::draw_cursor(AW_pos x, AW_pos y) { // @@@ remove return value
    if (cursor_shape) {
        delete cursor_shape;
        cursor_shape = 0;
    }

    cursor_shape = new ED4_CursorShape(ctype,
                                       SEQ_TERM_TEXT_YOFFSET+2,
                                       ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES),
                                       !awar_edit_rightward);

    cursor_shape->draw(window()->get_device(), int(x), int(y));

#if defined(TRACE_REFRESH)
    printf("draw_cursor(%i, %i)\n", int(x), int(y));
#endif // TRACE_REFRESH
    
    return ED4_R_OK;
}

ED4_returncode ED4_cursor::delete_cursor(AW_pos del_mark, ED4_base *target_terminal) {
    AW_pos x, y;
    target_terminal->calc_world_coords(&x, &y);
    ED4_base *temp_parent = target_terminal->get_parent(ED4_L_SPECIES);
    if (!temp_parent || temp_parent->flag.hidden || !is_partly_visible()) {
        return ED4_R_BREAK;
    }
    
    x = del_mark;
    win->world_to_win_coords(&x, &y);

    // refresh own terminal + terminal above + terminal below

    const int     MAX_AFFECTED = 3;
    int           affected     = 0;
    ED4_terminal *affected_terminal[MAX_AFFECTED];

    affected_terminal[affected++] = target_terminal->to_terminal();

    {
        ED4_terminal *term = target_terminal->to_terminal();
        bool backward = true;

        while (1) {
            int done = 0;

            term = backward ? term->get_prev_terminal() : term->get_next_terminal();
            if (!term) done = 1;
            else if ((term->is_sequence_terminal()) && !term->is_in_folded_group()) {
                affected_terminal[affected++] = term;
                done                          = 1;
            }

            if (done) {
                if (!backward) break;
                backward = false;
                term = target_terminal->to_terminal();
            }
        }
    }

    bool refresh_was_requested[MAX_AFFECTED];
    e4_assert(affected >= 1 && affected <= MAX_AFFECTED);
    for (int a = 0; a<affected; ++a) {
        ED4_terminal *term       = affected_terminal[a];
        refresh_was_requested[a] = term->update_info.refresh;
        term->request_refresh();
    }

    // clear rectangle where cursor is displayed

    AW_device *dev = win->get_device();
    dev->push_clip_scale();

    int xmin, xmax, ymin, ymax;

    e4_assert(cursor_shape);
    cursor_shape->get_bounding_box(int(x), int(y), xmin, ymin, xmax, ymax);

#if defined(TRACE_REFRESH)
    printf("delete_cursor(%i, %i)\n", int(x), int(y));
#endif // TRACE_REFRESH

    dev->set_font_overlap(true);

#define EXPAND_SIZE 0
    if (dev->reduceClipBorders(ymin-EXPAND_SIZE, ymax+EXPAND_SIZE, xmin-EXPAND_SIZE, xmax+EXPAND_SIZE)) {
        dev->clear_part(xmin, ymin, xmax-xmin+1, ymax-ymin+1, AW_ALL_DEVICES);
        // refresh terminal to hide cursor
        ED4_LocalWinContext uses(window());
        LocallyModify<bool> flag(allowed_to_draw, false);
        ED4_ROOT->special_window_refresh(false);
    }
    dev->pop_clip_scale();

    // restore old refresh flags (to avoid refresh of 3 complete terminals when cursor changes)
    for (int a = 0; a<affected; ++a) {
        affected_terminal[a]->update_info.refresh = refresh_was_requested[a];
    }

    return (ED4_R_OK);
}


ED4_cursor::ED4_cursor(ED4_window *win_) : win(win_) {
    init();
    allowed_to_draw = true;
    cursor_shape    = 0;

    ctype = (ED4_CursorType)(ED4_ROOT->aw_root->awar(ED4_AWAR_CURSOR_TYPE)->read_int()%ED4_CURSOR_TYPES);
}
void ED4_cursor::init() {
    // used by ED4_terminal-d-tor
    owner_of_cursor   = NULL;
    cursor_abs_x      = 0;
    screen_position   = 0;
}
ED4_cursor::~ED4_cursor() {
    delete cursor_shape;
}

int ED4_cursor::get_sequence_pos() const {
    ED4_remap *remap = ED4_ROOT->root_group_man->remap();
    size_t max_scrpos = remap->get_max_screen_pos();

    return remap->screen_to_sequence(size_t(screen_position)<=max_scrpos ? screen_position : max_scrpos);
}

bool ED4_species_manager::setCursorTo(ED4_cursor *cursor, int seq_pos, bool unfold_groups, ED4_CursorJumpType jump_type)
{
    ED4_group_manager *group_manager_to_unfold = is_in_folded_group();

    if (unfold_groups) {
        bool did_unfold = false;

        while (group_manager_to_unfold && unfold_groups) {
            ED4_base *base = group_manager_to_unfold->search_spec_child_rek(ED4_L_BRACKET);
            if (!base) break;

            base->to_bracket_terminal()->unfold();
            did_unfold = true;

            group_manager_to_unfold = is_in_folded_group();
        }

        if (did_unfold) ED4_ROOT->refresh_all_windows(1); // needed to recalculate world cache of target terminal
    }

    if (!group_manager_to_unfold) { // species manager is visible (now)
        ED4_terminal *terminal = search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_terminal();
        if (terminal) {
            if (seq_pos == -1) seq_pos = cursor->get_sequence_pos();
            cursor->set_to_terminal(terminal, seq_pos, jump_type);
            return true;
        }
    }

    return false;
}

static void jump_to_species(ED4_species_name_terminal *name_term, int seq_pos, bool unfold_groups, ED4_CursorJumpType jump_type)
{
    ED4_species_manager *species_manager = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();
    ED4_cursor *cursor = &current_cursor();
    bool jumped = false;

    if (species_manager) jumped = species_manager->setCursorTo(cursor, seq_pos, unfold_groups, jump_type);
    if (!jumped) cursor->HideCursor();
}

static bool ignore_selected_species_changes_cb = false;
static bool ignore_selected_SAI_changes_cb     = false;

static void select_named_sequence_terminal(const char *name) {
    GB_transaction ta(GLOBAL_gb_main);
    ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(name);
    if (name_term) {
        ED4_MostRecentWinContext context; // use the last used window for selection

        // lookup current name term
        ED4_species_name_terminal *cursor_name_term = 0;
        {
            ED4_cursor *cursor = &current_cursor();
            if (cursor) {
                ED4_sequence_terminal *cursor_seq_term = 0;

                if (cursor->owner_of_cursor) {
                    ED4_terminal *cursor_term = cursor->owner_of_cursor->to_text_terminal();
                    if (cursor_term->is_sequence_terminal()) {
                        cursor_seq_term = cursor_term->to_sequence_terminal();
                    }
                    else { // cursor is in a non-sequence text terminal -> search for corresponding sequence terminal
                        ED4_multi_sequence_manager *seq_man = cursor_term->get_parent(ED4_L_MULTI_SEQUENCE)->to_multi_sequence_manager();
                        if (seq_man) {
                            cursor_seq_term = seq_man->search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_sequence_terminal();
                        }
                    }
                }
                if (cursor_seq_term) {
                    cursor_name_term = cursor_seq_term->corresponding_species_name_terminal();
                }
            }
        }

        if (name_term!=cursor_name_term) { // do not change if already there!
#if defined(TRACE_JUMPS)
            printf("Jumping to species/SAI '%s'\n", name);
#endif
            jump_to_species(name_term, -1, false, ED4_JUMP_KEEP_POSITION);
        }
        else {
#if defined(TRACE_JUMPS)
            printf("Change ignored because same name term!\n");
#endif
        }
    }
}

void ED4_selected_SAI_changed_cb(AW_root * /* aw_root */) {
    if (!ignore_selected_SAI_changes_cb) {
        char *name = GBT_read_string(GLOBAL_gb_main, AWAR_SAI_NAME);

        if (name && name[0]) {
#if defined(TRACE_JUMPS)
            printf("Selected SAI is '%s'\n", name);
#endif // DEBUG
            LocallyModify<bool> flag(ED4_update_global_cursor_awars_allowed, false);
            select_named_sequence_terminal(name);
            free(name);
        }
    }
}

void ED4_selected_species_changed_cb(AW_root * /* aw_root */) {
    if (!ignore_selected_species_changes_cb) {
        char *name = GBT_read_string(GLOBAL_gb_main, AWAR_SPECIES_NAME);
        if (name && name[0]) {
#if defined(TRACE_JUMPS)
            printf("Selected species is '%s'\n", name);
#endif
            LocallyModify<bool> flag(ED4_update_global_cursor_awars_allowed, false);
            select_named_sequence_terminal(name);
        }
        free(name);
    }
    else {
#if defined(TRACE_JUMPS)
        printf("Change ignored because ignore_selected_species_changes_cb!\n");
#endif
    }
}

void ED4_jump_to_current_species(AW_window *aww, AW_CL) {
    ED4_LocalWinContext uses(aww);
    char *name = GBT_read_string(GLOBAL_gb_main, AWAR_SPECIES_NAME);
    if (name && name[0]) {
        GB_transaction ta(GLOBAL_gb_main);
#if defined(TRACE_JUMPS)
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

static bool multi_species_man_consensus_id_starts_with(ED4_base *base, AW_CL cl_start) { // TRUE if consensus id starts with 'start'
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

    return false;
}

ED4_multi_species_manager *ED4_new_species_multi_species_manager() {     // returns manager into which new species should be inserted
    ED4_base *manager = ED4_ROOT->root_group_man->find_first_that(ED4_L_MULTI_SPECIES, multi_species_man_consensus_id_starts_with, (AW_CL)"More Sequences");
    return manager ? manager->to_multi_species_manager() : 0;
}

void ED4_init_notFoundMessage() {
    not_found_counter = 0;
    not_found_message = GBS_stropen(10000);
    // GBS_strcat(not_found_message,"Species not found: ");
}
void ED4_finish_and_show_notFoundMessage() {
    if (not_found_counter != 0) {
        if (not_found_counter>MAX_SHOWN_MISSING_SPECIES) {
            GBS_strcat(not_found_message, GBS_global_string("(skipped display of %zu more species)\n", not_found_counter-MAX_SHOWN_MISSING_SPECIES));
        }
        char *out_message = GBS_strclose(not_found_message);
        aw_message(out_message);
        aw_message(GBS_global_string("Couldn't load %zu species:", not_found_counter));
        free(out_message);
    }
    else {
        GBS_strforget(not_found_message);
    }
    not_found_message = 0;
}

void ED4_get_and_jump_to_species(GB_CSTR species_name)
{
    e4_assert(species_name && species_name[0]);

    GB_transaction ta(GLOBAL_gb_main);
    ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(species_name);
    int loaded = 0;

    if (!name_term) { // insert new species
        ED4_multi_species_manager *insert_into_manager = ED4_new_species_multi_species_manager();
        ED4_group_manager         *group_man           = insert_into_manager->get_parent(ED4_L_GROUP)->to_group_manager();
        char                      *string              = (char*)GB_calloc(strlen(species_name)+3, sizeof(*string));
        int                        index               = 0;
        ED4_index                  y                   = 0;
        ED4_index                  lot                 = 0;

        ED4_init_notFoundMessage();

        sprintf(string, "-L%s", species_name);
        ED4_ROOT->database->fill_species(insert_into_manager,
                                         ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence(),
                                         string, &index, &y, 0, &lot, insert_into_manager->calc_group_depth(), NULL);
        loaded = 1;

        ED4_finish_and_show_notFoundMessage();

        {
            GBDATA *gb_species = GBT_find_species(GLOBAL_gb_main, species_name);
            GBDATA *gbd = GBT_find_sequence(gb_species, ED4_ROOT->alignment_name);

            if (gbd) {
                char *data = GB_read_string(gbd);
                int   len  = GB_read_string_count(gbd);

                group_man->update_bases(0, 0, data, len);
            }
        }

        name_term = ED4_find_species_name_terminal(species_name);
        if (name_term) {
            // new AAseqTerminals should be created if it is in ProtView mode
            if (ED4_ROOT->alignment_type == GB_AT_DNA) {
                PV_AddCorrespondingOrfTerminals(name_term);
            }
            ED4_ROOT->main_manager->request_resize(); // @@@ instead needs to be called whenever adding or deleting PV-terminals
            // it should create new AA Sequence terminals if the protein viewer is enabled
        }
        delete string;

        insert_into_manager->invalidate_species_counters();
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

void ED4_get_and_jump_to_current(AW_window *aww, AW_CL) {
    ED4_LocalWinContext uses(aww);
    char *name = GBT_read_string(GLOBAL_gb_main, AWAR_SPECIES_NAME);
    if (name && name[0]) {
        ED4_get_and_jump_to_species(name);
    }
    else {
        aw_message("Please select a species");
    }
}

void ED4_get_and_jump_to_current_from_menu(AW_window *aw, AW_CL cl, AW_CL) {
    ED4_get_and_jump_to_current(aw, cl);
}

void ED4_get_marked_from_menu(AW_window *, AW_CL, AW_CL) {
#define BUFFERSIZE 1000
    GB_transaction ta(GLOBAL_gb_main);
    int marked = GBT_count_marked_species(GLOBAL_gb_main);

    if (marked) {
        GBDATA *gb_species = GBT_first_marked_species(GLOBAL_gb_main);
        char   *buffer     = new char[BUFFERSIZE+1];
        char   *bp         = buffer;

        ED4_multi_species_manager *insert_into_manager = ED4_new_species_multi_species_manager();
        ED4_group_manager         *group_man           = insert_into_manager->get_parent(ED4_L_GROUP)->to_group_manager();

        int        group_depth       = insert_into_manager->calc_group_depth();
        int        index             = 0;
        ED4_index  y                 = 0;
        ED4_index  lot               = 0;
        int        inserted          = 0;
        char      *default_alignment = GBT_get_default_alignment(GLOBAL_gb_main);

        arb_progress progress("Loading species", marked);

        ED4_init_notFoundMessage();

        while (gb_species) {
            const char *name = GBT_read_name(gb_species);
            ED4_species_name_terminal *name_term = ED4_find_species_name_terminal(name);

            if (!name_term) { // species not found -> insert
                {
                    int namelen = strlen(name);
                    int buffree = BUFFERSIZE-int(bp-buffer);

                    if ((namelen+2)>buffree) {
                        *bp = 0;
                        ED4_ROOT->database->fill_species(insert_into_manager,
                                                         ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence(),
                                                         buffer, &index, &y, 0, &lot, group_depth, NULL);
                        bp = buffer;
                        index = 0;
                    }

                    *bp++ = 1;
                    *bp++ = 'L';
                    memcpy(bp, name, namelen);
                    bp += namelen;
                }

                {
                    GBDATA *gbd = GBT_find_sequence(gb_species, default_alignment);

                    if (gbd) {
                        char *data = GB_read_string(gbd);
                        int len = GB_read_string_count(gbd);
                        group_man->table().add(data, len);
                    }
                }

                inserted++;
            }
            gb_species = GBT_next_marked_species(gb_species);
            progress.inc();
        }

        if (bp>buffer) {
            *bp++ = 0;
            ED4_ROOT->database->fill_species(insert_into_manager,
                                             ED4_ROOT->ref_terminals.get_ref_sequence_info(), ED4_ROOT->ref_terminals.get_ref_sequence(),
                                             buffer, &index, &y, 0, &lot, group_depth, NULL);
        }

        aw_message(GBS_global_string("Loaded %i of %i marked species.", inserted, marked));

        ED4_finish_and_show_notFoundMessage();

        if (inserted) {
            // new AAseqTerminals should be created if it is in ProtView mode
            if (ED4_ROOT->alignment_type == GB_AT_DNA) {
                PV_AddOrfTerminalsToLoadedSpecies();
            }
            ED4_ROOT->main_manager->request_resize(); // @@@ instead needs to be called whenever adding or deleting PV-terminals
        }

        delete [] buffer;
        free(default_alignment);
    }
    else {
        aw_message("No species marked.");
    }

#undef BUFFERSIZE
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
        ED4_terminal *old_owner_of_cursor = owner_of_cursor;

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

bool ED4_update_global_cursor_awars_allowed = true;

static void trace_termChange_in_global_awar(ED4_terminal *term, char*& last_value, bool& ignore_flag, const char *awar_name) {
    char *species_name = term->get_name_of_species();

    if (!last_value || strcmp(last_value, species_name) != 0) {
        freeset(last_value, species_name);
        LocallyModify<bool> raise(ignore_flag, true);
#if defined(TRACE_JUMPS)
        fprintf(stderr, "Writing '%s' to awar '%s'\n", species_name, awar_name);
#endif
        GBT_write_string(GLOBAL_gb_main, awar_name, species_name);
    }
    else {
        free(species_name);
    }
}

void ED4_cursor::updateAwars(bool new_term_selected) {
    AW_root *aw_root = ED4_ROOT->aw_root;
    int      seq_pos = get_sequence_pos();

    if (ED4_update_global_cursor_awars_allowed && new_term_selected) {
        // @@@ last_set_XXX has to be window-specific (or better cursor specific)
        if (in_SAI_terminal()) {
            static char *last_set_SAI = 0;
            trace_termChange_in_global_awar(owner_of_cursor, last_set_SAI, ignore_selected_SAI_changes_cb, AWAR_SAI_NAME);
        }
        else if (in_species_seq_terminal()) {
            static char *last_set_species = 0;
            trace_termChange_in_global_awar(owner_of_cursor, last_set_species, ignore_selected_species_changes_cb, AWAR_SPECIES_NAME);
        }
    }

    // update awars for cursor position:
    aw_root->awar(win->awar_path_for_cursor)->write_int(info2bio(seq_pos));
    if (ED4_update_global_cursor_awars_allowed) {
        aw_root->awar(AWAR_CURSOR_POSITION)->write_int(info2bio(seq_pos)); // update ARB-global cursor position
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
    BI_ecoli_ref *ecoli = ED4_ROOT->ecoli_ref;
    if (ecoli->gotData()) {
        int ecoli_pos = ecoli->abs_2_rel(seq_pos+1); // inclusive position behind cursor
        aw_root->awar(win->awar_path_for_Ecoli)->write_int(ecoli_pos);
    }

    // update awar for base position:
    int base_pos = base_position.get_base_position(owner_of_cursor, seq_pos+1); // inclusive position behind cursor
    aw_root->awar(win->awar_path_for_basePos)->write_int(base_pos);

    // update awar for IUPAC:

#define MAXIUPAC 6

    char iupac[MAXIUPAC+1];
    strcpy(iupac, ED4_IUPAC_EMPTY);

    {
        char at[2] = "\0";

        if (in_consensus_terminal()) {
            ED4_group_manager *group_manager = owner_of_cursor->get_parent(ED4_L_GROUP)->to_group_manager();
            ED4_char_table&    groupTab      = group_manager->table();
            if (seq_pos<groupTab.size()) {
                groupTab.build_consensus_string_to(at, ExplicitRange(seq_pos, seq_pos));
            }
        }
        else if (in_species_seq_terminal() || in_SAI_terminal()) {
            int         len;
            const char *seq = owner_of_cursor->resolve_pointer_to_char_pntr(&len);

            if (seq_pos<len) at[0] = seq[seq_pos];
        }

        if (at[0]) {
            char base = at[0];
            const char *i = ED4_decode_iupac(base, ED4_ROOT->alignment_type);

            e4_assert(strlen(i)<=MAXIUPAC);
            strcpy(iupac, i);
        }
    }

#undef MAXIUPAC

    aw_root->awar(win->awar_path_for_IUPAC)->write_string(iupac);

    // update awar for helix#:
    const char *helixNr = ED4_ROOT->helix->helixNr(seq_pos);
    aw_root->awar(win->awar_path_for_helixNr)->write_string(helixNr ? helixNr : "");
}

int ED4_cursor::get_screen_relative_pos() const {
    const ED4_coords& coords = window()->coords;
    return cursor_abs_x - coords.window_left_clip_point;
}
void ED4_cursor::set_screen_relative_pos(int scroll_to_relpos) {
    int curr_rel_pos  = get_screen_relative_pos();
    int scroll_amount = curr_rel_pos-scroll_to_relpos;

    int length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);
    scroll_amount      = (scroll_amount/length_of_char)*length_of_char; // align to char-size

    if (scroll_amount != 0) {
        AW_window *aww = window()->aww;
        aww->set_horizontal_scrollbar_position(aww->slider_pos_horizontal + scroll_amount);
#if defined(TRACE_JUMPS)
        printf("set_screen_relative_pos(%i) auto-scrolls %i\n", scroll_to_relpos, scroll_amount);
#endif
        ED4_horizontal_change_cb(aww);
    }
}


void ED4_cursor::jump_screen_pos(int screen_pos, ED4_CursorJumpType jump_type) {
    if (!owner_of_cursor) {
        aw_message("First you have to place the cursor");
        return;
    }

    if (owner_of_cursor->is_hidden()) return; // do not jump if cursor terminal is hidden

    AW_pos terminal_x, terminal_y;
    owner_of_cursor->calc_world_coords(&terminal_x, &terminal_y);

#if defined(TRACE_JUMPS)
    printf("jump_screen_pos(%i)\n", screen_pos);
#endif

    ED4_terminal *term = owner_of_cursor;
    ED4_window   *ed4w = window();

    term->scroll_into_view(ed4w); // correct y-position of terminal

    int cursor_diff = screen_pos-screen_position;
    if (cursor_diff == 0) { // cursor position did not change
        if (jump_type == ED4_JUMP_KEEP_VISIBLE) return; // nothing special -> done
    }

    int terminal_pixel_length = ed4w->get_device()->get_string_size(ED4_G_SEQUENCES, 0, owner_of_cursor->to_text_terminal()->get_length());
    int length_of_char        = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);
    int abs_x_new             = cursor_abs_x+cursor_diff*length_of_char; // position in terminal

    if (abs_x_new > terminal_x+terminal_pixel_length+CHARACTEROFFSET || abs_x_new < terminal_x+CHARACTEROFFSET) {
        return; // don`t move out of terminal
    }

    ED4_coords *coords = &ed4w->coords;

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

    if (scroll_new_to >= 0) {   // scroll
        int rel_x_new     = abs_x_new-coords->window_left_clip_point;
        int scroll_amount = rel_x_new-scroll_new_to;

        if      (scroll_amount>0) scroll_amount += length_of_char/2;
        else if (scroll_amount<0) scroll_amount -= length_of_char/2;

        scroll_amount = (scroll_amount/length_of_char)*length_of_char; // align to char-size
        if (scroll_amount != 0) {
            AW_window *aww  = ed4w->aww;
            aww->set_horizontal_scrollbar_position(aww->slider_pos_horizontal + scroll_amount);
#if defined(TRACE_JUMPS)
            printf("jump_screen_pos auto-scrolls %i\n", scroll_amount);
#endif
            LocallyModify<bool> flag(allowed_to_draw, false);
            ED4_horizontal_change_cb(aww);
        }
    }

    LocallyModify<bool> flag(allowed_to_draw, true);
    if (cursor_diff >= 0) {
        ShowCursor(cursor_diff*length_of_char, ED4_C_RIGHT, abs(cursor_diff));
    }
    else {
        ShowCursor(cursor_diff*length_of_char, ED4_C_LEFT, abs(cursor_diff));
    }
#if defined(ASSERTION_USED)
    {
        int sp = get_screen_pos();
        e4_assert(sp == screen_pos);
    }
#endif
}

void ED4_cursor::jump_sequence_pos(int seq_pos, ED4_CursorJumpType jump_type) {
    ED4_remap *remap = ED4_ROOT->root_group_man->remap();

    if (remap->is_shown(seq_pos)) {
        int screen_pos = remap->sequence_to_screen(seq_pos);
        jump_screen_pos(screen_pos, jump_type);
#if defined(ASSERTION_USED)
        if (owner_of_cursor) {
            int res_seq_pos = get_sequence_pos();
            e4_assert(res_seq_pos == seq_pos);
        }
#endif
    }
    else {
        int scr_left, scr_right;
        remap->adjacent_screen_positions(seq_pos, scr_left, scr_right);
        jump_screen_pos(scr_right>=0 ? scr_right : scr_left, jump_type);
    }
}

void ED4_cursor::jump_base_pos(int base_pos, ED4_CursorJumpType jump_type) {
    int seq_pos = base2sequence_position(base_pos);
    jump_sequence_pos(seq_pos, jump_type);

#if defined(ASSERTION_USED)
    if (owner_of_cursor) {
        int bp = get_base_position();
        e4_assert(bp == base_pos);
    }
#endif
}

class has_base_at : public ED4_TerminalPredicate {
    int  seq_pos;
    bool want_base;
public:
    has_base_at(int seq_pos_, bool want_base_) : seq_pos(seq_pos_), want_base(want_base_) {}
    bool fulfilled_by(const ED4_terminal *terminal) const OVERRIDE {
        bool test_succeeded = false;

        if (terminal->is_sequence_terminal()) {
            const ED4_sequence_terminal *seqTerm = terminal->to_sequence_terminal();
            int len;
            char *seq = seqTerm->resolve_pointer_to_string_copy(&len);
            if (seq) {
                test_succeeded = len>seq_pos && bool(ADPP_IS_ALIGN_CHARACTER(seq[seq_pos]))!=want_base;
            }
            free(seq);
        }

        return test_succeeded;
    }
};

struct acceptAnyTerminal : public ED4_TerminalPredicate {
    bool fulfilled_by(const ED4_terminal *) const OVERRIDE { return true; }
};

static ED4_terminal *get_upper_lower_cursor_pos(ED4_manager *starting_point, ED4_cursor_move cursor_move, AW_pos current_y, bool isScreen, const ED4_TerminalPredicate& predicate) {
    // current_y is y-position of terminal at which search starts.
    // It may be in world or screen coordinates (isScreen marks which is used)
    // This is needed to move the cursor from top- to middle-area w/o scrolling middle-area to top-position.

    ED4_terminal *result = 0;

    int m      = 0;
    int last_m = starting_point->children->members()-1;
    int incr   = 1;

    if (cursor_move == ED4_C_UP) {
        std::swap(m, last_m); incr = -1; // revert search direction
        e4_assert(!isScreen);            // use screen coordinates for ED4_C_DOWN only!
    }

    while (!result) {
        ED4_base *member = starting_point->children->member(m);

        if (member->is_manager()) {
            if (!member->flag.hidden) { // step over hidden managers (e.g. folded groups)
                result = get_upper_lower_cursor_pos(member->to_manager(), cursor_move, current_y, isScreen, predicate);
            }
        }
        else {
            if (member->dynamic_prop & ED4_P_CURSOR_ALLOWED) {
                AW_pos x, y;
                member->calc_world_coords(&x, &y);
                if (isScreen) current_ed4w()->world_to_win_coords(&x, &y); // if current_y is screen, convert x/y to screen-coordinates as well

                bool pos_beyond = (cursor_move == ED4_C_UP) ? y<current_y : y>current_y;
                if (pos_beyond) {
                    bool skip_top_scrolled = false;
                    if (isScreen) {
                        ED4_multi_species_manager *marea_man = NULL;
                        if (member->get_area_level(&marea_man) == ED4_A_MIDDLE_AREA) {
                            // previous position was in top-area -> avoid selecting scrolled-out terminals
                            AW_pos y_area     = marea_man->parent->extension.position[Y_POS];
                            skip_top_scrolled = y<y_area;
                        }
                    }

                    if (!skip_top_scrolled) {
                        ED4_terminal *term = member->to_terminal();
                        if (predicate.fulfilled_by(term)) result = term;
                    }
                }
            }
        }

        if (m == last_m) break;
        m += incr;
    }

    return result;
}

struct acceptConsensusTerminal : public ED4_TerminalPredicate {
    bool fulfilled_by(const ED4_terminal *term) const OVERRIDE { return term->is_consensus_terminal(); }
};

ED4_returncode ED4_cursor::move_cursor(AW_event *event) {
    // move cursor up down
    ED4_cursor_move dir     = ED4_C_NONE;
    ED4_returncode  result  = ED4_R_OK;
    bool            endHome = false;

    switch (event->keycode) {
        case AW_KEY_UP:   dir = ED4_C_UP;   break;
        case AW_KEY_DOWN: dir = ED4_C_DOWN; break;
        case AW_KEY_HOME: dir = ED4_C_UP;   endHome = true; break;
        case AW_KEY_END:  dir = ED4_C_DOWN; endHome = true; break;
        default: e4_assert(0); break; // illegal call of move_cursor()
    }

    if (dir != ED4_C_NONE) {
        if (owner_of_cursor->is_hidden()) result = ED4_R_IMPOSSIBLE; // don't move cursor if terminal is hidden

        if (result == ED4_R_OK) {
            AW_pos x_dummy, y_world;

            owner_of_cursor->calc_world_coords(&x_dummy, &y_world);

            int seq_pos = get_sequence_pos();
            ED4_terminal *target_terminal = 0;

            // stay in current area
            ED4_multi_species_manager *start_at_manager = 0;
            ED4_AREA_LEVEL             area_level       = owner_of_cursor->get_area_level(&start_at_manager);

            if (event->keymodifier & AW_KEYMODE_CONTROL) {
                bool has_base = has_base_at(seq_pos, true).fulfilled_by(owner_of_cursor);

                if (!endHome) { // not End or Home
                    target_terminal = get_upper_lower_cursor_pos(start_at_manager, dir, y_world, false, has_base_at(seq_pos, !has_base));
                    if (target_terminal && !has_base && ED4_ROOT->aw_root->awar(ED4_AWAR_FAST_CURSOR_JUMP)->read_int()) {  // if jump_over group
                        target_terminal->calc_world_coords(&x_dummy, &y_world);
                        target_terminal = get_upper_lower_cursor_pos(start_at_manager, dir, y_world, false, has_base_at(seq_pos, false));
                    }
                }

                if (target_terminal == 0) {
                    // either already in last group (or no space behind last group) -> jump to end (or start)
                    target_terminal = get_upper_lower_cursor_pos(start_at_manager,
                                                                 dir == ED4_C_UP ? ED4_C_DOWN : ED4_C_UP, // use opposite movement direction
                                                                 dir == ED4_C_UP ? 0 : INT_MAX, false, // search for top-/bottom-most terminal
                                                                 acceptAnyTerminal());
                }
            }
            else {
                e4_assert(!endHome); // END and HOME w/o Ctrl should not call move_cursor()
                if (event->keymodifier & AW_KEYMODE_ALT) {
                    target_terminal = get_upper_lower_cursor_pos(start_at_manager, dir, y_world, false, acceptConsensusTerminal());
                }
                else {
                    bool isScreen = false;
                    if (dir == ED4_C_DOWN) {
                        if (area_level == ED4_A_TOP_AREA) {
                            window()->world_to_win_coords(&x_dummy, &y_world); // special handling to move cursor from top to bottom area
                            isScreen = true;
                        }
                    }
                    target_terminal = get_upper_lower_cursor_pos(ED4_ROOT->main_manager, dir, y_world, isScreen, acceptAnyTerminal());
                }
            }

            if (target_terminal) {
                set_to_terminal(target_terminal, seq_pos, ED4_JUMP_KEEP_VISIBLE);
            }
        }
    }

    return result;
}

void ED4_cursor::set_abs_x() {
    AW_pos x, y;
    owner_of_cursor->calc_world_coords(&x, &y);
    cursor_abs_x = int(get_sequence_pos()*ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES) + CHARACTEROFFSET + x);
}


ED4_returncode ED4_cursor::ShowCursor(ED4_index offset_x, ED4_cursor_move move, int move_pos) {
    AW_pos x=0, y=0, x_help = 0, y_help;

    owner_of_cursor->calc_world_coords(&x, &y);

    switch (move) {
        case ED4_C_RIGHT: screen_position += move_pos; break;
        case ED4_C_LEFT:  screen_position -= move_pos; break;
        case ED4_C_NONE: break; // no move - only redisplay
        default: e4_assert(0); break;
    }

    {
        LocallyModify<bool> flag(ED4_update_global_cursor_awars_allowed, ED4_update_global_cursor_awars_allowed && move != ED4_C_NONE);
        updateAwars(false);
    }

    x_help = cursor_abs_x + offset_x;
    y_help = y;

    window()->world_to_win_coords(&x_help, &y_help);

    if (allowed_to_draw) draw_cursor(x_help, y_help);
#if defined(DEBUG) && 0
    else printf("Skip draw_cursor (allowed_to_draw=false)\n");
#endif // DEBUG

    cursor_abs_x += offset_x;

    return (ED4_R_OK);
}

bool ED4_cursor::is_hidden_inside_group() const {
    if (!owner_of_cursor) return false;
    if (!owner_of_cursor->is_in_folded_group()) return false;
    if (in_consensus_terminal()) {
        ED4_base *group_man = owner_of_cursor->get_parent(ED4_L_GROUP);
        e4_assert(group_man);
        return group_man->is_in_folded_group();
    }
    return true;
}

bool ED4_cursor::is_partly_visible() const {
    e4_assert(owner_of_cursor);
    e4_assert(cursor_shape); // cursor is not drawn, cannot test visibility

    if (is_hidden_inside_group()) {
        printf("is_hidden_inside_group=true\n");
        return false;
    }

    AW_pos x, y;
    owner_of_cursor->calc_world_coords(&x, &y);

    int x1, y1, x2, y2;
    cursor_shape->get_bounding_box(cursor_abs_x, int(y), x1, y1, x2, y2);

    bool visible = false;

    switch (owner_of_cursor->get_area_level(0)) {
        case ED4_A_TOP_AREA:    visible = win->shows_xpos(x1) || win->shows_xpos(x2); break;
        case ED4_A_MIDDLE_AREA: visible = win->partly_shows(x1, y1, x2, y2); break;
        default: break;
    }

    return visible;
}

bool ED4_cursor::is_completely_visible() const {
    e4_assert(owner_of_cursor);
    e4_assert(cursor_shape); // cursor is not drawn, cannot test visibility

    if (is_hidden_inside_group()) return false;

    AW_pos x, y;
    owner_of_cursor->calc_world_coords(&x, &y);

    int x1, y1, x2, y2;
    cursor_shape->get_bounding_box(cursor_abs_x, int(y), x1, y1, x2, y2);

    bool visible = false;

    switch (owner_of_cursor->get_area_level(0)) {
        case ED4_A_TOP_AREA:    visible = win->shows_xpos(x1) && win->shows_xpos(x2); break;
        case ED4_A_MIDDLE_AREA: visible = win->completely_shows(x1, y1, x2, y2); break;
        default: break;
    }

    return visible;
}

#if defined(DEBUG) && 0
#define DUMP_SCROLL_INTO_VIEW
#endif

void ED4_terminal::scroll_into_view(ED4_window *ed4w) { // scroll y-position only
    ED4_LocalWinContext uses(ed4w);

    AW_pos termw_x, termw_y;
    calc_world_coords(&termw_x, &termw_y);

    ED4_coords *coords  = &current_ed4w()->coords;

    int term_height = int(extension.size[1]);
    int win_ysize   = coords->window_lower_clip_point - coords->window_upper_clip_point + 1;

    bool scroll = false;
    int  slider_pos_y;

    ED4_cursor_move direction = ED4_C_NONE;

    AW_pos termw_y_upper = termw_y; // upper border of terminal
    AW_pos termw_y_lower = termw_y + term_height; // lower border of terminal

    if (termw_y_upper > coords->top_area_height) { // don't scroll if terminal is in top area (always visible)
        if (termw_y_upper < coords->window_upper_clip_point) { // scroll up
#ifdef DUMP_SCROLL_INTO_VIEW
            printf("termw_y_upper(%i) < window_upper_clip_point(%i)\n",
                   int(termw_y_upper), int(coords->window_upper_clip_point));
#endif // DEBUG
            slider_pos_y = int(termw_y_upper - coords->top_area_height - term_height);
            scroll       = true;
            direction    = ED4_C_UP;
        }
        else if (termw_y_lower > coords->window_lower_clip_point) { // scroll down
#ifdef DUMP_SCROLL_INTO_VIEW
            printf("termw_y_lower(%i) > window_lower_clip_point(%i)\n",
                   int(termw_y_lower), int(coords->window_lower_clip_point));
#endif // DEBUG
            slider_pos_y = int(termw_y_upper - coords->top_area_height - win_ysize);
            scroll       = true;
            direction    = ED4_C_DOWN;
        }
    }

#ifdef DUMP_SCROLL_INTO_VIEW
    if (!scroll) {
        printf("No scroll needed (termw_y_upper=%i termw_y_lower=%i term_height=%i window_upper_clip_point=%i window_lower_clip_point=%i)\n",
               int(termw_y_upper), int(termw_y_lower), term_height,
               int(coords->window_upper_clip_point), int(coords->window_lower_clip_point));
    }
#endif // DEBUG

    if (scroll) {
        AW_window *aww = ed4w->aww;

        int pic_ysize         = int(aww->get_scrolled_picture_height());
        int slider_pos_yrange = pic_ysize - win_ysize;

        if (slider_pos_yrange<0) { // win_ysize > pic_ysize
            slider_pos_y = 0;
        }
        else {
            ED4_multi_species_manager *start_at_manager = 0;
            get_area_level(&start_at_manager);
            
            ED4_terminal *nextTerm = get_upper_lower_cursor_pos(start_at_manager, direction, termw_y, false, acceptAnyTerminal());
            if (!nextTerm) { // no next term in this area
                slider_pos_y = direction == ED4_C_UP ? 0 : slider_pos_yrange; // scroll to limit
            }
        }

        if (slider_pos_y>slider_pos_yrange) slider_pos_y = slider_pos_yrange;
        if (slider_pos_y<0) slider_pos_y                 = 0;

        aww->set_vertical_scrollbar_position(slider_pos_y);
        ED4_scrollbar_change_cb(aww);
    }
}

void ED4_cursor::set_to_terminal(ED4_terminal *terminal, int seq_pos, ED4_CursorJumpType jump_type) {
    if (seq_pos == -1) seq_pos = get_sequence_pos();

    bool new_term_selected = owner_of_cursor != terminal;
    if (new_term_selected) {
        if (owner_of_cursor) {
            if (get_sequence_pos() != seq_pos) {
                jump_sequence_pos(seq_pos, jump_type); // position to wanted column -- scrolls horizontally
            }
        }

        int scr_pos = ED4_ROOT->root_group_man->remap()->sequence_to_screen(seq_pos);
        show_cursor_at(terminal, scr_pos);

        if (!is_completely_visible()) {
            jump_sequence_pos(seq_pos, jump_type);
        }
    }
    else {
        jump_sequence_pos(seq_pos, jump_type);
    }

    GB_transaction ta(GLOBAL_gb_main);
    updateAwars(new_term_selected);
}

ED4_returncode ED4_cursor::show_cursor_at(ED4_terminal *target_terminal, ED4_index scr_pos) {
    bool new_term_selected = owner_of_cursor != target_terminal;
    
    if (owner_of_cursor) {
        if (is_partly_visible()) {
            delete_cursor(cursor_abs_x, owner_of_cursor);
        }
        owner_of_cursor = NULL;
        DRAW = 1;
    }

    target_terminal->scroll_into_view(window());

    AW_pos termw_x, termw_y;
    target_terminal->calc_world_coords(&termw_x, &termw_y);

    screen_position = scr_pos;

    int length_of_char = ED4_ROOT->font_group.get_width(ED4_G_SEQUENCES);

    AW_pos world_x = termw_x + length_of_char*screen_position + CHARACTEROFFSET;
    AW_pos world_y = termw_y;

    AW_pos win_x = world_x;
    AW_pos win_y = world_y;
    window()->world_to_win_coords(&win_x, &win_y);

    cursor_abs_x = (long int)world_x;
    owner_of_cursor = target_terminal;

    draw_cursor(win_x, win_y);

    GB_transaction ta(GLOBAL_gb_main);
    updateAwars(new_term_selected);

    return ED4_R_OK;
}

ED4_returncode ED4_cursor::show_clicked_cursor(AW_pos click_xpos, ED4_terminal *target_terminal)
{
    AW_pos termw_x, termw_y;
    target_terminal->calc_world_coords(&termw_x, &termw_y);

    ED4_index scr_pos = ED4_ROOT->pixel2pos(click_xpos - termw_x);
    return show_cursor_at(target_terminal, scr_pos);
}


/* --------------------------------------------------------------------------------
   ED4_base_position
   -------------------------------------------------------------------------------- */

static void ed4_bp_sequence_changed_cb(ED4_species_manager *, AW_CL cl_base_pos) {
    ED4_base_position *base_pos = (ED4_base_position*)cl_base_pos;
    base_pos->invalidate();
}

void ED4_base_position::remove_changed_cb() {
    if (calced4term) {
        ED4_species_manager *species_manager = calced4term->get_parent(ED4_L_SPECIES)->to_species_manager();
        species_manager->remove_sequence_changed_cb(ed4_bp_sequence_changed_cb, (AW_CL)this);

        calced4term = NULL;
    }
}

static bool is_gap(char c) { return ED4_is_align_character[safeCharIndex(c)]; }
static bool is_consensus_gap(char c) { return ED4_is_align_character[safeCharIndex(c)] || c == '='; }

void ED4_base_position::calc4term(const ED4_terminal *base) {
    e4_assert(base);

    ED4_species_manager *species_manager = base->get_parent(ED4_L_SPECIES)->to_species_manager();

    int   len;
    char *seq;

    if (base != calced4term) { // terminal changes => rebind callback to new manager
        remove_changed_cb();
        species_manager->add_sequence_changed_cb(ed4_bp_sequence_changed_cb, (AW_CL)this);
    }

    bool (*isGap_fun)(char);
    if (species_manager->is_consensus_manager()) {
        ED4_group_manager *group_manager = base->get_parent(ED4_L_GROUP)->to_group_manager();

        seq       = group_manager->table().build_consensus_string();
        len       = strlen(seq);
        isGap_fun = is_consensus_gap;
    }
    else {
        seq = base->resolve_pointer_to_string_copy(&len); 
        e4_assert((int)strlen(seq) == len);
        isGap_fun = is_gap;
    }

    e4_assert(seq);

#if defined(WARN_TODO)
#warning ED4_is_align_character is kinda CharPredicate - refactor
#endif
    CharPredicate pred_is_gap(isGap_fun);
    initialize(seq, len, pred_is_gap);
    calced4term = base;

    free(seq);
}
int ED4_base_position::get_base_position(const ED4_terminal *term, int sequence_position) {
    if (!term) return 0;
    set_term(term);
    return abs_2_rel(sequence_position);
}
int ED4_base_position::get_sequence_position(const ED4_terminal *term, int base_pos) {
    if (!term) return 0;
    set_term(term);
    return rel_2_abs(base_pos);
}

/* --------------------------------------------------------------------------------
   Store/Restore Cursorpositions
   -------------------------------------------------------------------------------- */

class CursorPos : virtual Noncopyable {
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

void ED4_store_curpos(AW_window *aww) {
    GB_transaction       ta(GLOBAL_gb_main);
    ED4_LocalWinContext  uses(aww);
    ED4_cursor          *cursor = &current_cursor();
    
    if (!cursor->owner_of_cursor) {
        aw_message("First you have to place the cursor.");
    }
    else {
        new CursorPos(cursor->owner_of_cursor, cursor->get_sequence_pos());
    }
}

void ED4_restore_curpos(AW_window *aww) {
    GB_transaction       ta(GLOBAL_gb_main);
    ED4_LocalWinContext  uses(aww);
    ED4_cursor          *cursor = &current_cursor();

    CursorPos *pos = CursorPos::get_head();
    if (!pos) {
        aw_message("No cursor position stored.");
        return;
    }

    pos->get_terminal()->setCursorTo(cursor, pos->get_seq_pos(), true, ED4_JUMP_KEEP_VISIBLE);
    pos->moveToEnd();
}

void ED4_clear_stored_curpos() {
    CursorPos::clear();
}

/* --------------------------------------------------------------------------------
   Other stuff
   -------------------------------------------------------------------------------- */

void ED4_helix_jump_opposite(AW_window *aww) {
    GB_transaction       ta(GLOBAL_gb_main);
    ED4_LocalWinContext  uses(aww);
    ED4_cursor          *cursor = &current_cursor();

    if (!cursor->owner_of_cursor) {
        aw_message("First you have to place the cursor.");
        return;
    }

    int           seq_pos  = cursor->get_sequence_pos();
    AW_helix     *helix    = ED4_ROOT->helix;
    BI_PAIR_TYPE  pairType = helix->pairtype(seq_pos);

    if (pairType != HELIX_NONE) {
        int pairing_pos = helix->opposite_position(seq_pos);
        cursor->jump_sequence_pos(pairing_pos, ED4_JUMP_KEEP_POSITION);
    }
    else {
        aw_message("Not at helix position");
    }
}

void ED4_change_cursor(AW_window *aww) {
    ED4_LocalWinContext uses(aww);
    ED4_cursor     *cursor = &current_cursor();
    ED4_CursorType  typ    = cursor->getType();

    cursor->changeType((ED4_CursorType)((typ+1)%ED4_CURSOR_TYPES));
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>
#include <test_helpers.h>

struct test_absrel {
    bool torel;
    test_absrel(bool r) : torel(r) {}
    virtual ~test_absrel() {}

    virtual int a2r(int a) const = 0;
    virtual int r2a(int r) const = 0;
    
    virtual int abs_size() const = 0;
    virtual int rel_size() const = 0;

    int gen(int i) const { return torel ? a2r(i) : r2a(i); }
    int get_size() const { return torel ? abs_size()+1 : rel_size(); }

    char *genResult() const;
};

struct membind {
    const test_absrel& me;
    membind(const test_absrel& ta) : me(ta) {}
    int operator()(int i) const { return me.gen(i); }
};

char *test_absrel::genResult() const {
    return collectIntFunResults(membind(*this), 0, get_size()-1, 3, 1, 1);
}

struct test_ecoli : public test_absrel {
    BI_ecoli_ref& eref;
    test_ecoli(BI_ecoli_ref& b, bool to_rel) : test_absrel(to_rel), eref(b) {}
    int a2r(int a) const OVERRIDE { return eref.abs_2_rel(a); }
    int r2a(int r) const OVERRIDE { return eref.rel_2_abs(r); }
    int abs_size() const OVERRIDE { return eref.abs_count(); }
    int rel_size() const OVERRIDE { return eref.base_count(); }
};

struct test_basepos : public test_absrel {
    BasePosition& bpos;
    test_basepos(BasePosition& bpos_, bool to_rel)
        : test_absrel(to_rel), bpos(bpos_) {}
    int a2r(int a) const OVERRIDE { return bpos.abs_2_rel(a); }
    int r2a(int r) const OVERRIDE { return bpos.rel_2_abs(r); }
    int abs_size() const OVERRIDE { return bpos.abs_count(); }
    int rel_size() const OVERRIDE { return bpos.base_count(); }
};

#define TEST_ABSREL_EQUALS(tester,expected) do {        \
        char *res = tester.genResult();                 \
        TEST_EXPECT_EQUAL(res,expected);                \
        free(res);                                      \
    } while(0)

#define TEST_ECOLIREF_EQUALS(data,alen,a2r,r2a) do {            \
        BI_ecoli_ref eref;                                      \
        eref.init(data,alen);                                   \
        TEST_ABSREL_EQUALS(test_ecoli(eref, true), a2r);        \
        TEST_ABSREL_EQUALS(test_ecoli(eref, false), r2a);       \
    } while(0)


#define TEST_BASE_POS_EQUALS(data,a2r,r2a) do {                                 \
        {                                                                       \
            BasePosition bpos(data, strlen(data), CharPredicate(is_gap));       \
            TEST_ABSREL_EQUALS(test_basepos(bpos, true), a2r);                  \
            TEST_ABSREL_EQUALS(test_basepos(bpos, false), r2a);                 \
        }                                                                       \
    } while(0)

void TEST_BI_ecoli_ref() {
    TEST_ECOLIREF_EQUALS("-.AC-G-T-.", 10,
                         "  [0]  0  0  0  1  2  2  3  3  4  4  4  [4]", // abs -> rel
                         "  [0]  2  3  5  7  [7]");                     // rel -> abs

    TEST_ECOLIREF_EQUALS("-",   1, "  [0]  0  0  [0]",       "  [0]  [0]");
    TEST_ECOLIREF_EQUALS("--A", 1, "  [0]  0  0  [0]",       "  [0]  [0]"); // part of sequence
    TEST_ECOLIREF_EQUALS("--",  2, "  [0]  0  0  0  [0]",    "  [0]  [0]");
    TEST_ECOLIREF_EQUALS("---", 3, "  [0]  0  0  0  0  [0]", "  [0]  [0]");
    
    TEST_ECOLIREF_EQUALS("A",   1, "  [0]  0  1  [1]",       "  [0]  0  [0]");
    TEST_ECOLIREF_EQUALS("A",   2, "  [0]  0  1  1  [1]",    "  [0]  0  [0]");

    TEST_ECOLIREF_EQUALS("A-",  2, "  [0]  0  1  1  [1]",    "  [0]  0  [0]");
    TEST_ECOLIREF_EQUALS("-A",  2, "  [0]  0  0  1  [1]",    "  [0]  1  [1]");

    TEST_ECOLIREF_EQUALS("A",   3, "  [0]  0  1  1  1  [1]", "  [0]  0  [0]"); // more than sequence
    TEST_ECOLIREF_EQUALS("A--", 3, "  [0]  0  1  1  1  [1]", "  [0]  0  [0]");
    TEST_ECOLIREF_EQUALS("-A-", 3, "  [0]  0  0  1  1  [1]", "  [0]  1  [1]");
    TEST_ECOLIREF_EQUALS("--A", 3, "  [0]  0  0  0  1  [1]", "  [0]  2  [2]");
}

void TEST_base_position() {
    ED4_init_is_align_character("-"); // count '.' as base

    TEST_BASE_POS_EQUALS("-.AC-G-T-.",
                         "  [0]  0  0  1  2  3  3  4  4  5  5  6  [6]", // abs -> rel
                         "  [0]  1  2  3  5  7  9  [9]");               // rel -> abs

    // ------------------------------
    ED4_init_is_align_character(".-");

    TEST_BASE_POS_EQUALS("-.AC-G-T-.",
                         "  [0]  0  0  0  1  2  2  3  3  4  4  4  [4]", // abs -> rel
                         "  [0]  2  3  5  7  [7]");                     // rel -> abs

    TEST_BASE_POS_EQUALS("-",   "  [0]  0  0  [0]",       "  [0]  [0]");
    TEST_BASE_POS_EQUALS("--",  "  [0]  0  0  0  [0]",    "  [0]  [0]");
    TEST_BASE_POS_EQUALS("---", "  [0]  0  0  0  0  [0]", "  [0]  [0]");

    TEST_BASE_POS_EQUALS("A",   "  [0]  0  1  [1]",       "  [0]  0  [0]");

    TEST_BASE_POS_EQUALS("A-",  "  [0]  0  1  1  [1]",    "  [0]  0  [0]");
    TEST_BASE_POS_EQUALS("-A",  "  [0]  0  0  1  [1]",    "  [0]  1  [1]");
    
    TEST_BASE_POS_EQUALS("A--", "  [0]  0  1  1  1  [1]", "  [0]  0  [0]");
    TEST_BASE_POS_EQUALS("-A-", "  [0]  0  0  1  1  [1]", "  [0]  1  [1]");
    TEST_BASE_POS_EQUALS("--A", "  [0]  0  0  0  1  [1]", "  [0]  2  [2]");
}

#endif // UNIT_TESTS


