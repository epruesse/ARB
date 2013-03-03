// =============================================================== //
//                                                                 //
//   File      : gb_data.h                                         //
//   Purpose   : GBDATA/GBCONTAINER                                //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_DATA_H
#define GB_DATA_H

#ifndef GB_LOCAL_H
#include "gb_local.h"
#endif
#ifndef GB_MEMORY_H
#include "gb_memory.h"
#endif


// --------------------------------------------------------------------------------

#define SIZOFINTERN 10

struct gb_extern_data {
    GB_REL_STRING rel_data;
    long          memsize;
    long          size;
};

struct GB_INTern_strings {
    char          data[SIZOFINTERN];
    unsigned char memsize;
    unsigned char size;
};

struct GB_INTern {
    char data[SIZOFINTERN];
};

union gb_data_base_type_union {
    int32_t            i;
    GBDATA            *ptr;
    GB_INTern_strings  istr;
    GB_INTern          in;
    gb_extern_data     ex;
};

// --------------------------------------------------------------------------------

struct gb_callback {
    gb_callback *next;
    GB_CB        func;
    GB_CB_TYPE   type;
    int         *clientdata;
    short        priority;
    short        running;
};

// --------------------------------------------------------------------------------

struct gb_db_extended {
    long                 creation_date;
    long                 update_date;
    gb_callback         *callback;
    gb_transaction_save *old;
};

// --------------------------------------------------------------------------------

struct gb_flag_types {                                                  // public flags
    unsigned int type : 4;
    unsigned int security_delete : 3;
    unsigned int security_write : 3;
    unsigned int security_read : 3;
    unsigned int compressed_data : 1;
    unsigned int unused : 1;                                            // last bit saved!
    unsigned int user_flags : 8;
    unsigned int temporary : 1;                                         // ==1 -> don't save entry
    unsigned int saved_flags : 8;
};

struct gb_flag_types2 {                                                 // private flags, abortable
    // uncritical section, undoable
    unsigned int last_updated : 8;
    unsigned int usr_ref : 7;                                           // for user access
    // critic section, do not update any below
    unsigned int folded_container : 1;
    unsigned int update_in_server : 1;                                  // already informed
    unsigned int extern_data : 1;                                       // data ref. by pntr
    unsigned int header_changed : 1;                                    // used by container
    unsigned int gbm_index : 8;                                         // memory section
    unsigned int tisa_index : 1;                                        // this should be indexed
    unsigned int is_indexed : 1;                                        // this db. field is indexed
};

struct gb_flag_types3 {                                                 // user and project flags (public); not abortable !!!
    unsigned int project : 8;
    unsigned int unused : 24;
};

// --------------------------------------------------------------------------------

struct gb_data_list {
    GB_REL_HLS rel_header;
    int        headermemsize; // array size (allocated entries)
    int        size;          // number of valid (non-deleted) items - not valid in transaction mode
    int        nheader;       // index of next new entry
};

inline gb_header_list *GB_DATA_LIST_HEADER(gb_data_list& dl) {
    return GB_RESOLVE(gb_header_list *, (&(dl)), rel_header);
}
inline void SET_GB_DATA_LIST_HEADER(gb_data_list& dl, gb_header_list *head) {
    GB_SETREL(&dl, rel_header, head);
}

// --------------------------------------------------------------------------------

#if defined(DEBUG)
#define ASSERT_STRICT_GBDATA_TYPES // just for refactoring/debugging (slow)
#endif

#if defined(ASSERT_STRICT_GBDATA_TYPES)
#define gb_strict_assert(cond) gb_assert(cond)
#else
#define gb_strict_assert(cond)
#endif

struct GBENTRY;
struct GBCONTAINER;

struct GBDATA {
    long              server_id;
    GB_REL_CONTAINER  rel_father;
    gb_db_extended   *ext;
    long              index;
    gb_flag_types     flags;
    gb_flag_types2    flags2;

    // ----------------------------------------
    // @@@ methods below are unused atm

    GB_TYPES type() const {
        gb_assert(this);
        return GB_TYPES(flags.type);
    }

    bool is_container() const { return type() == GB_DB; }
    bool is_entry() const { return !is_container(); }

    GBENTRY *as_entry() const {
        gb_strict_assert(is_entry());
        return (GBENTRY*)this;
    }
    GBCONTAINER *as_container() const {
        gb_strict_assert(is_container());
        return (GBCONTAINER*)this;
    }
};

struct GBENTRY : public GBDATA {
    gb_data_base_type_union info;

    int cache_index; // @@@ should be a member of gb_db_extended and of type gb_cache_idx
};

struct GBCONTAINER : public GBDATA {
    gb_flag_types3 flags3;
    gb_data_list   d;

    long index_of_touched_one_son;  /* index of modified son
                                     * in case of a single mod. son
                                     * -1 more than one (or one with ind = 0)
                                     *  0    no son
                                     * >0   index */
    long header_update_date;

    GB_MAIN_IDX main_idx;
    GB_REL_IFS  rel_ifs;

};

// --------------------
//      type access

inline GB_TYPES GB_TYPE(GBDATA *gbd) { return gbd->type(); } // @@@ elim

// ----------------------
//      parent access

inline GBCONTAINER* GB_FATHER(GBDATA *gbd)  { return GB_RESOLVE(GBCONTAINER*, gbd, rel_father); }
inline GBCONTAINER* GB_GRANDPA(GBDATA *gbd) { return GB_FATHER(GB_FATHER(gbd)); }

inline void SET_GB_FATHER(GBDATA *gbd, GBCONTAINER *father) { GB_SETREL(gbd, rel_father, father); }

// -----------------------
//      GB_MAIN access

extern GB_MAIN_TYPE *gb_main_array[];

inline GB_MAIN_TYPE *GBCONTAINER_MAIN(GBCONTAINER *gbc) { return gb_main_array[gbc->main_idx]; }

inline GB_MAIN_TYPE *GB_MAIN(GBDATA *gbd)               { return GBCONTAINER_MAIN(GB_FATHER(gbd)); }
inline GB_MAIN_TYPE *GB_MAIN(GBCONTAINER *gbc)          { return GBCONTAINER_MAIN(gbc); }

inline GB_MAIN_TYPE *GB_MAIN_NO_FATHER(GBDATA *gbd)    { return GB_TYPE(gbd) == GB_DB ? GBCONTAINER_MAIN((GBCONTAINER*)gbd) : GB_MAIN(gbd); }

// -----------------------
//      security flags

#define GB_GET_SECURITY_READ(gb)   ((gb)->flags.security_read)
#define GB_GET_SECURITY_WRITE(gb)  ((gb)->flags.security_write)
#define GB_GET_SECURITY_DELETE(gb) ((gb)->flags.security_delete)

#define GB_PUT_SECURITY_READ(gb, i)  ((gb)->flags.security_read   = (i))
#define GB_PUT_SECURITY_WRITE(gb, i) ((gb)->flags.security_write  = (i))
#define GB_PUT_SECURITY_DELETE(gb, i) ((gb)->flags.security_delete = (i))

// --------------------------------------------------------------------------------

#else
#error gb_data.h included twice
#endif // GB_DATA_H



