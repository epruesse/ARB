#ifndef awt_hxx_included
#define awt_hxx_included

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define awt_assert(bed) arb_assert(bed)

#ifndef aw_window_hxx_included
#include <aw_window.hxx>
#endif
#ifndef ARBDB_H
#include <arbdb.h>
#endif

const char * AWT_date_string(void);

// ------------------------------------------------------------
// filename related functions:

char       *AWT_fold_path(char *path, const char *pwd = "PWD");
char       *AWT_unfold_path(const char *path, const char *pwd = "PWD");
const char *AWT_valid_path(const char *path);

int   AWT_is_dir(const char *path);
int   AWT_is_file(const char *path);
int   AWT_is_link(const char *path);
char *AWT_extract_directory(const char *path);

GB_CSTR AWT_concat_full_path(const char *anypath_left, const char *anypath_right);
GB_CSTR AWT_path_in_ARBHOME(const char *relative_path);

GB_CSTR AWT_get_suffix(const char *fullpath);
GB_CSTR AWT_append_suffix(const char *name, const char *suffix);

// ------------------------------------------------------------

// holds all stuff needed to make ad_.. functions work with species _and_ genes (and more..)

typedef enum {
    AWT_QUERY_ITEM_SPECIES,
    AWT_QUERY_ITEM_GENES,
    AWT_QUERY_ITEM_EXPERIMENTS,

    AWT_QUERY_ITEM_TYPES // how many different types do we have

} AWT_QUERY_ITEM_TYPE;

typedef enum {
    AWT_QUERY_CURRENT_SPECIES,
    AWT_QUERY_MARKED_SPECIES,
    AWT_QUERY_ALL_SPECIES
} AWT_QUERY_RANGE;

struct ad_item_selector {
    AWT_QUERY_ITEM_TYPE type;

    // if user selects an item in the result list,
    // this callback sets the appropriate AWARs
    // - for species: AWAR_SPECIES_NAME is changed (item_name = 'species_name')
    // - for genes: AWAR_GENE_NAME and AWAR_SPECIES_NAME are changed (item_name = 'species_name/gene_name')
    void (*update_item_awars)(GBDATA* gb_main, AW_root *aw_root, const char *item_name);
    char *(*generate_item_name)(GBDATA *gb_main, AW_root *aw_root, GBDATA *gb_item);
    AW_CB selection_list_rescan_cb;
    int   item_name_length;

    const char *change_key_path;
    const char *item_name;      // "species" or "gene"
    const char *items_name;     // "species" or "genes"

    GBDATA *(*get_first_item_container)(GBDATA *, AW_root *, AWT_QUERY_RANGE); // for species this is normally awt_get_species_data
    GBDATA *(*get_next_item_container)(GBDATA *, AWT_QUERY_RANGE); // for species this is normally a function returning 0
    GBDATA *(*get_first_item)(GBDATA *); // for species this is normally GBT_first_species_rel_species_data
    GBDATA *(*get_next_item)(GBDATA *); // for species this is normally GBT_next_species

    GBDATA *(*get_selected_item)(GBDATA *gb_main, AW_root *aw_root); // searches the currently selected item
};

extern ad_item_selector AWT_species_selector;
extern ad_item_selector AWT_organism_selector;

/**************************************************************************
 *********************       File Selection Boxes    *******************
 ***************************************************************************/

void awt_create_selection_box(AW_window *aws, const char *awar_prefix, const char *at_prefix = "", const char *pwd = "PWD", AW_BOOL show_dir = AW_TRUE, AW_BOOL allow_wildcards = AW_FALSE);
/* Create a file selection box, this box needs 3 AWARS:

1. "$awar_prefix/filter"
2. "$awar_prefix/directory"
3. "$awar_prefix/file_name"

(Note: The function aw_create_selection_box_awars can be used to create them)
*/

/* the "$awar_prefix/file_name" contains the full filename
   Use awt_get_selected_fullname() to read it.

The items are placed at

1. "$at_prefix""filter"
2. "$at_prefix""box"
3. "$at_prefix""file_name"

if show_dir== AW_TRUE than show directories and files
else only files

pwd is a 'shell enviroment variable' which indicates the base directory
( mainly PWD or ARBHOME ) */

char *awt_get_selected_fullname(AW_root *awr, const char *awar_prefix);

void awt_refresh_selection_box(AW_root *awr, const char *awar_prefix);

// -------------------------------

AW_window *create_save_box_for_selection_lists(AW_root *aw_root,AW_CL selid);
AW_window *create_load_box_for_selection_lists(AW_root *aw_root,AW_CL selid);
void create_print_box_for_selection_lists(AW_window *aw_window,AW_CL selid);
/* Create a file selection box to save a selection list */

/**************************************************************************
 *********************   Various Database Selection Boxes    ***********
 ***************************************************************************/
