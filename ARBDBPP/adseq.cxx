/****************
 sequence.cxx
enthaelt code fuer      AD_CONT
                        AD_SEQ
                        AD_STAT
*******/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <memory.h>

#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_keysym.hxx>

#include "arbdb++.hxx"


/*********************
AD_STAT
**********************/

AD_STAT::AD_STAT()
{
    c_1 = '1';
    c_0 = '0';
    ad_cont = 0;
    last = 0;
    marktype = ad_none;
    gb_mark = 0;
    GB_INT_mark = 0;
    gb_char_mark = 0;
    GB_FLOAT_mark = 0;
    updated = 0;
    nmark = 0;
    markdata = 0;
    markdatafloat = 0;
    markdataint = 0;
}

AD_STAT::~AD_STAT()
// gibt speicherplatz fuer markierungen frei frei
{
    if (inited_object == 1)
        new AD_ERR("AD_STAT: no exit before ~");
}


AD_ERR * AD_STAT::init(AD_CONT * adptr)
//
{
    ad_cont = adptr;
    if (ad_cont->gb_ali == NULL)
        return new AD_ERR("AD_STAT: init without propper inited container",CORE);
    gb_mark = ad_cont->gb_ali;
    inited_object = 1;
    return 0;
}




AD_ERR * AD_STAT::release() {
    if (GB_INT_mark != 0) {
        GB_remove_callback(GB_INT_mark,GB_CB_CHANGED,(GB_CB)AD_STAT_updatecall,(int *)this);
        if (ad_cont->get_cach_flag() == MINCACH) {
            GB_release(GB_INT_mark);
        }
        if (markdataint != 0) free ((char *)markdataint);
        nmark = 0;
        GB_INT_mark = 0;
    }
    if (gb_char_mark != 0) {
        GB_remove_callback(gb_char_mark,GB_CB_CHANGED,(GB_CB)AD_STAT_updatecall,(int *)this);
        if (markdata  != 0)free ((char *)markdata);
        nmark = 0;
        gb_char_mark = 0;
    }
    if (GB_FLOAT_mark != 0) {
        GB_remove_callback(GB_FLOAT_mark,GB_CB_CHANGED,(GB_CB)AD_STAT_updatecall,(int *)this);
        if (markdatafloat != 0) free ((char *)markdatafloat);
        nmark = 0;
        GB_FLOAT_mark = 0;
    }
    if (marktype) marktype = ad_none;
    if (markkey) free ((char *)markkey);
    return 0;
}

AD_ERR * AD_STAT::exit()
{
    if (inited_object == 0)
        return new      AD_ERR("AD_STAT: exit() without init()");
    release();
    gb_mark = 0;
    gb_markdata = 0;
    inited_object = 0;
    return 0;
}

AD_ERR * AD_STAT::initpntr() {
    if (gb_markdata == 0) {
        release();
        last = 1;
        gb_char_mark    =  0;
        GB_FLOAT_mark = 0;
        GB_INT_mark = 0;
        return 0;
    }
    release();
    marktype = (AD_TYPES)GB_read_type(gb_markdata );
    markkey = GB_read_key(gb_markdata);
    switch (marktype) {
        case ad_ints:   /*if (GB_INT_mark) free ((char *)GB_INT_mark);
                          if (GB_FLOAT_mark) free ((char *)GB_FLOAT_mark);
                          if (gb_char_mark) free ((char *)gb_char_mark);*/
            GB_INT_mark =  gb_markdata;
            GB_FLOAT_mark = 0;
            gb_char_mark = 0;
            nmark = GB_read_ints_count(gb_markdata);
            markdataint = GB_read_ints(gb_markdata);
            GB_add_callback(gb_char_mark, GB_CB_CHANGED,
                            (GB_CB) AD_STAT_updatecall, (int *) this);
            last = 0;
            break;
        case ad_floats: GB_FLOAT_mark   = gb_markdata;
            GB_INT_mark = 0;
            gb_char_mark = 0;

            nmark = GB_read_floats_count(gb_markdata);
            markdatafloat = GB_read_floats(gb_markdata);
            GB_add_callback(GB_FLOAT_mark, GB_CB_CHANGED,
                            (GB_CB) AD_STAT_updatecall, (int *) this);
            last = 0;
            break;
        case ad_bits:   gb_char_mark    =  gb_markdata;
            GB_FLOAT_mark = 0;
            GB_INT_mark = 0;
            nmark = GB_read_bits_count(gb_markdata);
            markdata = GB_read_bits(gb_markdata,c_0,c_1);
            GB_add_callback(GB_INT_mark, GB_CB_CHANGED,
                            (GB_CB) AD_STAT_updatecall, (int *) this);
            last = 0;
            break;
        default :       gb_char_mark    =  0;
            GB_FLOAT_mark = 0;
            GB_INT_mark = 0;
            last = 1;

    }
    return 0;
}

