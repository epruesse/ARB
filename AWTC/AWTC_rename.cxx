#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "awtc_rename.hxx"

#include <names_client.h>
#include <servercntrl.h>
#include <client.h>

//  -----------------------------------
//      class NameServerConnection
//  -----------------------------------

class NameServerConnection {
private:
    aisc_com *link;
    T_AN_LOCAL locs;
    T_AN_MAIN com;

    //  ----------------------------------
    //      int init_local_com_names()
    //  ----------------------------------
    int init_local_com_names()
    {
        if (!link) return 1;    /*** create and init local com structure ***/
        if (aisc_create(link, AN_MAIN, com,
                        MAIN_LOCAL, AN_LOCAL, &locs,
                        LOCAL_WHOAMI, "i bin der arb_tree",
                        NULL)){
            return 1;
        }
        return 0;
    }

    NameServerConnection(const NameServerConnection& other);
    NameServerConnection& operator=(const NameServerConnection& /*other*/);

public:

    NameServerConnection() {
        link = 0;
        locs = 0;
        com = 0;
    }
    virtual ~NameServerConnection() {
        disconnect();
    }

    GB_ERROR connect(GBDATA *gb_main) {
        GB_ERROR err = 0;
        if (!link) {
            const char *name_server = "ARB_NAME_SERVER";

            if (arb_look_and_start_server(AISC_MAGIC_NUMBER,name_server,gb_main)) {
                err = "Sorry I can't start the NAME SERVER";
            }
            else {
                char *servername = (char *)GBS_read_arb_tcp(name_server);
                if (!servername) {
                    GB_CORE;
                    exit (-1);
                }

                link = (aisc_com *)aisc_open(servername, &com,AISC_MAGIC_NUMBER);
                if (init_local_com_names()) err = "Sorry I can't start the NAME SERVER";
                free(servername);
            }
        }
        return err;
    }

    void disconnect() {
        if (link) {
            aisc_close(link);
        }
        link = 0;
    }

    aisc_com *getLink() { return link; }
    T_AN_LOCAL getLocs() { return locs; }
};

static NameServerConnection name_server;


GB_ERROR AWTC_generate_one_name(GBDATA *gb_main, const char *full_name, const char *acc, char*& new_name, bool openstatus) {
    // create a unique short name for 'full_name'
    // the result is written into 'new_name' (as malloc-copy)
    // if fails: GB_ERROR!=0 && new_name==0
    // acc may be 0

    new_name = 0;
    if (!acc) acc = "";

    if (openstatus) aw_openstatus(GBS_global_string("Short name for '%s'", full_name));

    aw_status("Connecting to name server");
    aw_status((double)0);

    GB_ERROR err = name_server.connect(gb_main);
    if (err) return err;

    aw_status("Generating name");
    static char *shrt = 0;
    if (strlen(full_name)) {
        if (aisc_nput(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                      LOCAL_FULL_NAME,  full_name,
                      LOCAL_ACCESSION,  acc,
                      LOCAL_ADVICE,     "",
                      0)){
            err = "Connection Problems with the NAME_SERVER";
        }
        if (aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                     LOCAL_GET_SHORT,   &shrt,
                     0)){
            err = "Connection Problems with the NAME_SERVER";
        }
    }

    if (err) {
        free(shrt);
    }
    else {
        if (shrt) {
            new_name = shrt;
            shrt = 0;
        }
        else {
            err = GB_export_error("Generation of short name for '%s' failed", full_name);
        }
    }

    if (openstatus) aw_closestatus();
    name_server.disconnect();

    return err;
}


GB_ERROR AWTC_pars_names(GBDATA *gb_main, int update_status)
{
    GB_ERROR err = name_server.connect(gb_main);
    if (!err) {
        err = GBT_begin_rename_session(gb_main,1);
        if (!err) {
            char     *ali_name = GBT_get_default_alignment(gb_main);
            GB_HASH  *hash     = GBS_create_hash(GBS_SPECIES_HASH_SIZE,1);
            GB_ERROR  warning  = 0;
            long      spcount  = 0;
            long      count    = 0;

            if (update_status) spcount = GBT_count_species(gb_main);

            for (GBDATA *gb_species = GBT_first_species(gb_main);
                 gb_species && !err;
                 gb_species = GBT_next_species(gb_species))
            {
                if (update_status) aw_status(count++/(double)spcount);

                GBDATA *gb_full_name = GB_find(gb_species,"full_name",0,down_level);
                GBDATA *gb_name      = GB_find(gb_species,"name",0,down_level);
                GBDATA *gb_acc       = GBT_gen_accession_number(gb_species,ali_name);

                char *acc       = gb_acc ? GB_read_string(gb_acc) : strdup("");
                char *full_name = gb_full_name ? GB_read_string(gb_full_name) : strdup("");
                char *name      = GB_read_string(gb_name);

                /*static*/ char *shrt = 0;

                if (strlen(acc) + strlen(full_name) ) {
                    if (aisc_nput(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                                  LOCAL_FULL_NAME,  full_name,
                                  LOCAL_ACCESSION,  acc,
                                  LOCAL_ADVICE,     name,
                                  0)){
                        err = "Connection Problems with the NAME_SERVER";
                    }
                    if (aisc_get(name_server.getLink(), AN_LOCAL, name_server.getLocs(),
                                 LOCAL_GET_SHORT,   &shrt,
                                 0)){
                        err = "Connection Problems with the NAME_SERVER";
                    }
                }
                else {
                    shrt = strdup(name);
                }
                if (!err) {
                    if (GBS_read_hash(hash,shrt)) {
                        char *newshrt;
                        int i;
                        newshrt = (char *)GB_calloc(sizeof(char),strlen(shrt)+10);
                        for (i= 1 ; ; i++) {
                            sprintf(newshrt,"%s.%i",shrt,i);
                            if (!GBS_read_hash(hash,newshrt))break;
                        }
                        warning = "There are duplicated entries!!.\nThe duplicated entries contain a '.' character !!!";
                        free(shrt); shrt = newshrt;
                    }
                    GBS_incr_hash(hash,shrt);
                    err = GBT_rename_species(name,shrt);
                }

                if (name)       free(name);
                if (acc)        free(acc);
                if (full_name)  free(full_name);

                free(shrt);
            }

            if (err) {
                GBT_abort_rename_session();
            }
            else {
                aw_status("Renaming species in trees");
                aw_status((double)0);
                GBT_commit_rename_session(aw_status);
            }

            GBS_free_hash(hash);
            free(ali_name);

            if (!err) err = warning;
        }
        name_server.disconnect();
    }

    return err;
}


