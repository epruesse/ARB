#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cctype>
#include <cmath>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_keysym.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awtc_fast_aligner.hxx>

#include "ed4_awars.hxx"
#include "ed4_class.hxx"
#include "ed4_tools.hxx"
#include "ed4_block.hxx"
#include "ed4_edit_string.hxx"

// --------------------------------------------------------------------------------

static ED4_blocktype blocktype = ED4_BT_NOBLOCK;

// linerange for COLUMNBLOCK is stored in EDIT4 marks,
// columnrange for COLUMNBLOCK is stored here:

static int range_col1, range_col2;
static int columnBlockUsed = 0;

// --------------------------------------------------------------------------------

static void col_block_refresh_on_seq_term(ED4_sequence_terminal *seq_term) {
    seq_term->set_refresh(1);
    seq_term->parent->refresh_requested_by_child();

    ED4_columnStat_terminal *colStatTerm = seq_term->corresponding_columnStat_terminal();
    if (colStatTerm) {
        const char *probe_match_pattern = colStatTerm->build_probe_match_string(range_col1, range_col2);
        int len = strlen(probe_match_pattern);

        if (len>=4) {
            colStatTerm->set_refresh(1);
            colStatTerm->parent->refresh_requested_by_child();

            // automatically set probe-match awars to appropriate values:

            ED4_ROOT->aw_root->awar(ED4_AWAR_PROBE_SEARCH_MAX_MISMATCHES)->write_int(0); // no mismatches
            ED4_ROOT->aw_root->awar(ED4_AWAR_PROBE_SEARCH_AUTO_JUMP)->write_int(0); // disable auto jump
            ED4_ROOT->aw_root->awar(AWAR_MAX_MISMATCHES)->write_int(0); // probe search w/o mismatches
            ED4_ROOT->aw_root->awar(AWAR_ITARGET_STRING)->write_string(probe_match_pattern); // set probe search string
        }
    }
}
// --------------------------------------------------------------------------------

// if block_operation() returns NULL => no changes will be made to database
// otherwise the changed sequence(part) will be written to the database

static GB_ERROR perform_block_operation_on_whole_sequence(ED4_blockoperation block_operation, ED4_sequence_terminal *term, int repeat) {
    GBDATA *gbd = term->get_species_pointer();
    GB_ERROR error = 0;

    if (gbd) {
        char *seq = GB_read_string(gbd);
        int len = GB_read_string_count(gbd);

        int new_len;
        char *new_seq = block_operation(seq, len, repeat, &new_len, &error);

        if (new_seq) {
            if (new_len<len) {
                memcpy(seq, new_seq, new_len);
                char gap = ADPP_IS_ALIGN_CHARACTER(seq[len-1]) ? seq[len-1] : '.';
                int l;
                for (l=new_len; l<len; l++) {
                    seq[l] = gap;
                }
                seq[l] = 0;
            }
            else if (new_len>len) {
                for (int l=new_len-1; l>=len; l--) {
                    if (!ADPP_IS_ALIGN_CHARACTER(new_seq[l])) {
                        error = "Result of block-operation to large (not enough gaps at end of sequence data)";
                        break;
                    }
                }

                if (!error) { // there are enough gaps at end of sequence
                    memcpy(seq, new_seq, len);
                }
            }
            else {
                memcpy(seq, new_seq, len);
            }
            delete new_seq;

            if (!error) {
                error = GB_write_string(gbd, seq);
                if (!error) {
                    term->set_refresh();
                    term->parent->refresh_requested_by_child();
                }
            }
        }

        delete seq;
    }

    return error;
}

