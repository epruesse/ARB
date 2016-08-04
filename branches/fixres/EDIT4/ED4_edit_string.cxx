#include <arbdbt.h>
#include <AW_helix.hxx>

#include <fast_aligner.hxx>
#include <awt_map_key.hxx>

#include "ed4_edit_string.hxx"
#include "ed4_class.hxx"
#include "ed4_awars.hxx"
#include "ed4_seq_colors.hxx"

#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_advice.hxx>
#include <cctype>

#define SAFE_EDITING        // undef this to remove sequence test code

int ED4_Edit_String::nrepeat = 0;           // # of times command should be repeated
int ED4_Edit_String::nrepeat_zero_requested = 0;    // nrepeat should be set to zero
int ED4_Edit_String::nrepeat_is_already_set = 0;    // nrepeat was zero (and was set to 1)

GB_ERROR  ED4_Edit_String::insert(char *text, long position, int direction, int removeAtNextGap) {
    long i;
    int text_len = strlen(text);
    if (text_len == 0) return 0;

    int rest_len;

    if (direction>=0) {
        rest_len = seq_len-position;
    }
    else {
        rest_len = position;
    }

    if (rest_len<=0 || text_len>rest_len) {
        return GBS_global_string("You can't insert text after the end of the sequence!");
    }

    if (removeAtNextGap) {
        int nextGap = get_next_gap(position, direction);
        if (nextGap==-1) {
            goto no_gaps;
        }

        int afterGap = nextGap+direction;

        if (afterGap>=0 && afterGap<seq_len) { // in sequence
            if (seq[afterGap]!=seq[nextGap]) { // gap type changed -> warn to avoid overwrite of '?'
                return GBS_global_string("Only one gap %s text => insert aborted", direction>=0 ? "after" : "before");
            }
        }

        rest_len = (afterGap-position)*direction;
    }

    if (direction>=0) {
        if (text_len + position >= seq_len) {
            return GBS_global_string("You cannot insert that many characters after cursor!");
        }

        for (i = rest_len-text_len; i<rest_len; i++) {
            if (!ED4_is_gap_character(seq[position+i])) {
                goto no_gaps;
            }
        }

        for (i = position+rest_len-text_len-1; i>=position; i--) {
            seq[i+text_len] = seq[i];
        }

        for (i = 0; i<text_len; i++) {
            seq[position+i] = text[i];
        }
    }
    else {
        if (position - text_len < 0) {
            return GBS_global_string("You cannot insert that many characters before cursor!");
        }

        for (i = 0; i<text_len; i++) {
            if (i<0 || !ED4_is_gap_character(seq[i])) {
                goto no_gaps;
            }
        }
        for (i = 0; i<position-text_len; i++) {
            seq[i] = seq[i+text_len];
        }

        for (i = 0; i<text_len; i++) {
            seq[position-i-1] = text[i];
        }
    }

    return 0;

 no_gaps :
    return GBS_global_string("There are no/not enough gaps at %s of sequence => can't insert characters%s",
                             direction >= 0 ? "end" : "start",
                             direction >= 0 ? "\nMaybe your sequences are not formatted?" : "");
}

GB_ERROR ED4_Edit_String::remove(int len, long position, int direction, int insertAtNextGap) {

    int rest_len; // bases between cursor-position and the directed sequence end

    if (direction>=0) { // forward
        if ((len + position) >= seq_len) len = (int)(seq_len-position);
        rest_len = seq_len-position;
    }
    else { // backward
        position--;
        if (len>position) len = position;
        rest_len = position;
    }

    if ((position >= seq_len) || (len == 0) || (seq_len - len < 0)) {
        return GBS_global_string("Illegal delete position/length");
    }

    if (insertAtNextGap) {
        int nextGap = get_next_gap(position+len*direction, direction);

        if (nextGap!=-1) {
            rest_len = (nextGap-position)*direction;
        }
    }

    int new_len = rest_len-len; // no of bases that will be copied
    int i;

    if (direction>=0) {
        for (i=0; i<new_len; i++) {
            seq[position+i] = seq[position+len+i];
        }
        for (; i<rest_len; i++) {
            seq[position+i] = '.';
        }
    }
    else {
        for (i=0; i<new_len; i++) {
            seq[position-i] = seq[position-len-i];
        }
        for (; i<rest_len; i++) {
            seq[position-i] = '.';
        }
    }

    e4_assert(seq[seq_len]=='\0');

    return 0;
}

GB_ERROR ED4_Edit_String::replace(char *text, long position, int direction) {
    int text_len = strlen(text);
    int i;
    if (direction>=0) {

        if ((position + text_len > seq_len) || (position > seq_len)) {
            return GBS_global_string("Replace after end of sequence !");
        }
        for (i = 0; i < text_len;   i ++) {
            seq[i+position] = text[i];
        }
    }
    else {
        if ((position - text_len < 0) || (position > seq_len)) {
            return GBS_global_string("Replace before start of sequence !");
        }
        for (i = 0; i < text_len;   i ++) {
            seq[position - i - 1] = text[i];
        }
    }
    return 0;
}

