// =============================================================== //
//                                                                 //
//   File      : ed4_block.hxx                                     //
//   Purpose   : Block operations                                  //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de)                   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ED4_BLOCK_HXX
#define ED4_BLOCK_HXX

enum ED4_blocktype {
    ED4_BT_NOBLOCK,             // nothing is marked
    ED4_BT_LINEBLOCK,           // all species with EDIT4-marks are marked
    ED4_BT_COLUMNBLOCK,         // range is marked
    ED4_BT_MODIFIED_COLUMNBLOCK // was columnblock, but single species were removed/added
};

enum ED4_blockoperation_type {
    ED4_BO_UPPER_CASE,
    ED4_BO_LOWER_CASE,
    ED4_BO_REVERSE,
    ED4_BO_COMPLEMENT,
    ED4_BO_REVERSE_COMPLEMENT,
    ED4_BO_UNALIGN_LEFT,
    ED4_BO_UNALIGN_CENTER,
    ED4_BO_UNALIGN_RIGHT,
    ED4_BO_SHIFT_LEFT,
    ED4_BO_SHIFT_RIGHT
};

ED4_blocktype ED4_getBlocktype();
void          ED4_setBlocktype(ED4_blocktype bt);
void          ED4_toggle_block_type();
void          ED4_correctBlocktypeAfterSelection();
void          ED4_setColumnblockCorner(AW_event *event, ED4_sequence_terminal *seq_term);
bool          ED4_get_selected_range(ED4_terminal *term, PosRange& range);

class SeqPart;

class ED4_block_operator : virtual Noncopyable {
protected:
    mutable GB_ERROR error;
public:
    ED4_block_operator() : error(NULL) {}
    virtual ~ED4_block_operator() {}

    GB_ERROR get_error() const { return error; }
    virtual char *operate(const SeqPart& part, int& new_len) const = 0;
};

void ED4_perform_block_operation(ED4_blockoperation_type type);

AW_window *ED4_create_replace_window(AW_root *root);
AW_window *ED4_create_modsai_window(AW_root *root);

#else
#error ed4_block.hxx included twice
#endif // ED4_BLOCK_HXX