// uses range_col1 till range_col2 as range
static GB_ERROR perform_block_operation_on_part_of_sequence(ED4_blockoperation block_operation, ED4_sequence_terminal *term, int repeat) {
    GBDATA *gbd = term->get_species_pointer();
    GB_ERROR error = 0;

    if (gbd) {
        char *seq = GB_read_string(gbd);
        int len = GB_read_string_count(gbd);

        if (range_col1>=len || range_col2>=len) {
            error = "Column-range exceeds sequence length";
        }
        else {
            int len_part = range_col2-range_col1+1;
            char *seq_part = seq+range_col1;
            int new_len;
            char *new_seq_part = block_operation(seq_part, len_part, repeat, &new_len, &error);

            if (new_seq_part) {
                if (new_len<len_part) {
                    memcpy(seq_part, new_seq_part, new_len);
                    char gap = '-';
                    if (seq_part[len_part-1] == '.' || seq_part[len_part] == '.') gap = '.';

                    for (int l=new_len; l<len_part; l++) {
                        seq_part[l] = gap;
                    }
                }
                else if (new_len>len_part) {
                    for (int l=new_len-1; l>=len_part; l--) {
                        if (!ADPP_IS_ALIGN_CHARACTER(new_seq_part[l])) {
                            error = "Result of block-operation to large (not enough gaps at end of marked columnblock)";
                            break;
                        }
                    }

                    if (!error) { // there are enough gaps at end of sequence
                        memcpy(seq_part, new_seq_part, len_part);
                    }
                }
                else {
                    memcpy(seq_part, new_seq_part, len_part);
                }
                delete new_seq_part;

                if (!error) {
                    error = GB_write_as_string(gbd, seq);
                    if (!error) {
                        term->set_refresh();
                        term->parent->refresh_requested_by_child();
                    }
                }
            }
        }

        delete seq;
    }

    return error;
}



void ED4_with_whole_block(ED4_blockoperation block_operation, int repeat) {
    GB_ERROR               error    = GB_begin_transaction(gb_main);
    ED4_sequence_terminal *err_term = 0;
    //    ED4_terminal *term        = ED4_ROOT->root_group_man->get_first_terminal();

    ED4_cursor *cursor   = &ED4_ROOT->temp_ed4w->cursor;
    int         base_pos = (cursor && cursor->owner_of_cursor != 0) ? cursor->get_base_position() : -1;

    switch (blocktype) {
        case ED4_BT_NOBLOCK: {
            aw_message("No block marked -- use right mouse button");
            break;
        }
        case ED4_BT_LINEBLOCK: {
            ED4_list_elem *listElem = ED4_ROOT->selected_objects.first();
            while (listElem && !error) {
                ED4_selection_entry   *selectionEntry = (ED4_selection_entry*)listElem->elem();
                ED4_sequence_terminal *seqTerm        = selectionEntry->object->get_parent(ED4_L_SPECIES)->search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_sequence_terminal();

                error               = perform_block_operation_on_whole_sequence(block_operation, seqTerm, repeat);
                if (error) err_term = seqTerm;

                listElem = listElem->next();
            }
            break;
        }
        case ED4_BT_MODIFIED_COLUMNBLOCK:
        case ED4_BT_COLUMNBLOCK: {
            ED4_list_elem *listElem = ED4_ROOT->selected_objects.first();
            while (listElem && !error) {
                ED4_selection_entry *selectionEntry = (ED4_selection_entry*)listElem->elem();
                ED4_sequence_terminal *seqTerm = selectionEntry->object->get_parent(ED4_L_SPECIES)->search_spec_child_rek(ED4_L_SEQUENCE_STRING)->to_sequence_terminal();

                error = perform_block_operation_on_part_of_sequence(block_operation, seqTerm, repeat);
                if (error) err_term = seqTerm;

                listElem = listElem->next();
            }
            break;
        }
        default: {
            error = "Illegal blocktype";
            break;
        }
    }

    if (error) {
        char buffer[500];
        sprintf(buffer, "Error in blockoperation: %s", error);
        aw_message(buffer);

        GB_abort_transaction(gb_main);
    }
    else {
        GB_commit_transaction(gb_main);

        if (base_pos != -1) cursor->jump_left_right_cursor_to_base_pos(ED4_ROOT->temp_aww, base_pos); // restore cursor at same base
    }
}

