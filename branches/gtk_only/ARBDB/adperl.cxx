// =============================================================== //
//                                                                 //
//   File      : adperl.cxx                                        //
//   Purpose   : helper functions used by perl interface           //
//               (see ../PERL2ARB)                                 //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_local.h"
#include "adperl.h"

// used by perl interface, see ../PERL2ARB/ARB_ext.c@GBP_croak_function
void (*GBP_croak_function)(const char *message) = NULL;

static void die(const char *with_last_words) {
    // raise exception in caller (assuming caller is a perl script)

    if (GBP_croak_function) {
        GBP_croak_function(with_last_words);
    }
    else {
        fputs("Warning: GBP_croak_function undefined. terminating..\n", stderr);
        GBK_terminate(with_last_words);
    }
}

GB_shell4perl::~GB_shell4perl() {
    gb_close_unclosed_DBs();
}

// -------------------------------------------
//      "generic" enum<->string-conversion

union known_enum {
    int as_int;

    GB_SEARCH_TYPE    search_type;
    GB_CASE           case_sensitivity;
    GB_TYPES          db_type;
    GB_UNDO_TYPE      undo_type;
    GB_alignment_type ali_type;
};

#define ILLEGAL_VALUE (-666)


typedef const char *(*enum2string)(known_enum enumValue);

static known_enum next_known_enum_value(known_enum greaterThan, enum2string lookup) {
    known_enum enumValue, lookupLimit;

    enumValue.as_int   = greaterThan.as_int+1;
    lookupLimit.as_int = enumValue.as_int+256;

    while (enumValue.as_int <= lookupLimit.as_int) {
        const char *valueExists = lookup(enumValue);
        if (valueExists) return enumValue;
        enumValue.as_int++;
    }

    enumValue.as_int = ILLEGAL_VALUE;
    return enumValue;
}

static known_enum first_known_enum_value(known_enum greaterEqualThan, enum2string lookup) {
    return (lookup(greaterEqualThan))
        ? greaterEqualThan
        : next_known_enum_value(greaterEqualThan, lookup);
}

static known_enum string2enum(const char *string, enum2string lookup, known_enum start) {

    for (start = first_known_enum_value(start, lookup);
         start.as_int != ILLEGAL_VALUE;
         start = next_known_enum_value(start, lookup))
    {
        const char *asString = lookup(start);
        gb_assert(asString);
        if (strcasecmp(asString, string) == 0) break; // found
    }
    return start;
}

static char *buildAllowedValuesString(known_enum start, enum2string lookup) {
    char *allowed = NULL;

    for (start = first_known_enum_value(start, lookup);
         start.as_int != ILLEGAL_VALUE;
         start = next_known_enum_value(start, lookup))
    {
        const char *asString = lookup(start);
        gb_assert(asString);

        if (allowed) freeset(allowed, GBS_global_string_copy("%s, '%s'", allowed, asString));
        else allowed = GBS_global_string_copy("'%s'", asString);
    }

    if (!allowed) allowed = strdup("none (this is a bug)");

    return allowed;
}

static known_enum string2enum_or_die(const char *enum_name, const char *string, enum2string lookup, known_enum start) {
    known_enum found = string2enum(string, lookup, start);

    if (found.as_int == ILLEGAL_VALUE) {
        char *allowed_values = buildAllowedValuesString(start, lookup);
        char *usage          = GBS_global_string_copy("Error: value '%s' is not a legal %s\n"
                                                      "Known %ss are: %s",
                                                      string, enum_name, enum_name, allowed_values);
        free(allowed_values);
        die(usage);
    }

    return found;
}

/* --------------------------------------------------------------------------------
 * conversion declarations for different used enums
 *
 * To add a new enum type
 * - write a function to convert your enum-values into a string (example: GBP_gb_search_types_to_string)
 * - write a reverse-wrapper                                    (example: GBP_string_to_gb_search_types)
 *
 * [Code-Tag: enum_conversion_functions]
 * see also ../PERLTOOLS/arb_proto_2_xsub.cxx@enum_type_replacement
 */

// ------------------------
//      GB_SEARCH_TYPE

