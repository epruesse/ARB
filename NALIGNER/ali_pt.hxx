// =============================================================== //
//                                                                 //
//   File      : ali_pt.hxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ALI_PT_HXX
#define ALI_PT_HXX

#ifndef ALI_OTHER_STUFF_HXX
#include "ali_other_stuff.hxx"
#endif
#ifndef ALI_TLIST_HXX
#include "ali_tlist.hxx"
#endif
#ifndef ALI_SEQUENCE_HXX
#include "ali_sequence.hxx"
#endif
#ifndef CLIENT_H
#include <client.h>
#endif

enum ALI_PT_MODE { ServerMode, SpecifiedMode };

struct ALI_PT_CONTEXT {
    char *servername;
    GBDATA *gb_main;

    int matches_min;
    float percent_min;
    unsigned long fam_list_max;
    unsigned long ext_list_max;

    char *use_specified_family;
};

// class of family members
struct ali_pt_member : virtual Noncopyable {
    char *name;
    int matches;

    ali_pt_member(char *speciesname, int number_of_matches) {
        name = speciesname;
        matches = number_of_matches;
    }
    ~ali_pt_member() {
        if (name)
            free((char *) name);
    }
};


// Class for accessing the PT server
class ALI_PT : virtual Noncopyable {
private:
    ALI_PT_MODE mode;

    char *specified_family;
    unsigned long fam_list_max;
    unsigned long ext_list_max;
    float percent_min;
    int matches_min;

    aisc_com *link;
    T_PT_LOCS locs;
    T_PT_MAIN com;

    ALI_TLIST<ali_pt_member *> *family_list;
    ALI_TLIST<ali_pt_member *> *extension_list;


    int init_communication();
    char *get_family_member(char *specified_family, unsigned long number);
    char *get_extension_member(char *specified_family, unsigned long number);
    int open(char *servername);
    void close();

public:
    ALI_PT(ALI_PT_CONTEXT *context);
    ~ALI_PT();

    int find_family(ALI_SEQUENCE *sequence, int find_type = 1);

    ALI_TLIST<ali_pt_member *> *get_family_list();
    ALI_TLIST<ali_pt_member *> *get_extension_list();

    int first_family_(char **seq_name, int *matches);
    int next_family_(char **seq_name, int *matches);
};

#else
#error ali_pt.hxx included twice
#endif // ALI_PT_HXX
