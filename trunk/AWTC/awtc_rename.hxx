#ifndef awtc_rename_hxx_included
#define awtc_rename_hxx_included


#define AWT_RENAME_USE_ADVICE "awt_rename/use_advice"
#define AWT_RENAME_SAVE_DATA "awt_rename/save_data"
#define NAMES_FILE_LOCATION "$(ARBHOME)/lib/nas/names.dat"

void		create_rename_variables(AW_root *root,AW_default db1);
AW_window	*create_rename_window(AW_root *root, AW_CL gb_main);
void		awt_rename_cb(AW_window *aww,GBDATA *gb_main);

AW_window	*create_awtc_names_admin_window(AW_root *aw_root);

GB_ERROR	pars_names(GBDATA *gb_main, int update_status = 0);
GB_ERROR    generate_one_name(GBDATA *gb_main, const char *full_name, char*& new_name);

#endif
