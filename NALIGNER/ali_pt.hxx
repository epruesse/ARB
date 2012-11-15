
#ifndef _ALI_PT_INC_
#define _ALI_PT_INC_

#include "ali_other_stuff.hxx"
#include "ali_tlist.hxx"
#include "ali_sequence.hxx"
#include <client.h>

typedef enum {ServerMode, SpecifiedMode} ALI_PT_MODE;

typedef struct {
    char *servername;
    GBDATA *gb_main;

    int matches_min;
    float percent_min;
    unsigned long fam_list_max;
    unsigned long ext_list_max;

    char *use_specified_family;
} ALI_PT_CONTEXT;

/*
 * class of family members
 */
class ali_pt_member {
public:
    char *name;
    int matches;

    ali_pt_member(char *speciesname, int number_of_matches) {
        name = speciesname;
        matches = number_of_matches;
    }
    ~ali_pt_member(void) {
        if (name)
            free((char *) name);
    }
};


/*
 * Class for accessing the PT server
 */
class ALI_PT {
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


    int init_communication(void);
    char *get_family_member(char *specified_family, unsigned long number);
    char *get_extension_member(char *specified_family, unsigned long number);
    int open(char *servername,GBDATA *gb_main);
    void close(void);

public:
    ALI_PT(ALI_PT_CONTEXT *context);
    ~ALI_PT(void);

    int find_family(ALI_SEQUENCE *sequence, int find_type = 1);

    ALI_TLIST<ali_pt_member *> *get_family_list();
    ALI_TLIST<ali_pt_member *> *get_extension_list();

    int first_family_(char **seq_name, int *matches);
    int next_family_(char **seq_name, int *matches);
};

#endif
