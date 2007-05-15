/* -------------------- adlmacros.h -------------------- */

#define A_TO_I(c) if(c>'9') c-='A'-10; else c-='0';

#   define GB_MEMALIGN(a,b)     memalign(a,b)

#if defined(SUN5) || defined(ECGS)
# define GB_MEMCPY(d,s,n)       memcpy(d,s,n)
# define GB_FREE(d)             free(d)
# define GB_STRDUP(s)           strdup(s)
# define GB_MEMSET(d,v,n)       memset(d,v,n)
#else
# define GB_MEMCPY(d,s,n)       memcpy((char *)(d),(char*)(s),(int)(n))
# define GB_FREE(d)             free((char*)(d))
# define GB_STRDUP(s)           strdup((char *)(s))
# define GB_MEMSET(d,v,n)       memset((char*)(d),v,(int)(n))
#endif

#define GB_DELETE(a)        if (a) GB_FREE(a); a = 0

#if defined(HP) || defined(DIGITAL) || defined(DARWIN)
# undef GB_MEMALIGN
# define GB_MEMALIGN(a,b) malloc(b)
#endif

#define GB_MIN(a,b) ( (a)>(b) ? (b):(a) )
#define GB_MAX(a,b) ( (a)<(b) ? (b):(a) )

/********************* SECURITY ******************/

# define GB_GET_SECURITY_READ(gb)   ((gb)->flags.security_read)
# define GB_GET_SECURITY_WRITE(gb)  ((gb)->flags.security_write)
# define GB_GET_SECURITY_DELETE(gb)     ((gb)->flags.security_delete)

# define GB_PUT_SECURITY_READ(gb,i)     do { (gb)->flags.security_read = (i); } while(0)
# define GB_PUT_SECURITY_WRITE(gb,i)    do { (gb)->flags.security_write = (i); } while(0)
# define GB_PUT_SECURITY_DELETE(gb,i)   do { (gb)->flags.security_delete = (i); } while(0)

/********************* RELATIVE ADRESSING **********/

#if (MEMORY_TEST==1)

# define GB_RESOLVE(typ,struct_add,member_name) ( (typ)(struct_add->member_name) )
# define GB_SETREL(struct_add,member_name,creator) ( struct_add)->member_name = creator
# define GB_ENTRIES_ENTRY(entries,idx)      (entries)[idx]
# define SET_GB_ENTRIES_ENTRY(entries,idx,ie)  entries[idx] = ie;

#else /* !(MEMORY_TEST==1) */

# define GB_RESOLVE(typ,struct_add,member_name)                                      \
                        ((typ) (((struct_add)->member_name)                          \
                        ? (typ) (((char*)(struct_add))+((struct_add)->member_name))  \
                        : NULL ))

# define GB_SETREL(struct_add,member_name,creator)                      \
do {                                                                    \
    register char *pntr = (char *)(creator);                            \
    if (pntr)                                                           \
    {                                                                   \
        (struct_add)->member_name = (char*)(pntr)-(char*)(struct_add);  \
    }                                                                   \
    else {                                                              \
        (struct_add)->member_name = 0;                                  \
    }                                                                   \
} while(0)

# define GB_ENTRIES_ENTRY(entries,idx)      ((struct gb_if_entries *) ((entries)[idx] ? ((char*)entries)+(entries[idx]) : NULL))

# define SET_GB_ENTRIES_ENTRY(entries,idx,ie)           \
do {                                                    \
    if (ie)  {                                          \
        (entries)[idx] = (char*)(ie)-(char*)(entries);  \
    }                                                   \
    else {                                              \
        (entries)[idx] = 0;                             \
    }                                                   \
} while(0)

#endif /* !(MEMORY_TEST==1) */

/*  Please keep care that all 'member_name's of the following macros have different names
    Otherwise there will be NO WARNINGS from the compiler

    The parameter types for the following macros are:

    hl  struct gb_header_list_struct *
    dl  struct gb_data_list
    ie  struct gb_if_entries *
    if  struct gb_index_files_struct *
    gbc GBCONTAINER *
    ex  struct gb_extern_data *
    gbd GBDATA* or GBCONTAINER*
*/

/* -------------------------------------------------------------------------------- */

#ifdef __cplusplus

