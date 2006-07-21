#ifndef AW_RENAME_HXX
#define AW_RENAME_HXX


#define AWT_RENAME_USE_ADVICE "awt_rename/use_advice"
#define AWT_RENAME_SAVE_DATA "awt_rename/save_data"
#define NAMES_FILE_LOCATION "$(ARBHOME)/lib/nas/names.dat"

void       AWTC_create_rename_variables(AW_root *root,AW_default db1);
AW_window *AWTC_create_rename_window(AW_root *root, AW_CL gb_main);
void       awt_rename_cb(AW_window *aww,GBDATA *gb_main);

AW_window *create_awtc_names_admin_window(AW_root *aw_root);

GB_ERROR AWTC_pars_names(GBDATA *gb_main, int update_status = 0);
GB_ERROR AWTC_generate_one_name(GBDATA *gb_main, const char *full_name, const char *acc, char*& new_name, bool openstatus);
GB_ERROR AWTC_recreate_name(GBDATA *gb_main, bool update_status);

char *AWTC_makeUniqueShortName(const char *prefix, GBDATA *gb_species_data);
char *AWTC_generate_random_name(GBDATA *gb_species_data);

struct PersistantNameServerConnection {
    bool dummy;
    PersistantNameServerConnection();
    ~PersistantNameServerConnection();
};

#else
#error AW_rename.hxx included twice
#endif
