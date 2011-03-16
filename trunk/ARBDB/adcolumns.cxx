// =============================================================== //
//                                                                 //
//   File      : adcolumns.cxx                                     //
//   Purpose   : insert/delete columns                             //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <adGene.h>
#include <arb_progress.h>

#include "gb_local.h"

static char   *insDelBuffer = 0;
static size_t  insDelBuffer_size;

inline void free_insDelBuffer() {
    freenull(insDelBuffer);
}

static const char *gbt_insert_delete(const char *source, long srclen, long destlen, long *newlenPtr, long pos, long nchar, long mod, char insert_what, char insert_tail, int extraByte) {
    /* removes elems from or inserts elems into an array
     *
     * srclen           len of source
     * destlen          if != 0, then cut or append characters to get this len, otherwise keep srclen
     * newlenPtr        the resulting len
     * pos              where to insert/delete
     * nchar            and how many items
     * mod              size of an item
     * insert_what      insert this character (mod times)
     * insert_tail      append this character (if destlen>srclen)
     * extraByte        0 or 1. append extra zero byte at end? use 1 for strings!
     *
     * resulting array has destlen+nchar elements
     *
     * 1. array size is corrected to 'destlen' (by appending/cutting tail)
     * 2. part is deleted inserted
     */

    const char *result;

    pos     *= mod;
    nchar   *= mod;
    srclen  *= mod;
    destlen *= mod;

    if (!destlen) destlen                       = srclen; // if no destlen is set then keep srclen
    if ((nchar<0) && (pos-nchar>destlen)) nchar = pos-destlen; // clip maximum characters to delete at end of array

    if (destlen == srclen && (pos>srclen || nchar == 0)) { // length stays same and clip-range is empty or behind end of sequence
        /* before 26.2.09 the complete data was copied in this case - but nevertheless NULL(=failure) was returned.
         * I guess this was some test accessing complete data w/o writing anything back to DB,
         * but AFAIK it was not used anywhere --ralf
         */
        result = NULL;
    }
    else {
        long newlen = destlen+nchar;                // length of result (w/o trailing zero-byte)
        if (newlen == 0) {
            result = "";
        }
        else {
            size_t neededSpace = newlen+extraByte;

            if (insDelBuffer && insDelBuffer_size<neededSpace) freenull(insDelBuffer);
            if (!insDelBuffer) {
                insDelBuffer_size = neededSpace;
                insDelBuffer      = (char*)malloc(neededSpace);
            }

            char *dest = insDelBuffer;
            gb_assert(dest);

            if (pos>srclen) {                       // insert/delete happens inside appended range
                insert_what = insert_tail;
                pos         = srclen;               // insert/delete directly after source, to avoid illegal access below
            }

            gb_assert(pos >= 0);
            if (pos>0) {                            // copy part left of pos
                memcpy(dest, source, (size_t)pos);
                dest   += pos;
                source += pos; srclen -= pos;
            }

            if (nchar>0) {                          // insert
                memset(dest, insert_what, (size_t)nchar);
                dest += nchar;
            }
            else if (nchar<0) {                     // delete
                source += -nchar; srclen -= -nchar;
            }

            if (srclen>0) {                         // copy rest of source
                memcpy(dest, source, (size_t)srclen);
                dest   += srclen;
                source += srclen; srclen = 0;
            }

            long rest = newlen-(dest-insDelBuffer);
            gb_assert(rest >= 0);

            if (rest>0) {                           // append tail
                memset(dest, insert_tail, rest);
                dest += rest;
            }

            if (extraByte) dest[0] = 0;             // append zero byte (used for strings)

            result = insDelBuffer;
        }
        *newlenPtr = newlen/mod;                    // report result length
    }
    return result;
}

enum insDelTarget {
    IDT_SPECIES = 0,
    IDT_SAI,
    IDT_SECSTRUCT,
};

static GB_CSTR targetType[] = {
    "Species",
    "SAI",
    "SeceditStruct",
};