AD_ERR * AD_STAT::first() {
    char *key = 0;
    if (gb_mark != 0) {
        release();
        gb_markdata = GB_find(gb_mark, 0, 0, down_level);
        if (gb_markdata != 0) {
            key = GB_read_key(gb_markdata);
            if (strcmp(key, "data") == 0) {
                gb_markdata = GB_find(gb_markdata, 0, 0, this_level | search_next);
                if (gb_markdata != 0) {
                    key = GB_read_key(gb_markdata);
                }
            }
        }
        if (gb_markdata == 0) { // keine markierung vorhanden
            last = 1;
            return 0;
        }
        markkey = strdup(key);
        initpntr();
        return 0;
    }
    return new AD_ERR("AD_species first: NO AD_MAIN\n");
}


AD_ERR * AD_STAT::first(AD_TYPES typus) {
    AD_TYPES adtype;
    if (gb_mark != 0) {
        release();
        gb_markdata = GB_find(gb_mark, 0, 0, down_level);

        while (last != 0) {
            gb_markdata = GB_find(gb_mark, 0, 0, down_level);
            if (gb_markdata != 0) {
                adtype = (AD_TYPES) GB_read_type(gb_markdata);
                if (adtype == typus) {
                    marktype = adtype;
                    initpntr();
                    return 0;
                }
                else {
                    gb_markdata = GB_find(gb_markdata,0,0,this_level | search_next );
                }
            }
        }
        initpntr();
    }
    return 0;
}

AD_ERR * AD_STAT::next() {
    if (gb_markdata == 0 || last ==  1) {
        return new AD_ERR("AD_STAT::next() not possible, no first or last!");
    }
    release();
    gb_markdata = GB_find(gb_markdata,0,0,this_level | search_next );
    initpntr();
    return 0;
}


AD_ERR * AD_STAT::next(AD_TYPES typus) {
    AD_TYPES type;
    if (gb_markdata == 0 || last == 1) {
        return new AD_ERR("AD_STAT::next() not possible, no first or last!");
    }
    release();
    while (gb_markdata != 0) {
        gb_markdata = GB_find(gb_markdata,0,0,this_level | search_next );
        if (gb_markdata != 0) {
            type = (AD_TYPES)GB_read_type(gb_markdata);
            if (type == typus) {
                initpntr();
                return 0;
            }
        }
    }
    last = 1;
    marktype = typus;
    return 0;
}



int AD_STAT_updatecall(GBDATA *gb_char_mark,AD_STAT *ad_mark)
// behandelt ein update der Markierung in der Datenbank
// problem -> wenn editiert wird ni
//
//
{
    if ((gb_char_mark != ad_mark->GB_INT_mark) &&(gb_char_mark != ad_mark->gb_char_mark) && (gb_char_mark != ad_mark->GB_FLOAT_mark)) {
        new AD_ERR("AD_STAT: WRONG update callback",CORE);
    }
    if (gb_char_mark == ad_mark->gb_char_mark) {
        ad_mark->gb_markdata = gb_char_mark;
        ad_mark->initpntr();

    }
    if (gb_char_mark == ad_mark->GB_FLOAT_mark) {
        ad_mark->gb_markdata = gb_char_mark;
        ad_mark->initpntr();
    }
    if (gb_char_mark == ad_mark->GB_INT_mark) {
        ad_mark->gb_markdata = gb_char_mark;
        ad_mark->initpntr();
    }

    ad_mark->updated = 1;       // flag setzen
    return 0;
}