int ED4_get_selected_range(ED4_terminal *term, int *first_column, int *last_column) {
    if (blocktype==ED4_BT_NOBLOCK) return 0;

    if (blocktype==ED4_BT_COLUMNBLOCK || blocktype==ED4_BT_MODIFIED_COLUMNBLOCK) {
        *first_column = range_col1;
        *last_column = range_col2;
    }
    else {
        e4_assert(blocktype==ED4_BT_LINEBLOCK);
        *first_column = 0;
        *last_column = -1;
    }

    ED4_species_name_terminal *name_term = term->to_sequence_terminal()->corresponding_species_name_terminal();
    return name_term->flag.selected;
}

ED4_blocktype ED4_getBlocktype() {
    return blocktype;
}
void ED4_setBlocktype(ED4_blocktype bt) {
    if (blocktype!=bt) {
        blocktype = bt;

        ED4_list_elem *listElem = ED4_ROOT->selected_objects.first();
        while (listElem) {
            ED4_selection_entry *selected = (ED4_selection_entry*)listElem->elem();
            ED4_species_name_terminal *name_term = selected->object->to_species_name_terminal();
            ED4_sequence_terminal *seq_term = name_term->corresponding_sequence_terminal();

            name_term->set_refresh(1);
            name_term->parent->refresh_requested_by_child();
            if (seq_term) col_block_refresh_on_seq_term(seq_term);

            listElem = listElem->next();
        }

        if (blocktype==ED4_BT_COLUMNBLOCK || blocktype==ED4_BT_MODIFIED_COLUMNBLOCK) {
            columnBlockUsed = 1;
        }
    }
}

void ED4_toggle_block_type(void) {
    switch (blocktype) {
        case ED4_BT_NOBLOCK: {
            aw_message("No block selected.");
            break;
        }
        case ED4_BT_LINEBLOCK: {
            if (columnBlockUsed) {
                ED4_setBlocktype(ED4_BT_MODIFIED_COLUMNBLOCK);
            }
            else {
                aw_message("No columnblock marked so far  - I can't guess the column range");
            }
            break;
        }
        case ED4_BT_MODIFIED_COLUMNBLOCK:
        case ED4_BT_COLUMNBLOCK: {
            ED4_setBlocktype(ED4_BT_LINEBLOCK);
            break;
        }
        default: {
            e4_assert(0);
            break;
        }
    }
}

void ED4_correctBlocktypeAfterSelection() { // this has to be called every time the selection has changed

    if (ED4_ROOT->selected_objects.first()==0) { // no objects are selected
        ED4_setBlocktype(ED4_BT_NOBLOCK);
    }
    else {
        switch (ED4_getBlocktype()) {
            case ED4_BT_NOBLOCK: {
                ED4_setBlocktype(ED4_BT_LINEBLOCK);
                break;
            }
            case ED4_BT_COLUMNBLOCK: {
                ED4_setBlocktype(ED4_BT_MODIFIED_COLUMNBLOCK);
                break;
            }
            case ED4_BT_LINEBLOCK:
            case ED4_BT_MODIFIED_COLUMNBLOCK: {
                break;
            }
            default: {
                e4_assert(0);
                break;
            }
        }
    }
}

