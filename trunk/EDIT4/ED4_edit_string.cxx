#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_keysym.hxx>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <AW_helix.hxx>

#include <awtc_fast_aligner.hxx>
#include <awt_map_key.hxx>

#include "ed4_defs.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_class.hxx"
#include "ed4_awars.hxx"

#define SAFE_EDITING        // undef this to remove sequence test code

int ED4_Edit_String::nrepeat = 0;           // # of times command should be repeated
int ED4_Edit_String::nrepeat_zero_requested = 0;    // nrepeat should be set to zero
int ED4_Edit_String::nrepeat_is_already_set = 0;    // nrepeat was zero (and was set to 1)


unsigned char ED4_is_align_character[255];
void ED4_init_is_align_character(GB_CSTR gap_chars)
{
    memset(ED4_is_align_character, 0, 256);
    for (int p = 0; gap_chars[p]; ++p) {
        ED4_is_align_character[(unsigned char)(gap_chars[p])] = 1;
    }
}

ED4_ERROR * ED4_Edit_String::insert(char *text,long position, int direction, int removeAtNextGap) {
    long i;
    int text_len = strlen(text);
    if (text_len ==0) return 0;

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

    if (direction>=0){
        if ( text_len + position >= seq_len) {
            return GBS_global_string("You cannot insert that many characters after cursor!");
        }

        for (i = rest_len-text_len; i<rest_len; i++) {
            if (!ADPP_IS_ALIGN_CHARACTER(seq[position+i])) {
                goto no_gaps;
            }
        }

        for (i= position+rest_len-text_len-1; i>=position; i--) {
            seq[i+text_len] = seq[i];
        }

        for (i = 0 ;i<text_len;i++) {
            seq[position+i] = text[i];
        }
    }
    else {
        if ( position - text_len < 0) {
            return GBS_global_string("You cannot insert that many characters before cursor!");
        }

        for (i = 0; i<text_len; i++) {
            if (i<0 || !ADPP_IS_ALIGN_CHARACTER(seq[i])) {
                goto no_gaps;
            }
        }
        for (i= 0; i<position-text_len;i++) {
            seq[i] = seq[i+text_len];
        }

        for (i = 0; i<text_len;i++) {
            seq[position-i-1] = text[i];
        }
    }

    return 0;

 no_gaps:
    return GBS_global_string("There are no/not enough gaps at %s of sequence => can't insert characters%s",
                             direction >= 0 ? "end" : "start",
                             direction >= 0 ? "\nMaybe your sequences are not formatted?" : "");
}