void awt_rename_cb(AW_window *aww,GBDATA *gb_main)
{
    AWUSE(aww);
    //  int use_advice = (int)aww->get_root()->awar(AWT_RENAME_USE_ADVICE)->read_int();
    //  int save_data  = (int)aww->get_root()->awar(AWT_RENAME_SAVE_DATA)->read_int();
    aw_openstatus("Generating new names");
    GB_ERROR error     = AWTC_pars_names(gb_main,1);
    aw_closestatus();
    if (error) aw_message(error);
    
    aww->get_root()->awar(AWAR_TREE_REFRESH)->touch();
}


AW_window *AWTC_create_rename_window(AW_root *root, AW_CL gb_main)
{
    AWUSE(root);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "AUTORENAME_SPECIES", "AUTORENAME SPECIES");

    aws->load_xfig("awtc/autoren.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"rename.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP","H");

    aws->at("go");
    aws->highlight();
    aws->callback((AW_CB1)awt_rename_cb,gb_main);
    aws->create_button("GO", "GO","G");

    //  aws->at("advice");
    //  aws->create_option_menu(AWT_RENAME_USE_ADVICE,0,0);
    //  aws->insert_option("Create totally new names","n",0);
    //  aws->insert_default_option("Use Advice","U",1);
    //  aws->update_option_menu();

    //  aws->at("save");
    //  aws->create_option_menu(AWT_RENAME_SAVE_DATA,0,0);
    //  aws->insert_option("dont update","f",0);
    //  aws->insert_default_option("update $ARBHOME/lib/nas/names.dat","s",1);
    //  aws->update_option_menu();

    return (AW_window *)aws;
}

void AWTC_create_rename_variables(AW_root *root,AW_default db1){
    root->awar_int( AWT_RENAME_USE_ADVICE, 0  ,     db1);
    root->awar_int( AWT_RENAME_SAVE_DATA, 1  ,  db1);
}


//  ----------------------------------------------------------------------------------
//      inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data)
//  ----------------------------------------------------------------------------------
inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data) {
    return GBT_find_species_rel_species_data(gb_species_data, short_name)==0;
}

//  --------------------------------------------------------------------------------------
//      static char *makeUniqueShortName(const char *prefix, GBDATA *gb_species_data)
//  --------------------------------------------------------------------------------------
static char *makeUniqueShortName(const char *prefix, GBDATA *gb_species_data) {
    // generates a non-existing short-name (name starts with prefix)
    // returns 0 if failed

    char *result = 0;

    int prefix_len = strlen(prefix);
    gb_assert(prefix_len<8); // short name will be 8 chars - prefix has to be shorter!

    int max_num = 1;
    for (int l=prefix_len+1; l<=8; ++l) max_num *= 10; // calculate max possibilities

    if (max_num>1) {
        char short_name[9];
        strcpy(short_name, prefix);
        char *dig_pos = short_name+prefix_len;

        for (int x = 0; x<max_num; ++x) {
            sprintf(dig_pos, "%i", x);

            if (nameIsUnique(short_name, gb_species_data))  {
                result = strdup(short_name);
                break;
            }
        }
    }
    return result;
}

//  ------------------------------------------------------------------------------------
//      char *AWTC_makeUniqueShortName(const char *prefix, GBDATA *gb_species_data)
//  ------------------------------------------------------------------------------------
char *AWTC_makeUniqueShortName(const char *prefix, GBDATA *gb_species_data) {
    // generates a unique species name from prefix
    // (prefix will be fillup with '_00..' and the shortened down to first char)
    // returns 0 if fails

    int  len = strlen(prefix);
    gb_assert(len <= 8);

    char p[9];
    strcpy(p, prefix);

    if (len <= 6) p[len++] = '_';
    while (len<8) p[len++] = '0';

    p[len] = 0;

    char *result = 0;

    for (int l = len-1; l>0 && !result; --l) {
        p[l]   = 0;
        result = makeUniqueShortName(p, gb_species_data);
    }

    return result;
}

//  -----------------------------------------------------------------
//      char *AWTC_generate_random_name(GBDATA *gb_species_data)
//  -----------------------------------------------------------------
char *AWTC_generate_random_name(GBDATA *gb_species_data) {
    char *new_species_name = 0;
    char  short_name[9];
    short_name[8]          = 0;
    int   count            = 1000;

    while (count--) {
        for (int x=0; x<8; ++x) {
            int r = int((double(rand())/RAND_MAX)*36);
            short_name[x] = r<10 ? ('0'+r) : ('a'+r-10);
        }

        if (nameIsUnique(short_name, gb_species_data))  {
            new_species_name = strdup(short_name);
            break;
        }
    }

    return new_species_name;
}