char * AD_STAT::getbits() {
    if (marktype != ad_bits) {
        new AD_ERR("AD_STAT::getbits()  Wrong Type !");
        return 0;
    }
    return markdata;
}

float * AD_STAT::getfloat() {
    if (marktype != ad_floats) {
        new AD_ERR("AD_STAT::getfloat()  Wrong Type !");
        return 0;
    }
    return markdatafloat;
}

GB_UINT4 * AD_STAT::getint() {
    if (marktype != ad_ints) {
        new AD_ERR("AD_STAT::getint()  Wrong Type !");
        return 0;
    }
    return markdataint;
}


AD_ERR *AD_STAT::put() {
    char *error;
    if (gb_char_mark == 0 && GB_FLOAT_mark == 0 && GB_INT_mark == 0)
        return new AD_ERR("AD_SEQ::write not possible!");
    if (GB_FLOAT_mark != 0) {
        error = (char *)GB_write_floats(GB_FLOAT_mark,markdatafloat,nmark);
        if (error != 0) {
            return new AD_ERR(error);
        }
    }
    if  (gb_char_mark != 0) {
        char c_0_buf[] = "x";
        c_0_buf[0]     = c_0;
        error          = (char *)GB_write_bits(gb_char_mark,markdata,nmark,c_0_buf);
        if (error != 0) {
            return new AD_ERR(error);
        }
    }
    if  (GB_INT_mark != 0) {
        error = (char *)GB_write_ints(GB_INT_mark,markdataint,nmark);
        if (error != 0) {
            return new AD_ERR(error);
        }
    }
    return 0;
}

AD_ERR *AD_STAT::put(char *markings, int len) {
    // hier gehoert ein test ob in markings zulaessige
    // markierungsdaten stehen ... not implemented
    if (marktype != ad_bits) {
        return new AD_ERR("*AD_STAT::put(char * ... WRONG TYPE",CORE);
    }
    markdata = markings;
    nmark = len;
    put();
    return 0;
}

int AD_STAT::time_stamp(void)
{
    return GB_read_clock(gb_mark);
}


AD_ERR *AD_STAT::put(float *markings, int len) {
    // hier gehoert ein test ob in markings zulaessige
    // markierungsdaten stehen ... not implemented
    if (marktype != ad_floats) {
        return new AD_ERR("*AD_STAT::put(float *... WRONG TYPE",CORE);
    }
    markdatafloat = markings;
    nmark = len;
    put();
    return 0;
}

AD_ERR *AD_STAT::put(GB_UINT4 *markings, int len) {
    if (marktype != ad_ints) {
        return new AD_ERR("*AD_STAT::put(int * ... WRONG TYPE",CORE);
    }
    markdataint = markings;
    nmark = len;
    put();
    return 0;
}

AD_TYPES AD_STAT::type() {
    return marktype;
}




/*********************
AD_SEQ
**********************/
void AD_SEQ_delcall(GBDATA *gb_seq,AD_SEQ *ad_seq)
{
    gb_seq = gb_seq;
    ad_seq->exit();
}


int AD_SEQ_updatecall(GBDATA *gb_seq,AD_SEQ *ad_seq)
// behandelt ein update der sequenz in der Datenbank
// problem -> wenn editiert wird ni
//
//
{
    long new_time_stamp;
    if (gb_seq != ad_seq->gb_seq)
        new AD_ERR("AD_SEQ: WRONG update callback",CORE);
    new_time_stamp = GB_read_clock(gb_seq);
    if (ad_seq->timestamp != new_time_stamp) {
        ad_seq->timestamp = new_time_stamp;
        if (ad_seq->seq && ad_seq->gb_seq) {
            ad_seq->update();
        }
    }
    return 0;
    char *old = (char *)GB_read_old_value();
    if (!old) return 0;
    printf("old:        %40s\n",old);
    char *n = GB_read_char_pntr(gb_seq);
    printf("old:        %40s\n",n);
    return 0;
}

