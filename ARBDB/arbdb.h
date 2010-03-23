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

// ---------------------
//      simple types

typedef int GB_COMPRESSION_MASK;

// ------------------------------
//      forward declare types

struct GB_NUMHASH;

struct GBS_regex;
struct GBS_string_matcher;

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
    // see ../WINDOW/aw_root.hxx@sync_GB_TYPES_AW_VARIABLE_TYPE

    GB_TYPE_MAX = 16,

    GB_CREATE_CONTAINER = GB_DB,
    GB_FIND             = GB_NONE,

};

enum GB_CASE {
    GB_IGNORE_CASE    = 0,
    GB_MIND_CASE      = 1,
    GB_CASE_UNDEFINED = 2
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

typedef void (*GB_CB)(GBDATA *, int *clientdata, GB_CB_TYPE gbtype);

typedef long (*gb_hash_loop_type)(const char *key, long val, void *client_data);
typedef int (*gbs_hash_compare_function) (const char *key0, long val0, const char *key1, long val1);

typedef const char* (*gb_export_sequence_cb)(GBDATA *gb_species, size_t *seq_len, GB_ERROR *error);

typedef GBDATA* (*GB_Link_Follower)(GBDATA *GB_root, GBDATA *GB_elem, const char *link);

typedef int (*gb_compare_function)(const void *p0, const void *p1, void *client_data);

typedef void (*gb_error_handler_type)(const char *msg);
typedef void (*gb_warning_func_type)(const char *msg);
typedef void (*gb_information_func_type)(const char *msg);
typedef int (*gb_status_gauge_func_type)(double val);
typedef int (*gb_status_msg_func_type)(const char *val);

// -----------------------
//      GB_transaction

class GB_transaction : Noncopyable {
    GBDATA *ta_main;
    bool      ta_open;          // is transaction open ?
    GB_ERROR  ta_err;

public:
    GB_transaction(GBDATA *gb_main);
    ~GB_transaction();

    bool ok() const { return ta_open && !ta_err; }  // ready to work on DB?
    GB_ERROR close(GB_ERROR error);                 // abort transaction if error (e.g.: 'return ta.close(error);')
    ARB_ERROR close(ARB_ERROR& error);              // abort transaction if error (e.g.: 'return ta.close(error);')
};


// --------------------------------------------
//      include generated public prototypes

#include <ad_prot.h>

// ----------------------------------------------------
//      const wrappers for functions from ad_prot.h

inline char *GBS_find_string(char *str, GB_CSTR key, int match_mode) {
    return const_cast<char*>(GBS_find_string(const_cast<GB_CSTR>(str), key, match_mode));
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
