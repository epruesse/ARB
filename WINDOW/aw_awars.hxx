#ifndef aw_awars_hxx_included
#define aw_awars_hxx_included

#define CHANGE_KEY_PATH             "presets/key_data"
#define CHANGE_KEY_PATH_GENES       "presets/gene_key_data"
#define CHANGEKEY                   "key"
#define CHANGEKEY_NAME              "key_name"
#define CHANGEKEY_TYPE              "key_type"

#define AWAR_DB	                    "tmp/nt/arbdb/"
#define AWAR_DB_PATH                AWAR_DB "file_name"
#define AWAR_DEFAULT_ALIGNMENT      "presets/use"

#define AWAR_TREE                   "focus/tree_name"
#define AWAR_CONFIGURATION          "focus/configuration"

#define AWAR_SECURITY_LEVEL         "/tmp/etc/sequrity_level"
#define AWAR_GDE_EXPORT_FILTER      "/tmp/gde/export_filter"

#define AWAR_DB_COMMENT             "description"

#define AWAR_PT_SERVER	            "nt/pt_server"
#define AWAR_TARGET_STRING          "nt/target_string"
#define AWAR_MIN_MISMATCHES         "nt/min_mismatches"
#define AWAR_MAX_MISMATCHES         "nt/max_mismatches"
#define AWAR_ITARGET_STRING         "nt/itarget_string"

/* local awars for edit and edit4: */

#define AWAR_SPECIES_NAME_LOCAL     "tmp/edit/species_name"		/* string: name of selected species */
#define AWAR_CURSOR_POSITION_LOCAL  "tmp/edit/cursor_position"	/* int: position in sequence */
#define AWAR_EDITOR_ALIGNMENT       "tmp/edit/alignment"		/* string: alignment used by editor */

/* global awars */

#define AWAR_CURSOR_POSITION        "tmp/focus/cursor_position"     /* int: position in sequence */
#define AWAR_SET_CURSOR_POSITION    "tmp/focus/set_cursor_position" /* int: set position in sequence (remote control for ARB_EDIT4) */
#define AWAR_SPECIES_NAME           "tmp/focus/species_name"	    /* string: name of selected species */
#define AWAR_GENE_NAME              "tmp/focus/gene_name"	    /* string: name of selected gene */
#define AWAR_HELIX_NAME             "tmp/focus/helix_name"		    /* string: name of selected helix, for helix numbers append _NR, get default by GBT_get_default_helix */
#define AWAR_ERROR_MESSAGES         "tmp/Message"		            /* error: messages */




#endif
