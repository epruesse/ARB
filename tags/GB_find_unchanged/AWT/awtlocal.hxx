#ifndef awtlocal_hxx_included
#define awtlocal_hxx_included

struct adawcbstruct {
    // @@@ FIXME: rethink design - maybe split into base class +
    // several derived classes for the different usages.
    
    AW_window              *aws;
    AW_root                *awr;
    GBDATA                 *gb_main;
    GBDATA                 *gb_user;
    GBDATA                 *gb_edit;
    AW_selection_list      *id;
    char                   *comm;
    char                   *def_name;
    char                   *def_gbd;
    char                   *def_alignment;
    char                   *def_source;
    char                   *def_dest;
    char                   *def_filter;
    char                   *previous_filename;
    char                   *pwd;
    char                   *pwdx; // additional directories
    AW_BOOL                 show_dir;
    AW_BOOL                 leave_wildcards;
    char                    may_be_an_error;
    char                    show_only_marked;
    char                    scannermode;
    char                   *def_dir;
    const ad_item_selector *selector;
    AW_BOOL                 add_all_fields_pseudo_field; // true = > add a pseudo-field named '[all_fields]' (used by 'awt_create_selection_list_on_scandb_cb')
    AW_BOOL                 include_hidden_fields; // true       = > show hidden fields in selection list
};

struct awt_sel_list_for_tables {
    AW_window         *aws;
    GBDATA            *gb_main;
    AW_selection_list *id;
    const char        *table_name;
};

struct awt_sel_list_for_sai {
    AW_window         *aws;
    GBDATA            *gb_main;
    AW_selection_list *id;
    char *(*filter_poc)(GBDATA *gb_ext, AW_CL);
    AW_CL              filter_cd;
    AW_BOOL            add_selected_species;
};

typedef enum {
    AWT_QUERY_GENERATE,
    AWT_QUERY_ENLARGE,
    AWT_QUERY_REDUCE
} AWT_QUERY_MODES;

typedef enum {
    AWT_QUERY_MARKED,
    AWT_QUERY_MATCH,
    AWT_QUERY_DONT_MATCH
} AWT_QUERY_TYPES;

#define AWT_QUERY_SEARCHES 3 // no of search-lines in search tool

struct adaqbsstruct {
    AW_window         *aws;
    GBDATA            *gb_main;
    GBDATA            *gb_ref;  // second reference database
    AW_BOOL            look_in_ref_list; // for querys
    AWAR               species_name;
    AWAR               tree_name;
    AWAR               awar_keys[AWT_QUERY_SEARCHES];
    AWAR               awar_setkey;
    AWAR               awar_setprotection;
    AWAR               awar_setvalue;
    AWAR               awar_parskey;
    AWAR               awar_parsvalue;
    AWAR               awar_parspredefined;
    AWAR               awar_queries[AWT_QUERY_SEARCHES];
    AWAR               awar_not[AWT_QUERY_SEARCHES]; // not flags for queries
    AWAR               awar_operator[AWT_QUERY_SEARCHES]; // not flags for queries
    AWAR               awar_ere;
    AWAR               awar_where;
    AWAR               awar_by;
    AWAR               awar_use_tag;
    AWAR               awar_double_pars;
    AWAR               awar_deftag;
    AWAR               awar_tag;
    AWAR               awar_count;
    AW_selection_list *result_id;
    int                select_bit; // one of 1 2 4 8 .. 128 (one for each query box)

    const ad_item_selector *selector;
};

#define AWAR_TABLE_FIELD_REORDER_SOURCE_TEMPLATE "tmp/table/%s/field/reorder_source"
#define AWAR_TABLE_FIELD_REORDER_DEST_TEMPLATE  "tmp/table/%s/field/reorder_dest"
#define AWAR_TABLE_FIELD_NEW_NAME_TEMPLATE      "tmp/table/%s/new_name"
#define AWAR_TABLE_FIELD_REM_TEMPLATE           "tmp/table/%s/rem"
#define AWAR_TABLE_FIELD_NEW_TYPE_TEMPLATE      "tmp/table/%s/new_type"
#define AWAR_TABLE_SELECTED_FIELD_TEMPLATE      "tmp/table/%s/selected_field"

struct awt_table {
    GBDATA *gb_main;
    char   *table_name;
    char   *awar_field_reorder_source;
    char   *awar_field_reorder_dest;
    char   *awar_field_new_name;
    char   *awar_field_new_type;
    char   *awar_field_rem;
    char   *awar_selected_field;
    awt_table(GBDATA *gb_main,AW_root *awr,const char *table_name);
    ~awt_table();
};

#define AWAR_TABLE_NAME   "tmp/ad_table/table_name"
#define AWAR_TABLE_DEST   "tmp/ad_table/table_dest"
#define AWAR_TABLE_REM    "tmp/ad_table/table_rem"
#define AWAR_TABLE_EXPORT "tmp/ad_table/export_table"
#define AWAR_TABLE_IMPORT "tmp/ad_table/import_table"

#define ALL_FIELDS_PSEUDO_FIELD "[any field]"

long awt_query_update_list(void *dummy, struct adaqbsstruct *cbs);


#endif
