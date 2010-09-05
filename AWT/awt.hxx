// =========================================================== //
//                                                             //
//   File      : awt.hxx                                       //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_HXX
#define AWT_HXX

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define awt_assert(bed) arb_assert(bed)

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif
#ifndef ARBDB_H
#include <arbdb.h>
#endif

// ------------------------------------------------------------
// filename related functions:

char       *AWT_fold_path(char *path, const char *pwd = "PWD");
char       *AWT_unfold_path(const char *path, const char *pwd = "PWD");
const char *AWT_valid_path(const char *path);

int   AWT_is_dir(const char *path);
int   AWT_is_file(const char *path);
int   AWT_is_link(const char *path);
char *AWT_extract_directory(const char *path);

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

typedef enum {
    AWT_QUERY_SORT_NONE = 0,

    // "real" criteria:
    AWT_QUERY_SORT_BY_1STFIELD_CONTENT = 1,         // by content of first selected search field
    AWT_QUERY_SORT_BY_ID               = 2,         // by item id (not by parent)
    AWT_QUERY_SORT_BY_NESTED_PID       = 4,         // by nested parent id
    AWT_QUERY_SORT_BY_MARKED           = 8,         // marked items first
    AWT_QUERY_SORT_BY_HIT_DESCRIPTION  = 16,        // by hit description
    AWT_QUERY_SORT_REVERSE             = 32,        // revert following (may occur multiple times)

} AWT_QUERY_RESULT_ORDER;

#define AWT_QUERY_SORT_CRITERIA_BITS 6              // number of "real" sort criteria
#define AWT_QUERY_SORT_CRITERIA_MASK ((1<<AWT_QUERY_SORT_CRITERIA_BITS)-1)

struct ad_item_selector {
    AWT_QUERY_ITEM_TYPE type;

    // if user selects an item in the result list,
    // this callback sets the appropriate AWARs
    // - for species: AWAR_SPECIES_NAME is changed (item_name = 'species_name')
    // - for genes: AWAR_GENE_NAME and AWAR_SPECIES_NAME are changed (item_name = 'species_name/gene_name')
    void (*update_item_awars)(GBDATA* gb_main, AW_root *aw_root, const char *item_name);
    char *(*generate_item_id)(GBDATA *gb_main, GBDATA *gb_item);
    GBDATA *(*find_item_by_id)(GBDATA *gb_main, const char *id);
    AW_CB selection_list_rescan_cb;
    int   item_name_length; // -1 means "unknown" (might be long)

    const char *change_key_path;
    const char *item_name;                          // "species" or "gene" or "experiment" or "organism"
    const char *items_name;                         // "species" or "genes" or "experiments" or "organisms"
    const char *id_field;                           // e.g. "name" for species, genes

    GBDATA *(*get_first_item_container)(GBDATA *, AW_root *, AWT_QUERY_RANGE); // AW_root may be NULL for AWT_QUERY_ALL_SPECIES and AWT_QUERY_MARKED_SPECIES
    GBDATA *(*get_next_item_container)(GBDATA *, AWT_QUERY_RANGE); // use same AWT_QUERY_RANGE as in get_first_item_container()
    
    GBDATA *(*get_first_item)(GBDATA *);
    GBDATA *(*get_next_item)(GBDATA *);

    GBDATA *(*get_selected_item)(GBDATA *gb_main, AW_root *aw_root); // searches the currently selected item

    struct ad_item_selector *parent_selector;       // selector of parent item (or NULL if item has no parents)
    GBDATA *(*get_parent)(GBDATA *gb_item);         // if 'parent_selector' is defined, this function returns the parent of the item
};

char   *AWT_get_item_id(GBDATA *gb_main, const ad_item_selector *sel, GBDATA *gb_item);
GBDATA *AWT_get_item_with_id(GBDATA *gb_main, const ad_item_selector *sel, const char *id);

extern ad_item_selector AWT_species_selector;
extern ad_item_selector AWT_organism_selector;

/**************************************************************************
 *********************       File Selection Boxes    *******************
 ***************************************************************************/

void awt_create_selection_box(AW_window *aws, const char *awar_prefix, const char *at_prefix = "", const char *pwd = "PWD", bool show_dir = true, bool allow_wildcards = false);
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

if show_dir== true than show directories and files
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
 *********************   Query Box           *******************
 ***************************************************************************/


#define IS_QUERIED(gb_species,cbs)    (cbs->select_bit & GB_read_usr_private(gb_species))
class awt_query_struct {
public:
    awt_query_struct(void);

    GBDATA *gb_main;                                // the main database (in merge tool: source db in left query; dest db in right query)
    GBDATA *gb_ref;                                 // second reference database (only used by merge tool; dest db in left query; source db in right query)
    bool    expect_hit_in_ref_list;                 // merge-tool: when searching dups in fields: match only if hit exists in other DBs hitlist (true for DBII-query)
    AWAR    species_name;                           // AWAR containing current species name
    AWAR    tree_name;                              // AWAR containing current tree name

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
void awt_copy_selection_list_2_queried_species(struct adaqbsstruct *cbs, AW_selection_list *id, const char *hit_description);
struct adaqbsstruct *awt_create_query_box(AW_window *aws, awt_query_struct *awtqs, const char *query_id); // create the query box
/* Create the query box */
void awt_search_equal_entries(AW_window *dummy, struct adaqbsstruct *cbs, bool tokenize);
long awt_count_queried_items(struct adaqbsstruct *cbs, AWT_QUERY_RANGE range);
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

typedef void (*awt_fileChanged_cb)(const char *path, bool fileWasChanged, bool editorTerminated);
void AWT_edit(const char *path, awt_fileChanged_cb callback = 0, AW_window *aww = 0, GBDATA *gb_main = 0);

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

#if defined(DEBUG)
// database browser :
void AWT_create_db_browser_awars(AW_root *aw_root, AW_default aw_def);
void AWT_announce_db_to_browser(GBDATA *gb_main, const char *description);
void AWT_browser_forget_db(GBDATA *gb_main);

void AWT_create_debug_menu(AW_window *awmm);
#endif // DEBUG

#else
#error awt.hxx included twice
#endif // AWT_HXX
