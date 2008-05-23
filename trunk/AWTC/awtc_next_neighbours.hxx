#ifndef awtc_next_neighbours_hxx_included
#define awtc_next_neighbours_hxx_included

class AWTC_FIND_FAMILY_MEMBER {
public:
    AWTC_FIND_FAMILY_MEMBER();
    AWTC_FIND_FAMILY_MEMBER *next;
    char	*name;
    long	matches;
    ~AWTC_FIND_FAMILY_MEMBER();
};

struct struct_aisc_com;

class AWTC_FIND_FAMILY {
    struct struct_aisc_com *link;
    GBDATA *gb_main;
    long com;
    long locs;
    void delete_family_list();
    GB_ERROR init_communication(void);
    GB_ERROR open(char *servername);
    GB_ERROR find_family(char *sequence, int find_type, int max_hits);
    void close();
    
public:
    AWTC_FIND_FAMILY_MEMBER *family_list;
    
    AWTC_FIND_FAMILY(GBDATA *gb_maini);

    GB_ERROR go(int server_id,char *sequence, bool fast_flag, int max_hits);
    
    void print();
    ~AWTC_FIND_FAMILY();
};

#endif