inline GBDATA *                         GB_HEADER_LIST_GBD(struct gb_header_list_struct& hl)        { return GB_RESOLVE(struct gb_data_base_type *,(&(hl)),rel_hl_gbd); }
inline struct gb_header_list_struct *   GB_DATA_LIST_HEADER(struct gb_data_list& dl)                { return GB_RESOLVE(struct gb_header_list_struct *,(&(dl)),rel_header); }
inline struct gb_if_entries *           GB_IF_ENTRIES_NEXT(struct gb_if_entries *ie)                { return GB_RESOLVE(struct gb_if_entries *,ie,rel_ie_next); }
inline GBDATA *                         GB_IF_ENTRIES_GBD(struct gb_if_entries *ie)                 { return GB_RESOLVE(struct gb_data_base_type *,ie,rel_ie_gbd); }
inline struct gb_index_files_struct *   GB_INDEX_FILES_NEXT(struct gb_index_files_struct *ixf)      { return GB_RESOLVE(struct gb_index_files_struct *,ixf,rel_if_next); }
inline GB_REL_IFES *                    GB_INDEX_FILES_ENTRIES(struct gb_index_files_struct *ifs)   { return GB_RESOLVE(GB_REL_IFES *,ifs,rel_entries); }
inline struct gb_index_files_struct *   GBCONTAINER_IFS(GBCONTAINER *gbc)                           { return GB_RESOLVE(struct gb_index_files_struct *,gbc,rel_ifs); }
inline char *                           GB_EXTERN_DATA_DATA(struct gb_extern_data& ex)              { return GB_RESOLVE(char*,(&(ex)),rel_data); }

#else

#define GB_HEADER_LIST_GBD(hl)          GB_RESOLVE(struct gb_data_base_type *,(&(hl)),rel_hl_gbd)
#define GB_DATA_LIST_HEADER(dl)         GB_RESOLVE(struct gb_header_list_struct *,(&(dl)),rel_header)
#define GB_IF_ENTRIES_NEXT(ie)          GB_RESOLVE(struct gb_if_entries *,ie,rel_ie_next)
#define GB_IF_ENTRIES_GBD(ie)           GB_RESOLVE(struct gb_data_base_type *,ie,rel_ie_gbd)
#define GB_INDEX_FILES_NEXT(ixf)        GB_RESOLVE(struct gb_index_files_struct *,ixf,rel_if_next)
#define GB_INDEX_FILES_ENTRIES(if)      GB_RESOLVE(GB_REL_IFES *,if,rel_entries)
#define GBCONTAINER_IFS(gbc)            GB_RESOLVE(struct gb_index_files_struct *,gbc,rel_ifs)
#define GB_EXTERN_DATA_DATA(ex)         GB_RESOLVE(char*,(&(ex)),rel_data)

#endif

/* -------------------------------------------------------------------------------- */

#ifdef __cplusplus

inline void SET_GB_HEADER_LIST_GBD(struct gb_header_list_struct& hl, GBDATA *gbd)                               { GB_SETREL(&hl, rel_hl_gbd, gbd); }
inline void SET_GB_DATA_LIST_HEADER(struct gb_data_list& dl, struct gb_header_list_struct *head)                { GB_SETREL(&dl, rel_header, head); }
inline void SET_GB_IF_ENTRIES_NEXT(struct gb_if_entries *ie, struct gb_if_entries *next)                        { GB_SETREL(ie, rel_ie_next, next); }
inline void SET_GB_IF_ENTRIES_GBD(struct gb_if_entries *ie, GBDATA *gbd)                                        { GB_SETREL(ie, rel_ie_gbd, gbd); }
inline void SET_GB_INDEX_FILES_NEXT(struct gb_index_files_struct *ixf, struct gb_index_files_struct *next)      { GB_SETREL(ixf, rel_if_next, next); }
inline void SET_GB_INDEX_FILES_ENTRIES(struct gb_index_files_struct *ixf, struct gb_if_entries **entries)        { GB_SETREL(ixf, rel_entries, entries); }
inline void SET_GBCONTAINER_IFS(GBCONTAINER *gbc, struct gb_index_files_struct *ifs)                            { GB_SETREL(gbc, rel_ifs, ifs); }
inline void SET_GB_EXTERN_DATA_DATA(struct gb_extern_data& ex, char *data)                                      { GB_SETREL(&ex, rel_data, data); }

