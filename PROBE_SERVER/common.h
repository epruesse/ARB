//  ==================================================================== //
//                                                                       //
//    File      : common.h                                               //
//    Purpose   : Common code for all tools                              //
//    Note      : include only once in each executable!!!                //
//    Time-stamp: <Sun Sep/14/2003 08:47 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef COMMON_H
#define COMMON_H

#ifndef SKIP_SETDATABASESTATE
static GB_ERROR setDatabaseState(GBDATA *gb_main, const char *type, const char *state) {
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        // optionally set database type
        if (type) {
            GBDATA *gb_type = GB_search(gb_main, "database_type", GB_STRING);

            if (gb_type) error = GB_write_string(gb_type, type);
            else error         = GB_get_error();
        }
        else {
            GBDATA *gb_type = GB_search(gb_main, "database_type", GB_FIND);
            if (!gb_type) {
                error = GB_export_error("setDatabaseState() needs 'type' when called the first time.");
            }
        }
    }

    if (!error) {
        GBDATA *gb_state = GB_search(gb_main, "database_state", GB_STRING);

        if (gb_state) error = GB_write_string(gb_state, state);
        else error          = GB_get_error();
    }

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}
#endif

#ifndef SKIP_GETDATABASESTATE
static GB_ERROR getDatabaseState(GBDATA *gb_main, const char **typePtr, const char **statePtr) {
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error && typePtr) {
        GBDATA *gb_type = GB_search(gb_main, "database_type", GB_FIND);
        if (gb_type) {
            *typePtr             = GB_read_char_pntr(gb_type);
            if (!*typePtr) error = GB_get_error();
        }
        else {
            *typePtr = 0;
            error    = GB_get_error();
        }
    }
    if (!error && statePtr) {
        GBDATA *gb_type = GB_search(gb_main, "database_state", GB_FIND);
        if (gb_type) {
            *statePtr             = GB_read_char_pntr(gb_type);
            if (!*statePtr) error = GB_get_error();
        }
        else {
            *statePtr = 0;
            error     = GB_get_error();
        }
    }

    GB_pop_transaction(gb_main);
    return error;
}

static GB_ERROR checkDatabaseType(GBDATA *gb_main, const char *db_name, const char *expectedType, const char *expectedState) {
    const char *type, *state;
    GB_ERROR    error = getDatabaseState(gb_main, &type, &state);

    if (!error) {
        if (expectedType) {
            if (!type) {
                error = "type information not found";
            }
            else if (strcmp(type, expectedType) != 0) {
                error = GBS_global_string("type mismatch (expected='%s' found='%s')", expectedType, type);
            }
        }

        if (!error && expectedState) {
            if (!state) {
                error = "state information not found";
            }
            else if (strcmp(state, expectedState) != 0) {
                error = GBS_global_string("state mismatch (expected='%s' found='%s')", expectedState, state);
            }
        }
    }

    if (error) {
        error = GBS_global_string("database %s in '%s'", error, db_name);
    }

    return error;
}


#endif

#else
#error common.h included twice
#endif // COMMON_H

