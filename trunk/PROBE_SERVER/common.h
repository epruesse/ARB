//  ==================================================================== //
//                                                                       //
//    File      : common.h                                               //
//    Purpose   : Common code for all tools                              //
//    Note      : include only once in each executable!!!                //
//    Time-stamp: <Fri Oct/01/2004 20:19 MET Coder@ReallySoft.de>        //
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

#ifdef NEED_setDatabaseState
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

#ifdef NEED_getDatabaseState
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

// encoding/decoding tree node strings
// (Note: keep ../PROBE_WEB/CLIENT/TreeParser.java/decodeNodeInfo() up-to-date!)
//
// tree node strings may not contain ',' and ';'
// performed escapes :
//       _   <-> __
//       ,   <-> _.
//       ;   <-> _s
//       :   <-> _c
//       '   <-> _q

#if (defined(NEED_encodeTreeNode) || defined(NEED_decodeTreeNode))
static const char *PG_tree_node_escaped_chars = "_,;:'";
static const char *PG_tree_node_replace_chars = "_.scq";
#endif

#ifdef NEED_encodeTreeNode
static char *encodeTreeNode(const char *str) {
    int   len    = strlen(str);
    char *result = (char*)GB_calloc(2*len+1, 1);
    int   j      = 0;

    for (int i = 0; i<len; ++i) {
        const char *found = strchr(PG_tree_node_escaped_chars, str[i]);
        if (found) {
            result[j++] = '_';
            result[j++] = PG_tree_node_replace_chars[found-PG_tree_node_escaped_chars];
        }
        else {
            result[j++] = str[i];
        }
    }
    return result;
}
#endif

#ifdef NEED_decodeTreeNode
static char *decodeTreeNode(const char *str, GB_ERROR& error) {
    int   len    = strlen(str);
    char *result = (char*)GB_calloc(len+1, 1);
    int   j      = 0;

    for (int i = 0; i<len; ++i) {
        if (str[i] == '_') {
            char        c     = str[++i];
            const char *found = strchr(PG_tree_node_replace_chars, c);
            if (!found) {
                error = GBS_global_string("Illegal character '%c' after '_'", c);
                free(result);
                return 0;
            }

            result[j++] = PG_tree_node_escaped_chars[found-PG_tree_node_replace_chars];
        }
        else {
            result[j++] = str[i];
        }
    }

    return result;
}
#endif

#ifdef NEED_saveProbeContainerToString
template <typename C>
static GB_ERROR saveProbeContainerToString(GBDATA *gb_father, const char *entry_name, bool allow_overwrite,
                                           typename C::const_iterator start, typename C::const_iterator end) {
    GB_ERROR  error    = 0;
    GBDATA   *gb_entry = GB_find(gb_father, entry_name, 0, down_level);

    if (gb_entry && !allow_overwrite) {
        error = "tried to overwrite entry!";
    }
    else {
        if (!gb_entry) gb_entry = GB_create(gb_father, entry_name, GB_STRING);

        if (!gb_entry) {
            error = GB_get_error();
            if (!error) {
                error = GBS_global_string("Could not create entry '%s' (unknown reason)", entry_name);
            }
        }
        else {
            std::string data;
            {
                int len   = 0;
                int count = 0;

                for (typename C::const_iterator i = start; i != end; ++i) {
                    len += i->length();
                    ++count;
                }
                len += count-1;
                data.reserve(len);
            }
            for (typename C::const_iterator i = start; i != end; ++i) {
                if (!data.empty()) data.append(1, ',');
                data.append(*i);
            }

            error = GB_write_string(gb_entry, data.c_str());
        }
    }
    return error;
}
#endif

#else
#error common.h included twice
#endif // COMMON_H