GB_ERROR ED4_Edit_String::swap_gaps(long position, char ch) {
    long i;
    for (i = position; i < seq_len; i++) {
        if (!ED4_is_gap_character(seq[i])) break;
        seq[i] = ch;
    }
    for (i = position; i >= 0; i--) {
        if (!ED4_is_gap_character(seq[i])) break;
        seq[i] = ch;
    }
    return 0;
}


GB_ERROR ED4_Edit_String::moveBase(long source_position, long dest_position, unsigned char gap_to_use)
{
    e4_assert(source_position!=dest_position);
#ifdef SAFE_EDITING
    if (!legal_seqpos(dest_position) || !legal_seqpos(source_position)) {
        return "Position out of sequence";
    }
#endif

    int direction = dest_position<source_position ? -1 : 1;
    int base = seq[source_position];
    e4_assert(!ED4_is_gap_character(base));
    seq[source_position] = gap_to_use;

    long p = source_position+direction;
    while (p!=dest_position) {
#ifdef SAFE_EDITING
        if (!ED4_is_gap_character(seq[p])) {
            e4_assert(0);
            return "Internal error: Tried to change base order in moveBase()";
        }
#endif
        e4_assert(ED4_is_gap_character(seq[p]));
        seq[p] = gap_to_use;
        p += direction;
    }

    seq[dest_position] = base;
    return 0;
}

GB_ERROR ED4_Edit_String::shiftBases(long source_pos, long last_source, long dest_pos, int direction, long *last_dest, unsigned char gap_to_use)
    // shifts bases from source_pos-last_source to dest_pos
    // last_dest is set to the position after the last dest_pos (direction<0 ? pos. right of bases : pos. left of bases)
{
    GB_ERROR err = 0;

    if (direction<0) {
        e4_assert(dest_pos<source_pos);
        e4_assert(source_pos<=last_source);
        while (1) {
            err = moveBase(source_pos, dest_pos, gap_to_use);
            if (err || source_pos>=last_source) break;
            source_pos++;
            dest_pos++;
            while (!ED4_is_gap_character(seq[dest_pos])) {
                dest_pos++;
            }
        }
    }
    else {
        e4_assert(source_pos<dest_pos);
        e4_assert(last_source<=source_pos);
        while (1) {
            err = moveBase(source_pos, dest_pos, gap_to_use);
            if (err || source_pos<=last_source) break;
            source_pos--;
            dest_pos--;
            while (!ED4_is_gap_character(seq[dest_pos])) {
                dest_pos--;
            }
        }
    }

    if (last_dest) {
        *last_dest = dest_pos;
    }

    return err;
}

long ED4_Edit_String::get_next_base(long position, int direction) {
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos>=0 && pos < seq_len; pos += direction) {
        if (!ED4_is_gap_character(seq[pos])) break;
    }

    return pos<0 || pos>=seq_len ? -1 : pos;
}
long ED4_Edit_String::get_next_gap(long position, int direction) {
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos >= 0 && pos < seq_len; pos += direction) {
        if (ED4_is_gap_character(seq[pos])) break;
    }

    return pos<0 || pos>=seq_len ? -1 : pos;
}
long ED4_Edit_String::get_next_visible_base(long position, int direction)
{
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos>=0 && pos < seq_len; pos += direction) {
        if (!ED4_is_gap_character(seq[pos]) && remap->is_shown(pos)) {
            break;
        }
    }

    return pos<0 || pos>=seq_len ? -1 : pos;
}
long ED4_Edit_String::get_next_visible_gap(long position, int direction) {
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos >= 0 && pos < seq_len; pos += direction) {
        if (ED4_is_gap_character(seq[pos]) && remap->is_shown(pos)) {
            break;
        }
    }

    return pos<0 || pos>=seq_len ? -1 : pos;
}
long ED4_Edit_String::get_next_visible_pos(long position, int direction)
{
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos >= 0 && pos < seq_len; pos += direction) {
        if (remap->is_shown(pos)) {
            break;
        }
    }
    return pos<0 || pos>=seq_len ? -1 : pos;
}

unsigned char ED4_Edit_String::get_gap_type(long pos, int direction)
{
    pos += direction;
    if (!legal_seqpos(pos)) return '.';
    char gap = seq[pos];
    if (ED4_is_gap_character(gap)) return gap;
    return '-';
}