const char *GBP_GB_SEARCH_TYPE_2_charPtr(GB_SEARCH_TYPE search_type) {
    switch (search_type) {
        case SEARCH_BROTHER:       return "brother";
        case SEARCH_CHILD:         return "child";
        case SEARCH_GRANDCHILD:    return "grandchild";
        case SEARCH_NEXT_BROTHER:  return "next_brother";
        case SEARCH_CHILD_OF_NEXT: return "child_of_next";
    }

    return NULL;
}

GB_SEARCH_TYPE GBP_charPtr_2_GB_SEARCH_TYPE(const char *search_mode) {
    known_enum start; start.as_int = 0;
    known_enum found  = string2enum_or_die("search-type", search_mode, (enum2string)GBP_GB_SEARCH_TYPE_2_charPtr, start);
    return found.search_type;
}

// ------------------
//      GB_TYPES

const char *GBP_GB_TYPES_2_charPtr(GB_TYPES type) {
    switch (type) {
        case GB_NONE:   return "NONE";
        case GB_BIT:    return "BIT";
        case GB_BYTE:   return "BYTE";
        case GB_INT:    return "INT";
        case GB_FLOAT:  return "FLOAT";
        case GB_BITS:   return "BITS";
        case GB_BYTES:  return "BYTES";
        case GB_INTS:   return "INTS";
        case GB_FLOATS: return "FLOATS";
        case GB_STRING: return "STRING";
        case GB_DB:     return "CONTAINER";

        default: break;
    }
    return NULL;
}

GB_TYPES GBP_charPtr_2_GB_TYPES(const char *type_name) {
    known_enum start; start.as_int = 0;
    known_enum found  = string2enum_or_die("db-type", type_name, (enum2string)GBP_GB_TYPES_2_charPtr, start);
    return found.db_type;
}


// ----------------------
//      GB_UNDO_TYPE

const char *GBP_GB_UNDO_TYPE_2_charPtr(GB_UNDO_TYPE undo_type) {
    switch (undo_type) {
        case GB_UNDO_UNDO: return "undo";
        case GB_UNDO_REDO: return "redo";

        case GB_UNDO_NONE:
        case GB_UNDO_KILL:
        case GB_UNDO_UNDO_REDO:
            break;
    }
    return NULL;
}

GB_UNDO_TYPE GBP_charPtr_2_GB_UNDO_TYPE(const char *undo_type) {
    known_enum start; start.as_int = 0;
    known_enum found  = string2enum_or_die("undo-type", undo_type, (enum2string)GBP_GB_UNDO_TYPE_2_charPtr, start);
    return found.undo_type;
}


// -----------------
//      GB_CASE

const char *GBP_GB_CASE_2_charPtr(GB_CASE sensitivity) {
    switch (sensitivity) {
        case GB_IGNORE_CASE:    return "ignore_case";
        case GB_MIND_CASE:      return "mind_case";
        case GB_CASE_UNDEFINED: return "case_undef";
    }
    return NULL;
}

GB_CASE GBP_charPtr_2_GB_CASE(const char *sensitivity) {
    known_enum start; start.as_int = 0;
    known_enum found  = string2enum_or_die("sensitivity", sensitivity, (enum2string)GBP_GB_CASE_2_charPtr, start);
    return found.case_sensitivity;
}

// ---------------------------
//      GB_alignment_type

const char *GBP_GB_alignment_type_2_charPtr(GB_alignment_type ali_type) {
    switch (ali_type) {
        case GB_AT_RNA: return "RNA";
        case GB_AT_DNA: return "DNA";
        case GB_AT_AA:  return "AMINO";

        case GB_AT_UNKNOWN: break;
    }
    return NULL;
}

GB_alignment_type GBP_charPtr_2_GB_alignment_type(const char *ali_type) {
    known_enum start; start.as_int = 0;
    known_enum found  = string2enum_or_die("alignment-type", ali_type, (enum2string)GBP_GB_alignment_type_2_charPtr, start);
    return found.ali_type;
}

// -----------------------------------------
//      wrap functions moved to CORE lib
//
// As long as CORE lib is not xsub'd, we use wrappers for some
// functions used in perl

GB_ERROR GBC_await_error() {
    return GB_await_error();
}


