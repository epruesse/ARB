#ifndef gde_hxx_included
#define gde_hxx_included

// char *(*func)(void *THIS, GBDATA **&the_species, char **&the_sequences,
//		long &numberspecies,long &maxalignlen)
typedef unsigned char uchar;
typedef enum { CGSS_WT_DEFAULT, CGSS_WT_EDIT, CGSS_WT_EDIT4 } gde_cgss_window_type;

void create_gde_var(AW_root *aw_root, AW_default aw_def,
		    char *(*get_sequences)(void *THIS, GBDATA **&the_species,
					   uchar **&the_names,
					   uchar **&the_sequences,
					   long &numberspecies,long &maxalignlen)=0,
		    gde_cgss_window_type wt=CGSS_WT_DEFAULT,
		    void *THIS = 0);
AW_window *AP_open_gde_window( AW_root *root );
void GDE_load_menu(AW_window *awm,const char *menulabel=0,const char *menuitemlabel=0 );

#endif