static void select_and_update(ED4_sequence_terminal *term1, ED4_sequence_terminal *term2, ED4_index pos1, ED4_index pos2, int initial_call) {
    static ED4_sequence_terminal *last_term1, *last_term2;
    static ED4_index last_pos1, last_pos2;

    if (pos1>pos2) {
        range_col1 = pos2;
        range_col2 = pos1;
    }
    else {
        range_col1 = pos1;
        range_col2 = pos2;
    }

    if (blocktype==ED4_BT_MODIFIED_COLUMNBLOCK) {
        ED4_list_elem *listElem = ED4_ROOT->selected_objects.first();
        while (listElem) {
            ED4_selection_entry *selectionEntry = (ED4_selection_entry*)listElem->elem();
            ED4_species_name_terminal *name_term = selectionEntry->object->to_species_name_terminal();
            ED4_sequence_terminal *seq_term = name_term->corresponding_sequence_terminal();

            if (seq_term) col_block_refresh_on_seq_term(seq_term);

            listElem = listElem->next();
        }
    }
    else {
        { // ensure term1 is the upper terminal
            AW_pos dummy, y1, y2;

            term1->calc_world_coords(&dummy, &y1);
            term2->calc_world_coords(&dummy, &y2);

            if (y1>y2) {
                ED4_sequence_terminal *t = term1; term1 = term2; term2 = t;
                AW_pos y = y1; y1 = y2; y2 = y;
            }
        }

        int do_above = 1; // we have to update terminals between last_term1 and term1
        int do_below = 1; // we have to update terminals between term2 and last_term2

        ED4_terminal *term = term1;
        //	ED4_terminal *start_term = term1;
        int xRangeChanged = last_pos1!=range_col1 || last_pos2!=range_col2;

        while (term) {
            if (term->is_sequence_terminal()) {
                ED4_sequence_terminal *seq_term = term->to_sequence_terminal();

                if (seq_term==last_term1) {
                    do_above = 0;
                }
                if (seq_term==last_term2) {
                    do_below = 0;
                }

                ED4_species_name_terminal *name_term = seq_term->corresponding_species_name_terminal();
                if (name_term->flag.selected) { // already selected
                    if (xRangeChanged) {
                        col_block_refresh_on_seq_term(seq_term);
                    }
                }
                else { // select it
                    ED4_species_manager *species_man = name_term->get_parent(ED4_L_SPECIES)->to_species_manager();

                    if (!species_man->flag.is_consensus) {
                        ED4_ROOT->add_to_selected(name_term);
                    }
                }
            }
            if (term==term2) {
                break;
            }
            term = term->get_next_terminal();
        }

        if (!initial_call) {
            if (do_below) {
                while (term) {
                    if (term->is_species_name_terminal() && term->flag.selected) {
                        ED4_species_manager *species_man = term->get_parent(ED4_L_SPECIES)->to_species_manager();

                        if (!species_man->flag.is_consensus) {
                            ED4_ROOT->remove_from_selected(term);
                        }
                    }
                    if (term==last_term2) break;
                    term = term->get_next_terminal();
                }
            }

            if (do_above) {
                term = last_term1->corresponding_species_name_terminal();
                while (term && term!=term1) {
                    if (term->is_species_name_terminal() && term->flag.selected) {
                        ED4_species_manager *species_man = term->get_parent(ED4_L_SPECIES)->to_species_manager();

                        if (!species_man->flag.is_consensus) {
                            ED4_ROOT->remove_from_selected(term);
                        }
                    }
                    term = term->get_next_terminal();
                }
            }
        }
    }

    last_term1 = term1;
    last_term2 = term2;
    last_pos1 = range_col1;
    last_pos2 = range_col2;
}

// static inline double fabs(double d) {
    // return d<0 ? -d : d;
// }