/***********************    Macros  ************************/
AW_window *awt_open_macro_window(AW_root *aw_root,const char *application_id);
/***********************    Alignments  ************************/
void awt_create_selection_list_on_ad(GBDATA *gb_main,AW_window *aws,const char *varname,const char *comm);
/* Create selection lists on alignments, if comm is set, then
   pars the alignment type, show only those that are parsed */


/***********************    Trees       ************************/
/* Selection list for trees */
void awt_create_selection_list_on_trees(GBDATA *gb_main,AW_window *aws, const char *varname);

/*********************** Pt-Servers ******************************/
/* Selection list for pt-servers */

void awt_create_selection_list_on_pt_servers(AW_window *aws, const char *varname, AW_BOOL popup);

/***********************    Tables Fields       ************************/

void awt_create_selection_list_on_tables(GBDATA *gb_main, AW_window *aws,const char *varname);

void awt_create_selection_list_on_table_fields(GBDATA *gb_main, AW_window *aws,const char *tablename, const char *varname);
// if tablename == 0 take fields from species table !!!!

AW_window *AWT_create_tables_admin_window(AW_root *aw_root,GBDATA *gb_main);

/***********************    SAIS        ************************/
void *awt_create_selection_list_on_extendeds(GBDATA *gb_main,AW_window *aws, const char *varname,
                                             char *(*filter_poc)(GBDATA *gb_ext, AW_CL) = 0, AW_CL filter_cd = 0,
                                             AW_BOOL add_sel_species= AW_FALSE);
/* Selection list for all extendeds !!!!!!!!! */
/* if filter_proc is set then show only those items on which
   filter_proc returns a strdup(string) */

void awt_create_selection_list_on_extendeds_update(GBDATA *dummy, void *cbsid);
/* update the selection box defined by awt_create_selection_list_on_extendeds
   usefull only when filterproc is defined (changes to the SAI entries automatically
   calls awt_create_selection_list_on_extendeds_update*/

/***********************    CONFIGURATIONS      ************************/

void  awt_create_selection_list_on_configurations(GBDATA *gb_main,AW_window *aws, const char *varname);
char *awt_create_string_on_configurations(GBDATA *gb_main);

/***********************    FILES IN GENERAL        ************************/

AW_window *awt_create_load_box(AW_root *aw_root, const char *load_what, const char *file_extension, char **set_file_name_awar,
                               void (*callback)(AW_window*), // set callback ..
                               AW_window* (*create_popup)(AW_root *, AW_default)); // .. or create_popup  (both together not allowed)

/***********************    FIELD INFORMATIONS  ************************/
AW_CL awt_create_selection_list_on_scandb(GBDATA                 *gb_main,
                                          AW_window              *aws,
                                          const char             *varname,
                                          long                    type_filter,
                                          const char             *scan_xfig_label,
                                          const char             *rescan_xfig_label,
                                          const ad_item_selector *selector,
                                          size_t                  columns,
                                          size_t                  visible_rows,
                                          AW_BOOL                 popup_list_in_window        = false,
                                          AW_BOOL                 add_all_fields_pseudo_field = false,
                                          AW_BOOL                 include_hidden_fields       = false
                                          );
/* show fields of a species / extended / gene !!!
   type filter is a bitstring which controls what types are shown in
   the selection list: e.g 1<<GB_INT || 1 <<GB_STRING enables
   ints and strings */

enum awt_rescan_mode {
    AWT_RS_SCAN_UNKNOWN_FIELDS  = 1, // scan database for unknown fields and add them
    AWT_RS_DELETE_UNUSED_FIELDS = 2, // delete all unused fields
    AWT_RS_SHOW_ALL             = 4, // unhide all hidden fields

    AWT_RS_UPDATE_FIELDS  = AWT_RS_SCAN_UNKNOWN_FIELDS|AWT_RS_DELETE_UNUSED_FIELDS
} ;

void awt_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode); /* rescan it */
void awt_gene_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode);
void awt_experiment_field_selection_list_rescan(GBDATA *gb_main, long bitfilter, awt_rescan_mode mode);

void awt_selection_list_scan_unknown_cb(AW_window *aww,GBDATA *gb_main, long bitfilter);
void awt_selection_list_delete_unused_cb(AW_window *aww,GBDATA *gb_main, long bitfilter);
void awt_selection_list_unhide_all_cb(AW_window *aww,GBDATA *gb_main, long bitfilter);
void awt_selection_list_update_cb(AW_window *aww,GBDATA *gb_main, long bitfilter);

void awt_gene_field_selection_list_scan_unknown_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_delete_unused_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_unhide_all_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_gene_field_selection_list_update_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);

void awt_experiment_field_selection_list_scan_unknown_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_delete_unused_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_unhide_all_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);
void awt_experiment_field_selection_list_update_cb(AW_window *dummy,GBDATA *gb_main, long bitfilter);