static GB_ERROR toggle_cursor_group_folding() {
    GB_ERROR      error  = NULL;
    ED4_cursor&   cursor = current_cursor();
    if (cursor.owner_of_cursor) {
        ED4_terminal *cursor_term = cursor.owner_of_cursor;

        if (cursor_term->is_in_folded_group()) {
            // cursor not visible -> unfold group which hides cursor
            cursor_term->setCursorTo(&cursor, cursor.get_sequence_pos(), true, ED4_JUMP_KEEP_POSITION);
        }
        else {
            ED4_base *group = cursor.owner_of_cursor->get_parent(ED4_L_GROUP);
            if (group) {
                ED4_group_manager    *group_man    = group->to_group_manager();
                ED4_bracket_terminal *bracket_term = group_man->get_defined_level(ED4_L_BRACKET)->to_bracket_terminal();

                if (group_man->dynamic_prop & ED4_P_IS_FOLDED) {
                    bracket_term->unfold();
                }
                else {
                    ED4_species_manager *consensus_man = group_man->get_multi_species_manager()->get_consensus_manager();
                    if (consensus_man) consensus_man->setCursorTo(&cursor, cursor.get_sequence_pos(), true, ED4_JUMP_KEEP_POSITION);

                    bracket_term->fold();
                }
            }
        }
    }
    return error;
}

static void toggle_mark_of_specData(GBDATA *gb_data) {
    // toggle mark of species owning data 'gb_data'
    GB_transaction  ta(gb_data);
    GBDATA         *gb_species = GB_get_grandfather(gb_data);
    if (gb_species) GB_write_flag(gb_species, !GB_read_flag(gb_species));
}

