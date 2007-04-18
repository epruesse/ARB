#ifndef AW_RENAME_HXX
#define AW_RENAME_HXX

#define AWT_RENAME_USE_ADVICE "awt_rename/use_advice"
#define AWT_RENAME_SAVE_DATA  "awt_rename/save_data"

#define AWAR_NAMESERVER_ADDID "nt/nameserver_addid"

#ifndef AW_RENAME_SKIP_GUI

void       AWTC_create_rename_variables(AW_root *root,AW_default db1);
AW_window *AWTC_create_rename_window(AW_root *root, AW_CL gb_main);
void       awt_rename_cb(AW_window *aww,GBDATA *gb_main);

void       AW_create_namesadmin_awars(AW_root *aw_root, GBDATA *gb_main);
AW_window *AW_create_namesadmin_window(AW_root *aw_root, AW_CL cl_gb_main);

GB_ERROR AW_select_nameserver(GBDATA *gb_main, GBDATA *gb_other_main);

#endif // AW_RENAME_SKIP_GUI

GB_ERROR AWTC_pars_names(GBDATA *gb_main, int update_status = 0);
GB_ERROR AWTC_generate_one_name(GBDATA *gb_main, const char *full_name, const char *acc, const char *addid, char*& new_name, bool openstatus, bool showstatus);
GB_ERROR AWTC_recreate_name(GBDATA *gb_main, bool update_status);

// return name of additional field used for species identification
// (-> para 'addid' in AWTC_generate_one_name)
const char *AW_get_nameserver_addid(GBDATA *gb_main);

GB_ERROR AW_test_nameserver(GBDATA *gb_main); // create a test link to the nameserver

class UniqueNameDetector {
    // Note: If you add new items to the DB while one instance of this class exists,
    //       you have to call add_name() for these new species!

    GB_HASH *hash;
public:
    UniqueNameDetector(GBDATA *gb_item_data, int additionalEntries = 0);
    ~UniqueNameDetector();

    bool name_known(const char *name) { return GBS_read_hash(hash, name) == 1; }
    void add_name(const char *name) { GBS_write_hash(hash, name, 1); }
};

char *AWTC_makeUniqueShortName(const char *prefix, UniqueNameDetector& existingNames);
char *AWTC_generate_random_name(UniqueNameDetector& existingNames);

struct PersistantNameServerConnection {
    // create a PersistantNameServerConnection instance while calling AWTC_generate_one_name
    // to avoid repeated re-connections to name server
    bool dummy;
    PersistantNameServerConnection();
    ~PersistantNameServerConnection();
};

int AWTC_name_quality(const char *short_name); 

#else
#error AW_rename.hxx included twice
#endif