#else

# define SET_GB_HEADER_LIST_GBD(hl,gbd)             GB_SETREL((&(hl)),rel_hl_gbd,gbd)
# define SET_GB_DATA_LIST_HEADER(dl,head)           GB_SETREL((&(dl)),rel_header,head)
# define SET_GB_IF_ENTRIES_NEXT(ie,next)            GB_SETREL(ie,rel_ie_next,next)
# define SET_GB_IF_ENTRIES_GBD(ie,gbd)              GB_SETREL(ie,rel_ie_gbd,gbd)
# define SET_GB_INDEX_FILES_NEXT(if,next)           GB_SETREL(if,rel_if_next,next)
# define SET_GB_INDEX_FILES_ENTRIES(if,entries)     GB_SETREL(if,rel_entries,entries)
# define SET_GBCONTAINER_IFS(gbc,ifs)               GB_SETREL(gbc,rel_ifs,ifs)
# define SET_GB_EXTERN_DATA_DATA(ex,data)           GB_SETREL((&(ex)),rel_data,data)

#endif

/* -------------------------------------------------------------------------------- */

#ifdef __cplusplus

inline GBCONTAINER* GB_FATHER(GBDATA *gbd)                                  { return GB_RESOLVE(struct gb_data_base_type2 *,gbd,rel_father); }
inline GBCONTAINER* GB_FATHER(GBCONTAINER *gbc)                             { return GB_RESOLVE(struct gb_data_base_type2 *,gbc,rel_father); }
inline void SET_GB_FATHER(GBDATA *gbd, GBCONTAINER *father)                 { GB_SETREL(gbd, rel_father, father); }
inline void SET_GB_FATHER(GBCONTAINER *gbc, GBCONTAINER *father)            { GB_SETREL(gbc, rel_father, father); }

inline GBCONTAINER* GB_GRANDPA(GBDATA *gbd)                                 { return GB_FATHER(GB_FATHER(gbd)); }

inline GBDATA *EXISTING_GBCONTAINER_ELEM(GBCONTAINER *gbc,int idx)          { return GB_HEADER_LIST_GBD(GB_DATA_LIST_HEADER((gbc)->d)[idx]); }
inline GBDATA *GBCONTAINER_ELEM(GBCONTAINER *gbc,int idx)                   { if (idx<gbc->d.nheader) return EXISTING_GBCONTAINER_ELEM(gbc, idx); return (GBDATA*)0; }
inline void SET_GBCONTAINER_ELEM(GBCONTAINER *gbc, int idx, GBDATA *gbd)    { SET_GB_HEADER_LIST_GBD(GB_DATA_LIST_HEADER(gbc->d)[idx], gbd); }

inline GB_MAIN_TYPE *GBCONTAINER_MAIN(GBCONTAINER *gbc)                     { return gb_main_array[(gbc->main_idx) % GB_MAIN_ARRAY_SIZE]; }
inline GB_MAIN_TYPE *GB_MAIN(GBDATA *gbd)                                   { return GBCONTAINER_MAIN(GB_FATHER(gbd)); }
inline GB_MAIN_TYPE *GB_MAIN(GBCONTAINER *gbc)                              { return GBCONTAINER_MAIN(gbc); }

#else

# define GB_FATHER(gbd)                     GB_RESOLVE(struct gb_data_base_type2 *,gbd,rel_father)
# define SET_GB_FATHER(gbd,father)          GB_SETREL(gbd,rel_father,father)

# define GB_GRANDPA(gbd)                    GB_FATHER(GB_FATHER(gbd))

# define EXISTING_GBCONTAINER_ELEM(gbc,idx) (GB_HEADER_LIST_GBD(GB_DATA_LIST_HEADER((gbc)->d)[idx]))
# define GBCONTAINER_ELEM(gbc,idx)          ((struct gb_data_base_type *)((idx)<(gbc)->d.nheader ? EXISTING_GBCONTAINER_ELEM(gbc, idx) : NULL))
# define SET_GBCONTAINER_ELEM(gbc,idx,gbd)  SET_GB_HEADER_LIST_GBD(GB_DATA_LIST_HEADER((gbc)->d)[idx],gbd)

