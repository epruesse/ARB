// ----------------------
//     EDIT4 awars
// ----------------------

#define ED4_TEMP_AWAR        "/tmp/edit4/" // path for temporary awars
#define ED4_SEARCH_SAVE_BASE ED4_TEMP_AWAR "srchpara"

//      Level 1 Options

#define ED4_AWAR_SEQUENCE_INFO_WIDTH "sequence_info/width"
#define ED4_AWAR_TREE_WIDTH          "tree/width"

#define ED4_AWAR_COMPRESS_SEQUENCE_TYPE    "sequence/compression_type"
#define ED4_AWAR_COMPRESS_SEQUENCE_GAPS    "sequence/compression_type_gaps"
#define ED4_AWAR_COMPRESS_SEQUENCE_HIDE    "sequence/compression_type_hide"
#define ED4_AWAR_COMPRESS_SEQUENCE_PERCENT "sequence/compression_type_hide_percent"

#define ED4_AWAR_DIGITS_AS_REPEAT "edit/digits_as_repeat"
#define ED4_AWAR_FAST_CURSOR_JUMP "edit/fast_cursor_jump"
#define ED4_AWAR_CURSOR_TYPE      "edit/cursor_type"

#define ED4_AWAR_SCROLL_SPEED_X "scrolling/speed_x"
#define ED4_AWAR_SCROLL_SPEED_Y "scrolling/speed_y"

#define ED4_AWAR_GAP_CHARS                 "sequence/gap_chars"
#define ED4_AWAR_ANNOUNCE_CHECKSUM_CHANGES "sequence/announce_checksum_changes"

//      Expert Options

#define ED4_AWAR_GROUP_INDENT "group/indent" // Width of Group Bracket
#define ED4_AWAR_GROUP_COLOR  "group/color" // Color Nr for Brackets

//      consensus definition

#define ED4_AWAR_CONSENSUS_COUNTGAPS   "con/countgaps"
#define ED4_AWAR_CONSENSUS_GAPBOUND    "con/gapbound"
#define ED4_AWAR_CONSENSUS_GROUP       "con/group"
#define ED4_AWAR_CONSENSUS_CONSIDBOUND "con/considbound"
#define ED4_AWAR_CONSENSUS_UPPER       "con/upper"
#define ED4_AWAR_CONSENSUS_LOWER       "con/lower"
#define ED4_AWAR_CONSENSUS_SHOW        "con/show"

//      create species from consensus

#define ED4_AWAR_CREATE_FROM_CONS_REPL_EQUAL    "concreate/repl_equal"
#define ED4_AWAR_CREATE_FROM_CONS_REPL_POINT    "concreate/repl_point"
#define ED4_AWAR_CREATE_FROM_CONS_CREATE_POINTS "concreate/create_points"
#define ED4_AWAR_CREATE_FROM_CONS_ALL_UPPER     "concreate/all_upper"
#define ED4_AWAR_CREATE_FROM_CONS_DATA_SOURCE   "concreate/data_source"

//      Replace awars

#define ED4_AWAR_REP_SEARCH_PATTERN  "replace/spat"
#define ED4_AWAR_REP_REPLACE_PATTERN "replace/rpat"

//      NDS awars

#define ED4_AWAR_NDS_SELECT               "nds/select"
#define ED4_AWAR_NDS_SELECT_TEMPLATE      "nds/pat_%i/select"
#define ED4_AWAR_NDS_DESCRIPTION_TEMPLATE "nds/pat_%i/description"
#define ED4_AWAR_NDS_ACI_TEMPLATE         "nds/pat_%i/aci"
#define ED4_AWAR_NDS_WIDTH_TEMPLATE       "nds/pat_%i/width"
#define ED4_AWAR_NDS_BRACKETS             "nds/brackets"
#define ED4_AWAR_NDS_INFO_WIDTH           "nds/infowidth"
#define ED4_AWAR_NDS_ECOLI_NAME           "nds/ecoliname"

//      Search awars