GB_ERROR awt_add_new_changekey(GBDATA *gb_main,const char *name, int type);
/*  type == GB_TYPES
    add a new FIELD to the FIELD LIST */
GB_ERROR awt_add_new_changekey_to_keypath(GBDATA *gb_main,const char *name, int type, const char *keypath);
// same as awt_add_new_changekey but with given keypath (with this you can add fields to any item (species, gene, ...))


GBDATA   *awt_get_key(GBDATA *gb_main, const char *key, const char *change_key_path);
GB_TYPES  awt_get_type_of_changekey(GBDATA *gb_main,const char *field_name, const char *change_key_path);

/***********************    FILTERS     ************************/
AW_CL   awt_create_select_filter(AW_root *aw_root,GBDATA *gb_main, const char *def_name);
/* Create a data structure for filters (needed for awt_create_select_filter_win */
/* Create a filter selection box, this box needs 3 AWARS:
   1. "$def_name"
   2. "$def_name:/name=/filter"
   3. "$def_name:/name=/alignment"
   and some internal awars
*/
void awt_set_awar_to_valid_filter_good_for_tree_methods(GBDATA *gb_main,AW_root *awr, const char *awar_name);

AW_window *awt_create_select_filter_win(AW_root *aw_root,AW_CL res_of_create_select_filter);
/* not just the box, but the whole window */

class AP_filter;
AP_filter *awt_get_filter(AW_root *aw_root,AW_CL res_of_create_select_filter);

char *AWT_get_combined_filter_name(AW_root *aw_root, GB_CSTR prefix);

/**************************************************************************
 *********************   Various Database SCANNER Boxes  *******************
 ***************************************************************************/
/*  A scanner show all (rekursiv) information of a database entry:
    This information can be organized in two different ways:
    1. AWT_SCANNER: Show exact all (filtered) information stored in the DB
    2. AWT_VIEWER:  Create a list of all database fields (see FIELD INFORMATIONS)
    and if any information is stored under a field append it.
    example: fields:    name, full_name, acc, author
    DB entries:    name:e.coli full_name:esc.coli flag:test
    ->  name:       e.coli
    full_name:  esc.coli
    acc:
    author:
*/

typedef enum {
    AWT_SCANNER,
    AWT_VIEWER
} AWT_SCANNERMODE;

#define AWT_NDS_FILTER (1<<GB_STRING)|(1<<GB_BYTE)|(1<<GB_INT)|(1<<GB_FLOAT)|(1<<GB_BITS)|(1<<GB_LINK)
#define AWT_PARS_FILTER (1<<GB_STRING)|(1<<GB_BYTE)|(1<<GB_INT)|(1<<GB_FLOAT)|(1<<GB_BITS)|(1<<GB_LINK)
#define AWT_STRING_FILTER (1<<GB_STRING)|(1<<GB_BITS)|(1<<GB_LINK)

AW_CL awt_create_arbdb_scanner(GBDATA                 *gb_main, AW_window *aws,
                               const char             *box_pos_fig, /* the position for the box in the xfig file */
                               const char             *delete_pos_fig, /* create a delete button (which enables deleting) */
                               const char             *edit_pos_fig, /* the edit field (which enables editing) */
                               const char             *edit_enable_pos_fig, /* enable editing toggle */
                               AWT_SCANNERMODE         mode,
                               const char             *rescan_pos_fig, // AWT_VIEWER only (create a rescan FIELDS button)
                               const char             *mark_pos_fig, // Create a toggle which show the database flag
                               long                    type_filter, // AWT_VIEWER BITFILTER for TYPES
                               const ad_item_selector *selector);

void awt_map_arbdb_scanner(AW_CL  arbdb_scanid,
                           GBDATA               *gb_pntr, int show_only_marked_flag, const char *keypath);
/* map the Scanner to a database entry */
GBDATA                           *awt_get_arbdb_scanner_gbdata(AW_CL arbdb_scanid);
/* reverse mapping, should be used to keep track of deleted items */



/**************************************************************************
 *********************   Query Box           *******************
 ***************************************************************************/


#define AWT_MAX_QUERY_LIST_LEN        1000
#define IS_QUERIED(gb_species,cbs)    (cbs->select_bit & GB_read_usr_private(gb_species))
#define SET_QUERIED(gb_species,cbs)   GB_write_usr_private(gb_species,cbs->select_bit | GB_read_usr_private(gb_species))
#define CLEAR_QUERIED(gb_species,cbs) GB_write_usr_private(gb_species,(~cbs->select_bit) & GB_read_usr_private(gb_species))

class awt_query_struct {
public:
    awt_query_struct(void);

    GBDATA  *gb_main;           // the main database
    GBDATA  *gb_ref;            // second reference database
    AW_BOOL  look_in_ref_list;  // for querys
    AWAR     species_name;      // AWAR containing current species name
    AWAR     tree_name;         // AWAR containing current tree name

