// ================================================================ //
//                                                                  //
//   File      : arbdb.h                                            //
//   Purpose   : external ARB DB interface                          //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef ARBDB_H
#define ARBDB_H

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_ERROR_H
#include <arb_error.h>
#endif
#ifndef _STDINT_H
#include <stdint.h>
#endif

#define GB_SYSTEM_FOLDER   "__SYSTEM__"
#define GB_SYSTEM_KEY_DATA "@key_data"

#define GB_DEFAULT_ALIGNMENT "presets/use" // has to match ../WINDOW/aw_awar_defs.hxx@AWAR_DEFAULT_ALIGNMENT

// ---------------------
//      simple types

typedef int GB_COMPRESSION_MASK;

// ------------------------------
//      forward declare types

struct GB_NUMHASH;

struct GBS_regex;
struct GBS_string_matcher;
struct GBS_strstruct;
struct GEN_position;
struct DictData;
struct CharPtrArray;
struct StrArray;
struct ConstStrArray;

// -------------------
//      constants

#define GB_USERFLAG_ANY       127  // maximum for gb_flag_types2::usr_ref
#define GB_USERFLAG_QUERY     1    // bit used for search&query (used on species, genes, experiments)
#define GB_USERFLAG_GHOSTNODE 1    // bit used by gbt_write_tree (only used on tree-nodes; so it does not clash with GB_USERFLAG_QUERY)

// --------------
//      enums

enum GB_TYPES {                                     // supported DB entry types
    GB_NONE        = 0,
    GB_BIT         = 1,
    GB_BYTE        = 2,
    GB_INT         = 3,
    GB_FLOAT       = 4,
    GB_POINTER     = 5,                             // not savable! only allowed in temporary entries
    GB_BITS        = 6,
    // 7 is unused
    GB_BYTES       = 8,
    GB_INTS        = 9,
    GB_FLOATS      = 10,
    GB_LINK        = 11,
    GB_STRING      = 12,
    GB_STRING_SHRT = 13,                            // used automatically during save
    // 14 is unused
    GB_DB          = 15,

    // keep GB_TYPES consistent with AW_VARIABLE_TYPE
    // see ../WINDOW/aw_base.hxx@sync_GB_TYPES_AW_VARIABLE_TYPE

    GB_TYPE_MAX = 16,

    GB_CREATE_CONTAINER = GB_DB,
    GB_FIND             = GB_NONE,

};

enum GB_SEARCH_TYPE {
    SEARCH_BROTHER       = 1,                       // [was: this_level]
    SEARCH_CHILD         = 2,                       // [was: down_level]
    SEARCH_GRANDCHILD    = 4,                       // [was: down_2_level]
    SEARCH_NEXT_BROTHER  = SEARCH_BROTHER+8,        // [was: this_level|search_next]
    SEARCH_CHILD_OF_NEXT = SEARCH_CHILD+8,          // [was: down_level|search_next]
};

enum GB_UNDO_TYPE {
    GB_UNDO_NONE,                                   // no undo
    GB_UNDO_KILL,                                   // no undo and delete all old undos
    GB_UNDO_UNDO,                                   // normal undo -> deleted all redoes
    GB_UNDO_REDO,                                   // moves to UNDO_REDO
    GB_UNDO_UNDO_REDO                               // internal makes undo redoable
};


// -----------------------
//      callback types

typedef long (*gb_hash_loop_type)(const char *key, long val, void *client_data);
typedef void (*gb_hash_const_loop_type)(const char *key, long val, void *client_data);
typedef int (*gbs_hash_compare_function) (const char *key0, long val0, const char *key1, long val1);

typedef const char* (*gb_export_sequence_cb)(GBDATA *gb_species, size_t *seq_len, GB_ERROR *error);

typedef GBDATA* (*GB_Link_Follower)(GBDATA *GB_root, GBDATA *GB_elem, const char *link);

typedef const char *(*gb_getenv_hook)(const char *varname);

// -----------------------
//      GB_transaction

class GB_transaction : virtual Noncopyable {
    GBDATA   *ta_main;
    bool      ta_open;          // is transaction open ?
    GB_ERROR  ta_err;

    void init(GBDATA *gb_main, bool initial);
protected:
    GB_transaction(GBDATA *gb_main, bool) { init(gb_main, true); }
public:
    GB_transaction(GBDATA *gb_main) { init(gb_main, false); }
    ~GB_transaction();

    bool ok() const { return ta_open && !ta_err; }  // ready to work on DB?
    GB_ERROR close(GB_ERROR error);                 // abort transaction if error (e.g.: 'return ta.close(error);')
    ARB_ERROR close(ARB_ERROR& error);              // abort transaction if error (e.g.: 'return ta.close(error);')
};

struct GB_initial_transaction : public GB_transaction {
    GB_initial_transaction(GBDATA *gb_main) : GB_transaction(gb_main, true) {}
};

class GB_shell {
    // initialize and cleanup module ARBDB
    // No database usage is possible when no GB_shell exists!
public: 
    GB_shell();
    ~GB_shell();

    static void ensure_inside();
    static bool in_shell();
};

// --------------------------------------------

#if defined(DEBUG)
struct GB_SizeInfo {
    long containers; // no of containers
    long terminals;  // no of terminals
    long structure;  // structure size
    long data;       // data size
    long mem;        // data memory size (compressed)

    GB_SizeInfo() : containers(0), terminals(0), structure(0), data(0), mem(0) {}

    void collect(GBDATA *gbd);
};
#endif

// --------------------------------------------
//      include generated public prototypes

#include <ad_prot.h>

// to avoid arb-wide changes atm include some headers from CORE lib
#ifndef ARB_MSG_H
#include <arb_msg.h>
#endif
#ifndef ARB_STRING_H
#include <arb_string.h>
#endif

// ----------------------------------------------------
//      const wrappers for functions from ad_prot.h

inline char *GBS_find_string(char *content, GB_CSTR key, int match_mode) {
    return const_cast<char*>(GBS_find_string(const_cast<GB_CSTR>(content), key, match_mode));
}

// ---------------------------------
//      error delivery functions

inline void GB_end_transaction_show_error(GBDATA *gbd, ARB_ERROR& error, void (*error_handler)(GB_ERROR)) {
    GB_end_transaction_show_error(gbd, error.deliver(), error_handler);
}
inline ARB_ERROR GB_end_transaction(GBDATA *gbd, ARB_ERROR& error) {
    return GB_end_transaction(gbd, error.deliver());
}


#else
#error arbdb.h included twice
#endif // ARBDB_H
