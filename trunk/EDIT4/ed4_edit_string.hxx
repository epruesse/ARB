
struct ED4_work_info;

extern unsigned char ED4_is_align_character[255];
inline bool ADPP_IS_ALIGN_CHARACTER(unsigned char chr) { return ED4_is_align_character[chr] != 0; }
void ED4_init_is_align_character(GB_CSTR gap_chars);

const char SEQ_POINT = '.';

class ED4_remap;

class ED4_Edit_String {
    GBDATA	*gbd;
    char	*seq;
    long	seq_len;
    ED4_remap   *remap;
    static int	nrepeat;
    static int	nrepeat_is_already_set;
    static int	nrepeat_zero_requested;

    int legal_curpos(long pos) const 		{ return pos>=0 && pos<=seq_len; }
    int legal_seqpos(long pos) const 		{ return pos>=0 && pos<seq_len; }

    ED4_ERROR *moveBase(long source_position, long dest_position, unsigned char gap_to_use); // moves a base from source_position to dest_position

    ED4_ERROR *shiftBases(long source_pos, long source_endpos, long dest_position,
			  int direction, long *dest_endpos, unsigned char gap_to_use); // shifts a line of bases from source_position to dest_position

    long	get_next_base(long seq_position, int direction);
    long	get_next_gap(long seq_position, int direction);

    long 	get_next_visible_base(long position, int direction);
    long 	get_next_visible_gap(long position, int direction);
    long 	get_next_visible_pos(long position, int direction);

    ED4_ERROR 	*insert(char *,long position, int direction, int removeAtNextGap);
    ED4_ERROR 	*remove(int len,long position, int direction, int insertAtNextGap);
    ED4_ERROR 	*replace(char *text,long position, int direction);
    ED4_ERROR	*swap_gaps(long position, char ch);
    ED4_ERROR	*command( AW_key_mod keymod, AW_key_code keycode, char key, int direction, ED4_EDITMODI mode, bool is_consensus,
			  long &cursorpos, bool& changed_flag, bool& center_cursor, bool& cannot_handle, bool& write_fault, GBDATA *gb_data, bool is_sequence);
    unsigned char get_gap_type(long pos, int direction);

public:
    char *old_seq;
    long old_seq_len;

    ED4_Edit_String();
    void init_edit();
    void edit(ED4_work_info *info);
    void finish_edit();
    ~ED4_Edit_String();

    int use_nrepeat() { // external functions use this to get and use nrepeat
	int nrep = nrepeat==0 ? 1 : nrepeat;
	nrepeat = 0;
	return nrep;
    }
};