#define ED4_AWAR_USER1_SEARCH_PATTERN        "search/user1/pattern"
#define ED4_AWAR_USER1_SEARCH_MIN_MISMATCHES "search/user1/min_mismatches"
#define ED4_AWAR_USER1_SEARCH_MAX_MISMATCHES "search/user1/max_mismatches"
#define ED4_AWAR_USER1_SEARCH_CASE           "search/user1/case"
#define ED4_AWAR_USER1_SEARCH_TU             "search/user1/tu"
#define ED4_AWAR_USER1_SEARCH_PAT_GAPS       "search/user1/pat_gaps"
#define ED4_AWAR_USER1_SEARCH_SEQ_GAPS       "search/user1/seq_gaps"
#define ED4_AWAR_USER1_SEARCH_SHOW           "search/user1/show"
#define ED4_AWAR_USER1_SEARCH_OPEN_FOLDED    "search/user1/open_folded"
#define ED4_AWAR_USER1_SEARCH_AUTO_JUMP      "search/user1/autoJump"
#define ED4_AWAR_USER1_SEARCH_REVERSE        "search/user1/reverse"
#define ED4_AWAR_USER1_SEARCH_COMPLEMENT     "search/user1/complement"
#define ED4_AWAR_USER1_SEARCH_EXACT          "search/user1/exact"

#define ED4_AWAR_USER2_SEARCH_PATTERN        "search/user2/pattern"
#define ED4_AWAR_USER2_SEARCH_MIN_MISMATCHES "search/user2/min_mismatches"
#define ED4_AWAR_USER2_SEARCH_MAX_MISMATCHES "search/user2/max_mismatches"
#define ED4_AWAR_USER2_SEARCH_CASE           "search/user2/case"
#define ED4_AWAR_USER2_SEARCH_TU             "search/user2/tu"
#define ED4_AWAR_USER2_SEARCH_PAT_GAPS       "search/user2/pat_gaps"
#define ED4_AWAR_USER2_SEARCH_SEQ_GAPS       "search/user2/seq_gaps"
#define ED4_AWAR_USER2_SEARCH_SHOW           "search/user2/show"
#define ED4_AWAR_USER2_SEARCH_OPEN_FOLDED    "search/user2/open_folded"
#define ED4_AWAR_USER2_SEARCH_AUTO_JUMP      "search/user2/autoJump"
#define ED4_AWAR_USER2_SEARCH_REVERSE        "search/user2/reverse"
#define ED4_AWAR_USER2_SEARCH_COMPLEMENT     "search/user2/complement"
#define ED4_AWAR_USER2_SEARCH_EXACT          "search/user2/exact"

#define ED4_AWAR_PROBE_SEARCH_PATTERN        AWAR_TARGET_STRING // s.a.: PROBE_DESIGN/probe_design.cxx
#define ED4_AWAR_PROBE_SEARCH_MIN_MISMATCHES AWAR_MIN_MISMATCHES
#define ED4_AWAR_PROBE_SEARCH_MAX_MISMATCHES AWAR_MAX_MISMATCHES
#define ED4_AWAR_PROBE_SEARCH_CASE           "search/probe/case"
#define ED4_AWAR_PROBE_SEARCH_TU             "search/probe/tu"
#define ED4_AWAR_PROBE_SEARCH_PAT_GAPS       "search/probe/pat_gaps"
#define ED4_AWAR_PROBE_SEARCH_SEQ_GAPS       "search/probe/seq_gaps"
#define ED4_AWAR_PROBE_SEARCH_SHOW           "search/probe/show"
#define ED4_AWAR_PROBE_SEARCH_OPEN_FOLDED    "search/probe/open_folded"
#define ED4_AWAR_PROBE_SEARCH_AUTO_JUMP      "search/probe/autoJump"
#define ED4_AWAR_PROBE_SEARCH_REVERSE        "search/probe/reverse"
#define ED4_AWAR_PROBE_SEARCH_COMPLEMENT     "search/probe/complement"
#define ED4_AWAR_PROBE_SEARCH_EXACT          "search/probe/exact"