# define GBCONTAINER_MAIN(gbc)              gb_main_array[((gbc)->main_idx) % GB_MAIN_ARRAY_SIZE]
# define GB_MAIN(gbd)                       GBCONTAINER_MAIN(GB_FATHER(gbd))

#endif

/********************* SOME FLAGS ******************/

#ifdef __cplusplus

inline int GB_KEY_QUARK(GBDATA *gbd)                        { return GB_DATA_LIST_HEADER(GB_FATHER(gbd)->d)[gbd->index].flags.key_quark; }
inline char *GB_KEY(GBDATA *gbd)                            { return GB_MAIN(gbd)->keys[GB_KEY_QUARK(gbd)].key; }
inline GB_TYPES GB_TYPE(GBDATA *gbd)                        { return GB_TYPES(gbd->flags.type); }
inline GB_TYPES GB_TYPE(GBCONTAINER *gbd)                   { return GB_TYPES(gbd->flags.type); }
inline GB_TYPES GB_TYPE_TS(struct gb_transaction_save *ts)  { return GB_TYPES(ts->flags.type); }

inline gb_header_flags& GB_ARRAY_FLAGS(GBDATA *gbd)         { return GB_DATA_LIST_HEADER(GB_FATHER(gbd)->d)[gbd->index].flags; }
inline gb_header_flags& GB_ARRAY_FLAGS(GBCONTAINER *gbc)    { return GB_DATA_LIST_HEADER(GB_FATHER(gbc)->d)[gbc->index].flags; }

#else

#define GB_KEY_QUARK(gbd) (GB_DATA_LIST_HEADER(GB_FATHER(gbd)->d)[(gbd)->index].flags.key_quark)
#define GB_KEY(gbd)       (GB_MAIN(gbd)->keys[GB_KEY_QUARK(gbd)].key)
#define GB_TYPE(gbd)      ((GB_TYPES)gbd->flags.type) // return type was 'int', changed 11.Mai.07 hopefully w/o harm --ralf
#define GB_TYPE_TS(ts)    ((GB_TYPES)ts->flags.type)

#define GB_ARRAY_FLAGS(gbd)         (GB_DATA_LIST_HEADER(GB_FATHER(gbd)->d)[(gbd)->index].flags)

#endif

/********************* INDEX ******************/

# define GB_GBM_INDEX(gbd) ((gbd)->flags2.gbm_index)

#ifdef __cplusplus

inline long GB_QUARK_2_GBMINDEX(GB_MAIN_TYPE *Main, int key_quark)  { return (Main->keys[key_quark].nref<GBM_MAX_UNINDEXED_ENTRIES) ? 0 : key_quark; }
inline long GB_GBD_2_GBMINDEX(GBDATA *gbd)                          { return GB_QUARK_2_GBMINDEX(GB_MAIN(gbd), GB_KEY_QUARK(gbd)); }

#else

# define GB_QUARK_2_GBMINDEX(Main,key_quark)        (((Main)->keys[key_quark].nref<GBM_MAX_UNINDEXED_ENTRIES) ? 0 : (key_quark))
# define GB_GBD_2_GBMINDEX(gbd)                     GB_QUARK_2_GBMINDEX(GB_MAIN(gbd), GB_KEY_QUARK(gbd))

#endif

/********************* READ and WRITE ******************/

#ifdef __cplusplus

inline GB_BOOL GB_COMPRESSION_ENABLED(GBDATA *gbd, long size) { return (!GB_MAIN(gbd)->compression_mask) && size>=GBTUM_SHORT_STRING_SIZE; }

inline void GB_TEST_TRANSACTION(GBDATA *gbd) {
    if (!GB_MAIN(gbd)->transaction) {
        GB_internal_error("no running transaction\nuse GB_transaction(gb_main)\n");
        GB_CORE;
    }
}

#else

#define GB_COMPRESSION_ENABLED(gbd,size) ((!GB_MAIN(gbd)->compression_mask) && (size)>=GBTUM_SHORT_STRING_SIZE)

#define GB_TEST_TRANSACTION(gbd)                                                            \
do {                                                                                        \
    if (!GB_MAIN(gbd)->transaction)                                                         \
    {                                                                                       \
        GB_internal_error("no running transaction\ncall GB_begin_transaction(gb_main)\n");  \
        GB_CORE;                                                                            \
    }                                                                                       \
} while(0)

