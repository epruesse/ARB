enum g_b_undo_entry_type {
        GB_UNDO_ENTRY_TYPE_DELETED,
        GB_UNDO_ENTRY_TYPE_CREATED,
        GB_UNDO_ENTRY_TYPE_MODIFY,
        GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY
    };

struct g_b_undo_gbd_struct {
    GBQUARK         key;
    struct gb_data_base_type *gbd;
};

struct g_b_undo_struct;
struct g_b_undo_entry_struct {
    struct g_b_undo_struct *father;
    struct g_b_undo_entry_struct *next;
    short type;
    short flag;

    struct gb_data_base_type    *source;
    int             gbm_index;
            /* The original(changed element) or father */
    long                sizeof_this;
    union {
    struct gb_transaction_save *ts;
    struct g_b_undo_gbd_struct gs;
    } d;
};

struct g_b_undo_header_struct;
struct g_b_undo_struct {
    struct g_b_undo_header_struct   *father;
    struct g_b_undo_entry_struct    *entries;
    struct g_b_undo_struct      *next;
    long        time_of_day;    /* the begin of the transaction */
    long        sizeof_this;    /* the size of one undo */
};

struct g_b_undo_header_struct {
    struct g_b_undo_struct *stack;
    long        sizeof_this;    /* the size of all existing undos */
    long        nstack;     /* number of available undos */
};

struct g_b_undo_mgr_struct {
    long            max_size_of_all_undos;
    struct g_b_undo_struct      *valid_u;
    struct g_b_undo_header_struct   *u; /* undo */
    struct g_b_undo_header_struct   *r; /* redo */
};