AD_SEQ::AD_SEQ()
{
    memset(this,0,sizeof(AD_SEQ));      // no virtual members !!!
}

AD_SEQ::~AD_SEQ()
// gibt speicherplatz fuer sequenz frei
{
    if (seq){
        new AD_ERR("AD_SEQ: no exit() !!");
        delete seq;
        seq = 0;
    }
}


AD_ERR * AD_SEQ::init(AD_CONT * adptr)
// nur moeglich fals schon eine Sequence existiert
{
    ad_cont = adptr;
    if (ad_cont->gb_ali == NULL)
        return new AD_ERR("AD_SEQ.init() : not existing sequence\n");
    this->update();
    if (gb_seq){
        GB_add_callback(gb_seq, GB_CB_CHANGED, (GB_CB) AD_SEQ_updatecall, (int *) this);
        GB_add_callback(gb_seq, GB_CB_DELETE, (GB_CB) AD_SEQ_delcall, (int *) this);
    }else{
        timestamp = 10000000;
    }

    return 0;
}


AD_ERR * AD_SEQ::exit()
{
    delete seq;
    const char *s = "Sequence Deleted";
    seq_len = strlen(s);
    seq = new char[seq_len+1];
    strcpy(seq, s);

    if (gb_seq) {
        // GB_remove_callback(gb_seq,GB_CB_CHANGED,(GB_CB)AD_SEQ_updatecall,(int *)this);       // already deleted
        // GB_remove_callback(gb_seq,GB_CB_DELETE,(GB_CB)AD_SEQ_delcall,(int *)this);
        gb_seq = 0;
        gbdataptr = gb_seq;
        return 0;
    } else
        return new      AD_ERR("AD_SEQ: exit() without init()");
}


AD_ERR * AD_SEQ::update()
{
    // holt die sequenz nocheinmal aus der Datenbank
    // z.B. nach update_callback notwendig
    long laenge,i;
    const char *sequenz = 0;

    seq_len = ad_cont->ad_ali->len();
    if (!gb_seq){
        gb_seq = GB_find(ad_cont->gb_ali,"data",0,down_level);
        if (gb_seq) nseq_but_filter = 0;
    }
    if (!gb_seq) {
        gb_seq = GB_find(ad_cont->gb_ali,"bits",0,down_level);
        if (gb_seq && GB_read_type(gb_seq) == GB_BITS) nseq_but_filter = 1;
        else    gb_seq = 0;
    }

    if (gb_seq) {
        if (!nseq_but_filter) {
            sequenz = GB_read_char_pntr(gb_seq);
            laenge = GB_read_string_count(gb_seq);
#if 0
            if (laenge>seq_len){
                seq_len = laenge;
                long i;
                for (i= seq_len; i < laenge; i++) {
                    if (sequenz[i] != SEQ_POINT) break;
                }
                if ( i < laenge) {
                    gb_seq = 0;
                    sequenz = "Too long sequence: Please increase alignment length";
                    laenge = strlen(sequenz);
                }else{
                    laenge = seq_len;
                }
            }
#endif
        }else{
            laenge = GB_read_bits_count(gb_seq);
            //                  if (laenge>seq_len){
            //                          seq_len = laenge;
            //                          gb_seq = 0;
            //                          sequenz = "Too long filter: Please increase alignment length";
            //                          laenge = strlen(sequenz);
            //                  }else{
            sequenz = GB_read_bits_pntr(gb_seq,'.','x');
            //                  }
            nseq_but_filter = 1;
        }
    }

    if (!gb_seq) {
        if (!sequenz) sequenz = "No Sequence available.................";
        laenge = strlen(sequenz);
    }
    gbdataptr = gb_seq;
    if (laenge>seq_len) seq_len = laenge;                       /* overlong sequence */
    delete seq;
    seq = (char *)malloc((size_t)seq_len + 1);
    strncpy(seq,sequenz,(int)seq_len);
    for ( i = laenge; i < seq_len; i++) {
        seq[i] = SEQ_POINT;
    }
    seq[seq_len] = '\0';
    timestamp = time_stamp();

    return 0;
}