#endif // __cplusplus

#define GB_TEST_READ(gbd,ty,error)                                                  \
    do {                                                                            \
        GB_TEST_TRANSACTION(gbd);                                                   \
        if (GB_ARRAY_FLAGS(gbd).changed == gb_deleted) {                            \
            GB_internal_error("%s: %s",error,"Entry is deleted !!");                \
            return 0;}                                                              \
        if ( GB_TYPE(gbd) != ty && (ty != GB_STRING || GB_TYPE(gbd) == GB_LINK)) {  \
            GB_internal_error("%s: %s",error,"wrong type");                         \
            return 0;}                                                              \
    } while(0)

#define GB_TEST_WRITE(gbd,ty,gerror)                                                \
    do {                                                                            \
        GB_TEST_TRANSACTION(gbd);                                                   \
        if ( GB_ARRAY_FLAGS(gbd).changed == gb_deleted) {                           \
            GB_internal_error("%s: %s",gerror,"Entry is deleted !!");               \
            return 0;}                                                              \
        if ( GB_TYPE(gbd) != ty && (ty != GB_STRING || GB_TYPE(gbd) != GB_LINK)) {  \
            GB_internal_error("%s: %s",gerror,"type conflict !!");                  \
            return 0;}                                                              \
        if (GB_GET_SECURITY_WRITE(gbd)>GB_MAIN(gbd)->security_level)                \
            return gb_security_error(gbd);                                          \
    } while(0)

#define GB_TEST_NON_BUFFER(x,gerror)                                            \
    do {                                                                        \
        if ( (x)== gb_local->buffer) {                                          \
            GB_export_error("%s:%s", gerror,                                    \
            "you are not allowed to write any data, which you get by pntr");    \
            GB_print_error();                                                   \
            GB_CORE; }                                                          \
    } while(0)

/********************* INDEX CHECK  ******************/

#ifdef __cplusplus

inline void GB_INDEX_CHECK_IN(GBDATA *gbd) { if (gbd->flags2.tisa_index) gb_index_check_in(gbd); }
inline void GB_INDEX_CHECK_OUT(GBDATA *gbd) { if ((gbd)->flags2.is_indexed) gb_index_check_out(gbd); }

#else

#define GB_INDEX_CHECK_IN(gbd)  do { if ((gbd)->flags2.tisa_index) gb_index_check_in(gbd); } while(0)
#define GB_INDEX_CHECK_OUT(gbd) do { if ((gbd)->flags2.is_indexed) gb_index_check_out(gbd); } while(0)

#endif // __cplusplus

/********************* EXTENDED DATA ******************/

#ifdef __cplusplus

inline struct gb_callback *GB_GET_EXT_CALLBACKS(GBDATA *gbd) { return gbd->ext ? gbd->ext->callback : 0; }
inline long GB_GET_EXT_CREATION_DATE(GBDATA *gbd) { return gbd->ext ? gbd->ext->creation_date : 0; }
inline long GB_GET_EXT_UPDATE_DATE(GBDATA *gbd) { return gbd->ext ? gbd->ext->update_date : 0; }
inline struct gb_transaction_save *GB_GET_EXT_OLD_DATA(GBDATA *gbd) { return gbd->ext ? gbd->ext->old : 0; }

inline struct gb_cache_struct& GB_GET_CACHE(GB_MAIN_TYPE *gbmain) { return gbmain->cache; }
inline void GB_SET_CACHE(GB_MAIN_TYPE *gbmain, struct gb_cache_struct& val) { gbmain->cache = val; }

inline void GB_CREATE_EXT(GBDATA *gbd) { if (!gbd->ext) gb_create_extended(gbd); }
inline void _GB_DELETE_EXT(GBDATA *gbd, long gbm_index) {
    if (gbd->ext) {
        gbm_free_mem((char *) gbd->ext, sizeof(struct gb_db_extended), gbm_index);
        gbd->ext = 0;
    }
}

#else