void ED4_setColumnblockCorner(AW_event *event, ED4_sequence_terminal *seq_term) {
    static ED4_sequence_terminal *fix_term = 0;
    static ED4_index fix_pos = 0;

    ED4_index seq_pos;
    {
        AW_pos termw_x, termw_y;
        seq_term->calc_world_coords(&termw_x, &termw_y);

        ED4_index scr_pos = ED4_ROOT->pixel2pos(event->x - termw_x);
        ED4_remap *remap = ED4_ROOT->root_group_man->remap();
        seq_pos = remap->screen_to_sequence(scr_pos);
    }

    switch (event->type) {
        case AW_Mouse_Press: {
            if (blocktype==ED4_BT_NOBLOCK) { // initial columnblock
                ED4_setBlocktype(ED4_BT_COLUMNBLOCK);

                fix_term = seq_term;
                fix_pos = seq_pos;

                select_and_update(fix_term, seq_term, fix_pos, seq_pos, 1);
            }
            else if (blocktype==ED4_BT_LINEBLOCK) { // change lineblock to columnblock
                ED4_setBlocktype(ED4_BT_MODIFIED_COLUMNBLOCK);

                fix_term = seq_term;
                if (seq_pos<(MAXSEQUENCECHARACTERLENGTH/2)) { // in first half of sequence
                    fix_pos = MAXSEQUENCECHARACTERLENGTH;
                }
                else {
                    fix_pos = 0;
                }

                select_and_update(fix_term, seq_term, fix_pos, seq_pos, 1);
            }
            else { // expand columnblock (search nearest corner/border -> fix opposite corner/border)
                e4_assert(blocktype==ED4_BT_COLUMNBLOCK || blocktype==ED4_BT_MODIFIED_COLUMNBLOCK);

                ED4_list_elem *listElem = ED4_ROOT->selected_objects.first();
                e4_assert(listElem);

                if (blocktype==ED4_BT_COLUMNBLOCK) {
                    AW_pos min_term_y = LONG_MAX;
                    AW_pos max_term_y = LONG_MIN;
                    ED4_species_name_terminal *min_term = 0;
                    ED4_species_name_terminal *max_term = 0;
                    AW_pos xpos, ypos;

                    while (listElem) {
                        ED4_selection_entry *selected = (ED4_selection_entry*)listElem->elem();
                        ED4_species_name_terminal *name_term = selected->object->to_species_name_terminal();

                        name_term->calc_world_coords(&xpos, &ypos);

                        if (ypos<min_term_y) {
                            min_term_y = ypos;
                            min_term = name_term;
                        }
                        if (ypos>max_term_y) {
                            max_term_y = ypos;
                            max_term = name_term;
                        }

                        listElem = listElem->next();
                    }

                    seq_term->calc_world_coords(&xpos, &ypos);
                    ED4_species_name_terminal *fix_name_term;
                    if (fabs(ypos-min_term_y)<fabs(ypos-max_term_y)) { // seq_term is closer to min_term
                        fix_name_term = max_term; // select max_term as fixed corner
                    }
                    else {
                        fix_name_term = min_term;
                    }
                    fix_term = fix_name_term->corresponding_sequence_terminal();
                }

                long scr_col1, scr_col2;
                AW_rectangle area_rect;
                {
                    AW_pos ex = event->x;
                    AW_pos ey = event->y;
                    ED4_ROOT->world_to_win_coords(ED4_ROOT->temp_ed4w->aww, &ex, &ey);

                    if (ED4_ROOT->get_area_rectangle(&area_rect, ex, ey)!=ED4_R_OK) {
                        e4_assert(0);
                        break;
                    }
                }


                const ED4_remap *rm = ED4_ROOT->root_group_man->remap();

                seq_term->calc_intervall_displayed_in_rectangle(&area_rect, &scr_col1, &scr_col2);
                rm->clip_screen_range(&scr_col1, &scr_col2);

                int range_scr_col1 = rm->sequence_to_screen(range_col1);
                int range_scr_col2 = rm->sequence_to_screen(range_col2);
                int scr_pos = rm->sequence_to_screen(seq_pos);

                int use_scr_col1 = range_scr_col1<scr_col1 ? scr_col1 : range_scr_col1;
                int use_scr_col2 = range_scr_col2>scr_col2 ? scr_col2 : range_scr_col2;

                if (abs(scr_pos-use_scr_col1) < abs(scr_pos-use_scr_col2)) { // scr_pos is closer to use_scr_col1
                    fix_pos = range_col2;
                }
                else {
                    fix_pos = range_col1;
                }

                select_and_update(fix_term, seq_term, fix_pos, seq_pos, 0);
            }
            break;
        }
        case AW_Mouse_Drag: {
            select_and_update(fix_term, seq_term, fix_pos, seq_pos, 0);
            break;
        }
        case AW_Mouse_Release: {
            select_and_update(fix_term, seq_term, fix_pos, seq_pos, 0);
            break;
        }
        default: {
            e4_assert(0);
            break;
        }
    }
}

// --------------------------------------------------------------------------------
// 	Replace
// --------------------------------------------------------------------------------