ED4_ERROR *ED4_Edit_String::remove(int len, long position, int direction, int insertAtNextGap) {

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

/*
  ED4_ERROR *ED4_Edit_String::remove(int len, long position, int direction, int insertAtNextGap) {
  long new_len;
  long i;

  if (direction<0) {
  position -= len;
  }
  if ((position >= seq_len) || (len == 0) || (seq_len - len < 0)) {
  return GBS_global_string("Delete outside sequence!");
  }

  if ((len + position) >= seq_len) {
  len = (int)(seq_len-position);
  }

  int rest_len = direction>=0 ? seq_len-position : position;

  if (insertAtNextGap) {
  int nextGap = get_next_gap(position+len*direction, direction);

  if (nextGap!=-1) {
  rest_len = (nextGap-position)*direction;
  }
  }

  new_len = rest_len-len;
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

*/

ED4_ERROR *ED4_Edit_String::replace(char *text,long position, int direction) {
    int text_len = strlen(text);
    int i;
    if (direction>=0){

        if ((position + text_len > seq_len) || (position > seq_len)) {
            return GBS_global_string("Replace after end of sequence !");
        }
        for ( i = 0; i < text_len ; i ++) {
            seq[i+position] = text[i];
        }
    }else{
        if ((position - text_len < 0 ) || (position > seq_len)) {
            return GBS_global_string("Replace before start of sequence !");
        }
        for ( i = 0; i < text_len ; i ++) {
            seq[position - i - 1] = text[i];
        }
    }
    return 0;
}

ED4_ERROR *ED4_Edit_String::swap_gaps(long position, char ch){
    long i;
    for (i = position; i < seq_len; i++) {
        if ( !ADPP_IS_ALIGN_CHARACTER(seq[i])) break;
        seq[i] = ch;
    }
    for (i = position; i >= 0; i--) {
        if ( !ADPP_IS_ALIGN_CHARACTER(seq[i])) break;
        seq[i] = ch;
    }
    return 0;
}


// ED4_ERROR *ED4_Edit_String::check_base(char chr,long position, int direction)
// {
//     if (direction<0) position--;

//     printf("chr=%c seq[%i]=%c\n", chr, position, seq[position]);

//     if (position <0 && position >= seq_len){
//  return GBS_global_string("End of sequence reached");
//     }
//     if (seq[position] != chr){
//  return GBS_global_string("Base does not match");
//     }
//     return 0;
// }

ED4_ERROR *ED4_Edit_String::moveBase(long source_position, long dest_position, unsigned char gap_to_use)
{
    e4_assert(source_position!=dest_position);
#ifdef SAFE_EDITING
    if (!legal_seqpos(dest_position) || !legal_seqpos(source_position)) {
        return "Position out of sequence";
    }
#endif

    int direction = dest_position<source_position ? -1 : 1;
    int base = seq[source_position];
    e4_assert(!ADPP_IS_ALIGN_CHARACTER(base));
    seq[source_position] = gap_to_use;

    long p = source_position+direction;
    while (p!=dest_position) {
#ifdef SAFE_EDITING
        if (!ADPP_IS_ALIGN_CHARACTER(seq[p])) {
            e4_assert(0);
            return "Internal error: Tried to change base order in moveBase()";
        }
#endif
        e4_assert(ADPP_IS_ALIGN_CHARACTER(seq[p]));
        seq[p] = gap_to_use;
        p += direction;
    }

    seq[dest_position] = base;
    return 0;
}

ED4_ERROR *ED4_Edit_String::shiftBases(long source_pos, long last_source, long dest_pos, int direction, long *last_dest, unsigned char gap_to_use)
    // shifts bases from source_pos-last_source to dest_pos
    // last_dest is set to the position after the last dest_pos (direction<0 ? pos. right of bases : pos. left of bases)
{
    ED4_ERROR *err = 0;

    //    printf("shiftBases(%li,%li,%li,%i,%c)\n", source_pos, last_source, dest_pos, direction, gap_to_use);

    if (direction<0) {
        e4_assert(dest_pos<source_pos);
        e4_assert(source_pos<=last_source);
        while(1) {
            err = moveBase(source_pos,dest_pos,gap_to_use);
            if (err || source_pos>=last_source) break;
            source_pos++;
            dest_pos++;
            while (!ADPP_IS_ALIGN_CHARACTER(seq[dest_pos])) {
                dest_pos++;
            }
        }
    }
    else {
        e4_assert(source_pos<dest_pos);
        e4_assert(last_source<=source_pos);
        while(1) {
            err = moveBase(source_pos,dest_pos,gap_to_use);
            if (err || source_pos<=last_source) break;
            source_pos--;
            dest_pos--;
            while (!ADPP_IS_ALIGN_CHARACTER(seq[dest_pos])) {
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
    for (pos = position; pos>=0 && pos < seq_len; pos += direction){
        if (!ADPP_IS_ALIGN_CHARACTER(seq[pos])) break;
    }

    return pos<0 || pos>=seq_len ? -1 : pos;
}
long ED4_Edit_String::get_next_gap(long position, int direction) {
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos >= 0 && pos < seq_len; pos += direction){
        if (ADPP_IS_ALIGN_CHARACTER(seq[pos])) break;
    }

    return pos<0 || pos>=seq_len ? -1 : pos;
}
long ED4_Edit_String::get_next_visible_base(long position, int direction)
{
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos>=0 && pos < seq_len; pos += direction){
        if (!ADPP_IS_ALIGN_CHARACTER(seq[pos]) && remap->is_visible(pos)) {
            break;
        }
    }

    return pos<0 || pos>=seq_len ? -1 : pos;
}
long ED4_Edit_String::get_next_visible_gap(long position, int direction) {
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos >= 0 && pos < seq_len; pos += direction){
        if (ADPP_IS_ALIGN_CHARACTER(seq[pos]) && remap->is_visible(pos)) {
            break;
        }
    }

    return pos<0 || pos>=seq_len ? -1 : pos;
}
long ED4_Edit_String::get_next_visible_pos(long position, int direction)
{
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos >= 0 && pos < seq_len; pos += direction){
        if (remap->is_visible(pos)) {
            break;
        }
    }
    return pos<0 || pos>=seq_len ? -1 : pos;
}