#define GB_GET_EXT_CALLBACKS(gbd)   (((gbd)->ext)?(gbd)->ext->callback:0)
#define GB_GET_EXT_CREATION_DATE(gbd)   (((gbd)->ext)?(gbd)->ext->creation_date:0)
#define GB_GET_EXT_UPDATE_DATE(gbd)     (((gbd)->ext)?(gbd)->ext->update_date:0)
#define GB_GET_EXT_OLD_DATA(gbd)    (((gbd)->ext)?(gbd)->ext->old:0)
#define GB_GET_CACHE(gbd)       ((gbd)->cache)
#define GB_SET_CACHE(gbd,val)       do { (gbd)->cache = (val); } while(0)


#define GB_CREATE_EXT(gbd)  do { if (!(gbd)->ext) gb_create_extended(gbd); } while(0)

#define _GB_DELETE_EXT(gbd, gbm_index)                                                  \
do {                                                                                    \
    if ((gbd)->ext) {                                                                   \
        gbm_free_mem((char *) (gbd)->ext, sizeof(struct gb_db_extended), gbm_index);    \
        (gbd)->ext = 0;                                                                 \
    }                                                                                   \
} while(0)

#endif

/********************* CALLBACKS  ******************/

#ifdef __cplusplus

inline void GB_DO_CALLBACKS(GBDATA *gbd) {
    if (GB_MAIN(gbd)->transaction<0) {
        GBDATA *_gbdn,*_gbd;
        for (_gbd= gbd;_gbd;_gbd=_gbdn) {
            struct gb_callback *cb,*cbn;
            _gbdn = GB_get_father(_gbd);
            for (cb = GB_GET_EXT_CALLBACKS(_gbd); cb; cb = cbn) {
                cbn = cb->next;
                if (cb->type & GB_CB_CHANGED) {
                    cb->func(_gbd,cb->clientdata,GB_CB_CHANGED);
                }
            }
        }
    }
}

#else

#define GB_DO_CALLBACKS(gbd)                                        \
do {                                                                \
    if (GB_MAIN(gbd)->transaction<0) {                              \
        GBDATA *_gbdn,*_gbd;                                        \
        for (_gbd= (gbd);_gbd;_gbd=_gbdn) {                         \
            struct gb_callback *cb,*cbn;                            \
            _gbdn = GB_get_father(_gbd);                            \
            for (cb = GB_GET_EXT_CALLBACKS(_gbd); cb; cb = cbn) {   \
                cbn = cb->next;                                     \
                if (cb->type & GB_CB_CHANGED) {                     \
                    cb->func(_gbd,cb->clientdata,GB_CB_CHANGED);    \
                }                                                   \
            }                                                       \
        }                                                           \
    }                                                               \
}while(0)

#endif

/********************* DATA ACCESS  ******************/

#ifdef __cplusplus

inline long GB_GETSIZE(const GBDATA *gbd)       { return gbd->flags2.extern_data ? gbd->info.ex.size     : gbd->info.istr.size; }
inline long GB_GETMEMSIZE(const GBDATA *gbd)    { return gbd->flags2.extern_data ? gbd->info.ex.memsize  : gbd->info.istr.memsize; }
inline char *GB_GETDATA(GBDATA *gbd)            { return gbd->flags2.extern_data ? GB_EXTERN_DATA_DATA(gbd->info.ex)  : &((gbd)->info.istr.data[0]); }

inline long GB_GETSIZE_TS(struct gb_transaction_save *ts)       { return ts->flags2.extern_data ? ts->info.ex.size     : ts->info.istr.size; }
inline long GB_GETMEMSIZE_TS(struct gb_transaction_save *ts)    { return ts->flags2.extern_data ? ts->info.ex.memsize  : ts->info.istr.memsize; }
inline char *GB_GETDATA_TS(struct gb_transaction_save *ts)      { return ts->flags2.extern_data ? ts->info.ex.data     : &(ts->info.istr.data[0]); }

#else

#define GB_GETSIZE(gbd)     (((gbd)->flags2.extern_data)    ? (gbd)->info.ex.size  : (gbd)->info.istr.size)
#define GB_GETMEMSIZE(gbd)  (((gbd)->flags2.extern_data)    ? (gbd)->info.ex.memsize  : (gbd)->info.istr.memsize)
#define GB_GETDATA(gbd)     (((gbd)->flags2.extern_data)    ? GB_EXTERN_DATA_DATA((gbd)->info.ex)  : &((gbd)->info.istr.data[0]))