static bool insdel_shall_be_applied_to(GBDATA *gb_data, enum insDelTarget target) {
    bool        apply = true;
    const char *key   = GB_read_key_pntr(gb_data);

    if (key[0] == '_') {                            // don't apply to keys starting with '_'
        switch (target) {
            case IDT_SECSTRUCT:
            case IDT_SPECIES:
                apply = false;
                break;

            case IDT_SAI:
                if (strcmp(key, "_REF") != 0) {     // despite key is _REF
                    apply = false;
                }
                break;
        }
    }

    return apply;
}

struct insDel_params {
    char       *ali_name;                           // name of alignment
    long        ali_len;                            // wanted length of alignment
    long        pos;                                // start position of insert/delete
    long        nchar;                              // number of elements to insert/delete
    const char *delete_chars;                       // characters allowed to delete (array with 256 entries, value == 0 means deletion allowed)
};



static GB_ERROR gbt_insert_character_gbd(GBDATA *gb_data, enum insDelTarget target, const insDel_params *params) {
    GB_ERROR error = 0;
    GB_TYPES type  = GB_read_type(gb_data);

    if (type == GB_DB) {
        GBDATA *gb_child;
        for (gb_child = GB_child(gb_data); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
            error = gbt_insert_character_gbd(gb_child, target, params);
        }
    }
    else {
        gb_assert(params->pos >= 0);
        if (type >= GB_BITS && type != GB_LINK) {
            long size = GB_read_count(gb_data);

            if (params->ali_len != size || params->nchar != 0) { // nothing would change
                if (insdel_shall_be_applied_to(gb_data, target)) {
                    GB_CSTR source      = 0;
                    long    mod         = sizeof(char);
                    char    insert_what = 0;
                    char    insert_tail = 0;
                    char    extraByte   = 0;
                    long    pos         = params->pos;
                    long    nchar       = params->nchar;

                    switch (type) {
                        case GB_STRING: {
                            source      = GB_read_char_pntr(gb_data);
                            extraByte   = 1;
                            insert_what = '-';
                            insert_tail = '.';

                            if (source) {
                                if (nchar > 0) {    // insert
                                    if (pos<size) { // otherwise insert pos is behind (old and too short) sequence -> dots are inserted at tail
                                        if ((pos>0 && source[pos-1] == '.') || source[pos] == '.') { // dot at insert position?
                                            insert_what = '.'; // insert dots
                                        }
                                    }
                                }
                                else {              // delete
                                    long    after        = pos+(-nchar); // position after deleted part
                                    long    p;
                                    GB_CSTR delete_chars = params->delete_chars;

                                    if (after>size) after = size;
                                    for (p = pos; p<after; p++) {
                                        if (delete_chars[((const unsigned char *)source)[p]]) {
                                            error = GBS_global_string("You tried to delete '%c' at position %li  -> Operation aborted", source[p], p);
                                        }
                                    }
                                }
                            }

                            break;
                        }
                        case GB_BITS:   source = GB_read_bits_pntr(gb_data, '-', '+');  insert_what = '-'; insert_tail = '-'; break;
                        case GB_BYTES:  source = GB_read_bytes_pntr(gb_data);           break;
                        case GB_INTS:   source = (GB_CSTR)GB_read_ints_pntr(gb_data);   mod = sizeof(GB_UINT4); break;
                        case GB_FLOATS: source = (GB_CSTR)GB_read_floats_pntr(gb_data); mod = sizeof(float); break;

                        default:
                            error = GBS_global_string("Unhandled type '%i'", type);
                            GB_internal_error(error);
                            break;
                    }

                    if (!error) {
                        if (!source) error = GB_await_error();
                        else {
                            long    modified_len;
                            GB_CSTR modified = gbt_insert_delete(source, size, params->ali_len, &modified_len, pos, nchar, mod, insert_what, insert_tail, extraByte);

                            if (modified) {
                                gb_assert(modified_len == (params->ali_len+params->nchar));

                                switch (type) {
                                    case GB_STRING: error = GB_write_string(gb_data, modified);                          break;
                                    case GB_BITS:   error = GB_write_bits  (gb_data, modified, modified_len, "-");       break;
                                    case GB_BYTES:  error = GB_write_bytes (gb_data, modified, modified_len);            break;
                                    case GB_INTS:   error = GB_write_ints  (gb_data, (GB_UINT4*)modified, modified_len); break;
                                    case GB_FLOATS: error = GB_write_floats(gb_data, (float*)modified, modified_len);    break;

                                    default: gb_assert(0); break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return error;
}

static GB_ERROR gbt_insert_character_item(GBDATA *gb_item, enum insDelTarget item_type, const insDel_params *params) {
    GB_ERROR  error  = 0;
    GBDATA   *gb_ali = GB_entry(gb_item, params->ali_name);

    if (gb_ali) {
        error = gbt_insert_character_gbd(gb_ali, item_type, params);
        if (error) {
            const char *item_name = GBT_read_name(gb_item);
            error = GBS_global_string("%s '%s': %s", targetType[item_type], item_name, error);
        }
    }

    return error;
}

static GB_ERROR gbt_insert_character(GBDATA *gb_item_data, const char *item_field, enum insDelTarget item_type, const insDel_params *params) {
    GBDATA       *gb_item;
    GB_ERROR      error      = 0;
    long          item_count = GB_number_of_subentries(gb_item_data);
    arb_progress  progress(item_field, item_count);

    for (gb_item = GB_entry(gb_item_data, item_field);
         gb_item && !error;
         gb_item = GB_nextEntry(gb_item))
    {
        error = gbt_insert_character_item(gb_item, item_type, params);
        progress.inc_and_check_user_abort(error);
    }
    return error;
}

static GB_ERROR gbt_insert_character_secstructs(GBDATA *gb_secstructs, const insDel_params *params) {
    GB_ERROR  error  = 0;
    GBDATA   *gb_ali = GB_entry(gb_secstructs, params->ali_name);
    
    if (gb_ali) {
        long item_count = GB_number_of_subentries(gb_ali)-1;

        if (item_count<1) item_count = 1;
        arb_progress progress("secstructs", item_count);

        GBDATA *gb_item;
        for (gb_item = GB_entry(gb_ali, "struct");
             gb_item && !error;
             gb_item = GB_nextEntry(gb_item))
        {
            GBDATA *gb_ref = GB_entry(gb_item, "ref");
            if (gb_ref) {
                error = gbt_insert_character_gbd(gb_ref, IDT_SECSTRUCT, params);
                if (error) {
                    const char *item_name = GBT_read_name(gb_item);
                    error = GBS_global_string("%s '%s': %s", targetType[IDT_SECSTRUCT], item_name, error);
                }
            }
            progress.inc_and_check_user_abort(error);
        }
    }
    return error;
}

static GB_ERROR GBT_check_lengths(GBDATA *Main, const char *alignment_name) {
    GB_ERROR  error            = 0;
    GBDATA   *gb_presets       = GBT_find_or_create(Main, "presets", 7);
    GBDATA   *gb_species_data  = GBT_find_or_create(Main, "species_data", 7);
    GBDATA   *gb_extended_data = GBT_find_or_create(Main, "extended_data", 7);
    GBDATA   *gb_secstructs    = GB_search(Main, "secedit/structs", GB_CREATE_CONTAINER);
    GBDATA   *gb_ali;

    insDel_params params = { 0, 0, 0, 0, 0 };

    for (gb_ali = GB_entry(gb_presets, "alignment");
         gb_ali && !error;
         gb_ali = GB_nextEntry(gb_ali))
    {
        GBDATA *gb_name = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_CHILD);

        if (gb_name) {
            arb_progress progress(3); // SAI, species and secstructs
            GBDATA *gb_len = GB_entry(gb_ali, "alignment_len");

            params.ali_name = GB_read_string(gb_name);
            params.ali_len  = GB_read_int(gb_len);

            error             = gbt_insert_character(gb_extended_data, "extended", IDT_SAI,     &params);
            if (!error) error = gbt_insert_character(gb_species_data,  "species",  IDT_SPECIES, &params);
            if (!error) error = gbt_insert_character_secstructs(gb_secstructs, &params);

            freenull(params.ali_name);
        }
    }
    free_insDelBuffer();
    return error;
}

GB_ERROR GBT_format_alignment(GBDATA *Main, const char *alignment_name) {
    GB_ERROR err = 0;

    if (strcmp(alignment_name, GENOM_ALIGNMENT) != 0) { // NEVER EVER format 'ali_genom'
        err           = GBT_check_data(Main, alignment_name); // detect max. length
        if (!err) err = GBT_check_lengths(Main, alignment_name); // format sequences in alignment
        if (!err) err = GBT_check_data(Main, alignment_name); // sets state to "formatted"
    }
    else {
        err = "It's forbidden to format '" GENOM_ALIGNMENT "'!";
    }
    return err;
}


GB_ERROR GBT_insert_character(GBDATA *Main, const char *alignment_name, long pos, long count, const char *char_delete)
{
    /* if count > 0     insert 'count' characters at pos
     * if count < 0     delete pos to pos+|count|
     *
     * Note: deleting is only performed, if found characters in deleted range are listed in 'char_delete'
     *       otherwise function returns with error
     *
     * This affects all species' and SAIs having data in given 'alignment_name' and
     * modifies several data entries found there
     * (see insdel_shall_be_applied_to for details which fields are affected).
     */

    GB_ERROR error = 0;

    if (pos<0) {
        error = GB_export_error("Illegal sequence position");
    }
    else {
        GBDATA *gb_ali;
        GBDATA *gb_presets       = GBT_find_or_create(Main, "presets", 7);
        GBDATA *gb_species_data  = GBT_find_or_create(Main, "species_data", 7);
        GBDATA *gb_extended_data = GBT_find_or_create(Main, "extended_data", 7);
        GBDATA *gb_secstructs    = GB_search(Main, "secedit/structs", GB_CREATE_CONTAINER);
        char    char_delete_list[256];

        if (strchr(char_delete, '%')) {
            memset(char_delete_list, 0, 256);
        }
        else {
            int ch;
            for (ch = 0; ch<256; ch++) {
                if (char_delete) {
                    if (strchr(char_delete, ch)) char_delete_list[ch] = 0;
                    else                        char_delete_list[ch] = 1;
                }
                else {
                    char_delete_list[ch] = 0;
                }
            }
        }

        for (gb_ali = GB_entry(gb_presets, "alignment");
             gb_ali && !error;
             gb_ali = GB_nextEntry(gb_ali))
        {
            GBDATA *gb_name = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_CHILD);

            if (gb_name) {
                GBDATA *gb_len = GB_entry(gb_ali, "alignment_len");
                long    len    = GB_read_int(gb_len);
                char   *use    = GB_read_string(gb_name);

                if (pos > len) {
                    error = GBS_global_string("Can't insert at position %li (exceeds length %li of alignment '%s')", pos, len, use);
                }
                else {
                    if (count < 0 && pos-count > len) count = pos - len;
                    error = GB_write_int(gb_len, len + count);
                }

                if (!error) {
                    insDel_params params = { use, len, pos, count, char_delete_list };

                    error             = gbt_insert_character(gb_species_data,   "species",  IDT_SPECIES,   &params);
                    if (!error) error = gbt_insert_character(gb_extended_data,  "extended", IDT_SAI,       &params);
                    if (!error) error = gbt_insert_character_secstructs(gb_secstructs, &params);
                }
                free(use);
            }
        }

        free_insDelBuffer();

        if (!error) GB_disable_quicksave(Main, "a lot of sequences changed");
    }
    return error;
}
