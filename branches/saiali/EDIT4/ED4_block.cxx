#include <arbdbt.h>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <fast_aligner.hxx>

#include "ed4_awars.hxx"
#include "ed4_class.hxx"
#include "ed4_tools.hxx"
#include "ed4_block.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_list.hxx"

#include <climits>
#include <cctype>
#include <map>

using namespace std;

// --------------------------------------------------------------------------------

class ED4_block : virtual Noncopyable {
    // stores columnrange of selected region
    // (linerange is stored in EDIT4 marks)
    ED4_blocktype type;
    bool          columnBlockUsed;
    PosRange      range;

public:
    ED4_block()
        : type(ED4_BT_NOBLOCK),
          columnBlockUsed(false)
    {}

    ED4_blocktype get_type() const { return type; }
    void set_type(ED4_blocktype bt);
    void toggle_type();
    void autocorrect_type();

    const PosRange& get_range() const { return range; }
    void set_range(const PosRange& new_range) { range = new_range; }
};

static ED4_block block;

// --------------------------------------------------------------------------------

class SeqPart {
    const char *seq;

    int offset;
    int len; // of part

    static char to_gap(char c) { return ADPP_IS_ALIGN_CHARACTER(c) ? c : 0; }

public:
    SeqPart(const char *seq_, int offset_, int len_)
        : seq(seq_),
          offset(offset_),
          len(len_)
    {}

    const char *data() const { return seq+offset; }
    int length() const { return len; }

    // detect which gap to use at border of SeqPart:
    char left_gap() const {
        char gap                = to_gap(seq[offset]);
        if (!gap && offset) gap = to_gap(seq[offset-1]);
        if (!gap) gap           = '-';
        return gap;
    }
    char right_gap() const {
        char gap      = to_gap(seq[offset+len-1]);
        if (!gap) gap = to_gap(seq[offset+len]);
        if (!gap) gap = '-';
        return gap;
    }
};

// --------------------------------------------------------------------------------

static void col_block_refresh_on_seq_term(ED4_sequence_terminal *seq_term) {
    seq_term->request_refresh();

    // @@@ code below is more than weird. why do sth with column-stat here ? why write probe match awars here ? 
    ED4_columnStat_terminal *colStatTerm = seq_term->corresponding_columnStat_terminal();
    if (colStatTerm) {
        const char *probe_match_pattern = colStatTerm->build_probe_match_string(block.get_range());
        int len = strlen(probe_match_pattern);

        if (len>=4) {
            colStatTerm->request_refresh();

            // automatically set probe-match awars to appropriate values:

            ED4_ROOT->aw_root->awar(ED4_AWAR_PROBE_SEARCH_MAX_MISMATCHES)->write_int(0); // no mismatches
            ED4_ROOT->aw_root->awar(ED4_AWAR_PROBE_SEARCH_AUTO_JUMP)->write_int(0); // disable auto jump
            ED4_ROOT->aw_root->awar(AWAR_MAX_MISMATCHES)->write_int(0); // probe search w/o mismatches
            ED4_ROOT->aw_root->awar(AWAR_ITARGET_STRING)->write_string(probe_match_pattern); // set probe search string
        }
    }
}

static void refresh_selected(bool refresh_name_terminals) {
    ED4_selected_elem *listElem = ED4_ROOT->selected_objects->head();
    while (listElem) {
        ED4_species_name_terminal *name_term = listElem->elem()->object;
        ED4_sequence_terminal     *seq_term  = name_term->corresponding_sequence_terminal();

        if (refresh_name_terminals) name_term->request_refresh();
        if (seq_term) col_block_refresh_on_seq_term(seq_term);

        listElem = listElem->next();
    }
}

// --------------------------------------------------------------------------------

void ED4_block::set_type(ED4_blocktype bt) {
    if (type != bt) {
        type = bt;
        refresh_selected(true);
        if (type==ED4_BT_COLUMNBLOCK || type==ED4_BT_MODIFIED_COLUMNBLOCK) {
            columnBlockUsed = true;
        }
    }
}