char * AD_SEQ::get()
{
    return seq;
}

int AD_SEQ::len()
{
    return seq_len;
}

AD_ERR * AD_SEQ::put()
{
    char *error;
    if (gb_seq == 0)
        return new AD_ERR("AD_SEQ::write not possible!");
    if (nseq_but_filter) {
        error = (char *)GB_write_bits(gb_seq,seq,seq_len, ".");
    }else{
        error = (char *)GB_write_string(gb_seq,seq);
    }

    if (!error) {
        timestamp = time_stamp();
        return 0;
    } else {
        this->update();
        return new AD_ERR(error);
    }
}

AD_ERR *AD_SEQ::create(void) {
    return 0;
}


int AD_SEQ::time_stamp(void)
{
    if (!gb_seq) return 0x3fffffff;
    return GB_read_clock(gb_seq);
}


/*** EDIT Functions *******/
// dont call put operations !!!!!!!!!!

AD_ERR * AD_SEQ::insert(char *text,long position, int direction) {
    long i;
    int text_len = strlen(text);
    if (text_len ==0) return 0;

    if ( position + text_len > seq_len) {
        return new AD_ERR("AD_SEQ::insert after end of sequence !");
    }
    if (direction>=0){
        if ( text_len + position >= seq_len) {
            return new AD_ERR("AD_SEQ::You cannot insert that many characters after cursor !");
        }
        for (i = seq_len - text_len; i<seq_len; i++) {
            if (!ADPP_IS_ALIGN_CHARACTER(seq[i])) {
                return new AD_ERR("AD_SEQ::insert: end of alignment reached !");
            }
        }
        for (i= seq_len-text_len-1; i>=position;i--) {
            seq[i+text_len] = seq[i];
        }

        for (i = 0 ;i<text_len;i++) {
            seq[position + i] = text[i];
        }
    }else{
        if ( position - text_len < 0) {
            return new AD_ERR("AD_SEQ::You cannot insert that many characters before cursor !");
        }
        for (i = 0; i<text_len; i++) {
            if (!ADPP_IS_ALIGN_CHARACTER(seq[i])) {
                return new AD_ERR("AD_SEQ::insert: start of alignment reached !");
            }
        }
        for (i= 0; i<position-text_len;i++) {
            seq[i] = seq[i+text_len];
        }

        for (i = 0 ;i<text_len;i++) {
            seq[position - i -1] = text[i];
        }
    }
    return 0;
}

AD_ERR  * AD_SEQ::remove(int len,long position, int direction) {
    long new_len;
    long i;
    if (direction<0) position -= len;
    if ((position >= seq_len) || (len == 0) || (seq_len - len < 0)) {
        return new AD_ERR("AD_SEQ::delete outside sequence !");
    }
    if ((len + position) >= seq_len) {
        len = (int)(seq_len-position);
    }
    new_len = seq_len - len;
    if (direction>=0){
        strncpy((char *)&seq[position],
                (const char *)&seq[position+len],
                (int)(new_len-position));
        for (i = new_len; i<seq_len; i++) {
            seq[i] = SEQ_POINT;
        }
    }else{
        for (i=position+len-1;i>=len;i--) {
            seq[i] = seq[i-len];
        }
        for (i = 0; i<len; i++) {
            seq[i] = SEQ_POINT;
        }
    }
    seq[seq_len] = '\0';
    return 0;
}


AD_ERR * AD_SEQ::replace(char *text,long position, int direction) {
    int text_len = strlen(text);
    int i;
    if (direction>=0){

        if ((position + text_len > seq_len) || (position > seq_len)) {
            return new AD_ERR("AD_SEQ.replace() ERROR ! Replace after end of sequence !");
        }
        for ( i = 0; i < text_len ; i ++) {
            seq[i+position] = text[i];
        }
    }else{
        if ((position - text_len < 0 ) || (position > seq_len)) {
        return new AD_ERR("AD_SEQ.replace() ERROR ! Replace before start of sequence !");
    }
    for ( i = 0; i < text_len ; i ++) {
        seq[position - i - 1] = text[i];
    }
}
return 0;
}