    const ad_item_selector *selector;  // which kind of item do we handle?

    //     bool query_genes;           // true -> create gene query box
    //     AWAR gene_name;             // AWAR containing current gene name

    int select_bit;             // one of 1 2 4 8 .. 128 (one for each query box)
    int use_menu;               // put additional commands in menu

    const char *ere_pos_fig;    // rebuild enlarge reduce
    const char *where_pos_fig;  // current, marked or all species (used for sub-items of species)
    const char *by_pos_fig;     // fit query dont fit, marked

    const char *qbox_pos_fig;   // key box for queries
    const char *rescan_pos_fig; // rescan label
    const char *key_pos_fig;    // the key
    const char *query_pos_fig;  // the query


    const char *result_pos_fig; // the result box
    const char *count_pos_fig;

    const char *do_query_pos_fig;
    const char *config_pos_fig;
    const char *do_mark_pos_fig;
    const char *do_unmark_pos_fig;
    const char *do_delete_pos_fig;
    const char *do_set_pos_fig; // multi set a key
    const char *open_parser_pos_fig;
    const char *do_refresh_pos_fig;
    AW_CL       create_view_window;     // AW_window *(*create_view_window)(AW_root *aw_root)

    const char *info_box_pos_fig;

};

struct adaqbsstruct;
void awt_copy_selection_list_2_queried_species(struct adaqbsstruct *cbs, AW_selection_list * id);
struct adaqbsstruct *awt_create_query_box(AW_window *aws, awt_query_struct *awtqs); // create the query box
/* Create the query box */
void awt_search_equal_entries(AW_window *dummy,struct adaqbsstruct *cbs,int tokenize);
long awt_count_queried_species(struct adaqbsstruct *cbs);
void awt_unquery_all(void *dummy, struct adaqbsstruct *cbs);

AW_window *awt_create_item_colorizer(AW_root *aw_root, GBDATA *gb_main, const ad_item_selector *sel);

/**************************************************************************
 *********************   Simple Awar Controll Procs  *******************
 ***************************************************************************/

void awt_set_long(AW_window *aws, AW_CL varname, AW_CL value);      // set an awar
void awt_set_string(AW_window *aws, AW_CL varname, AW_CL value);    // set an awar

/**************************************************************************
 *********************   Call External Editor        *******************
 ***************************************************************************/

void awt_edit(AW_root *awr, const char *path, int x = 640, int y = 400, const char *font = "7x13");



void AWT_create_ascii_print_window(AW_root *awr, const char *text_to_print,const char *title=0);
void AWT_write_file(const char *filename,const char *file);
void AWT_show_file(AW_root *awr, const char *filename);


enum AD_MAP_VIEWER_TYPE {
    ADMVT_INFO,
    ADMVT_WWW,
    ADMVT_SELECT
};
void AD_map_viewer(GBDATA *gbd,AD_MAP_VIEWER_TYPE type = ADMVT_INFO);

// open database viewer using input-mask-file
class awt_item_type_selector;
GB_ERROR AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* mask_name, bool localMask);

//  ----------------------------------------
//      class awt_input_mask_descriptor
//  ----------------------------------------
class awt_input_mask_descriptor {
private:
    char *title;                // title of the input mask
    char *internal_maskname;    // starts with 0 for local mask and with 1 for global mask
    // followed by file name w/o path
    char *itemtypename;         // name of the itemtype
    bool  local_mask;           // true if mask file was found in "~/.arb_prop/inputMasks"
    bool  hidden;               // if true, mask is NOT shown in Menus

public:
    awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_, bool local, bool hidden_);
    awt_input_mask_descriptor(const awt_input_mask_descriptor& other);
    virtual ~awt_input_mask_descriptor();

    awt_input_mask_descriptor& operator = (const awt_input_mask_descriptor& other);

    const char *get_title() const { return title; }
    const char *get_maskname() const { return internal_maskname+1; }
    const char *get_internal_maskname() const { return internal_maskname; }
    const char *get_itemtypename() const { return itemtypename; }

    bool is_local_mask() const { return local_mask; }
    bool is_hidden() const { return hidden; }
};

const awt_input_mask_descriptor *AWT_look_input_mask(int id); // id starts with 0; returns 0 if no more masks

// void       AWT_input_mask_create_global_awars(AW_root *awr); // needed by AWT_create_new_input_mask
// AW_window *AWT_create_window_input_mask_new(AW_root *awr); // create new user mask (interactively)


// database browser :
void       AWT_create_db_browser_awars(AW_root *aw_root, AW_default aw_def);
void       AWT_announce_db_to_browser(GBDATA *gb_main, const char *description);
AW_window *AWT_create_db_browser(AW_root *aw_root);

#endif
