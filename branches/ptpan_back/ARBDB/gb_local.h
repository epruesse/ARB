// =============================================================== //
//                                                                 //
//   File      : gb_local.h                                        //
//   Purpose   : declarations needed to include local prototypes   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_LOCAL_H
#define GB_LOCAL_H

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#define gb_assert(cond) arb_assert(cond)

// ------------------
//      constants

#define GB_MAX_USERS 4

#define GBTUM_MAGIC_NUMBER        0x17488400
#define GBTUM_MAGIC_NUMBER_FILTER 0xffffff00
#define GBTUM_MAGIC_REVERSED      0x00844817

#define GB_SYSTEM_FOLDER   "__SYSTEM__"

// ---------------------
//      simple types

typedef short GB_MAIN_IDX;      // random-index

// ------------------------------
//      forward declare types

struct GBCONTAINER;
struct GB_MAIN_TYPE;

struct gb_transaction_save;
struct gb_header_list;
struct gb_index_files;

struct GB_DICTIONARY;
struct gb_compress_list;
struct gb_compress_tree;

struct gb_map_header;

struct gb_scandir;

struct gbcmc_comm;

// -------------------
//      enum types

enum gb_undo_commands {
    _GBCMC_UNDOCOM_REQUEST_NOUNDO_KILL,
    _GBCMC_UNDOCOM_REQUEST_NOUNDO,
    _GBCMC_UNDOCOM_REQUEST_UNDO,
    _GBCMC_UNDOCOM_INFO_UNDO,
    _GBCMC_UNDOCOM_INFO_REDO,
    _GBCMC_UNDOCOM_UNDO,
    _GBCMC_UNDOCOM_REDO,

    _GBCMC_UNDOCOM_SET_MEM = 10000      // Minimum
};

enum GB_CHANGE {
    GB_UNCHANGED         = 0,
    GB_SON_CHANGED       = 2,
    GB_NORMAL_CHANGE     = 4,
    GB_CREATED           = 5,
    GB_DELETED           = 6,
    GB_DELETED_IN_MASTER = 7
};

enum GB_COMPRESSION_TYPES {
    GB_COMPRESSION_NONE       = 0,
    GB_COMPRESSION_RUNLENGTH  = 1,
    GB_COMPRESSION_HUFFMANN   = 2,
    GB_COMPRESSION_DICTIONARY = 4,
    GB_COMPRESSION_SEQUENCE   = 8,
    GB_COMPRESSION_SORTBYTES  = 16,
    GB_COMPRESSION_BITS       = 32,
    GB_COMPRESSION_LAST       = 128
};

enum GBCM_ServerResultCode {
    GBCM_SERVER_OK      = 0,
    GBCM_SERVER_FAULT   = 1,
    GBCM_SERVER_ABORTED = 2,
    GBCM_SERVER_OK_WAIT = 3,
};

class GBCM_ServerResult {
    GBCM_ServerResultCode code;
    GB_ERROR              error;

    GBCM_ServerResult(GBCM_ServerResultCode code_) : code(code_), error(NULL) {
        gb_assert(code == GBCM_SERVER_OK || code == GBCM_SERVER_OK_WAIT || code == GBCM_SERVER_ABORTED);
    }
    GBCM_ServerResult(GBCM_ServerResultCode code_, GB_ERROR error_) : code(code_), error(error_) {
        gb_assert(code == GBCM_SERVER_FAULT);
    }
public:
    GBCM_ServerResult(const GBCM_ServerResult& other) {
        gb_assert(!error); // dropping errors not permitted :p
        code  = other.code;
        error = other.error;
    }
    static GBCM_ServerResult OK() { return GBCM_ServerResult(GBCM_SERVER_OK); }
    static GBCM_ServerResult OK_WAIT() { return GBCM_ServerResult(GBCM_SERVER_OK_WAIT); }
    static GBCM_ServerResult ABORTED() { return GBCM_ServerResult(GBCM_SERVER_ABORTED); }
    static GBCM_ServerResult FAULT(GB_ERROR error) { return GBCM_ServerResult(GBCM_SERVER_FAULT, error); }

    bool ok() const { return code == GBCM_SERVER_OK; }
    bool failed() const { return !ok(); }

    GBCM_ServerResultCode get_code() const { return code; }
    GB_ERROR get_error() const { return error; }

    void annotate(const char *msg) { error = GBS_global_string("%s (reason: %s)", msg, error); }
};

// ------------------------------------------------------
//      include generated local prototypes and macros

#ifndef GB_PROT_H
#include "gb_prot.h"
#endif


#else
#error gb_local.h included twice
#endif // GB_LOCAL_H