void ED4_block::toggle_type() {
    switch (type) {
        case ED4_BT_NOBLOCK: {
            aw_message("No block selected.");
            break;
        }
        case ED4_BT_LINEBLOCK: {
            if (columnBlockUsed) {
                set_type(ED4_BT_MODIFIED_COLUMNBLOCK);
            }
            else {
                aw_message("No columnblock marked so far  - I can't guess the column range");
            }
            break;
        }
        case ED4_BT_MODIFIED_COLUMNBLOCK:
        case ED4_BT_COLUMNBLOCK: {
            set_type(ED4_BT_LINEBLOCK);
            break;
        }
    }
}

void ED4_block::autocorrect_type() {
    // this has to be called every time the selection has changed

    if (ED4_ROOT->selected_objects->head()==0) { // no objects are selected
        set_type(ED4_BT_NOBLOCK);
    }
    else {
        switch (type) {
            case ED4_BT_NOBLOCK: {
                set_type(ED4_BT_LINEBLOCK);
                break;
            }
            case ED4_BT_COLUMNBLOCK: {
                set_type(ED4_BT_MODIFIED_COLUMNBLOCK);
                break;
            }
            case ED4_BT_LINEBLOCK:
            case ED4_BT_MODIFIED_COLUMNBLOCK: {
                break;
            }
        }
    }
}

// --------------------------------------------------------------------------------

// if block_operation returns NULL => no changes will be made to database
// otherwise the changed sequence(part) will be written to the database

static GB_ERROR perform_block_operation_on_whole_sequence(const ED4_block_operator& block_operator, ED4_sequence_terminal *term) {
    GBDATA *gbd = term->get_species_pointer();
    GB_ERROR error = 0;

    if (gbd) {
        char *seq = GB_read_string(gbd);
        int len = GB_read_string_count(gbd);

        int   new_len;
        char *new_seq = block_operator.operate(SeqPart(seq, 0, len), new_len);
        error         = block_operator.get_error();

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

            if (!error) {
                error = GB_write_string(gbd, seq);
                if (!error) term->request_refresh();
            }
            free(new_seq);
        }
        free(seq);
    }

    return error;
}

// uses range_col1 till range_col2 as range
static GB_ERROR perform_block_operation_on_part_of_sequence(const ED4_block_operator& block_operator, ED4_sequence_terminal *term) {
    GBDATA *gbd = term->get_species_pointer();
    GB_ERROR error = 0;

    if (gbd) {
        char *seq = GB_read_string(gbd);
        int   len = GB_read_string_count(gbd);

        ExplicitRange range(block.get_range(), len);

        int   len_part     = range.size();
        char *seq_part     = seq+range.start();
        int   new_len;
        char *new_seq_part = block_operator.operate(SeqPart(seq, range.start(), len_part), new_len);
        error              = block_operator.get_error();

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
                if (!error) term->request_refresh();
            }
        }

        delete seq;
    }

    return error;
}

static void ED4_with_whole_block(const ED4_block_operator& block_operator) {
    GB_ERROR error = GB_begin_transaction(GLOBAL_gb_main);

    typedef map<ED4_window*, int> CursorPositions;
    CursorPositions at_base;

    for (ED4_window *win = ED4_ROOT->first_window; win; win = win->next) {
        ED4_cursor& cursor = win->cursor;
        if (cursor.owner_of_cursor) at_base[win] = cursor.get_base_position();
    }


    switch (block.get_type()) {
        case ED4_BT_NOBLOCK: {
            aw_message("No block marked -- use right mouse button");
            break;
        }
        case ED4_BT_LINEBLOCK:
        case ED4_BT_MODIFIED_COLUMNBLOCK:
        case ED4_BT_COLUMNBLOCK: {
            ED4_selected_elem *listElem = ED4_ROOT->selected_objects->head();
            while (listElem && !error) {
                ED4_species_name_terminal *nameTerm = listElem->elem()->object;
                ED4_sequence_terminal     *seqTerm  = nameTerm->corresponding_sequence_terminal();

                error = block.get_type() == ED4_BT_LINEBLOCK
                    ? perform_block_operation_on_whole_sequence(block_operator, seqTerm)
                    : perform_block_operation_on_part_of_sequence(block_operator, seqTerm);

                listElem = listElem->next();
            }
            break;
        }
        default: {
            error = "Illegal blocktype";
            break;
        }
    }

    if (error) error = GBS_global_string("[In block operation] %s", error);
    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);

    if (!error) {
        for (CursorPositions::iterator ab = at_base.begin(); ab != at_base.end(); ++ab) {
            ED4_window *win = ab->first;
            win->cursor.jump_base_pos(ab->second, ED4_JUMP_KEEP_VISIBLE); // restore cursor at same base
            
#if defined(DEBUG)
            ED4_cursor& cursor = win->cursor;
            if (cursor.owner_of_cursor) {
                int bp_now = cursor.get_base_position();
                e4_assert(bp_now == ab->second); // @@@ restore cursor does not work properly with 2 windows
            }
#endif
        }
    }
}

