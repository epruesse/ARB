
struct GBENTRY : public GBDATA {
private:
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

struct GBCONTAINER : public GBDATA {
private:
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

inline GB_ERROR gb_commit_transaction_local_rek(GBCONTAINER*& gbc, long mode, int *pson_created) {
    return gb_commit_transaction_local_rek(StrictlyAliased_BasePtrRef<GBCONTAINER,GBDATA>(gbc).forward(), mode, pson_created);
}

inline void gb_abort_transaction_local_rek(GBCONTAINER*& gbc) {
    gb_abort_transaction_local_rek(StrictlyAliased_BasePtrRef<GBCONTAINER,GBDATA>(gbc).forward());
}

// --------------------------------------------------------------------------------

#else
#error gb_data.h included twice
#endif // GB_DATA_H



