#ifndef AW_AWARS_HXX
#define AW_AWARS_HXX

#define CHANGE_KEY_PATH             "presets/key_data"
#define CHANGE_KEY_PATH_GENES       "presets/gene_key_data"
#define CHANGE_KEY_PATH_EXPERIMENTS "presets/experiment_key_data"

#define CHANGEKEY        "key"
#define CHANGEKEY_NAME   "key_name"
#define CHANGEKEY_TYPE   "key_type"
#define CHANGEKEY_HIDDEN "key_hidden"

#define AWAR_DB                "tmp/nt/arbdb/"
#define AWAR_DB_PATH           AWAR_DB "file_name"
#define AWAR_DB_NAME           AWAR_DB_PATH "_without_path" //  awar is automatically updated when AWAR_DB_PATH changes
#define AWAR_DEFAULT_ALIGNMENT "presets/use"

#define AWAR_TREE          "focus/tree_name"
#define AWAR_TREE_REFRESH  "tmp/focus/tree_refresh" // touch this awar to refresh the tree display
#define AWAR_CONFIGURATION "focus/configuration"

#define AWAR_SECURITY_LEVEL    "/tmp/etc/security_level"
#define AWAR_GDE_EXPORT_FILTER "/tmp/gde/export_filter"

#define AWAR_DB_COMMENT "description"

#define AWAR_PT_SERVER            "nt/pt_server"
#define AWAR_TARGET_STRING        "nt/target_string"
#define AWAR_PRIMER_TARGET_STRING "nt/primer_target_string"
#define AWAR_GENE_CONTENT         "nt/gene_content"
#define AWAR_MIN_MISMATCHES       "nt/min_mismatches"
#define AWAR_MAX_MISMATCHES       "nt/max_mismatches"
#define AWAR_ITARGET_STRING       "nt/itarget_string"

#define AWAR_SAI_COLOR_STR       "tmp/sai_color_str" //to visualize SAIs in probe Match window

/* local awars for edit and edit4: */

#define AWAR_SPECIES_NAME_LOCAL    "tmp/edit/species_name" /* string: name of selected species */
#define AWAR_CURSOR_POSITION_LOCAL "tmp/edit/cursor_position" /* int: position in sequence */
#define AWAR_EDITOR_ALIGNMENT      "tmp/edit/alignment" /* string: alignment used by editor */

/* global awars */

#define AWAR_CURSOR_POSITION     "tmp/focus/cursor_position" /* int: position in sequence */
#define AWAR_SET_CURSOR_POSITION "tmp/focus/set_cursor_position" /* int: set position in sequence (remote control for ARB_EDIT4) */

#define AWAR_SPECIES_NAME       "tmp/focus/species_name" /* string: name of selected species */
#define AWAR_INFO_BUTTON_TEXT   "tmp/focus/info_button_text" /* string: to display selected species */
#define AWAR_ORGANISM_NAME      "tmp/focus/organism_name" /* string: name of selected organism (differs from AWAR_SPECIES_NAME only for pseudo-gene-species) */
/* - normally AWAR_ORGANISM_NAME contains the same value as AWAR_SPECIES_NAME
   - if AWAR_SPECIES_NAME contains the name of a pseudo gene-species then AWAR_ORGANISM_NAME
   contains the name of the species the pseudo gene-species originated from */
// #define AWAR_EXPERIMENT_NAME    "tmp/focus/experiment_name" /* string :  name of selected experiment */
#define AWAR_SAI_NAME           "tmp/focus/sai_name" /* string :  name of selected SAI (e.g. in EDIT4 and 'Manage SAI') */
#define AWAR_SAI_GLOBAL         "tmp/focus/sai_global" /* string :  name of selected SAI in EDIT4/Visualize SAI Tool - used in RNA3D Tool*/

#define AWAR_HELIX_NAME "tmp/focus/helix_name" /* string: name of selected helix, for helix numbers append _NR, get default by GBT_get_default_helix */

#define AWAR_ERROR_MESSAGES "tmp/message/displayed" /* error-messages: display awar */

#define AWAR_FOOTER     "tmp/LeftFooter" // mode line (showing help for selected mode)
#define AWAR_FOOTER_MAX_LEN 100 // max allowed textlen for mode line

#else
#error aw_awars.hxx included twice
#endif
