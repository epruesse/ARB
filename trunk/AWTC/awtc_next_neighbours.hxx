#ifndef awtc_next_neighbours_hxx_included
#define awtc_next_neighbours_hxx_included

class AWTC_FIND_FAMILY_MEMBER {
    // list is sorted either by 'matches' or 'rel_matches' (descending)
    // depending on 'rel_matches' paramater to findFamily() 
public:
    AWTC_FIND_FAMILY_MEMBER *next;

    char   *name;
    long    matches;
    double  rel_matches;

    AWTC_FIND_FAMILY_MEMBER();
    ~AWTC_FIND_FAMILY_MEMBER();
};

struct struct_aisc_com;

enum FF_complement {
    FF_FORWARD            = 1,
    FF_REVERSE            = 2,
    FF_REVERSE_COMPLEMENT = 4,
    FF_COMPLEMENT         = 8,
    // do NOT change the order here or change ff_find_family()
};

class AWTC_FIND_FAMILY {
    struct_aisc_com *link;
    GBDATA          *gb_main;
    long             com;
    long             locs;
    
    void     delete_family_list();
    GB_ERROR init_communication(void);
    GB_ERROR open(char *servername);
    GB_ERROR retrieve_family(char *sequence, int oligo_len, int mismatches, bool fast_flag, bool rel_matches, FF_complement compl_mode, int max_results);
    void     close();

    // valid after calling retrieve_family():
    AWTC_FIND_FAMILY_MEMBER *family_list;

    bool hits_truncated;
    int  real_hits;

public:

    AWTC_FIND_FAMILY(GBDATA *gb_maini);
    ~AWTC_FIND_FAMILY();

    GB_ERROR findFamily(int server_id,char *sequence, int oligo_len, int mismatches, bool fast_flag, bool rel_matches, FF_complement compl_mode, int max_results);

    const AWTC_FIND_FAMILY_MEMBER *getFamilyList() const { return family_list; }
    bool hits_were_truncated() const { return hits_truncated; }
    int getRealHits() const { return real_hits; }

    void print();
};

#endif