bool ED4_get_selected_range(ED4_terminal *term, PosRange& range) { // @@@ function will get useless, when multi-column-blocks are possible
    if (block.get_type()==ED4_BT_NOBLOCK) return false;

    if (!term->containing_species_manager()->is_selected()) return false;

    if (block.get_type()==ED4_BT_COLUMNBLOCK || block.get_type()==ED4_BT_MODIFIED_COLUMNBLOCK) {
        range = block.get_range();
    }
    else {
        e4_assert(block.get_type()==ED4_BT_LINEBLOCK);
        range = PosRange::whole();
    }

    return true;
}

ED4_blocktype ED4_getBlocktype() { return block.get_type(); }
void ED4_setBlocktype(ED4_blocktype bt) { block.set_type(bt); }
void ED4_toggle_block_type() { block.toggle_type(); }
void ED4_correctBlocktypeAfterSelection() { block.autocorrect_type(); }

static void select_and_update(ED4_sequence_terminal *term1, ED4_sequence_terminal *term2, ED4_index pos1, ED4_index pos2, int initial_call) {
    static ED4_sequence_terminal *last_term1, *last_term2;
    static PosRange               last_range;

    if (!term1) return; // e.g. when 1st click was into consensus

    if (pos1>pos2) {
        block.set_range(PosRange(pos2, pos1));
    }
    else {
        block.set_range(PosRange(pos1, pos2));
    }

    if (block.get_type()==ED4_BT_MODIFIED_COLUMNBLOCK) {
        refresh_selected(false);
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

        ED4_terminal *term          = term1;
        int           xRangeChanged = block.get_range() != last_range;

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
                ED4_species_manager *species_man = name_term->containing_species_manager();
                if (species_man->is_selected()) { // already selected
                    if (xRangeChanged) {
                        col_block_refresh_on_seq_term(seq_term);
                    }
                }
                else { // select it
                    if (!species_man->is_consensus_manager()) {
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
                    if (term->is_species_name_terminal()) {
                        ED4_species_manager *species_man = term->containing_species_manager();
                        if (species_man->is_selected() && !species_man->is_consensus_manager()) {
                            ED4_ROOT->remove_from_selected(term->to_species_name_terminal());
                        }
                    }
                    if (term==last_term2) break;
                    term = term->get_next_terminal();
                }
            }

            if (do_above) {
                term = last_term1->corresponding_species_name_terminal();
                while (term && term!=term1) {
                    if (term->is_species_name_terminal()) {
                        ED4_species_manager *species_man = term->containing_species_manager();
                        if (species_man->is_selected() && !species_man->is_consensus_manager()) {
                            ED4_ROOT->remove_from_selected(term->to_species_name_terminal());
                        }
                    }
                    term = term->get_next_terminal();
                }
            }
        }
    }

    last_term1 = term1;
    last_term2 = term2;
    last_range = block.get_range();
}

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
            if (block.get_type() == ED4_BT_NOBLOCK) { // initial columnblock
                if (!seq_term->is_consensus_terminal()) {
                    block.set_type(ED4_BT_COLUMNBLOCK);

                    fix_term = seq_term;
                    fix_pos = seq_pos;

                    select_and_update(fix_term, seq_term, fix_pos, seq_pos, 1);
                }
            }
            else if (block.get_type()==ED4_BT_LINEBLOCK) { // change lineblock to columnblock
                block.set_type(ED4_BT_MODIFIED_COLUMNBLOCK);

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
                e4_assert(block.get_type()==ED4_BT_COLUMNBLOCK || block.get_type()==ED4_BT_MODIFIED_COLUMNBLOCK);

                ED4_selected_elem *listElem = ED4_ROOT->selected_objects->head();
                e4_assert(listElem);

                if (block.get_type()==ED4_BT_COLUMNBLOCK) {
                    AW_pos min_term_y = LONG_MAX;
                    AW_pos max_term_y = LONG_MIN;
                    ED4_species_name_terminal *min_term = 0;
                    ED4_species_name_terminal *max_term = 0;
                    AW_pos xpos, ypos;

                    while (listElem) {
                        ED4_species_name_terminal *name_term = listElem->elem()->object;
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
                    e4_assert(fix_name_term);
                    fix_term = fix_name_term->corresponding_sequence_terminal();
                }

                AW_screen_area area_rect;
                {
                    AW_pos ex = event->x;
                    AW_pos ey = event->y;
                    current_ed4w()->world_to_win_coords(&ex, &ey);

                    if (ED4_ROOT->get_area_rectangle(&area_rect, ex, ey)!=ED4_R_OK) {
                        e4_assert(0);
                        break;
                    }
                }


                const ED4_remap *rm = ED4_ROOT->root_group_man->remap();

                PosRange screen_range = rm->clip_screen_range(seq_term->calc_interval_displayed_in_rectangle(&area_rect));
                int      scr_pos      = rm->sequence_to_screen(seq_pos);

                PosRange block_screen_range = rm->sequence_to_screen(block.get_range());
                PosRange block_visible_part = intersection(screen_range, block_screen_range);
                
                if (block_visible_part.is_empty()) {
                    if (scr_pos>block_screen_range.end()) {
                        fix_pos = block.get_range().start();
                    }
                    else {
                        e4_assert(scr_pos<block_screen_range.start());
                        fix_pos = block.get_range().end();
                    }
                }
                else {
                    int dist_left  = abs(scr_pos-block_visible_part.start());
                    int dist_right = abs(scr_pos-block_visible_part.end());

                    if (dist_left < dist_right) {   // click nearer to left border of visible part of block
                        fix_pos = block.get_range().end(); // keep right block-border
                    }
                    else {
                        fix_pos = block.get_range().start();
                    }
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
//      Replace
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

class replace_op : public ED4_block_operator { // derived from Noncopyable
    const char *oldString;
    const char *newString;
    
    int olen;
    int nlen;

    bool oldStringContainsJoker;

public:
    replace_op(const char *oldString_, const char *newString_)
        : oldString(oldString_),
          newString(newString_)
    {
        oldStringContainsJoker = strchr(oldString, '?') != 0;
        olen = strlen(oldString);
        nlen = strlen(newString);
    }

    char* operate(const SeqPart& part, int& new_len) const {
        int maxlen;
        int len   = part.length();
        int max_o = len-olen;

        if (nlen<=olen) {
            maxlen = len;
        }
        else {
            maxlen = (len/olen+1)*nlen;
        }

        char *new_seq  = (char*)GB_calloc(maxlen+1, sizeof(*new_seq));
        int   replaced = 0;
        int   o        = 0;
        int   n        = 0;
        char  ostart   = oldString[0];

        const char *sequence = part.data();
        if (oldStringContainsJoker) {
            while (o<len) {
                if (o <= max_o && strncmpWithJoker(sequence+o, oldString, olen) == 0) {
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
                if (o <= max_o && sequence[o]==ostart && strncmp(sequence+o, oldString, olen) == 0) { // occurrence of oldString
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
            new_len = n;
        }
        else {
            freenull(new_seq);
        }

        return new_seq;
    }
};

static void replace_in_block(AW_window*) {
    AW_root *awr = ED4_ROOT->aw_root;
    ED4_with_whole_block(
        replace_op(awr->awar(ED4_AWAR_REP_SEARCH_PATTERN)->read_char_pntr(),
                         awr->awar(ED4_AWAR_REP_REPLACE_PATTERN)->read_char_pntr()));
}

AW_window *ED4_create_replace_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;

    aws->init(root, "REPLACE", "Search & Replace");
    aws->load_xfig("edit4/replace.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "Close", "C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP, (AW_CL)"e4_replace.hlp");
    aws->create_button("HELP", "Help", "H");

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
//      Other block operations
// --------------------------------------------------------------------------------

class case_op : public ED4_block_operator {
    bool to_upper;
public:
    case_op(bool to_upper_) : to_upper(to_upper_) {}

    char *operate(const SeqPart& part, int& new_len) const {
        int         len     = part.length();
        const char *seq     = part.data();
        char       *new_seq = (char*)GB_calloc(len+1, sizeof(*new_seq));

        if (to_upper) {
            for (int i=0; i<len; i++) new_seq[i] = toupper(seq[i]);
        }
        else {
            for (int i=0; i<len; i++) new_seq[i] = tolower(seq[i]);
        }
        
        new_len = len;
        return new_seq;
    }
};

class revcomp_op : public ED4_block_operator {
    bool reverse;
    bool complement;
    char T_or_U;

public:
    revcomp_op(GB_alignment_type aliType, bool reverse_, bool complement_)
        : reverse(reverse_),
          complement(complement_),
          T_or_U(0)
    {
        if (complement) {
            error = GBT_determine_T_or_U(aliType, &T_or_U, reverse ? "reverse-complement" : "complement");
        }
    }

    char *operate(const SeqPart& part, int& new_len) const {
        char *result = NULL;
        if (!error) {
            int len = part.length();
            if (complement) {
                result = GBT_complementNucSequence(part.data(), len, T_or_U);
                if (reverse) freeset(result, GBT_reverseNucSequence(result, len));
            }
            else if (reverse) result = GBT_reverseNucSequence(part.data(), len);

            new_len = len;
        }
        return result;
    }

};

class unalign_op : public ED4_block_operator {
    int direction;
public:
    unalign_op(int direction_) : direction(direction_) {}

    char *operate(const SeqPart& part, int& new_len) const {
        bool rightward = direction>0;

        int         len    = part.length();
        const char *seq    = part.data();
        char       *result = (char*)GB_calloc(len+1, sizeof(*result));

        int o = 0;
        int n = 0;

        while (o<len) {
            if (!ADPP_IS_ALIGN_CHARACTER(seq[o])) result[n++] = seq[o];
            o++;
        }

        if (n<len) { // (move and) dot rest
            int gapcount = len-n;
            if (rightward) {
                memmove(result+gapcount, result, n);
                memset(result, part.left_gap(), gapcount);
            }
            else {
                memset(result+n, part.right_gap(), gapcount);
            }
        }

        if (memcmp(result, seq, len) == 0) {
            return NULL;
        }
        new_len = len;
        return result;
    }


};

class shift_op : public ED4_block_operator {
    int direction;

    char *shift_left_sequence(const SeqPart& part, int& new_len) const {
        char       *result = 0;
        const char *seq    = part.data();

        if (!ADPP_IS_ALIGN_CHARACTER(seq[0])) {
            error = "Need a gap at block start for shifting left";
        }
        else {
            int len       = part.length();
            result        = (char*)GB_calloc(len+1, sizeof(*result));
            new_len       = len;
            result[len-1] = part.right_gap();
            memcpy(result, seq+1, len-1);
        }
        return result;
    }

    char *shift_right_sequence(const SeqPart& part, int& new_len) const {
        char       *result = 0;
        const char *seq    = part.data();
        int         len    = part.length();

        if (!ADPP_IS_ALIGN_CHARACTER(seq[len-1])) {
            error = "Need a gap at block end for shifting right";
        }
        else {
            result    = (char*)GB_calloc(len+1, sizeof(*result));
            new_len   = len;
            result[0] = part.left_gap();
            memcpy(result+1, seq, len-1);
        }
        return result;
    }

    
public:
    shift_op(int direction_) : direction(direction_) {}

    char *operate(const SeqPart& part, int& new_len) const {
        return direction<0
            ? shift_left_sequence(part, new_len)
            : shift_right_sequence(part, new_len);
    }
};

void ED4_perform_block_operation(ED4_blockoperation_type operationType) {
    switch (operationType) {
        case ED4_BO_UPPER_CASE: ED4_with_whole_block(case_op(true));  break;
        case ED4_BO_LOWER_CASE: ED4_with_whole_block(case_op(false)); break;

        case ED4_BO_REVERSE:            ED4_with_whole_block(revcomp_op(ED4_ROOT->alignment_type, true,  false)); break;
        case ED4_BO_COMPLEMENT:         ED4_with_whole_block(revcomp_op(ED4_ROOT->alignment_type, false, true));  break;
        case ED4_BO_REVERSE_COMPLEMENT: ED4_with_whole_block(revcomp_op(ED4_ROOT->alignment_type, true,  true));  break;

        case ED4_BO_UNALIGN:       ED4_with_whole_block(unalign_op(-1)); break;
        case ED4_BO_UNALIGN_RIGHT: ED4_with_whole_block(unalign_op(1));  break;

        case ED4_BO_SHIFT_LEFT:  ED4_with_whole_block(shift_op(-1)); break;
        case ED4_BO_SHIFT_RIGHT: ED4_with_whole_block(shift_op(1));  break;

        default: {
            e4_assert(0);
            break;
        }
    }
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static arb_test::match_expectation blockop_expected_io(const ED4_block_operator& blockop, const char *oversized_input, const char *expected_result, const char *part_of_error) {
    int      whole_len = strlen(oversized_input);
    SeqPart  part(oversized_input, 1, whole_len-2);
    int      new_len; 
    char    *result    = blockop.operate(part, new_len);

    using namespace arb_test;

    expectation_group expectations;
    expectations.add(part_of_error
                     ? that(blockop.get_error()).does_contain(part_of_error)
                     : that(blockop.get_error()).is_equal_to(NULL));
    expectations.add(that(result).is_equal_to(expected_result));
    if (expected_result) expectations.add(that(new_len).is_equal_to(whole_len-2));

    free(result);

    return all().ofgroup(expectations);
}

#define TEST_ASSERT_BLOCKOP_PERFORMS(oversized_input,blockop,expected)         TEST_EXPECT(blockop_expected_io(blockop, oversized_input, expected, NULL))
#define TEST_ASSERT_BLOCKOP_PERFORMS__BROKEN(oversized_input,blockop,expected) TEST_EXPECT__BROKEN(blockop_expected_io(blockop, oversized_input, expected, NULL))
#define TEST_ASSERT_BLOCKOP_ERRORHAS(oversized_input,blockop,expected)         TEST_EXPECT(blockop_expected_io(blockop, oversized_input, NULL, expected))
#define TEST_ASSERT_BLOCKOP_ERRORHAS__BROKEN(oversized_input,blockop,expected) TEST_EXPECT__BROKEN(blockop_expected_io(blockop, oversized_input, NULL, expected))

void TEST_block_operators() {
    ED4_init_is_align_character("-.");

    // replace_op
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-C--", replace_op("-",  "."),  "A.C.");
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-C--", replace_op("?",  "."),  "....");
    TEST_ASSERT_BLOCKOP_PERFORMS("AACAG-", replace_op("AC", "CA"), "CAAG");
    TEST_ASSERT_BLOCKOP_PERFORMS("-ACAG-", replace_op("A?", "Ax"), "AxAx");

    TEST_ASSERT_BLOCKOP_PERFORMS("GACAG-", replace_op("GA", "AG"), NULL); // unchanged
    TEST_ASSERT_BLOCKOP_PERFORMS("GAGAGA", replace_op("GA", "AG"), "AAGG"); 
    TEST_ASSERT_BLOCKOP_PERFORMS("GACAGA", replace_op("GA", "AG"), NULL);
    TEST_ASSERT_BLOCKOP_PERFORMS("AGAGAG", replace_op("GA", "AG"), "AGAG");

    // case_op
    TEST_ASSERT_BLOCKOP_PERFORMS("-AcGuT-", case_op(true), "ACGUT");
    TEST_ASSERT_BLOCKOP_PERFORMS("-AcGuT-", case_op(false), "acgut");

    // revcomp_op
    TEST_ASSERT_BLOCKOP_PERFORMS("-Ac-GuT-", revcomp_op(GB_AT_RNA, false, false), NULL);     // noop
    TEST_ASSERT_BLOCKOP_PERFORMS("-Ac-GuT-", revcomp_op(GB_AT_RNA, true,  false), "TuG-cA");
    TEST_ASSERT_BLOCKOP_PERFORMS("-Ac-GuT-", revcomp_op(GB_AT_RNA, false, true),  "Ug-CaA");
    TEST_ASSERT_BLOCKOP_PERFORMS("-Ac-GuT-", revcomp_op(GB_AT_RNA, true,  true),  "AaC-gU");
    TEST_ASSERT_BLOCKOP_PERFORMS("-Ac-GuT-", revcomp_op(GB_AT_DNA, true,  true),  "AaC-gT");
    
    TEST_ASSERT_BLOCKOP_PERFORMS("-AR-DQF-", revcomp_op(GB_AT_AA, false, false), NULL); // noop             
    TEST_ASSERT_BLOCKOP_PERFORMS("-AR-DQF-", revcomp_op(GB_AT_AA, true,  false), "FQD-RA");
    TEST_ASSERT_BLOCKOP_ERRORHAS("-AR-DQF-", revcomp_op(GB_AT_AA, false, true),  "complement not available");
    TEST_ASSERT_BLOCKOP_ERRORHAS("-AR-DQF-", revcomp_op(GB_AT_AA, true,  true),  "reverse-complement not available");

    // unalign_op
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-c-G--T-", unalign_op(-1), "AcGT----");
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-c-G-T-T", unalign_op(-1), "AcGT----");
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-c-G--TT", unalign_op(-1), "AcGT----");
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-c-G--T.", unalign_op(-1), "AcGT....");
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-c-G-T.T", unalign_op(-1), "AcGT....");

    TEST_ASSERT_BLOCKOP_PERFORMS("A-Ac-G--T-", unalign_op(+1), "----AcGT");
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-c-G--T-", unalign_op(+1), "----AcGT");
    TEST_ASSERT_BLOCKOP_PERFORMS("A.Ac-G--T-", unalign_op(+1), "....AcGT");
    TEST_ASSERT_BLOCKOP_PERFORMS(".A-c-G--T-", unalign_op(+1), "....AcGT");
    TEST_ASSERT_BLOCKOP_PERFORMS("AA-c-G--T-", unalign_op(+1), "----AcGT");

    TEST_ASSERT_BLOCKOP_PERFORMS("-ACGT-", unalign_op(-1), NULL);
    TEST_ASSERT_BLOCKOP_PERFORMS("-ACGT-", unalign_op(+1), NULL);

    // shift_op
    TEST_ASSERT_BLOCKOP_PERFORMS("-A-C--", shift_op(+1), "-A-C"); // take gap outside region
    TEST_ASSERT_BLOCKOP_PERFORMS(".A-C--", shift_op(+1), ".A-C"); // -"-
    TEST_ASSERT_BLOCKOP_PERFORMS("-.AC--", shift_op(+1), "..AC"); // take gap inside region

    TEST_ASSERT_BLOCKOP_PERFORMS("--A-C-", shift_op(-1), "A-C-"); // same for other direction
    TEST_ASSERT_BLOCKOP_PERFORMS("--A-C.", shift_op(-1), "A-C.");
    TEST_ASSERT_BLOCKOP_PERFORMS("--AC..", shift_op(-1), "AC..");

    TEST_ASSERT_BLOCKOP_PERFORMS("G-TTAC", shift_op(-1), "TTA-"); // no gap reachable

    TEST_ASSERT_BLOCKOP_ERRORHAS("GATTAC", shift_op(-1), "Need a gap at block");  // no space
    TEST_ASSERT_BLOCKOP_ERRORHAS("GATTAC", shift_op(+1), "Need a gap at block");  // no space
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