AD_ERR  *AD_SEQ::swap_gaps(long position, char ch){
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


AD_ERR *AD_SEQ::check_base(char chr,long position, int direction) {
    if (direction < 0) position --;
    if (position <0 && position >= seq_len) return new AD_ERR();
    if (seq[position] != chr){
        return new AD_ERR();            // beep beep
    }
    return 0;
}

AD_ERR *AD_SEQ::push(long position, int direction) {
    long i;
    long end = this->get_next_gap(position,direction);
    if ( end < 0   || end  >= seq_len  )
        return 0;       // end reached
    if (end == position) return 0;              // dont push '.'

    int gap = '-';
    int offset;

    if (direction < 0) {
        position --;
        end --;
        if (end<0) return 0;
        offset = 1;
    }else{
        offset = -1;
    }

    if ( position+offset <= 0 || position+offset >= seq_len-1 ||
         seq[position+offset] == '.') gap = '.';

    for (i= end  ; i!= position; i -= direction) {
        seq[i] = seq[i- direction];
    };
    seq[position] = gap;
    return 0;
}


AD_ERR *AD_SEQ::jump(long position, int direction) {
    int offset = 0; if (direction <0) offset = -1;
    long npos = get_next_base(position,-direction) + offset;
    int swap;
    position += offset;
    swap = seq[position]; seq[position] = seq[npos]; seq[npos] = swap;
    return 0;
}

AD_ERR *AD_SEQ::fetch(long position, int direction) {
    int offset = 0; if (direction <0) offset = -1;
    long npos = get_next_base(position,direction) + offset;
    if (npos <0 || npos >= seq_len) return 0;
    int swap;
    position += offset;
    swap = seq[position]; seq[position] = seq[npos]; seq[npos] = swap;
    return 0;
}

long AD_SEQ::get_next_base(long position, int direction) {
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos>=0 && pos < seq_len; pos += direction){
        if (!ADPP_IS_ALIGN_CHARACTER(seq[pos])) break;
    }
    if (direction<0) pos++;
    if (pos <  0 ) pos = 0;
    if (pos > seq_len) pos = seq_len;
    return pos;
}

long AD_SEQ::get_next_gap(long position, int direction) {
    long pos;
    if (direction < 0) position--;
    for (pos = position; pos >= 0 && pos < seq_len; pos += direction){
        if (ADPP_IS_ALIGN_CHARACTER(seq[pos])) break;
    }
    if (direction<0) pos++;
    if (pos <  0 ) pos = 0;
    if (pos > seq_len) pos = seq_len;
    return pos;
}


