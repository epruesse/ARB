#define HELIX_MAX_NON_ST 10

#define HELIX_AWAR_PAIR_TEMPLATE "Helix/pairs/%s"
#define HELIX_AWAR_SYMBOL_TEMPLATE "Helix/symbols/%s"

typedef enum {
    HELIX_NONE,
    HELIX_STRONG_PAIR,
    HELIX_PAIR,
    HELIX_WEAK_PAIR,
    HELIX_NO_PAIR,
    HELIX_USER0,
    HELIX_USER1,
    HELIX_USER2,
    HELIX_USER3,
    HELIX_DEFAULT,
    HELIX_NON_STANDART0,
    HELIX_NON_STANDART1,
    HELIX_NON_STANDART2,
    HELIX_NON_STANDART3,
    HELIX_NON_STANDART4,
    HELIX_NON_STANDART5,
    HELIX_NON_STANDART6,
    HELIX_NON_STANDART7,
    HELIX_NON_STANDART8,
    HELIX_NON_STANDART9,
    HELIX_NO_MATCH,
    HELIX_MAX
} BI_PAIR_TYPE;


class BI_helix {
    private:
    int _check_pair(char left, char right, BI_PAIR_TYPE pair_type);
    public:
        // read only section
    struct BI_helix_entry {
        long pair_pos;
        BI_PAIR_TYPE pair_type;
        char    *helix_nr;
    } *entries;
    // **read only:
    char    *pairs[HELIX_MAX];
    char    *char_bind[HELIX_MAX];
    long size;
    static char error[256];

    // ***** read and write
#ifdef _USE_AW_WINDOW
    BI_helix(AW_root *aw_root);
#else
    BI_helix(void);
#endif
    ~BI_helix(void);

    char *init(GBDATA *gb_main);
    char *init(GBDATA *gb_main, char *alignment_name, char *helix_nr_name, char *helix_name);
    char *init(GBDATA *gb_helix_nr,GBDATA *gb_helix,long size);
    char *init(char *helix_nr, char *helix, long size);

    int check_pair(char left, char right, BI_PAIR_TYPE pair_type); // returns 1 if bases form a pair

#ifdef _USE_AW_WINDOW
    char get_symbol(char left, char right, BI_PAIR_TYPE pair_type);

    int show_helix( void *device, int gc1 , char *sequence,
        AW_pos x, AW_pos y,
        AW_bitset filter,
        AW_CL cd1, AW_CL cd2);
#endif
};

#ifdef _USE_AW_WINDOW
AW_window *create_helix_props_window(AW_root *awr, AW_cb_struct * /*owner*/awcbs);
#endif