#define ED4_AWAR_PRIMER1_SEARCH_PATTERN        AWAR_PRIMER_TARGET_STRING
#define ED4_AWAR_PRIMER1_SEARCH_MIN_MISMATCHES "search/primer1/min_mismatches"
#define ED4_AWAR_PRIMER1_SEARCH_MAX_MISMATCHES "search/primer1/max_mismatches"
#define ED4_AWAR_PRIMER1_SEARCH_CASE           "search/primer1/case"
#define ED4_AWAR_PRIMER1_SEARCH_TU             "search/primer1/tu"
#define ED4_AWAR_PRIMER1_SEARCH_PAT_GAPS       "search/primer1/pat_gaps"
#define ED4_AWAR_PRIMER1_SEARCH_SEQ_GAPS       "search/primer1/seq_gaps"
#define ED4_AWAR_PRIMER1_SEARCH_SHOW           "search/primer1/show"
#define ED4_AWAR_PRIMER1_SEARCH_OPEN_FOLDED    "search/primer1/open_folded"
#define ED4_AWAR_PRIMER1_SEARCH_AUTO_JUMP      "search/primer1/autoJump"
#define ED4_AWAR_PRIMER1_SEARCH_REVERSE        "search/primer1/reverse"
#define ED4_AWAR_PRIMER1_SEARCH_COMPLEMENT     "search/primer1/complement"
#define ED4_AWAR_PRIMER1_SEARCH_EXACT          "search/primer1/exact"

#define ED4_AWAR_PRIMER2_SEARCH_PATTERN        "search/primer2/pattern"
#define ED4_AWAR_PRIMER2_SEARCH_MIN_MISMATCHES "search/primer2/min_mismatches"
#define ED4_AWAR_PRIMER2_SEARCH_MAX_MISMATCHES "search/primer2/max_mismatches"
#define ED4_AWAR_PRIMER2_SEARCH_CASE           "search/primer2/case"
#define ED4_AWAR_PRIMER2_SEARCH_TU             "search/primer2/tu"
#define ED4_AWAR_PRIMER2_SEARCH_PAT_GAPS       "search/primer2/pat_gaps"
#define ED4_AWAR_PRIMER2_SEARCH_SEQ_GAPS       "search/primer2/seq_gaps"
#define ED4_AWAR_PRIMER2_SEARCH_SHOW           "search/primer2/show"
#define ED4_AWAR_PRIMER2_SEARCH_OPEN_FOLDED    "search/primer2/open_folded"
#define ED4_AWAR_PRIMER2_SEARCH_AUTO_JUMP      "search/primer2/autoJump"
#define ED4_AWAR_PRIMER2_SEARCH_REVERSE        "search/primer2/reverse"
#define ED4_AWAR_PRIMER2_SEARCH_COMPLEMENT     "search/primer2/complement"
#define ED4_AWAR_PRIMER2_SEARCH_EXACT          "search/primer2/exact"

#define ED4_AWAR_PRIMER3_SEARCH_PATTERN        "search/primer3/pattern"
#define ED4_AWAR_PRIMER3_SEARCH_MIN_MISMATCHES "search/primer3/min_mismatches"
#define ED4_AWAR_PRIMER3_SEARCH_MAX_MISMATCHES "search/primer3/max_mismatches"
#define ED4_AWAR_PRIMER3_SEARCH_CASE           "search/primer3/case"
#define ED4_AWAR_PRIMER3_SEARCH_TU             "search/primer3/tu"
#define ED4_AWAR_PRIMER3_SEARCH_PAT_GAPS       "search/primer3/pat_gaps"
#define ED4_AWAR_PRIMER3_SEARCH_SEQ_GAPS       "search/primer3/seq_gaps"
#define ED4_AWAR_PRIMER3_SEARCH_SHOW           "search/primer3/show"
#define ED4_AWAR_PRIMER3_SEARCH_OPEN_FOLDED    "search/primer3/open_folded"
#define ED4_AWAR_PRIMER3_SEARCH_AUTO_JUMP      "search/primer3/autoJump"
#define ED4_AWAR_PRIMER3_SEARCH_REVERSE        "search/primer3/reverse"
#define ED4_AWAR_PRIMER3_SEARCH_COMPLEMENT     "search/primer3/complement"
#define ED4_AWAR_PRIMER3_SEARCH_EXACT          "search/primer3/exact"

