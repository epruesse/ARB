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

struct gb_callback_list;

// --------------------------------------------------------------------------------

#define SIZOFINTERN 10

struct gb_extern_data {
    GB_REL_STRING rel_data;
    long          memsize;
    long          size;

    char *get_data() { return GB_RESOLVE(char*, this, rel_data); }
    void set_data(char *data) { GB_SETREL(this, rel_data, data); }
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

struct gb_db_extended {
    long                 creation_date;
    long                 update_date;
    gb_callback_list    *callback;
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

struct gb_flag_types2 { // private flags (DB server and each client have their own copy!), abortable
    // uncritical section, undoable
    unsigned int last_updated : 8;
    unsigned int user_bits : 7;                                         // user flags (see GB_USERFLAG_...)
    // critic section, do not update any below
    unsigned int folded_container : 1;
    unsigned int update_in_server : 1;                                  // already informed
    unsigned int extern_data : 1;                                       // data ref. by pntr
    unsigned int header_changed : 1;                                    // used by container
    unsigned int gbm_index : 8;                                         // memory section
    unsigned int should_be_indexed : 1;                                 // this should be indexed
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

#define GB_GBM_INDEX(gbd) ((gbd)->flags2.gbm_index)

struct GBDATA {
    long              server_id;
    GB_REL_CONTAINER  rel_father;
    gb_db_extended   *ext;
    long              index;
    gb_flag_types     flags;
    gb_flag_types2    flags2;

    // ----------------------------------------

    GB_TYPES type() const {
        gb_assert(this);
        return GB_TYPES(flags.type);
    }

    bool is_a_string() const { return type() == GB_STRING || type() == GB_LINK; }
    bool is_indexable() const { return is_a_string(); }

    bool is_container() const { return type() == GB_DB; }
    bool is_entry() const { return !is_container(); }

    GBENTRY *as_entry() const {
        gb_assert(this); // use GBDATA::as_entry() instead
        gb_strict_assert(is_entry());
        return (GBENTRY*)this;
    }
    GBCONTAINER *as_container() const {
        gb_assert(this); // use GBDATA::as_container() instead
        gb_strict_assert(is_container());
        return (GBCONTAINER*)this;
    }

    static GBCONTAINER *as_container(GBDATA *gbd) { return gbd ? gbd->as_container() : NULL; }
    static GBENTRY     *as_entry    (GBDATA *gbd) { return gbd ? gbd->as_entry()     : NULL; }

    // meant to be used in client interface (i.e. on any GBDATA* passed from outside)
    GBENTRY *expect_entry() const {
        if (!is_entry()) GBK_terminate("expected a DB entry, got a container");
        return as_entry();
    }
    GBCONTAINER *expect_container() const {
        if (!is_container()) GBK_terminate("expected a DB container, got an entry");
        return as_container();
    }

    inline GBCONTAINER *get_father();

    void create_extended() {
        if (!ext) {
            ext = (gb_db_extended *)gbm_get_mem(sizeof(gb_db_extended), GB_GBM_INDEX(this));
        }
    }
    void destroy_extended() {
        if (ext) {
            gbm_free_mem(ext, sizeof(gb_db_extended), GB_GBM_INDEX(this));
            ext = NULL;
        }
    }

    void touch_creation(long cdate)           { ext->creation_date = cdate; }
    void touch_update(long udate)             { ext->update_date   = udate; }
    void touch_creation_and_update(long date) { ext->creation_date = ext->update_date = date; }

    long creation_date() const { return ext ? ext->creation_date : 0; }
    long update_date()   const { return ext ? ext->update_date   : 0; }

    gb_callback_list *get_callbacks() const { return ext ? ext->callback : NULL; }
    gb_transaction_save *get_oldData() const { return ext ? ext->old : 0; }
};

class GBENTRY : public GBDATA {
    // calls that make no sense:
    bool is_entry() const;
    GBENTRY *as_entry() const;
public:
    gb_data_base_type_union info;

    int cache_index;

    void mark_as_intern() { flags2.extern_data = 0; }
    void mark_as_extern() { flags2.extern_data = 1; }

    bool stored_external() const { return flags2.extern_data; }
    bool stored_internal() const { return !stored_external(); }