AD_ERR  *AD_SEQ::command( AW_key_mod keymod, AW_key_code keycode, char key, int direction, long &cursorpos, int & changed_flag)
{
    static int nrepeat = 0;
    long oldcursorpos = cursorpos;
    char str[2];
    str[0] = key;
    str[1] = 0;
    changed_flag = 0;
    if (direction > 0) {
        direction = 1;          // may be inverted to allow right to left insert
    }else{
        direction = -1;
    }

    if ((cursorpos > seq_len) || (cursorpos < 0 )) {
        return new AD_ERR("AD_SEQ.command ERROR ! Cursor out of sequence !");
    }

    AD_EDITMODI mode = this->get_ad_main()->mode;
    AD_ERR *ad_err = 0;
    long        h,offset;

    switch (keycode) {
        case AW_KEY_ASCII:
            if (key == ' ') {
                long left;
                int l,r;
                left = cursorpos-1; if (left<0) left = 0;
                l = seq[left]; r = seq[cursorpos];
                key = '-';
                if (ADPP_IS_ALIGN_CHARACTER(l)) key = l;
                if (ADPP_IS_ALIGN_CHARACTER(r)) key = r;
                str[0] = key;
            }
            switch (mode) {
                case AD_allign:
                    if (isdigit(key)) {
                        nrepeat = nrepeat * 10 + (key - '0');
                        break;
                    }
                    if (ADPP_IS_ALIGN_CHARACTER(key)){
                        long left;
                        int l,r;
                        left = cursorpos-1; if (left<0) left = 0;
                        l = seq[left]; r = seq[cursorpos];
                        if (ADPP_IS_ALIGN_CHARACTER(l) && l!=key) {
                            ad_err = this->swap_gaps(left,key);
                        }else if (ADPP_IS_ALIGN_CHARACTER(r) && r!=key ) {
                            ad_err = this->swap_gaps(cursorpos,key);
                        }else if (!ad_err){
                            if (!nrepeat) nrepeat = 1;
                            char *nstr = (char *)calloc(1,nrepeat+1);
                            int i;
                            for (i = 0; i< nrepeat; i++) nstr[i] = key;
                            ad_err = this->insert(nstr,cursorpos,direction);
                            if (!ad_err) cursorpos+=direction*nrepeat;
                            delete nstr;
                        }
                        changed_flag = 1;
                    }else{
                        cursorpos = get_next_base(cursorpos,direction);
                        ad_err = check_base(key,cursorpos,direction);
                        if (!ad_err) cursorpos+=direction;
                    }
                    nrepeat = 0;
                    break;
                case AD_replace:
                    ad_err = this->replace(str,cursorpos,direction);
                    if (!ad_err) cursorpos+=direction;
                    changed_flag = 1;
                    break;
                case AD_insert:
                    ad_err = this->insert(str,cursorpos,direction);
                    if (!ad_err) cursorpos+=direction;
                    if (!ad_err) changed_flag = 1;
                    break;
                default:        break;
            }
            break;
        case AW_KEY_HOME:
            nrepeat = 0;
            cursorpos = get_next_base(0,1);
            break;
        case AW_KEY_END:
            nrepeat = 0;
            cursorpos = get_next_base(seq_len,-1);
            break;
        case AW_KEY_DELETE:
        case AW_KEY_BACKSPACE:
            if (!nrepeat) nrepeat = 1;
            if (mode != AD_allign) nrepeat = 1;
            if (keycode == AW_KEY_DELETE || keymod ) {
                h = cursorpos;
            }else{
                h = cursorpos;
                if (direction > 0) {
                    cursorpos-=nrepeat;
                }else{
                    cursorpos+= nrepeat;
                }
                if (cursorpos <0 || cursorpos >= seq_len) {
                    cursorpos = h;
                    if (nrepeat >1) {
                        ad_err = new AD_ERR("Out of sequence");
                    }
                    nrepeat = 0;
                    break;
                }
                h = cursorpos;
            }
            switch (mode) {
                case AD_allign:
                    {
                        int len;
                        ad_err = 0;
                        if (direction>=0) offset = 0; else offset = -nrepeat;
                        for (len = nrepeat-1; len>=0; len --) {
                            if (!ADPP_IS_ALIGN_CHARACTER(seq[h+offset+len])){
                                ad_err = new AD_ERR(
                                                    "You cannot remove bases in align mode");
                                break;
                            }
                        }
                        if (ad_err) break;
                    }
                case AD_replace:
                case AD_insert:
                    ad_err = this->remove(nrepeat,h,direction);
                    if (!ad_err) changed_flag = 1;
                    break;
                default:        break;
            }
            nrepeat = 0;

            break;

        case AW_KEY_LEFT:
        case AW_KEY_RIGHT:
            direction = 1;
            if (keycode == AW_KEY_LEFT) direction = -1;
            if (keymod == 0) {
                if (!nrepeat) nrepeat = 1;
                cursorpos+= direction * nrepeat;
                nrepeat = 0;
                break;
            }
            nrepeat = 0;
            if (direction>=0) offset = 0; else offset = -1;
            if (keymod & AW_KEY_CONTROL) {      // push pull
                if (ADPP_IS_ALIGN_CHARACTER(seq[cursorpos+offset])){
                    // pull operation
                    h = this->get_next_gap(     cursorpos,-direction);
                }else{  // push
                    h = cursorpos;
                }
                ad_err = this->push(h,direction);
                if (!ad_err) {
                    changed_flag = 1;
                    cursorpos += direction;
                }
            }else if (keymod & AW_KEY_ALT) {    // jump fetch
                if (ADPP_IS_ALIGN_CHARACTER(seq[cursorpos+offset])){
                    // fetch
                    ad_err = fetch(cursorpos,direction);
                }else if (ADPP_IS_ALIGN_CHARACTER(seq[cursorpos-1-offset])){
                    ad_err = jump(cursorpos,direction);
                }else{
                    ad_err = new AD_ERR("You can only jump single bases !!!");
                }
                if (!ad_err) {
                    changed_flag = 1;
                    cursorpos += direction;
                }
            }else{
                if (ADPP_IS_ALIGN_CHARACTER(seq[cursorpos+offset])){
                    cursorpos = get_next_base(cursorpos,direction);
                }else{
                    cursorpos = get_next_gap(cursorpos,direction);
                }
            }
            break;
        default:
            break;

    }
    if (ad_err) cursorpos = oldcursorpos;
    if (cursorpos < 0 ) cursorpos = 0;
    if (cursorpos > seq_len) cursorpos = seq_len;

    return ad_err;
}