GB_ERROR ED4_Edit_String::command(AW_key_mod keymod, AW_key_code keycode, char key, int direction, ED4_EDITMODE mode, bool is_consensus,
                                  long &seq_pos, bool &changed_flag, ED4_CursorJumpType& cursor_jump, bool &cannot_handle, bool &write_fault, GBDATA* gb_data, bool is_sequence)
{
    changed_flag = 0;
    write_fault = 0;
    if (keymod+keycode+key==0) return 0;

    e4_assert(nrepeat>0);

    long old_seq_pos = seq_pos;
    int  screen_len  = remap->sequence_to_screen(seq_len);
    int  cursorpos   = remap->sequence_to_screen(seq_pos);

    char str[2];
    str[0] = key;
    str[1] = 0;

    direction = direction>0 ? 1 : -1;

    if ((cursorpos > screen_len) || (cursorpos < 0)) {
        if (cursorpos<MAXSEQUENCECHARACTERLENGTH && cursorpos>=0) {
            char *seq2 = new char[MAXSEQUENCECHARACTERLENGTH+1];

            memcpy(seq2, seq, seq_len);
            memset(seq2+seq_len, '.', MAXSEQUENCECHARACTERLENGTH-seq_len);
            seq_len = MAXSEQUENCECHARACTERLENGTH;
            seq2[seq_len] = 0;

            delete seq;
            seq = seq2;

            changed_flag = 1;
        }
        else {
            return GBS_global_string("Cursor out of sequence at screen_pos #%i!", cursorpos);
        }
    }

    GB_ERROR ad_err = 0;
    long h;
    bool reinterpret_key = true;

    while (reinterpret_key) {
        reinterpret_key = false;

        switch (keycode) {
            case AW_KEY_HOME: {
                int new_seq_pos = get_next_visible_base(0, 1);
                if (new_seq_pos>=0) seq_pos = new_seq_pos==seq_pos ? 0 : new_seq_pos;
                else seq_pos = 0;
                break;
            }
            case AW_KEY_END: {
                int new_seq_pos = get_next_visible_base(seq_len, -1);
                if (new_seq_pos>=0) {
                    int new_gap_pos = get_next_visible_gap(new_seq_pos, 1);
                    if (new_gap_pos >= 0) {
                        seq_pos = new_gap_pos==seq_pos ? seq_len : new_gap_pos;
                    }
                    else {
                        seq_pos = seq_len;
                    }
                }
                else {
                    seq_pos = seq_len;
                }
                break;
            }
            case AW_KEY_LEFT:
            case AW_KEY_RIGHT: {
                direction = keycode==AW_KEY_RIGHT ? 1 : -1;

                // no key modifier -> just move cursor

                int n = nrepeat;

                if (keymod == AW_KEYMODE_NONE) {
                    while (n--) {
                        do {
                            seq_pos += direction;
                        }
                        while (legal_curpos(seq_pos) && !remap->is_shown(seq_pos));
                    }
                    break;
                }

                if (mode==AD_NOWRITE) { write_fault = 1; break; }

                int jump_or_fetch = 0;  // 1=jump selected 2=fetch selected (for repeat only)
                int push_or_pull = 0;   // 1=push selected 2=pull selected (for repeat only)

                // ------------------
                // loop over nrepeat:

                while (!ad_err && n-->0 && legal_curpos(seq_pos)) {
                    cursorpos = remap->sequence_to_screen(seq_pos);

                    int adjacent_scr_pos = cursorpos - (direction<0); // screen position next to the cursor
                    if (adjacent_scr_pos<0 || size_t(adjacent_scr_pos)>remap->get_max_screen_pos()) {
                        ad_err = GBS_global_string("Action beyond end of screen!");
                        break;
                    }

                    int adjacent_seq_pos = remap->screen_to_sequence(adjacent_scr_pos); // _visible_ sequence position next to the cursor
                    int real_adjacent_seq_pos = seq_pos - (direction<0); // sequence position next to cursor (not necessarily visible)

                    if (adjacent_seq_pos<0 || adjacent_seq_pos>=seq_len) {
                        ad_err = GBS_global_string("Action beyond end of sequence!");
                        break;
                    }

                    // Ctrl+Cursor = move cursor to next end of word (or to end of next word)

                    if (keymod & AW_KEYMODE_CONTROL) {
                        if (adjacent_scr_pos>=0) {
                            long pos = adjacent_seq_pos;

                            if (ED4_ROOT->aw_root->awar(ED4_AWAR_FAST_CURSOR_JUMP)->read_int()) { // should cursor jump over next group?
                                if (ED4_is_gap_character(seq[pos])) {
                                    pos = get_next_visible_base(pos, direction);
                                    if (pos>=0) pos = get_next_visible_gap(pos, direction);
                                }
                                else {
                                    pos = get_next_visible_gap(pos, direction);
                                }

                                seq_pos = (pos>=0
                                           ? (direction<0 ? pos+1 : pos)
                                           : (direction<0 ? 0 : seq_len-1));
                            }
                            else {
                                if (ED4_is_gap_character(seq[pos])) { seq_pos = get_next_visible_base(pos, direction); }
                                else                                { seq_pos = get_next_visible_gap(pos, direction); }

                                if (direction<0) { seq_pos = seq_pos==-1 ? 0       : seq_pos+1; }
                                else             { seq_pos = seq_pos==-1 ? seq_len : seq_pos; }
                            }
                        }
                        continue;
                    }

                    // ALT/META+Cursor = jump & fetch

                    if (keymod & (AW_KEYMODE_ALT)) {
                        if (is_consensus) { cannot_handle = 1; return 0; }

                        if (ED4_is_gap_character(seq[adjacent_seq_pos])) { // there's a _gap_ next to the cursor -> let's fetch
                            if (jump_or_fetch!=1) {
                                jump_or_fetch = 2;
                                long source_pos = get_next_base(adjacent_seq_pos, direction); // position of base to fetch
                                if (source_pos==-1) { // there is nothing to fetch
                                    n = 0;
                                }
                                else {
                                    ad_err = moveBase(source_pos, adjacent_seq_pos, get_gap_type(source_pos, direction));
                                    seq_pos = adjacent_seq_pos + (direction>0);
                                }
                            }
                            else {
                                n = 0;
                            }
                        }
                        else { // there's a _base_ next to the cursor -> let it jump
                            if (jump_or_fetch!=2) {
                                jump_or_fetch = 1;
                                int next_gap = adjacent_seq_pos - direction;

                                if (ED4_is_gap_character(seq[next_gap])) {
                                    int dest_pos = get_next_base(next_gap, -direction);

                                    if (dest_pos<0) {
                                        dest_pos = direction>0 ? 0 : seq_len-1;
                                    }
                                    else {
                                        dest_pos += direction;
                                    }

                                    if (ED4_is_gap_character(seq[dest_pos])) {
                                        ad_err = moveBase(adjacent_seq_pos, dest_pos, get_gap_type(adjacent_seq_pos, direction));
                                        if (!ad_err) {
                                            seq_pos = get_next_base(seq_pos, direction)+(direction<0);
                                            if (seq_pos==-1) {
                                                seq_pos = direction<0 ? 0 : seq_len;
                                            }
                                        }
                                    }
                                    else {
                                        e4_assert(0);
                                    }
                                }
                                else {
                                    ad_err = GBS_global_string("You can only jump single bases.");
                                }
                            }
                            else {
                                n = 0;
                            }
                        }

                        if (!ad_err) {
                            changed_flag = 1;
                        }

                        continue;
                    }

                    // Shift+Cursor = push/pull character

                    if (is_consensus) { cannot_handle = 1; return 0; };

                    if (ED4_is_gap_character(seq[real_adjacent_seq_pos])) { // pull
                        long dest_pos = real_adjacent_seq_pos;
                        long source_pos = real_adjacent_seq_pos-direction;

                        if (!ED4_is_gap_character(seq[source_pos]) && push_or_pull!=1) {
                            push_or_pull = 2;

                            long next_gap = get_next_gap(source_pos, -direction);
                            long last_source = next_gap>=0 ? next_gap : (direction>0 ? 0 : seq_len-1);

                            if (ED4_is_gap_character(seq[last_source])) {
                                last_source = get_next_base(last_source, direction);
                            }

                            ad_err = shiftBases(source_pos, last_source, dest_pos, direction, 0,
                                                is_sequence ? get_gap_type(last_source, -direction) : '.');

                            if (!ad_err) {
                                seq_pos = dest_pos + (direction>0);
                                changed_flag = 1;
                            }
                        }
                        else {
                            n = 0;
                        }
                    }
                    else { // push
                        long next_gap = get_next_gap(adjacent_seq_pos, direction);

                        if (next_gap>=0 && push_or_pull!=2) {
                            push_or_pull = 1;
                            long dest_pos = next_gap;
                            long source_pos = get_next_base(next_gap, -direction);
                            long last_source = adjacent_seq_pos;

                            e4_assert(source_pos>=0);
                            ad_err = shiftBases(source_pos, last_source, dest_pos, direction, &dest_pos,
                                                is_sequence ? get_gap_type(last_source, -direction) : '.');

                            if (!ad_err) {
                                seq_pos = dest_pos + (direction<0);
                                changed_flag = 1;
                            }
                        }
                        else {
                            n = 0;
                        }
                    }
                }
                break;
            }

            case AW_KEY_BACKSPACE:
                h = seq_pos;

                if (direction>0) {
                    seq_pos -= nrepeat;
                }
                else {
                    seq_pos += nrepeat;
                }
                if (seq_pos <0 || seq_pos >= seq_len) {
                    seq_pos = h;
                    break;
                }
                // fall-through

            case AW_KEY_DELETE:
                h = seq_pos;

                if (is_consensus) { cannot_handle = 1; return 0; };
                if (keymod) { return 0; }

                if (mode==AD_NOWRITE) { write_fault = 1; break; }

                switch (mode) {
                    case AD_ALIGN:
                        {
                            int len;
                            int offset;

                            ad_err = 0;
                            if (direction>=0)   offset = 0;
                            else        offset = -nrepeat;

                            for (len = nrepeat-1; len>=0; len--) {
                                if (!ED4_is_gap_character(seq[h+offset+len])) {
                                    ad_err = GBS_global_string("You cannot remove bases in align mode");
                                    break;
                                }
                            }
                            if (ad_err) break;
                        }
                    case AD_REPLACE:
                    case AD_INSERT:
                        ad_err = remove(nrepeat, h, direction, !is_sequence);
                        if (!ad_err) {
                            changed_flag = 1;
                        }
                        break;
                    default:
                        break;
                }

                break;

            case AW_KEY_RETURN: 
                ad_err = toggle_cursor_group_folding();
                break; 

            case AW_KEY_ASCII: {

                //      keyboard layout:
                //
                //      - CTRL-A    Align                               ok
                //      - CTRL-D    Toggle view differences             ok
                //      - CTRL-E    Toggle edit/align                   ok
                //      - CTRL-I    Toggle insert/replace               ok
                //      - CTRL-J    Jump opposite helix position        ok
                //      - CTRL-K    Toggle compression on/off           ok
                //      - CTRL-L    Refresh                             ok
                //      - CTRL-M    Invert mark                         ok
                //      - CTRL-O    = ALT-LEFT                          ok
                //      - CTRL-P    = ALT-RIGHT                         ok
                //      - CTRL-R    Set aligner OR viewDiff reference species       ok
                //      - CTRL-S    Repeat last search                  ok
                //      - CTRL-U    Undo                                @@@ crashes! disabled atm!


                if (key >0 && key<=26) { // CTRL-Keys
                    switch (key+'A'-1) {
                        case 'A': { // CTRL-A = Start Fast-Aligner
                            AW_window *aw_tmp = current_aww();
                            if (is_consensus) { cannot_handle = 1; return 0; };
                            if (mode==AD_NOWRITE) { write_fault = 1; return 0; }

                            AlignDataAccess localDataAccess(*ED4_get_aligner_data_access()); // use local modified copy
                            localDataAccess.do_refresh = false;

                            FastAligner_set_align_current(ED4_ROOT->aw_root, ED4_ROOT->props_db);
                            AW_clock_cursor(ED4_ROOT->aw_root);
                            GB_commit_transaction(localDataAccess.gb_main);

                            FastAligner_start(aw_tmp, &localDataAccess);

                            GB_begin_transaction(localDataAccess.gb_main);
                            AW_normal_cursor(ED4_ROOT->aw_root);

                            int basesLeftOf = 0;
                            int pos;

                            for (pos=0; pos < seq_pos && pos<seq_len; pos++) {    // count # of bases left of cursorpos
                                if (!ED4_is_gap_character(seq[pos])) {
                                    basesLeftOf++;
                                }
                            }

                            char *aligned_seq = GB_read_string(gb_data); // read new sequence
                            ad_err            = GB_write_string(gb_data, seq); // restore old sequence

                            freeset(seq, aligned_seq); // set new sequence
                            changed_flag = 1;       // and mark changed

                            {
                                int basesLeftOf2   = 0;
                                int lastCorrectPos = -1;
                                for (pos=0; pos<seq_pos && pos<seq_len; pos++) {    // count # of bases left of cursorpos
                                    if (!ED4_is_gap_character(seq[pos])) {
                                        basesLeftOf2++;
                                        if (basesLeftOf2 == basesLeftOf) lastCorrectPos = pos;
                                    }
                                }

                                if (basesLeftOf != basesLeftOf2) { // old cursor-position has different basepos
                                    if (lastCorrectPos != -1) { // already seen position with same basepos
                                        seq_len = lastCorrectPos+1;
                                    }
                                    else {
                                        for (; pos<seq_len; pos++) {
                                            if (!ED4_is_gap_character(seq[pos])) {
                                                basesLeftOf2++;
                                                if (basesLeftOf2 == basesLeftOf) {
                                                    seq_pos = pos+1;
                                                    break; // stop after first matching position
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        case 'R': {  // CTRL-R = set aligner reference species OR set reference for diff-mode
                            ED4_reference *ref = ED4_ROOT->reference;
                            if (ref->is_set()) { // if "view differences" is active = > set new reference
                                ED4_viewDifferences_setNewReference();
                            }
                            else { // otherwise set aligner reference
                                if (is_consensus) { cannot_handle = 1; return 0; };
                                FastAligner_set_reference_species(ED4_ROOT->aw_root);
                            }
                            break;
                        }
                        case 'D': { // CTRL-D = Toggle view differences
                            ED4_toggle_viewDifferences(ED4_ROOT->aw_root);
                            break;
                        }
                        case 'E': { // CTRL-E = Toggle Edit/Align-Mode
                            ED4_ROOT->aw_root->awar(AWAR_EDIT_MODE)->toggle_toggle();
                            break;
                        }
                        case 'I': { // CTRL-I = Toggle Insert/Replace-Mode
                            ED4_ROOT->aw_root->awar(AWAR_INSERT_MODE)->toggle_toggle();
                            break;
                        }
                        case 'J': { // CTRL-J = Jump to opposite helix position
                            AW_helix *helix = ED4_ROOT->helix;

                            if (!helix->has_entries()) ad_err = strdup("Got no helix information");
                            else if (helix->pairtype(seq_pos) != HELIX_NONE) {
                                seq_pos = helix->opposite_position(seq_pos);
                                cursor_jump = ED4_JUMP_KEEP_POSITION;
                            }
                            else ad_err = strdup("Not at helix position");
                            break;
                        }
                        case 'K': { // Ctrl-K = Compression on/off
                            static int last_mode;
                            int current_mode = (ED4_remap_mode)ED4_ROOT->aw_root->awar(ED4_AWAR_COMPRESS_SEQUENCE_GAPS)->read_int();
                            int next_mode = last_mode;

                            if (current_mode==0) { // if not compressed
                                if (last_mode==0) next_mode = 2; // force compress
                            }
                            else {  // if compressed
                                next_mode = 0; // force uncompressed
                            }

                            ED4_ROOT->aw_root->awar(ED4_AWAR_COMPRESS_SEQUENCE_GAPS)->write_int(next_mode);

                            last_mode = current_mode;
                            break;
                        }
                        case 'L': {  // CTRL-L = Refresh
                            ED4_request_full_refresh();
                            ED4_request_relayout();
                            cursor_jump = ED4_JUMP_CENTERED;
                            break;
                        }
                        case 'M': { // CTRL-M = Invert mark(s)
                            if (is_consensus) { cannot_handle = 1; return 0; }
                            toggle_mark_of_specData(gb_data);
                            break;
                        }
                        case 'O': { //  for ALT-left
                            keycode = AW_KEY_LEFT;
                            keymod = AW_KEYMODE_ALT;
                            reinterpret_key = true;
                            break;
                        }
                        case 'P': { // for ALT-right
                            keycode = AW_KEY_RIGHT;
                            keymod = AW_KEYMODE_ALT;
                            reinterpret_key = true;
                            break;
                        }
                        case 'S': { // CTRL-S = Repeat last search
                            ad_err      = ED4_repeat_last_search(current_ed4w());
                            seq_pos     = current_cursor().get_sequence_pos();
                            cursor_jump = ED4_JUMP_KEEP_POSITION;
                            break;
                        }
                        case 'U': {
                            // ad_err = GB_undo(gb_main, GB_UNDO_UNDO); // @@@ stuerzt ab - wahrscheinlich weil Transaktion offen ist
                            break;
                        }
                    }
                }
                else { // normal characters
                    if (is_consensus) { cannot_handle = 1; return 0; }
                    if (mode==AD_NOWRITE) { write_fault = 1; break; }

                    if (key == ' ') {
                        if (is_sequence) {
                            long left = seq_pos>0 ? seq_pos-1 : 0;
                            int  l    = seq[left];
                            int  r    = seq[seq_pos];

                            char gapchar_at_pos = ED4_is_gap_character(l) ? l : (ED4_is_gap_character(r) ? r : 0);

                            switch (keymod) {
                                case AW_KEYMODE_NONE:
                                    key = gapchar_at_pos ? gapchar_at_pos : '-'; // insert (same) gap
                                    break;

                                case AW_KEYMODE_CONTROL:
                                    if (gapchar_at_pos) key = gapchar_at_pos == '-' ? '.' : '-'; // toggle gaptype
                                    break;

                                default: break;
                            }
                        }
                        str[0] = key;
                    }

                    if (ED4_is_gap_character(key) && keymod == AW_KEYMODE_CONTROL) { // gap-type change works in all modes
                        // gap type functions ('.' <-> '-')

                        long left = seq_pos>0 ? seq_pos-1 : 0;
                        int  l    = seq[left];
                        int  r    = seq[seq_pos];

                        if      (ED4_is_gap_character(l) && l!=key) { ad_err = swap_gaps(left, key);    changed_flag = 1; }
                        else if (ED4_is_gap_character(r) && r!=key) { ad_err = swap_gaps(seq_pos, key); changed_flag = 1; }
                    }
                    else {
                        switch (mode) {
                            case AD_ALIGN: {
                                if (ED4_is_gap_character(key)) {
                                    if (keymod == AW_KEYMODE_NONE) {
                                        if (!ad_err) {
                                            char *nstr = (char *)ARB_calloc(1, nrepeat+1);
                                            int i;

                                            for (i = 0; i< nrepeat; i++) nstr[i] = key;
                                            ad_err = insert(nstr, seq_pos, direction, 0);
                                            if (!ad_err) seq_pos = get_next_visible_pos(seq_pos+(direction>=0 ? nrepeat : 0), direction);
                                            delete nstr;
                                        }
                                        changed_flag = 1;
                                    }
                                }
                                else { // check typed bases against sequence
                                    while (nrepeat && !ad_err) {
                                        nrepeat--;
                                        seq_pos = get_next_visible_base(seq_pos, direction);
                                        if (seq_pos<0 || seq_pos>=seq_len) {
                                            ad_err = GBS_global_string("End of sequence reached");
                                        }
                                        else if (seq[seq_pos]!=key) {
                                            ad_err = GBS_global_string("Base '%c' at %li does not match '%c'", seq[seq_pos], seq_pos, key);
                                        }
                                        else {
                                            seq_pos = get_next_visible_pos(seq_pos, direction)+1;
                                            ad_err = 0;
                                        }
                                        if (ad_err) {
                                            ed4_beep();
                                        }
                                    }
                                }
                                break;
                            }
                            case AD_REPLACE: {
                                while (nrepeat && !ad_err) {
                                    nrepeat--;
                                    if (direction>0) {
                                        ad_err = replace(str, seq_pos, 1);

                                        if (!ad_err) {
                                            seq_pos++;
                                            changed_flag = 1;
                                        }
                                    }
                                    else {
                                        int left_pos = get_next_visible_pos(seq_pos, -1);

                                        if (left_pos>=0)    ad_err = replace(str, left_pos+1, -1);
                                        else            ad_err = "End of sequence";

                                        if (!ad_err) {
                                            seq_pos = left_pos;
                                            changed_flag = 1;
                                        }
                                    }
                                }
                                break;
                            }
                            case AD_INSERT: {
                                while (nrepeat && !ad_err) {
                                    nrepeat--;
                                    if (direction>0) {
                                        ad_err = insert(str, seq_pos, 1, !is_sequence);

                                        if (!ad_err) {
                                            seq_pos++;
                                            changed_flag = 1;
                                        }
                                    }
                                    else {
                                        int left_pos = get_next_visible_pos(seq_pos, -1);

                                        if (left_pos>=0) {
                                            ad_err = insert(str, left_pos+1, -1, !is_sequence);
                                        }
                                        else {
                                            ad_err = "End of sequence";
                                        }

                                        if (!ad_err) {
                                            seq_pos = left_pos;
                                            changed_flag = 1;
                                        }
                                    }
                                }
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                    }

                }
                break;
            }
            default: {
                break;
            }

        }
    }

    if (ad_err) seq_pos = old_seq_pos;
    seq_pos = seq_pos<0 ? 0 : (seq_pos>seq_len ? seq_len : seq_pos);    // ensure legal position

    return ad_err;
}

ED4_Edit_String::ED4_Edit_String()
{
    memset((char *)this, 0, sizeof(*this));
    remap = ED4_ROOT->root_group_man->remap();
}

ED4_Edit_String::~ED4_Edit_String()
{
    free(old_seq);
}

GB_ERROR ED4_Edit_String::edit(ED4_work_info *info) {
    e4_assert(info->working_terminal != 0);

    if (!info->rightward) { // 5'<-3'
        info->char_position++;
    }

    info->out_seq_position = remap->screen_to_sequence(info->char_position);
    info->refresh_needed   = false;
    info->cursor_jump      = ED4_JUMP_KEEP_VISIBLE;
    info->cannot_handle    = false;
    old_seq                = 0;

    if (info->string) {
        seq = info->string;
        seq_len = strlen(seq);
    }
    else {
        e4_assert(info->gb_data);
        int seq_len_int;
        seq = info->working_terminal->resolve_pointer_to_string_copy(&seq_len_int);
        seq_len = seq_len_int;
    }

    int map_key;
    if (info->event.keymodifier == AW_KEYMODE_NONE) {
        map_key = ED4_ROOT->edk->map_key(info->event.character);
        if (map_key != info->event.character) { // remapped
            if (info->event.character == ' ') { // space
                AW_advice("You have keymapped the space-key!\n"
                          "The new functionality assigned to space-key is not available when\n"
                          "key-mapping is enabled.\n",
                          AW_ADVICE_TOGGLE_AND_HELP, "Obsolete default keymapping", "nekey_map.hlp");
            }
        }
    }
    else {
        map_key = info->event.character;
    }
    GB_ERROR err = 0;

    nrepeat_zero_requested = 1;
    if (!nrepeat) {
        nrepeat_is_already_set = 1;
        nrepeat = 1;
    }

    if (info->event.keycode==AW_KEY_ASCII &&
        isdigit(map_key) &&
        ED4_ROOT->aw_root->awar(ED4_AWAR_DIGITS_AS_REPEAT)->read_int()) { // handle digits for repeated commands

        if (nrepeat_is_already_set) { nrepeat = 0; }
        nrepeat_zero_requested = 0;

        for (int r=0; r<info->repeat_count; r++) {
            nrepeat = nrepeat * 10 + (map_key - '0');
            if (nrepeat>10000) {
                nrepeat = 10000;
            }
            else if (nrepeat<0) {
                nrepeat = 0;
            }
        }
#ifdef DEBUG
        if (nrepeat>1) {
            printf("nrepeat=%i\n", nrepeat);
        }
#endif
    }
    else if (info->event.keymodifier==AW_KEYMODE_NONE && info->event.keycode==AW_KEY_NONE) {
        nrepeat_zero_requested = 0;
    }
    else {
        nrepeat += info->repeat_count-1;

#ifdef DEBUG
        if (nrepeat>1) {
            printf("nrepeat is %i\n", nrepeat);
        }
#endif

        int is_remapped_sequence = !info->is_sequence && ED4_ROOT->aw_root->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->read_int()!=ED4_RM_NONE;
        if (info->mode==AD_NOWRITE_IF_COMPRESSED) {
            if (is_remapped_sequence) {
                info->mode = AD_NOWRITE;
            }
            else {
                info->mode = AD_INSERT;
            }
        }

        bool write_fault = 0;

        err = command(info->event.keymodifier,
                      info->event.keycode,
                      map_key,
                      info->rightward,
                      info->mode,
                      (int)(info->string!=0),
                      info->out_seq_position,
                      info->refresh_needed,
                      info->cursor_jump,
                      info->cannot_handle,
                      write_fault,
                      info->gb_data,
                      info->is_sequence);

        e4_assert(!(err && info->cannot_handle));

        if (write_fault) {
            e4_assert(info->mode==AD_NOWRITE);
            aw_message(is_remapped_sequence
                       ? "Check 'Show all gaps' when editing remarks"
                       : "This terminal is write-protected!");
        }
    }

    if (!err) {
        if (info->gb_data) {
            if (info->refresh_needed) {
                e4_assert(info->working_terminal->get_species_pointer() == info->gb_data);

                int old_seq_len_int;
                old_seq     = info->working_terminal->resolve_pointer_to_string_copy(&old_seq_len_int);
                old_seq_len = old_seq_len_int;

                err = info->working_terminal->write_sequence(seq, seq_len);
                if (err) {
                    info->out_seq_position = remap->screen_to_sequence(info->char_position);    // correct cursor_pos if protection error occurred
                }
            }
            freenull(seq);
        }
    }

    if (!info->rightward) {
        info->char_position    = remap->sequence_to_screen_PLAIN(info->out_seq_position);
        e4_assert(info->char_position >= 0);
        info->char_position--;
        info->out_seq_position = remap->screen_to_sequence(info->char_position);
    }

    return err;
}

void ED4_Edit_String::finish_edit()
{
    nrepeat_is_already_set = 0;
    if (nrepeat_zero_requested) {
        nrepeat_zero_requested = 0;
        nrepeat = 0;
    }
}

void ED4_Edit_String::init_edit() {
    freenull(old_seq);
    seq_len = 0;
    seq = 0;
    gbd = 0;
    old_seq_len = 0;
    remap = ED4_ROOT->root_group_man->remap();
}