static int strncmpWithJoker(GB_CSTR s1, GB_CSTR s2, int len) { // s2 contains '?' as joker
    int cmp = 0;

    while (len-- && !cmp) {
        int c1 = *s1++;
        int c2 = *s2++;

        if (!c1) {
            cmp = -1;
        }
        else if (!c2) {
            cmp = 1;
        }
        else if (c2!='?') {
            cmp = c1-c2;
        }
    }

    return cmp;
}

static char *oldString, *newString;
static int oldStringContainsJoker;

static char* replace_in_sequence(const char *sequence, int len, int /*repeat*/, int *new_len, GB_ERROR*) {
    int maxlen;
    int olen = strlen(oldString);
    int nlen = strlen(newString);

    if (nlen<=olen) {
        maxlen = len;
    }
    else  {
        maxlen = (len/olen+1)*nlen;
    }

    char *new_seq = (char*)GB_calloc(maxlen+1, sizeof(*new_seq));
    int replaced = 0;
    int o = 0;
    int n = 0;
    char ostart = oldString[0];

    if (oldStringContainsJoker) {
        while (o<len) {
            if (strncmpWithJoker(sequence+o, oldString, olen)==0) {
                memcpy(new_seq+n, newString, nlen);
                n += nlen;
                o += olen;
                replaced++;
            }
            else {
                new_seq[n++] = sequence[o++];
            }
        }
    }
    else {
        while (o<len) {
            if (sequence[o]==ostart && strncmp(sequence+o, oldString, olen)==0) { // occurance of oldString
                memcpy(new_seq+n, newString, nlen);
                n += nlen;
                o += olen;
                replaced++;
            }
            else {
                new_seq[n++] = sequence[o++];
            }
        }
    }
    new_seq[n] = 0;

    if (replaced) {
        if (new_len) {
            *new_len = n;
        }
    }
    else {
        delete new_seq;
        new_seq = 0;
    }

    return new_seq;
}

static void replace_in_block(AW_window*) {
    oldString = ED4_ROOT->aw_root->awar(ED4_AWAR_REP_SEARCH_PATTERN)->read_string();
    newString = ED4_ROOT->aw_root->awar(ED4_AWAR_REP_REPLACE_PATTERN)->read_string();

    oldStringContainsJoker = strchr(oldString, '?')!=0;
    ED4_with_whole_block(replace_in_sequence, 1);

    delete oldString; oldString = 0;
    delete newString; newString = 0;
}

AW_window *ED4_create_replace_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "REPLACE", "Search & Replace");
    aws->load_xfig("edit4/replace.fig");

    aws->at("close");
    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "Close","C");

    aws->at("help");
    aws->callback( AW_POPUP_HELP,(AW_CL)"e4_replace.hlp");
    aws->create_button("HELP", "Help","H");

    aws->at("spattern");
    aws->create_input_field(ED4_AWAR_REP_SEARCH_PATTERN, 30);

    aws->at("rpattern");
    aws->create_input_field(ED4_AWAR_REP_REPLACE_PATTERN, 30);

    aws->at("go");
    aws->callback(replace_in_block);
    aws->create_button("GO", "Go", "G");

    return aws;
}

// --------------------------------------------------------------------------------
//	Other block operations
// --------------------------------------------------------------------------------

static char *sequence_to_upper_case(const char *seq, int len, int /*repeat*/, int *new_len, GB_ERROR*) {
    char *new_seq = (char*)GB_calloc(len+1, sizeof(*new_seq));
    int l;

    for (l=0; l<len; l++) {
        new_seq[l] = toupper(seq[l]);
    }

    if (new_len) *new_len = len;
    return new_seq;
}
static char *sequence_to_lower_case(const char *seq, int len, int /*repeat*/, int *new_len, GB_ERROR*) {
    char *new_seq = (char*)GB_calloc(len+1, sizeof(*new_seq));
    int l;

    for (l=0; l<len; l++) {
        new_seq[l] = tolower(seq[l]);
    }

    if (new_len) *new_len = len;
    return new_seq;
}