AD_ERR * AD_SEQ::changemode(AD_EDITMODI mod) {
    // check rights
    // not implemented
    this->get_ad_main()->mode = mod;
    return 0;
}

AD_EDITMODI AD_SEQ::mode() {
    return this->get_ad_main()->mode;
}





/***************************
AD_CONT
container wird mit spezies,alignment initialisiert
entspricht ungefaehr dem ali_xxx container der ARB DB
********************/

AD_CONT::AD_CONT()
{
    ad_species =0;
    ad_ali =0;
    gb_ali =0;
    gb_species = 0;
}

AD_CONT::~AD_CONT()
{
    // vergessen ein exit zu machen !
    if (ad_species) {
        new AD_ERR("AD_CONT: NO exit() !!",CORE);
    }
}

AD_ERR * AD_CONT::init(AD_SPECIES * adptr1,AD_ALI * adptr2)
{
    if (!con_insert(adptr1,adptr2)) // test ob container schoneinmal initialisiert
        return new AD_ERR("AD_CONT::init  ONLY ONE AD_CONT PER SPECIES/ALIGN");
    ad_species = adptr1;
    ad_ali      = adptr2;
    gb_species = ad_species->gb_species;
    gb_ali = GB_find(gb_species,ad_ali->name(),0,down_level);
    if (gb_ali) {
        (ad_species->count) ++; // in species eintragen
        (ad_ali->count) ++;
    }else{
        con_remove(ad_species,ad_ali);
    }
    AD_READWRITE::gbdataptr = gb_ali;
    return 0;
}

AD_ERR *AD_CONT::create(AD_SPECIES * adptr1,AD_ALI *adptr2)
{
    GBDATA *erg =  GBT_add_data(adptr1->gb_species,adptr2->name(),"data", GB_STRING);
    if (!erg) return new AD_ERR((char *)GB_get_error());
    return this->init(adptr1,adptr2);
}

AD_ERR * AD_CONT::exit()
{
    if (ad_species && gb_ali) {
        ad_species->count --; // verweis in AD_SPEC loeschen
        ad_ali->count --;
        con_remove(ad_species,ad_ali);
    }
    ad_species = 0;ad_ali = 0;
    AD_READWRITE::gbdataptr = 0;
    return 0;
}

int AD_CONT::eof(void)
{
    if (gb_ali) return 0;
    return 1;
}

int AD_CONT::con_insert(AD_SPECIES * adptr1,AD_ALI * adptr2)
{
    class CONTLIST *cont;
    cont = adptr1->container;
    return cont->insert(adptr1,adptr2);
    // ist der selbe container schon vorhanden ?
}


void AD_CONT::con_remove(AD_SPECIES * adptr1,AD_ALI * adptr2)
// entfernt ein AD_CONT aus der Liste in AD_species
{
    class CONTLIST *cont;
    cont = adptr1->container;
    cont->remove(adptr1,adptr2);
    // entfernt den container
}

int AD_CONT::get_cach_flag()
// implementiert um mehrfachzeiger zu vermeiden
{
    AD_MAIN * adptr;
    adptr = ad_species->ad_main;
    return adptr->get_cach_flag();
}