    size_t size()    const { return stored_external() ? info.ex.size    : info.istr.size; }
    size_t memsize() const { return stored_external() ? info.ex.memsize : info.istr.memsize; }

    inline size_t uncompressed_size() const;

    char *data() { return stored_external() ? info.ex.get_data() : &(info.istr.data[0]); }

    inline char *alloc_data(long Size, long Memsize);
    inline void insert_data(const char *Data, long Size, long Memsize);
    void free_data() {
        index_check_out();
        if (stored_external()) {
            char *exdata = info.ex.get_data();
            if (exdata) {
                gbm_free_mem(exdata, (size_t)(info.ex.memsize), GB_GBM_INDEX(this));
                info.ex.set_data(0);
            }
        }
    }

    void index_check_in();
    void index_re_check_in() { if (flags2.should_be_indexed) index_check_in(); }

    void index_check_out();
};

class GBCONTAINER : public GBDATA {
    // calls that make no sense:
    bool is_container() const;
    GBCONTAINER *as_container() const;
public:
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

    void set_touched_idx(int idx) {
        if (!index_of_touched_one_son || index_of_touched_one_son == idx+1) {
            index_of_touched_one_son = idx+1;
        }
        else {
            index_of_touched_one_son = -1;
        }
    }
};

// ----------------------
//      parent access

inline GBCONTAINER* GB_FATHER(GBDATA *gbd)  { return GB_RESOLVE(GBCONTAINER*, gbd, rel_father); }
inline GBCONTAINER* GB_GRANDPA(GBDATA *gbd) { return GB_FATHER(GB_FATHER(gbd)); }

inline void SET_GB_FATHER(GBDATA *gbd, GBCONTAINER *father) { GB_SETREL(gbd, rel_father, father); }

GBCONTAINER *GBDATA::get_father() {
    // like GB_FATHER, but returns NULL for root_container
    GBCONTAINER *father = GB_FATHER(this);
    if (father && !GB_FATHER(father)) return NULL;
    return father;
}


// -----------------------
//      GB_MAIN access

extern GB_MAIN_TYPE *gb_main_array[];

inline GB_MAIN_TYPE *GBCONTAINER_MAIN(GBCONTAINER *gbc) { return gb_main_array[gbc->main_idx]; }

inline GB_MAIN_TYPE *GB_MAIN(GBDATA *gbd)               { return GBCONTAINER_MAIN(GB_FATHER(gbd)); }
inline GB_MAIN_TYPE *GB_MAIN(GBCONTAINER *gbc)          { return GBCONTAINER_MAIN(gbc); }

inline GB_MAIN_TYPE *GB_MAIN_NO_FATHER(GBDATA *gbd) {
    return gbd->is_container() ? GBCONTAINER_MAIN(gbd->as_container()) : GB_MAIN(gbd->as_entry());
}

// -----------------------
//      security flags

#define GB_GET_SECURITY_READ(gb)   ((gb)->flags.security_read)
#define GB_GET_SECURITY_WRITE(gb)  ((gb)->flags.security_write)
#define GB_GET_SECURITY_DELETE(gb) ((gb)->flags.security_delete)

#define GB_PUT_SECURITY_READ(gb, i)  ((gb)->flags.security_read   = (i))
#define GB_PUT_SECURITY_WRITE(gb, i) ((gb)->flags.security_write  = (i))
#define GB_PUT_SECURITY_DELETE(gb, i) ((gb)->flags.security_delete = (i))

// ---------------------------------------------------------
//      strictly-aliased forwarders for some functions:

inline __ATTR__USERESULT GB_ERROR gb_commit_transaction_local_rek(GBCONTAINER*& gbc, long mode, int *pson_created) {
    return gb_commit_transaction_local_rek(StrictlyAliased_BasePtrRef<GBCONTAINER,GBDATA>(gbc).forward(), mode, pson_created);
}

inline void gb_abort_transaction_local_rek(GBCONTAINER*& gbc) {
    gb_abort_transaction_local_rek(StrictlyAliased_BasePtrRef<GBCONTAINER,GBDATA>(gbc).forward());
}

// --------------------------------------------------------------------------------

#ifndef GB_STORAGE_H
#include "gb_storage.h"
#endif

#else
#error gb_data.h included twice
#endif // GB_DATA_H