#define EVEN_REPEAT "repeat-count is even -> no changes!"

static char *reverse_sequence(const char *seq, int len, int repeat, int *new_len, GB_ERROR *error) {
    if ((repeat&1)==0) { *error = GBS_global_string(EVEN_REPEAT); return 0; }

    char *new_seq = GBT_reverseNucSequence(seq, len);
    if (new_len) *new_len = len;
    return new_seq;
}
static char *complement_sequence(const char *seq, int len, int repeat, int *new_len, GB_ERROR *error) {
    if ((repeat&1)==0) {
        *error = GBS_global_string(EVEN_REPEAT);
        return 0;
    }

    char T_or_U;
    *error = GBT_determine_T_or_U(ED4_ROOT->alignment_type, &T_or_U, "complement");
    if (*error) return 0;

    //     char T_or_U = ED4_ROOT->alignment_type==GB_AT_DNA ? 'T' : 'U';

    char *new_seq         = GBT_complementNucSequence(seq, len, T_or_U);
    if (new_len) *new_len = len;
    return new_seq;
}
static char *reverse_complement_sequence(const char *seq, int len, int repeat, int *new_len, GB_ERROR *error) {
    if ((repeat&1) == 0) {
        *error      = GBS_global_string(EVEN_REPEAT);
        return 0;
    }

    char T_or_U;
    *error = GBT_determine_T_or_U(ED4_ROOT->alignment_type, &T_or_U, "reverse-complement");
    if (*error) return 0;
    // char  T_or_U = ED4_ROOT->alignment_type==GB_AT_DNA ? 'T' : 'U';
    char *new_seq1  = GBT_complementNucSequence(seq, len, T_or_U);
    char *new_seq2  = GBT_reverseNucSequence(new_seq1, len);

    delete new_seq1;
    if (new_len) *new_len = len;
    return new_seq2;
}

static char *unalign_sequence_internal(const char *seq, int len, int *new_len, bool to_the_right) {
    char *new_seq   = (char*)GB_calloc(len+1, sizeof(*new_seq));
    int   o         = 0;
    int   n         = 0;
    char  gap       = '-';

    if (to_the_right) {
        if (ADPP_IS_ALIGN_CHARACTER(seq[0])) gap = seq[0]; // use first position of block if it's a gap
        else if (ADPP_IS_ALIGN_CHARACTER(seq[-1])) gap = seq[-1]; // WARNING:  might be out-side sequence and undefined behavior
    }
    else {
        if (ADPP_IS_ALIGN_CHARACTER(seq[len-1])) gap = seq[len-1]; // use first position of block if it's a gap
        else if (ADPP_IS_ALIGN_CHARACTER(seq[len])) gap = seq[len]; // otherwise check position behind
    }

    while (o<len) {
        if (!ADPP_IS_ALIGN_CHARACTER(seq[o])) new_seq[n++] = seq[o];
        o++;
    }
    
    if (n<len) {                // (move and) dot rest
        int gapcount = len-n;
        if (to_the_right) {
            memmove(new_seq+gapcount, new_seq, n);
            memset(new_seq, gap, gapcount);
        }
        else {
            memset(new_seq+n, gap, gapcount);
        }
    }

    if (new_len) *new_len = len;
    return new_seq;
}

static char *unalign_left_sequence(const char *seq, int len, int /*repeat*/, int *new_len, GB_ERROR *) {
    return unalign_sequence_internal(seq, len, new_len, false);
}

static char *unalign_right_sequence(const char *seq, int len, int /*repeat*/, int *new_len, GB_ERROR *) {
    return unalign_sequence_internal(seq, len, new_len, true);
}