extern AWTC_faligner_cd faligner_client_data;

unsigned char ED4_Edit_String::get_gap_type(long pos, int direction)
{
    pos += direction;
    if (!legal_seqpos(pos)) return '.';
    char gap = seq[pos];
    if (ADPP_IS_ALIGN_CHARACTER(gap)) return gap;
    return '-';
}

ED4_ERROR *ED4_Edit_String::command( AW_key_mod keymod, AW_key_code keycode, char key, int direction, ED4_EDITMODI mode, bool is_consensus,
                                     long &seq_pos, bool &changed_flag, bool &center_cursor, bool &cannot_handle, bool &write_fault, GBDATA* gb_data, bool is_sequence)
{
    changed_flag = 0;
    write_fault = 0;
    if (keymod+keycode+key==0) return 0;

    e4_assert(nrepeat>0);

    long old_seq_pos = seq_pos;
    int screen_len = remap->sequence_to_screen_clipped(seq_len);
    int cursorpos = remap->sequence_to_screen_clipped(seq_pos);

    char str[2];
    str[0] = key;
    str[1] = 0;

    direction = direction>0 ? 1 : -1;

    if ((cursorpos > screen_len) || (cursorpos < 0 )) {
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

    ED4_ERROR *ad_err = 0;
    long h;
    GB_BOOL reinterpret_key = 1;

    while (reinterpret_key) {
        reinterpret_key = 0;

        switch (keycode) {
            case AW_KEY_HOME:
                {
                    int new_seq_pos = get_next_visible_base(0,1);
                    if (new_seq_pos>=0) seq_pos = new_seq_pos==seq_pos ? 0 : new_seq_pos;
                    else seq_pos = 0;
                    break;
                }
            case AW_KEY_END:
                {
                    int new_seq_pos = get_next_visible_base(seq_len, -1);
                    if (new_seq_pos>=0) {
                        new_seq_pos++;
                        seq_pos = new_seq_pos==seq_pos ? seq_len : new_seq_pos;
                    }
                    else {
                        seq_pos = seq_len; 
                    }
                    break;
                }
            case AW_KEY_LEFT:
            case AW_KEY_RIGHT:
                {
                    direction = keycode==AW_KEY_RIGHT ? 1 : -1;

                    // no key modifier -> just move cursor

                    int n = nrepeat;

                    if (keymod == 0) {
                        while (n--) {
                            do {
                                seq_pos += direction;
                            }
                            while (legal_curpos(seq_pos) && !remap->is_visible(seq_pos));
                        }
                        break;
                    }

                    if (mode==AD_NOWRITE) { write_fault = 1; break; }

                    int jump_or_fetch = 0;  // 1=jump selected 2=fetch selected (for repeat only)
                    int push_or_pull = 0;   // 1=push selected 2=pull selected (for repeat only)

                    // ------------------
                    // loop over nrepeat:
                    // ------------------

                    while (!ad_err && n-->0 && legal_curpos(seq_pos)) {
                        cursorpos = remap->sequence_to_screen_clipped(seq_pos);

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

                        if (keymod&AW_KEY_CONTROL) {
                            if (adjacent_scr_pos>=0) {
                                long pos = adjacent_seq_pos;

                                if (ED4_ROOT->aw_root->awar(ED4_AWAR_FAST_CURSOR_JUMP)->read_int()) { // should cursor jump over next group?
                                    if (ADPP_IS_ALIGN_CHARACTER(seq[pos])) {
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
                                    if (ADPP_IS_ALIGN_CHARACTER(seq[pos]))  { seq_pos = get_next_visible_base(pos, direction); }
                                    else                    { seq_pos = get_next_visible_gap(pos, direction); }

                                    if (direction<0)    { seq_pos = seq_pos==-1 ? 0       : seq_pos+1; }
                                    else        { seq_pos = seq_pos==-1 ? seq_len : seq_pos;   }
                                }
                            }
                            continue;
                        }

                        // ALT/META+Cursor = jump & fetch

                        if (keymod & (AW_KEY_META|AW_KEY_ALT)) {
                            if (is_consensus) { cannot_handle = 1; return 0; }

                            if (ADPP_IS_ALIGN_CHARACTER(seq[adjacent_seq_pos])) { // there's a _gap_ next to the cursor -> let's fetch
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

                                    if (ADPP_IS_ALIGN_CHARACTER(seq[next_gap])) {
                                        int dest_pos = get_next_base(next_gap, -direction);

                                        if (dest_pos<0) {
                                            dest_pos = direction>0 ? 0 : seq_len-1;
                                        }
                                        else {
                                            dest_pos += direction;
                                        }

                                        if (ADPP_IS_ALIGN_CHARACTER(seq[dest_pos])) {
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
#if 0
                                    if (gap_behind<0 || gap_behind>seq_len) {
                                        ad_err = GBS_global_string("Cannot jump out of sequence!");
                                    }
                                    else {
                                        if (ADPP_IS_ALIGN_CHARACTER(seq[gap_behind])) { // jump only one base
                                            long dest_pos = get_next_base(gap_behind, direction); // destination pos of jumped base

                                            if (dest_pos<0) {
                                                dest_pos = direction<0 ? 0 : seq_len-1;
                                                if (!ADPP_IS_ALIGN_CHARACTER(seq[dest_pos])) {
                                                    dest_pos = -1;
                                                }
                                            }
                                            else {
                                                dest_pos -= direction;
                                            }

                                            if (dest_pos>=0) {
                                                ad_err = moveBase(adjacent_seq_pos, dest_pos, get_gap_type(adjacent_seq_pos, -direction));

                                                if (!ad_err) { // move cursor into opposite direction (to jump next base)
                                                    seq_pos = get_next_base(adjacent_seq_pos, -direction);
                                                    if (seq_pos==-1) {
                                                        seq_pos = direction>0 ? 0 : seq_len; // if no next base -> cursor to beg or end of seq
                                                    }
                                                    else {
                                                        seq_pos += (direction<0);
                                                    }
                                                }
                                            }
                                            else {
                                                n = 0;
                                            }
                                        }
                                        else { // jump two or more bases
                                            long next_gap = get_next_gap(adjacent_seq_pos, direction);

                                            if (next_gap>=0) {
                                                long dest_pos = get_next_base(next_gap, direction);
                                                long source_pos = get_next_base(next_gap, -direction);
                                                long last_source = direction>0 ? seq_pos : seq_pos-1;

                                                dest_pos = dest_pos==-1 ? (direction<0 ? 0 : seq_len-1) : dest_pos-direction;
                                                e4_assert(source_pos>=0);

                                                ad_err = shiftBases(source_pos, last_source, dest_pos, direction, &dest_pos,
                                                                    get_gap_type(adjacent_seq_pos, -direction));

                                                if (!ad_err) {
                                                    seq_pos = dest_pos + (direction<0);
                                                }
                                            }
                                            else {
                                                n = 0;
                                            }
                                        }
                                    }
#endif
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

                        if (ADPP_IS_ALIGN_CHARACTER(seq[real_adjacent_seq_pos])) { // pull
                            long dest_pos = real_adjacent_seq_pos;
                            long source_pos = real_adjacent_seq_pos-direction;

                            if (!ADPP_IS_ALIGN_CHARACTER(seq[source_pos]) && push_or_pull!=1) {
                                push_or_pull = 2;

                                long next_gap = get_next_gap(source_pos, -direction);
                                long last_source = next_gap>=0 ? next_gap : (direction>0 ? 0 : seq_len-1);

                                if (ADPP_IS_ALIGN_CHARACTER(seq[last_source])) {
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
                    } // end-while
                    break;
                }
            case AW_KEY_DELETE:
            case AW_KEY_BACKSPACE:
                {
                    if (is_consensus) { cannot_handle = 1; return 0; };
                    if (keymod) { return 0; }

                    if (mode==AD_NOWRITE) { write_fault = 1; break; }

                    h = seq_pos;

                    if (direction>0) {
                        seq_pos -= nrepeat;
                    }else{
                        seq_pos += nrepeat;
                    }
                    if (seq_pos <0 || seq_pos >= seq_len) {
                        seq_pos = h;
                        break;
                    }

                    h = seq_pos;

                    switch (mode) {
                        case AD_ALIGN:
                            {
                                int len;
                                int offset;

                                ad_err = 0;
                                if (direction>=0)   offset = 0;
                                else        offset = -nrepeat;

                                for (len = nrepeat-1; len>=0; len--) {
                                    if (!ADPP_IS_ALIGN_CHARACTER(seq[h+offset+len])){
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

                    //      nrepeat_zero_requested = 1;
                    break;
                }
            case AW_KEY_ASCII: {

                //      Tastaturbelegung:
                //
                //      - CTRL-A    Align                               ok
                //      - CTRL-E    Edit/Align umschalten               ok
                //      - CTRL-I    Insert/Replace umschalten           ok
                //      - CTRL-J    Jump opposite helix position        ok
                //      - CTRL-L    Refresh                             ok
                //      - CTRL-R    set aligner reference species       ok
                //      - CTRL-S    Repeat last search                  ok
                //      - CTRL-U    Undo                                @@@ absturz!!!


                if (key >0 && key<=26) { // CTRL-Keys
                    switch (key+'A'-1) {
                        case 'R':  { // CTRL-R = set aligner refernce species
                            if (is_consensus) { cannot_handle = 1; return 0; };

                            AWTC_set_reference_species_name(0, (AW_CL)ED4_ROOT->aw_root);
                            break;
                        }
                        case 'A': { // CTRL-A = Start Fast-Aligner
                            AW_window *aw_tmp = ED4_ROOT->temp_aww;
                            if (is_consensus) { cannot_handle = 1; return 0; };
                            if (mode==AD_NOWRITE) { write_fault = 1; return 0; }

                            int old_refresh                 = faligner_client_data.do_refresh;
                            faligner_client_data.do_refresh = 0;

                            ED4_init_faligner_data(&faligner_client_data);

                            AWTC_awar_set_current_sequence(ED4_ROOT->aw_root, ED4_ROOT->db);
                            AW_clock_cursor(ED4_ROOT->aw_root);
                            AWTC_start_faligning(aw_tmp, (AW_CL)&faligner_client_data);
                            AW_normal_cursor(ED4_ROOT->aw_root);
                            faligner_client_data.do_refresh = old_refresh;

                            int basesLeftOf = 0;
                            int pos;

                            for (pos=0; pos<seq_pos && pos<seq_len; pos++) {    // count # of bases left of cursorpos
                                if (!ADPP_IS_ALIGN_CHARACTER(seq[pos])) {
                                    basesLeftOf++;
                                }
                            }

                            char *aligned_seq = GB_read_string(gb_data);        // read new sequence
                            ad_err = GB_write_string(gb_data, seq);         // restore old sequence
                            free(seq);
                            seq = aligned_seq;                  // set new sequence
                            changed_flag=1;                     // and mark changed

                            for (pos=0; basesLeftOf>=0 && pos<seq_len; pos++) {
                                if (!ADPP_IS_ALIGN_CHARACTER(seq[pos])) {
                                    basesLeftOf--;
                                }
                            }

                            seq_pos = pos ? pos-1 : pos;
                            break;
                        }
                        case 'E': { // CTRL-E = Toggle Edit/Align-Mode
                            AW_awar *edit_mode = ED4_ROOT->aw_root->awar(AWAR_EDIT_MODE);

                            edit_mode->write_int(edit_mode->read_int()==0);
                            break;
                        }
                        case 'I': { // CTRL-I = Toggle Insert/Replace-Mode
                            AW_awar *insert_mode = ED4_ROOT->aw_root->awar(AWAR_INSERT_MODE);

                            insert_mode->write_int(insert_mode->read_int()==0);
                            break;
                        }
                        case 'J': { // CTRL-J = Jump to opposite helix position
                            AW_helix *helix = ED4_ROOT->helix;

                            if (size_t(seq_pos)<helix->size && helix->entries[seq_pos].pair_type) {
                                int pairing_pos = helix->entries[seq_pos].pair_pos;

                                e4_assert(helix->entries[pairing_pos].pair_pos==seq_pos);
                                seq_pos = pairing_pos;
                                center_cursor = true;
                            }
                            else {
                                ad_err = GBS_global_string("No helix position");
                            }
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
                        case 'L':  { // CTRL-L = Refresh
                            ED4_refresh_window(0, 0, 0);
                            center_cursor = true;
                            break;
                        }
                        case 'S': { // CTRL-S = Repeat last search
                            ad_err = ED4_repeat_last_search();
                            seq_pos = ED4_ROOT->temp_ed4w->cursor.get_sequence_pos();
                            center_cursor = true;
                            break;
                        }
                        case 'U': {
                            // ad_err = GB_undo(gb_main, GB_UNDO_UNDO); // @@@ stuerzt ab - wahrscheinlich weil Transaktion offen ist
                            break;
                        }
                        case 'O': { //  for ALT-left
                            keycode = AW_KEY_LEFT;
                            keymod = AW_KEY_META;
                            reinterpret_key = 1;
                            break;
                        }
                        case 'P': { // for ALT-right
                            keycode = AW_KEY_RIGHT;
                            keymod = AW_KEY_META;
                            reinterpret_key = 1;
                            break;
                        }
                    }
                }
                else { // normal characters
                    if (is_consensus) { cannot_handle = 1; return 0; }
                    if (mode==AD_NOWRITE) { write_fault = 1; break; }
                    //      if (is_compressed_but_no_sequence(is_sequence)) { break; }

                    if (key == ' ') { // insert gap
                        if (is_sequence) {
                            long left;
                            int l,r;
                            left = cursorpos-1; if (left<0) left = 0;
                            l = seq[left]; r = seq[cursorpos];
                            key = '-';
                            if (ADPP_IS_ALIGN_CHARACTER(l)) key = l;
                            if (ADPP_IS_ALIGN_CHARACTER(r)) key = r;
                        }
                        str[0] = key;
                    }

                    switch (mode) {
                        case AD_ALIGN: {
                            if (ADPP_IS_ALIGN_CHARACTER(key)) {
                                long left;
                                int l,r;

                                left = seq_pos-1;
                                if (left<0) left = 0;

                                l = seq[left];
                                r = seq[seq_pos];

                                if (ADPP_IS_ALIGN_CHARACTER(l) && l!=key)       ad_err = swap_gaps(left,key);
                                else if (ADPP_IS_ALIGN_CHARACTER(r) && r!=key )     ad_err = swap_gaps(seq_pos,key);
                                else if (!ad_err){
                                    char *nstr = (char *)GB_calloc(1,nrepeat+1);
                                    int i;

                                    for (i = 0; i< nrepeat; i++) nstr[i] = key;
                                    ad_err = insert(nstr, seq_pos, direction, 0);
                                    if (!ad_err) seq_pos = get_next_visible_pos(seq_pos+(direction>=0?nrepeat:0), direction);
                                    delete nstr;
                                }
                                changed_flag = 1;
                            }
                            else {// check typed bases against sequence
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
                                    int left_pos = get_next_visible_pos(seq_pos,-1);

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
                                    int left_pos = get_next_visible_pos(seq_pos,-1);

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
    memset((char *)this,0,sizeof(*this));
    remap = ED4_ROOT->root_group_man->remap();
}

ED4_Edit_String::~ED4_Edit_String()
{
    free(old_seq);
}

void ED4_Edit_String::edit(ED4_work_info *info)
{
    e4_assert(info->working_terminal != 0);

    info->out_seq_position = remap->screen_to_sequence(info->char_position);
    info->refresh_needed   = false;
    info->center_cursor    = false;
    info->cannot_handle    = false;
    old_seq                = 0;

    if (info->string){
        seq = info->string;
        seq_len = strlen(seq);
    }
    else {
        e4_assert(info->gb_data);
        int seq_len_int;
        seq = info->working_terminal->resolve_pointer_to_string_copy(&seq_len_int);
        seq_len = seq_len_int;

        //         seq     = GB_read_string(info->gb_data);
        //         seq_len = GB_read_string_count(info->gb_data);
    }

    int map_key = ED4_ROOT->edk->map_key(info->event.character);
    ED4_ERROR *err = 0;

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
                      info->direction,
                      info->mode,
                      (int)(info->string!=0),
                      info->out_seq_position,
                      info->refresh_needed,
                      info->center_cursor,
                      info->cannot_handle,
                      write_fault,
                      info->gb_data,
                      info->is_sequence);

        if (write_fault) {
            e4_assert(info->mode==AD_NOWRITE);

            if (is_remapped_sequence) {
                int check = aw_message("Please check 'Preferences/Options/OSC/Show all gaps' in order to edit remarks", "Ok,Check it for me");
                if (check) {
                    ED4_ROOT->aw_root->awar(ED4_AWAR_COMPRESS_SEQUENCE_TYPE)->write_int(ED4_RM_NONE);
                }
            }
            else {
                aw_message("This terminal is write-protected!");
            }
        }
    }

    if (!err){
        if (info->gb_data){
            if (info->refresh_needed){
                e4_assert(info->working_terminal->get_species_pointer() == info->gb_data);

                int old_seq_len_int;
                old_seq     = info->working_terminal->resolve_pointer_to_string_copy(&old_seq_len_int);
                old_seq_len = old_seq_len_int;

                err = info->working_terminal->write_sequence(seq, seq_len);
                if (err) {
                    info->out_seq_position = remap->screen_to_sequence(info->char_position);    // correct cursor_pos if protection error occurred
                }
            }
            free(seq);
            seq = 0;
        }
    }
    info->error = (char *)err;
}

void ED4_Edit_String::finish_edit()
{
    nrepeat_is_already_set = 0;
    if (nrepeat_zero_requested){
        nrepeat_zero_requested = 0;
        nrepeat = 0;
    }
}

void ED4_Edit_String::init_edit()
{
    delete old_seq; old_seq = 0;
    seq_len = 0;
    seq = 0;
    gbd = 0;
    old_seq_len = 0;
    remap = ED4_ROOT->root_group_man->remap();
}

