// =============================================================== //
//                                                                 //
//   File      : ed4_edit_string.hxx                               //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ED4_EDIT_STRING_HXX
#define ED4_EDIT_STRING_HXX

#ifndef ED4_DEFS_HXX
#include "ed4_defs.hxx"
#endif
#ifndef CHARTABLE_H
#include <chartable.h>
#endif

struct ED4_work_info;

inline bool ED4_is_gap_character(char chr) { return BaseFrequencies::isGap(safeCharIndex(chr)); }
void ED4_setup_gaps_and_alitype(const char *gap_chars, GB_alignment_type alitype);

const char SEQ_POINT = '.';

class ED4_remap;

class ED4_Edit_String : virtual Noncopyable {
    GBDATA      *gbd;
    char        *seq;
    long        seq_len;
    ED4_remap   *remap;
    static int  nrepeat;
    static int  nrepeat_is_already_set;
    static int  nrepeat_zero_requested;

    int legal_curpos(long pos) const            { return pos>=0 && pos<=seq_len; }
    int legal_seqpos(long pos) const            { return pos>=0 && pos<seq_len; }

    GB_ERROR moveBase(long source_position, long dest_position, unsigned char gap_to_use); // moves a base from source_position to dest_position

    GB_ERROR shiftBases(long source_pos, long source_endpos, long dest_position,
                        int direction, long *dest_endpos, unsigned char gap_to_use); // shifts a line of bases from source_position to dest_position

    long get_next_base(long seq_position, int direction);
    long get_next_gap(long seq_position, int direction);

    long get_next_visible_base(long position, int direction);
    long get_next_visible_gap(long position, int direction);
    long get_next_visible_pos(long position, int direction);

    GB_ERROR insert(char *, long position, int direction, int removeAtNextGap);
    GB_ERROR remove(int len, long position, int direction, int insertAtNextGap);
    GB_ERROR replace(char *text, long position, int direction);
    GB_ERROR swap_gaps(long position, char ch);

    GB_ERROR command(AW_key_mod keymod, AW_key_code keycode, char key, int direction, ED4_EDITMODE mode, bool is_consensus,
                     long &cursorpos, bool& changed_flag, ED4_CursorJumpType& cursor_jump, bool& cannot_handle, bool& write_fault, GBDATA *gb_data, bool is_sequence);

    unsigned char get_gap_type(long pos, int direction);

public:
    char *old_seq;
    long old_seq_len;

    ED4_Edit_String();
    void init_edit();
    GB_ERROR edit(ED4_work_info *info) __ATTR__USERESULT;
    void finish_edit();
    ~ED4_Edit_String();

    int use_nrepeat() { // external functions use this to get and use nrepeat
        int nrep = nrepeat==0 ? 1 : nrepeat;
        nrepeat = 0;
        return nrep;
    }
};

#else
#error ed4_edit_string.hxx included twice
#endif // ED4_EDIT_STRING_HXX