static char *shift_left_sequence(const char *seq, int len, int repeat, int *new_len, GB_ERROR *error) {
    char *new_seq = 0;

    if (repeat>=len) {
        *error = "Repeat count exceeds block length";
    }
    else
    {
        int enough_space = 1;
        for (int i=0; enough_space && i<repeat; i++) {
            if (!ADPP_IS_ALIGN_CHARACTER(seq[i])) {
                enough_space = 0;
            }
        }

        if (enough_space) {
            char gap = '-';

            new_seq               = (char*)GB_calloc(len+1, sizeof(*new_seq));
            if (new_len) *new_len = len;
            memcpy(new_seq, seq+repeat, len-repeat);

            if (ADPP_IS_ALIGN_CHARACTER(seq[len-1])) gap    = seq[len-1];
            else if (ADPP_IS_ALIGN_CHARACTER(seq[len])) gap = seq[len];

            memset(new_seq+len-repeat, gap, repeat);
        }
        else {
            *error = GBS_global_string("Shift left needs %i gap%s at block start", repeat, repeat==1 ? "" : "s");
        }
    }

    return new_seq;
}

static char *shift_right_sequence(const char *seq, int len, int repeat, int *new_len, GB_ERROR *error) {
    char *new_seq = 0;

    if (repeat>=len) {
        *error = "Repeat count exceeds block length";
    }
    else
    {
        int enough_space = 1;
        for (int i=0; enough_space && i<repeat; i++) {
            if (!ADPP_IS_ALIGN_CHARACTER(seq[len-i-1])) {
                enough_space = 0;
            }
        }

        if (enough_space) {
            char gap = '-';

            new_seq = (char*)GB_calloc(len+1, sizeof(*new_seq));
            if (new_len) *new_len = len;

            if (ADPP_IS_ALIGN_CHARACTER(seq[0])) gap       = seq[0];
            else if (ADPP_IS_ALIGN_CHARACTER(seq[-1])) gap = seq[-1]; // a working hack

            memset(new_seq, gap, repeat);
            memcpy(new_seq+repeat, seq, len-repeat);
        }
        else {
            *error = GBS_global_string("Shift right needs %i gap%s at block end", repeat, repeat==1 ? "" : "s");
        }
    }

    return new_seq;
}

#if 0
static char *shift_right_sequence(const char *seq, int len, int repeat, int *new_len, GB_ERROR *error) {
    char *new_seq = 0;

    if (ADPP_IS_ALIGN_CHARACTER(seq[len-1])) {
        char gap = seq[len-1];
        new_seq = (char*)GB_calloc(len+1, sizeof(*new_seq));
        if (new_len) *new_len = len;
        memcpy(new_seq+1, seq, len-1);
        if (ADPP_IS_ALIGN_CHARACTER(seq[0])) {
            new_seq[0] = seq[0];
        }
        else {
            new_seq[0] = gap;
        }
    }
    else {
        *error = "Shift right needs a gap as last block character";
    }

    return new_seq;
}
#endif

void ED4_perform_block_operation(ED4_blockoperation_type operationType) {
    int nrepeat = ED4_ROOT->edit_string->use_nrepeat();

    switch (operationType) {
        case ED4_BO_UPPER_CASE:         ED4_with_whole_block(sequence_to_upper_case, nrepeat);          break;
        case ED4_BO_LOWER_CASE:         ED4_with_whole_block(sequence_to_lower_case, nrepeat);          break;
        case ED4_BO_REVERSE:            ED4_with_whole_block(reverse_sequence, nrepeat);                break;
        case ED4_BO_COMPLEMENT:         ED4_with_whole_block(complement_sequence, nrepeat);             break;
        case ED4_BO_REVERSE_COMPLEMENT: ED4_with_whole_block(reverse_complement_sequence, nrepeat);     break;
        case ED4_BO_UNALIGN:            ED4_with_whole_block(unalign_left_sequence, nrepeat);           break;
        case ED4_BO_UNALIGN_RIGHT:      ED4_with_whole_block(unalign_right_sequence, nrepeat);          break;
        case ED4_BO_SHIFT_LEFT:         ED4_with_whole_block(shift_left_sequence, nrepeat);             break;
        case ED4_BO_SHIFT_RIGHT:        ED4_with_whole_block(shift_right_sequence, nrepeat);            break;

        default: {
            e4_assert(0);
            break;
        }
    }
}