#define ED4_AWAR_SIG1_SEARCH_PATTERN        AWAR_GENE_CONTENT
#define ED4_AWAR_SIG1_SEARCH_MIN_MISMATCHES "search/sig1/min_mismatches"
#define ED4_AWAR_SIG1_SEARCH_MAX_MISMATCHES "search/sig1/max_mismatches"
#define ED4_AWAR_SIG1_SEARCH_CASE           "search/sig1/case"
#define ED4_AWAR_SIG1_SEARCH_TU             "search/sig1/tu"
#define ED4_AWAR_SIG1_SEARCH_PAT_GAPS       "search/sig1/pat_gaps"
#define ED4_AWAR_SIG1_SEARCH_SEQ_GAPS       "search/sig1/seq_gaps"
#define ED4_AWAR_SIG1_SEARCH_SHOW           "search/sig1/show"
#define ED4_AWAR_SIG1_SEARCH_OPEN_FOLDED    "search/sig1/open_folded"
#define ED4_AWAR_SIG1_SEARCH_AUTO_JUMP      "search/sig1/autoJump"
#define ED4_AWAR_SIG1_SEARCH_REVERSE        "search/sig1/reverse"
#define ED4_AWAR_SIG1_SEARCH_COMPLEMENT     "search/sig1/complement"
#define ED4_AWAR_SIG1_SEARCH_EXACT          "search/sig1/exact"

#define ED4_AWAR_SIG2_SEARCH_PATTERN        "search/sig2/pattern"
#define ED4_AWAR_SIG2_SEARCH_MIN_MISMATCHES "search/sig2/min_mismatches"
#define ED4_AWAR_SIG2_SEARCH_MAX_MISMATCHES "search/sig2/max_mismatches"
#define ED4_AWAR_SIG2_SEARCH_CASE           "search/sig2/case"
#define ED4_AWAR_SIG2_SEARCH_TU             "search/sig2/tu"
#define ED4_AWAR_SIG2_SEARCH_PAT_GAPS       "search/sig2/pat_gaps"
#define ED4_AWAR_SIG2_SEARCH_SEQ_GAPS       "search/sig2/seq_gaps"
#define ED4_AWAR_SIG2_SEARCH_SHOW           "search/sig2/show"
#define ED4_AWAR_SIG2_SEARCH_OPEN_FOLDED    "search/sig2/open_folded"
#define ED4_AWAR_SIG2_SEARCH_AUTO_JUMP      "search/sig2/autoJump"
#define ED4_AWAR_SIG2_SEARCH_REVERSE        "search/sig2/reverse"
#define ED4_AWAR_SIG2_SEARCH_COMPLEMENT     "search/sig2/complement"
#define ED4_AWAR_SIG2_SEARCH_EXACT          "search/sig2/exact"

#define ED4_AWAR_SIG3_SEARCH_PATTERN        "search/sig3/pattern"
#define ED4_AWAR_SIG3_SEARCH_MIN_MISMATCHES "search/sig3/min_mismatches"
#define ED4_AWAR_SIG3_SEARCH_MAX_MISMATCHES "search/sig3/max_mismatches"
#define ED4_AWAR_SIG3_SEARCH_CASE           "search/sig3/case"
#define ED4_AWAR_SIG3_SEARCH_TU             "search/sig3/tu"
#define ED4_AWAR_SIG3_SEARCH_PAT_GAPS       "search/sig3/pat_gaps"
#define ED4_AWAR_SIG3_SEARCH_SEQ_GAPS       "search/sig3/seq_gaps"
#define ED4_AWAR_SIG3_SEARCH_SHOW           "search/sig3/show"
#define ED4_AWAR_SIG3_SEARCH_OPEN_FOLDED    "search/sig3/open_folded"
#define ED4_AWAR_SIG3_SEARCH_AUTO_JUMP      "search/sig3/autoJump"
#define ED4_AWAR_SIG3_SEARCH_REVERSE        "search/sig3/reverse"
#define ED4_AWAR_SIG3_SEARCH_COMPLEMENT     "search/sig3/complement"
#define ED4_AWAR_SIG3_SEARCH_EXACT          "search/sig3/exact"

//      Other internal awars

#define ED4_AWAR_SPECIES_TO_CREATE ED4_TEMP_AWAR "species_to_create"















