#ifndef seer_hxx_included
#define seer_hxx_included

class AW_window;
class AW_root;
class SeerInterface;
typedef long AW_CL;
AW_window *SEER_create_login_window(AW_root *aw_root,AW_CL);

#define AWAR_SEER_USER_NAME            "tmp/seer/login/username"
#define AWAR_SEER_USER_PASSWD          "tmp/seer/login/userpasswd"
#define AWAR_SEER_ALIGNMENT_NAME       "tmp/seer/alignment_name"
#define AWAR_SEER_TABLE_PREFIX         "tmp/seer/table"
#define AWAR_SEER_TABLE_LOAD_BEHAVIOUR "tmp/seer/table/load_behaviour"
#define AWAR_SEER_LOAD_SEQUENCES       "tmp/seer/table/load_sequences"
#define AWAR_SEER_QUERY_ATTRIBUTE      "tmp/seer/query/attribute"
#define AWAR_SEER_QUERY_STRING         "tmp/seer/query/query_string"

#define AWAR_SEER_UPLOAD_WHAT         "tmp/seer/upload/what"
#define AWAR_SEER_UPLOAD_TYPE         "tmp/seer/upload/type"
#define AWAR_SEER_SAVE_SEQUENCES      "tmp/seer/upload/sequences"
#define AWAR_SEER_SAVE_SAI            "tmp/seer/upload/SAIs"
#define AWAR_SEER_UPLOAD_TABLE_PREFIX "tmp/seer/upload/table"



#define SEER_KEY_FLAG "_seer_loaded_" // indicate source of datastructure

enum SEER_OPENS_ARB_TYPE {
    SEER_OPENS_NEW_ARB,
    SEER_OPENS_ARB_FULL,
    SEER_OPENS_ARB_SKELETON
};

enum SEER_TABLE_LOAD_BEHAVIOUR {
    SEER_TABLE_LOAD_AT_STARTUP = 0,
    SEER_TABLE_LOAD_ON_DEMAND  = 1
};

typedef struct gbs_hash_struct GB_HASH;
class AW_selection_list;

enum SEER_SAVE_TYPE {
    SST_ALL,                    // export everything to Seer
    SST_SINCE_LOGIN,            // export all changes since login
    SST_QUICK_FILE              // export all changes since big file
};

enum SEER_SAVE_WHAT_TYPE {
    SSWT_ALL    = 0,
    SSWT_MARKED = 1
};

struct SEER_GLOBAL {
    long        transaction_counter_at_login;
    long        save_transaction_counter_limit; // everything which is older will not be saved
    GB_BOOL     save_external_data_too;
    GB_BOOL     delayed_load_of_tables; // initialized by populate tables

    AW_window *login_window;
    AW_window *arb_login_window;
    AW_window *seer_select_attributes_window;
    AW_window *query_seer_window;
    AW_window *seer_upload_window;
    AW_selection_list *attribute_sellection_list;
    AW_selection_list *export_attribute_sellection_list;

    char        *alignment_name;
    int          alignment_type; // SeerInterfaceAlignmentType

    SeerInterface *interface;
    GB_HASH     *alignment_hash;
    GB_HASH     *attribute_hash;
    GB_HASH     *arb_attribute_hash; // all attributes which are in arb
    GB_HASH     *table_hash;
    long        last_upload_transaction_counter;
    AW_selection_list *tmp_selection_list;
};

extern SEER_GLOBAL seer_global;
class SeerInterfaceData;
const char *SEER_open(const char *username, const char *userpasswd);
void SEER_logout(); // panic logout
const char *SEER_opens_arb(AW_root *awr, SEER_OPENS_ARB_TYPE type); // returns errorstring or 0
const char *SEER_populate_tables(AW_root *awr);

long seer_sort_attributes(const char *,long val0,const char *,long val1);

const char *SEER_query(const char *alignmentname, const char *attribute,const char *querystring,int load_sequence_flag);
void SEER_load_marked(const char *alignmentname,int load_sequence_flag);
void SEER_create_skeleton_for_tree(GBT_TREE *tree); // assumes a running transaction
void SEER_strip_arb_file();
void  SEER_upload_to_seer_create_window(AW_window *aww);
int SEER_retrieve_data_rek(GB_HASH *attribute_hash,GBDATA *gb_species, SeerInterfaceData *id,int deep, int test_only, int filterAttributes);


#endif