#define GB_GETSIZE_TS(ts)       GB_GETSIZE(ts)
#define GB_GETMEMSIZE_TS(ts)    GB_GETMEMSIZE(ts)
#define GB_GETDATA_TS(ts)   (((ts)->flags2.extern_data) ? ts->info.ex.data : & ((ts)->info.istr.data[0]))

#endif

/********************* DATA MODIFY  ******************/

#ifdef __cplusplus

inline void GB_FREE_TRANSACTION_SAVE(GBDATA *gbd) {
    if (gbd->ext && gbd->ext->old) {
        gb_del_ref_gb_transaction_save(gbd->ext->old);
        gbd->ext->old = NULL;
    }
}

/* frees local data */
inline void GB_FREEDATA(GBDATA *gbd) {
    GB_INDEX_CHECK_OUT(gbd);
    if (gbd->flags2.extern_data && GB_EXTERN_DATA_DATA(gbd->info.ex)) {
        gbm_free_mem(GB_EXTERN_DATA_DATA(gbd->info.ex), (size_t)(gbd->info.ex.memsize), GB_GBM_INDEX(gbd));
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,0);
    }
}

#else

#define GB_FREE_TRANSACTION_SAVE(gbd)                       \
do {                                                        \
    if ((gbd)->ext && (gbd)->ext->old) {                    \
        gb_del_ref_gb_transaction_save((gbd)->ext->old);    \
        (gbd)->ext->old = NULL;                             \
    }                                                       \
} while(0)

/* frees local data */
#define GB_FREEDATA(gbd)                                                                                        \
do {                                                                                                            \
    GB_INDEX_CHECK_OUT(gbd);                                                                                    \
    if ((gbd)->flags2.extern_data && GB_EXTERN_DATA_DATA((gbd)->info.ex)) {                                     \
        gbm_free_mem(GB_EXTERN_DATA_DATA((gbd)->info.ex), (size_t)((gbd)->info.ex.memsize), GB_GBM_INDEX(gbd)); \
        SET_GB_EXTERN_DATA_DATA((gbd)->info.ex,0);                                                              \
    }                                                                                                           \
} while(0)

#endif

#ifdef __cplusplus

inline GB_BOOL GB_CHECKINTERN(int size, int memsize) { return size<256 && memsize<SIZOFINTERN; }
inline void GB_SETINTERN(GBDATA *gbd) { gbd->flags2.extern_data = 0; }
inline void GB_SETEXTERN(GBDATA *gbd) { gbd->flags2.extern_data = 1; }

#else

#define GB_CHECKINTERN(size,memsize)    (((int)(size) < 256) && ((int)(memsize) < SIZOFINTERN))
#define GB_SETINTERN(gbd)       (gbd)->flags2.extern_data = 0
#define GB_SETEXTERN(gbd)       (gbd)->flags2.extern_data = 1

#endif

    /** insert external data into any db. field
        Warning: this function has a lot of side effects:
        1. extern_data must be set by the user before calling this
        2. if !extern_data the data is not set
        -> better use GB_SETSMDMALLOC
    */

#ifdef __cplusplus

inline void GB_SETSMD(GBDATA *gbd, long siz, long memsiz, char *dat) {
    if (gbd->flags2.extern_data) {
        gbd->info.ex.size = siz;
        gbd->info.ex.memsize = memsiz;
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,dat);
    }else{
        gbd->info.istr.size = (unsigned char)siz;
        gbd->info.istr.memsize = (unsigned char)memsiz;
    }
    GB_INDEX_CHECK_IN(gbd);
}

inline void GB_SETSMDMALLOC(GBDATA *gbd, long siz, long memsiz, char *dat) {
    if (GB_CHECKINTERN(siz,memsiz)) {
        GB_SETINTERN(gbd);
        gbd->info.istr.size = (unsigned char)siz;
        gbd->info.istr.memsize = (unsigned char)memsiz;
        if (dat) GB_MEMCPY(&(gbd->info.istr.data[0]), (char *)dat, (size_t)(memsiz));
    }else{
        char *exData;
        GB_SETEXTERN(gbd);
        gbd->info.ex.size = siz;
        gbd->info.ex.memsize = memsiz;
        exData = gbm_get_mem((size_t)memsiz,GB_GBM_INDEX(gbd));
        SET_GB_EXTERN_DATA_DATA(gbd->info.ex,exData);
        if (dat) GB_MEMCPY(exData, (char *)dat, (size_t)(memsiz));
    }
    GB_INDEX_CHECK_IN(gbd);
}

