enum {                          // JAVA HEADERS
    T2J_NLR_INDEXING   = 10,
    T2J_LEVEL_INDEXING = 11
};

struct T2J_item {
    char *key;
    int   color;
    char *value;
};


struct T2J_transfer_struct {
    T2J_transfer_struct(long size, enum CAT_FIELDS key_index);

    enum CAT_FIELDS key_index;  // which entry should be used
    
    // as a key
    long             nitems;
    struct T2J_item *items;
};
