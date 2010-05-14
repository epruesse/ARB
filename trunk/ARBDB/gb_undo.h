// =============================================================== //
//                                                                 //
//   File      : gb_undo.h                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_UNDO_H
#define GB_UNDO_H

#ifndef GB_LOCAL_H
#include "gb_local.h"
#endif

struct gb_transaction_save;
struct g_b_undo_list;

enum g_b_undo_entry_type {
    GB_UNDO_ENTRY_TYPE_DELETED,
    GB_UNDO_ENTRY_TYPE_CREATED,
    GB_UNDO_ENTRY_TYPE_MODIFY,
    GB_UNDO_ENTRY_TYPE_MODIFY_ARRAY
};

struct g_b_undo_gbd {
    GBQUARK  key;
    GBDATA  *gbd;
};

struct g_b_undo_entry {
    g_b_undo_list  *father;
    g_b_undo_entry *next;
    short           type;
    short           flag;

    GBDATA *source;                                 // The original(changed) element or father(of deleted)
    int     gbm_index;
    long    sizeof_this;
    union {
        gb_transaction_save *ts;
        g_b_undo_gbd         gs;
    } d;
};

struct g_b_undo_header {
    g_b_undo_list *stack;
    long           sizeof_this;                     // the size of all existing undos
    long           nstack;                          // number of available undos
};

struct g_b_undo_list {
    g_b_undo_header *father;
    g_b_undo_entry  *entries;
    g_b_undo_list   *next;
    long             time_of_day;                   // the begin of the transaction
    long             sizeof_this;                   // the size of one undo
};

struct g_b_undo_mgr {
    long             max_size_of_all_undos;
    g_b_undo_list   *valid_u;
    g_b_undo_header *u;                             // undo
    g_b_undo_header *r;                             // redo
};

#else
#error gb_undo.h included twice
#endif // GB_UNDO_H