#else

#define GB_SETSMD(gbd,siz,memsiz,dat)                       \
do {                                                        \
    if ((gbd)->flags2.extern_data) {                        \
        (gbd)->info.ex.size = (siz);                        \
        (gbd)->info.ex.memsize = (memsiz);                  \
        SET_GB_EXTERN_DATA_DATA((gbd)->info.ex,(dat));      \
    }else{                                                  \
        (gbd)->info.istr.size = (unsigned char)(siz);       \
        (gbd)->info.istr.memsize = (unsigned char)(memsiz); \
    }                                                       \
    GB_INDEX_CHECK_IN(gbd);                                 \
} while(0)

    /** copies any data into any db field */
#define GB_SETSMDMALLOC(gbd,siz,memsiz,dat)                                                 \
do {                                                                                        \
    if (GB_CHECKINTERN(siz,memsiz)) {                                                       \
        GB_SETINTERN(gbd);                                                                  \
        (gbd)->info.istr.size = (unsigned char)(siz);                                       \
        (gbd)->info.istr.memsize = (unsigned char)(memsiz);                                 \
        if (dat) GB_MEMCPY(&((gbd)->info.istr.data[0]), (char *)(dat), (size_t)(memsiz));   \
    }else{                                                                                  \
        char *exData;                                                                       \
        GB_SETEXTERN(gbd);                                                                  \
        (gbd)->info.ex.size = (siz);                                                        \
        (gbd)->info.ex.memsize = (memsiz);                                                  \
        exData = gbm_get_mem((size_t)(memsiz),GB_GBM_INDEX(gbd));                           \
        SET_GB_EXTERN_DATA_DATA((gbd)->info.ex,exData);                                     \
        if (dat) GB_MEMCPY(exData, (char *)(dat), (size_t)(memsiz));                        \
    }                                                                                       \
    GB_INDEX_CHECK_IN(gbd);                                                                 \
}while(0)


    /**     copy all info + external data mem
        to an one step undo buffer (to abort a transaction */

#endif

/********************* UNDO  ******************/

#ifdef __cplusplus

inline void _GB_CHECK_IN_UNDO_DELETE(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    if (Main->undo_type) gb_check_in_undo_delete(Main,gbd,0);
    else gb_delete_entry(gbd);
}
inline void _GB_CHECK_IN_UNDO_CREATE(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    if (Main->undo_type) gb_check_in_undo_create(Main,gbd);
}
inline void _GB_CHECK_IN_UNDO_MODIFY(GB_MAIN_TYPE *Main, GBDATA *gbd) {
    if (Main->undo_type) gb_check_in_undo_modify(Main,gbd);
}
inline void STATIC_BUFFER(char*& strvar, int minlen) {
    if (strvar && (long)strlen(strvar) < (minlen-1)) { free((void*)(strvar)); strvar=NULL; }
    if (!strvar) strvar=(char*)GB_calloc(minlen,1);
}

#else

#define _GB_CHECK_IN_UNDO_DELETE(Main,gbd)                      \
do {                                                            \
    if ((Main)->undo_type) gb_check_in_undo_delete(Main,gbd,0); \
    else gb_delete_entry(gbd);                                  \
} while(0)

#define _GB_CHECK_IN_UNDO_CREATE(Main,gbd)                      \
do {                                                            \
    if ((Main)->undo_type) gb_check_in_undo_create(Main,gbd);   \
} while(0)

#define _GB_CHECK_IN_UNDO_MODIFY(Main,gbd)                      \
do {                                                            \
    if ((Main)->undo_type) gb_check_in_undo_modify(Main,gbd);   \
} while(0)

#define STATIC_BUFFER(strvar,static_buffer_minlen)                                                  \
do {                                                                                                \
    int static_buffer_len = (static_buffer_minlen);                                                 \
    if ((strvar) && (long)strlen(strvar) < (static_buffer_len-1)) { free(strvar); (strvar)=NULL; }  \
    if (!(strvar)) (strvar)=(char*)GB_calloc(static_buffer_len,1);                                  \
} while(0)

#endif
