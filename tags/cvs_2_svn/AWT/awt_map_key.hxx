#ifndef awt_map_key_hxx_included
#define awt_map_key_hxx_included

#define MAX_MAPPED_KEYS 20

class ed_key {
    char map[256];

public:
    ed_key(void);

    char        map_key(char);
    void        create_awars(AW_root *root);
    friend void ed_rehash_mapping(AW_root *awr, ed_key *ek);
};

AW_window *create_key_map_window(AW_root *root);


#endif
