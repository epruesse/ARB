#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#include "adlocal.h"
#include "arbdbt.h"
#include "adGene.h"
#include "ad_config.h"

#if defined(DEVEL_RALF)
#warning split this file into semantic sections (alignment, insertDelete, tree, items_species_sai, rest)
#endif /* DEVEL_RALF */


#define GBT_GET_SIZE 0
#define GBT_PUT_DATA 1

#define DEFAULT_LENGTH        0.1 /* default length of tree-edge w/o given length */
#define DEFAULT_LENGTH_MARKER -1000.0 /* tree-edges w/o length are marked with this value during read and corrected in GBT_scale_tree */

/********************************************************************************************
                    some alignment header functions
********************************************************************************************/

char **GBT_get_alignment_names(GBDATA *gbd)
{
    /* get all alignment names out of a database
       (array of strings, the last stringptr is zero)
       Note: use GBT_free_names() to free strings+array
    */

    GBDATA *presets;
    GBDATA *ali;
    GBDATA *name;
    long size;

    char **erg;
    presets = GB_search(gbd,"presets",GB_CREATE_CONTAINER);
    size = 0;
    for (ali = GB_entry(presets,"alignment"); ali; ali = GB_nextEntry(ali)) {
        size ++;
    }
    erg = (char **)GB_calloc(sizeof(char *),(size_t)size+1);
    size = 0;
    for (ali = GB_entry(presets,"alignment"); ali; ali = GB_nextEntry(ali)) {
        name = GB_entry(ali,"alignment_name");
        if (!name) {
            erg[size] = strdup("alignment_name ???");
        }else{
            erg[size] = GB_read_string(name);
        }
        size ++;
    }
    return erg;
}

static char *gbt_nonexisting_alignment(GBDATA *gbMain) {
    char  *ali_other = 0;
    int    counter;

    for (counter = 1; !ali_other; ++counter) {
        ali_other = GBS_global_string_copy("ali_x%i", counter);
        if (GBT_get_alignment(gbMain, ali_other) != 0) freeset(ali_other, 0); // exists -> continue
    }

    return ali_other;
}

GB_ERROR GBT_check_alignment_name(const char *alignment_name)
{
    GB_ERROR error;
    if ( (error = GB_check_key(alignment_name)) ) return error;
    if (strncmp(alignment_name,"ali_",4)){
        return GB_export_error("your alignment_name '%s' must start with 'ali_'",
                               alignment_name);
    }
    return 0;
}

static GB_ERROR create_ali_strEntry(GBDATA *gb_ali, const char *field, const char *strval, long write_protection) {
    GB_ERROR  error  = 0;
    GBDATA   *gb_sub = GB_create(gb_ali, field, GB_STRING);

    if (!gb_sub) error = GB_await_error();
    else {
        error             = GB_write_string(gb_sub, strval);
        if (!error) error = GB_write_security_delete(gb_sub, 7);
        if (!error) error = GB_write_security_write(gb_sub, write_protection);
    }

    if (error) {
        error = GBS_global_string("failed to create alignment subentry '%s'\n"
                                  "(Reason: %s)", field, error);
    }

    return error;
}
static GB_ERROR create_ali_intEntry(GBDATA *gb_ali, const char *field, int intval, long write_protection) {
    GB_ERROR  error  = 0;
    GBDATA   *gb_sub = GB_create(gb_ali, field, GB_INT);

    if (!gb_sub) error = GB_await_error();
    else {
        error             = GB_write_int(gb_sub, intval);
        if (!error) error = GB_write_security_delete(gb_sub, 7);
        if (!error) error = GB_write_security_write(gb_sub, write_protection);
    }

    if (error) {
        error = GBS_global_string("failed to create alignment subentry '%s'\n"
                                  "(Reason: %s)", field, error);
    }

    return error;
}

GBDATA *GBT_create_alignment(GBDATA *gbd, const char *name, long len, long aligned, long security, const char *type) {
    GB_ERROR  error      = NULL;
    GBDATA   *gb_presets = GB_search(gbd, "presets", GB_CREATE_CONTAINER);
    GBDATA   *result     = NULL;

    if (!gb_presets) {
        error = GBS_global_string("can't find/create 'presets' (Reason: %s)", GB_await_error());
    }
    else {
        error = GBT_check_alignment_name(name);
        if (!error && (security<0 || security>6)) {
            error = GBS_global_string("Illegal security value %li (allowed 0..6)", security);
        }
        if (!error) {
            const char *allowed_types = ":dna:rna:ami:usr:";
            int         tlen          = strlen(type);
            const char *found         = strstr(allowed_types, type);
            if (!found || found == allowed_types || found[-1] != ':' || found[tlen] != ':') {
                error = GBS_global_string("Invalid alignment type '%s'", type);
            }
        }

        if (!error) {
            GBDATA *gb_name = GB_find_string(gb_presets, "alignment_name", name, GB_IGNORE_CASE, down_2_level);

            if (gb_name) error = GBS_global_string("Alignment '%s' already exists", name);
            else {
                GBDATA *gb_ali     = GB_create_container(gb_presets, "alignment");
                if (!gb_ali) error = GB_await_error();
                else {
                    error = GB_write_security_delete(gb_ali, 6);
                    if (!error) error = create_ali_strEntry(gb_ali, "alignment_name",           name, 6);
                    if (!error) error = create_ali_intEntry(gb_ali, "alignment_len",            len, 0);
                    if (!error) error = create_ali_intEntry(gb_ali, "aligned",                  aligned <= 0 ? 0 : 1, 0);
                    if (!error) error = create_ali_intEntry(gb_ali, "alignment_write_security", security, 6);
                    if (!error) error = create_ali_strEntry(gb_ali, "alignment_type",           type, 0);
                }

                if (!error) result = gb_ali;
            }
        }
    }

    if (!result) {
        ad_assert(error);
        GB_export_error("in GBT_create_alignment: %s", error);
    }
#if defined(DEBUG)
    else ad_assert(!error);
#endif /* DEBUG */

    return result;
}

NOT4PERL GB_ERROR GBT_check_alignment(GBDATA *gb_main, GBDATA *preset_alignment, GB_HASH *species_name_hash)
/* check if alignment is of the correct size
   and whether all data is present.
   Sets the security deletes and writes.

   If 'species_name_hash' is not NULL,
   it initially has to contain value == 1 for each existing species.
   Afterwards it contains value == 2 for each species where an alignment has been found.
*/
{
    GBDATA *gb_species_data  = GBT_find_or_create(gb_main,"species_data",7);
    GBDATA *gb_extended_data = GBT_find_or_create(gb_main,"extended_data",7);

    GB_ERROR  error    = 0;
    char     *ali_name = GBT_read_string(preset_alignment, "alignment_name");
    if (!ali_name) {
        error = GBS_global_string("Alignment w/o 'alignment_name' (%s)", GB_get_error());
    }

    if (!error) {
        long    security_write = -1;
        long    stored_ali_len = -1;
        long    found_ali_len  = -1;
        long    aligned        = 1;
        GBDATA *gb_ali_len     = 0;

        {
            GBDATA *gb_ali_wsec = GB_entry(preset_alignment,"alignment_write_security");
            if (!gb_ali_wsec) {
                error = "has no 'alignment_write_security' entry";
            }
            else {
                security_write = GB_read_int(gb_ali_wsec);
            }
        }


        if (!error) {
            gb_ali_len = GB_entry(preset_alignment,"alignment_len");
            if (!gb_ali_len) {
                error = "has no 'alignment_len' entry";
            }
            else {
                stored_ali_len = GB_read_int(gb_ali_len);
            }
        }

        if (!error) {
            GBDATA *gb_species;
            for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
                 gb_species && !error;
                 gb_species = GBT_next_species(gb_species))
            {
                GBDATA     *gb_name        = GB_entry(gb_species,"name");
                const char *name           = 0;
                int         alignment_seen = 0;
                GBDATA     *gb_ali         = 0;

                if (!gb_name) {
                    // fatal: name is missing -> create a unique name
                    char *unique = GBT_create_unique_species_name(gb_main, "autoname.");
                    error        = GBT_write_string(gb_species, "name", unique);

                    if (!error) {
                        gb_name = GB_entry(gb_species, "name");
                        GBS_write_hash(species_name_hash, unique, 1); // not seen before
                        GB_warning("Seen unnamed species (gave name '%s')", unique);
                    }
                    free(unique);
                }
                
                if (!error) {
                    name = GB_read_char_pntr(gb_name);
                    if (species_name_hash) {
                        int seen = GBS_read_hash(species_name_hash, name);

                        gb_assert(seen != 0); // species_name_hash not initialized correctly
                        if (seen == 2) alignment_seen = 1; // already seen an alignment
                    }
                }

                if (!error) error = GB_write_security_delete(gb_name,7);
                if (!error) error = GB_write_security_write(gb_name,6);

                if (!error) {
                    gb_ali = GB_entry(gb_species, ali_name);
                    if (gb_ali) {
                        GBDATA *gb_data = GB_entry(gb_ali, "data");
                        if (!gb_data) {
                            error = GBT_write_string(gb_ali, "data", "Error: entry 'data' was missing and therefore was filled with this text.");
                            GB_warning("No '%s/data' entry for species '%s' (has been filled with dummy data)", ali_name, name);
                        }
                        else {
                            if (GB_read_type(gb_data) != GB_STRING){
                                GB_delete(gb_data);
                                error = GBS_global_string("'%s/data' of species '%s' had wrong DB-type (%s) and has been deleted!",
                                                          ali_name, name, GB_read_key_pntr(gb_data));
                            }
                            else {
                                long data_len = GB_read_string_count(gb_data);
                                if (found_ali_len != data_len) {
                                    if (found_ali_len>0)        aligned       = 0;
                                    if (found_ali_len<data_len) found_ali_len = data_len;
                                }

                                error = GB_write_security_delete(gb_data,7);

                                if (!alignment_seen && species_name_hash) { // mark as seen
                                    GBS_write_hash(species_name_hash, name, 2); // 2 means "species has data in at least 1 alignment"
                                }
                            }
                        }
                    }
                }

                if (!error) error = GB_write_security_delete(gb_species,security_write);
            }
        }

        if (!error) {
            GBDATA *gb_sai;
            for (gb_sai = GBT_first_SAI_rel_SAI_data(gb_extended_data);
                 gb_sai && !error;
                 gb_sai = GBT_next_SAI(gb_sai) )
            {
                GBDATA *gb_sai_name = GB_entry(gb_sai, "name");
                GBDATA *gb_ali;

                if (!gb_sai_name) continue;

                GB_write_security_delete(gb_sai_name,7);

                gb_ali = GB_entry(gb_sai, ali_name);
                if (gb_ali) {
                    GBDATA *gb_sai_data;
                    for (gb_sai_data = GB_child(gb_ali) ;
                         gb_sai_data;
                         gb_sai_data = GB_nextChild(gb_sai_data))
                    {
                        long type = GB_read_type(gb_sai_data);
                        long data_len;

                        if (type == GB_DB || type < GB_BITS) continue;
                        if (GB_read_key_pntr(gb_sai_data)[0] == '_') continue; // e.g. _STRUCT (of secondary structure)

                        data_len = GB_read_count(gb_sai_data);

                        if (found_ali_len != data_len) {
                            if (found_ali_len>0)        aligned       = 0;
                            if (found_ali_len<data_len) found_ali_len = data_len;
                        }
                    }
                }
            }
        }

        if (!error && stored_ali_len != found_ali_len) error = GB_write_int(gb_ali_len, found_ali_len);
        if (!error) error = GBT_write_int(preset_alignment, "aligned", aligned);

        if (error) {
            error = GBS_global_string("alignment '%s': %s\n"
                                      "Database corrupted - try to fix if possible, save with different name and restart application.",
                                      ali_name, error);
        }
    }

    free(ali_name);

    return error;
}

static GB_ERROR gbt_rename_alignment_of_item(GBDATA *gb_item_container, const char *item_name, const char *item_entry_name,
                                             const char *source, const char *dest, int copy, int dele)
{
    GB_ERROR  error = 0;
    GBDATA   *gb_item;

    for (gb_item = GB_entry(gb_item_container, item_entry_name);
         gb_item && !error;
         gb_item = GB_nextEntry(gb_item))
    {
        GBDATA *gb_ali = GB_entry(gb_item, source);
        if (!gb_ali) continue;

        if (copy) {
            GBDATA *gb_new = GB_entry(gb_item, dest);
            if (gb_new) {
                error = GBS_global_string("Entry '%s' already exists", dest);
            }
            else {
                gb_new = GB_create_container(gb_item,dest);
                if (!gb_new) {
                    error = GB_get_error();
                }
                else {
                    error = GB_copy(gb_new,gb_ali);
                }
            }
        }
        if (dele) {
            error = GB_delete(gb_ali);
        }
    }

    if (error && gb_item) {
        error = GBS_global_string("%s\n(while renaming alignment for %s '%s')", error, item_name, GBT_read_name(gb_item));
    }

    return error;
}

GB_ERROR GBT_rename_alignment(GBDATA *gbMain, const char *source, const char *dest, int copy, int dele)
{
    /*  if copy     == 1 then create a copy
        if dele     == 1 then delete old */

    GB_ERROR  error            = 0;
    int       is_case_error    = 0;
    GBDATA   *gb_presets       = GB_entry(gbMain, "presets");
    GBDATA   *gb_species_data  = GB_entry(gbMain, "species_data");
    GBDATA   *gb_extended_data = GB_entry(gbMain, "extended_data");

    if (!gb_presets)            error = "presets not found";
    else if (!gb_species_data)  error = "species_data not found";
    else if (!gb_extended_data) error = "extended_data not found";


    /* create copy and/or delete old alignment description */
    if (!error) {
        GBDATA *gb_old_alignment = GBT_get_alignment(gbMain, source);

        if (!gb_old_alignment) {
            error = GBS_global_string("source alignment '%s' not found", source);
        }
        else {
            if (copy) {
                GBDATA *gbh = GBT_get_alignment(gbMain, dest);
                if (gbh) {
                    error         = GBS_global_string("destination alignment '%s' already exists", dest);
                    is_case_error = (strcasecmp(source, dest) == 0); // test for case-only difference
                }
                else {
                    error = GBT_check_alignment_name(dest);
                    if (!error) {
                        GBDATA *gb_new_alignment = GB_create_container(gb_presets,"alignment");
                        error                    = GB_copy(gb_new_alignment, gb_old_alignment);
                        if (!error) error        = GBT_write_string(gb_new_alignment, "alignment_name", dest);
                    }
                }
            }

            if (dele && !error) {
                error = GB_delete(gb_old_alignment);
            }
        }
    }

    /* change default alignment */
    if (!error && dele && copy) {
        error = GBT_write_string(gb_presets, "use", dest);
    }

    /* copy and/or delete alignment entries in species */
    if (!error) {
        error = gbt_rename_alignment_of_item(gb_species_data, "Species", "species", source, dest, copy, dele);
    }

    /* copy and/or delete alignment entries in SAIs */
    if (!error) {
        error = gbt_rename_alignment_of_item(gb_extended_data, "SAI", "extended", source, dest, copy, dele);
    }

    if (is_case_error) {
        /* alignments source and dest only differ in case */
        char *ali_other = gbt_nonexisting_alignment(gbMain);
        ad_assert(copy);

        printf("Renaming alignment '%s' -> '%s' -> '%s' (to avoid case-problem)\n", source, ali_other, dest);

        error             = GBT_rename_alignment(gbMain, source, ali_other, 1, dele);
        if (!error) error = GBT_rename_alignment(gbMain, ali_other, dest, 1, 1);

        free(ali_other);
    }

    return error;
}

GBDATA *GBT_find_or_create(GBDATA *Main,const char *key,long delete_level)
{
    GBDATA *gbd;
    gbd = GB_entry(Main,key);
    if (gbd) return gbd;
    gbd = GB_create_container(Main,key);
    GB_write_security_delete(gbd,delete_level);
    return gbd;
}

/********************************************************************************************
                    check the database !!!
********************************************************************************************/

static long check_for_species_without_data(const char *species_name, long value, void *counterPtr) {
    if (value == 1) {
        long cnt = *((long*)counterPtr);
        if (cnt<40) {
            GB_warning("Species '%s' has no data in any alignment", species_name);
        }
        *((long*)counterPtr) = cnt+1;
    }
    return value; // new hash value
}

#if defined(DEVEL_RALF)
#warning GBT_check_data ignores given 'alignment_name' if we have a default alignment. seems wrong!
#endif /* DEVEL_RALF */


GB_ERROR GBT_check_data(GBDATA *Main, const char *alignment_name)
/* if alignment_name == 0 -> check all existing alignments
 * otherwise check only one alignment
 */
{
    GB_ERROR  error             = 0;
    GBDATA   *gb_sd             = GBT_find_or_create(Main,"species_data",7);
    GBDATA   *gb_presets        = GBT_find_or_create(Main,"presets",7);
    GB_HASH  *species_name_hash = 0;

    GBT_find_or_create(Main,"extended_data",7);
    GBT_find_or_create(Main,"tree_data",7);

    if (alignment_name) {
        GBDATA *gb_ali_name = GB_find_string(gb_presets, "alignment_name", alignment_name, GB_IGNORE_CASE, down_2_level);
        if (!gb_ali_name) {
            error = GBS_global_string("Alignment '%s' does not exist - it can't be checked.", alignment_name);
        }
    }

    if (!error) {
        // check whether we have an default alignment
        GBDATA *gb_use = GB_entry(gb_presets, "use");
        if (!gb_use) {
            // if we have no default alignment -> look for any alignment
            GBDATA *gb_ali_name = GB_find_string(gb_presets,"alignment_name",alignment_name,GB_IGNORE_CASE,down_2_level);
            
            error = gb_ali_name ?
                GBT_write_string(gb_presets, "use", GB_read_char_pntr(gb_ali_name)) :
                "No alignment defined";
        }
    }

    if (!alignment_name && !error) {
        // if all alignments are checked -> use species_name_hash to detect duplicated species and species w/o data
        GBDATA *gb_species;
        long    duplicates = 0;
        species_name_hash  = GBS_create_hash(GBT_get_species_hash_size(Main), GB_IGNORE_CASE);

        if (!error) {
            for (gb_species = GBT_first_species_rel_species_data(gb_sd);
                 gb_species;
                 gb_species = GBT_next_species(gb_species))
            {
                const char *name = GBT_read_name(gb_species);

                if (GBS_read_hash(species_name_hash, name)) duplicates++;
                GBS_incr_hash(species_name_hash, name);
            }
        }

        if (duplicates) {
            error = GBS_global_string("Database is corrupted:\n"
                                      "Found %li duplicated species with identical names!\n"
                                      "Fix the problem using\n"
                                      "   'Search For Equal Fields and Mark Duplikates'\n"
                                      "in ARB_NTREE search tool, save DB and restart ARB."
                                      , duplicates);
        }
    }

    if (!error) {
        GBDATA *gb_ali;

        for (gb_ali = GB_entry(gb_presets,"alignment");
             gb_ali && !error;
             gb_ali = GB_nextEntry(gb_ali))
        {
            error = GBT_check_alignment(Main, gb_ali, species_name_hash);
        }
    }

    if (species_name_hash) {
        if (!error) {
            long counter = 0;
            GBS_hash_do_loop(species_name_hash, check_for_species_without_data, &counter);
            if (counter>0) {
                GB_warning("Found %li species without alignment data (only some were listed)", counter);
            }
        }

        GBS_free_hash(species_name_hash);
    }

    return error;
}

/* ----------------------- */
/*      insert/delete      */

static char *insDelBuffer = 0;
static size_t insDelBuffer_size;

static void free_insDelBuffer() {
    freeset(insDelBuffer, 0);
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

    if (!destlen) destlen                       = srclen; /* if no destlen is set then keep srclen */
    if ((nchar<0) && (pos-nchar>destlen)) nchar = pos-destlen; /* clip maximum characters to delete at end of array */

    if (destlen == srclen && (pos>srclen || nchar == 0)) { /* length stays same and clip-range is empty or behind end of sequence */
        /* before 26.2.09 the complete data was copied in this case - but nevertheless NULL(=failure) was returned.
         * I guess this was some test accessing complete data w/o writing anything back to DB,
         * but AFAIK it was not used anywhere --ralf
         */
        result = NULL;
    }
    else {
        long newlen = destlen+nchar;     /* length of result (w/o trailing zero-byte) */
        if (newlen == 0) {
            result = "";
        }
        else {
            size_t neededSpace = newlen+extraByte;
            
            if (insDelBuffer && insDelBuffer_size<neededSpace) freeset(insDelBuffer, 0);
            if (!insDelBuffer) {
                insDelBuffer_size = neededSpace;
                insDelBuffer      = (char*)malloc(neededSpace);
            }

            char *dest = insDelBuffer;
            gb_assert(dest);

            if (pos>srclen) {   /* insert/delete happens inside appended range */
                insert_what = insert_tail;
                pos         = srclen; /* insert/delete directly after source, to avoid illegal access below */
            }

            gb_assert(pos >= 0);
            if (pos>0) { /* copy part left of pos */
                memcpy(dest, source, (size_t)pos);
                dest   += pos;
                source += pos; srclen -= pos;
            }

            if (nchar>0) {                          /* insert */
                memset(dest, insert_what, (size_t)nchar);
                dest += nchar;
            }
            else if (nchar<0) {                   /* delete */
                source += -nchar; srclen -= -nchar;
            }

            if (srclen>0) { /* copy rest of source */
                memcpy(dest, source, (size_t)srclen);
                dest   += srclen;
                source += srclen; srclen = 0;
            }

            long rest = newlen-(dest-insDelBuffer);
            gb_assert(rest >= 0);

            if (rest>0) { /* append tail */
                memset(dest, insert_tail, rest);
                dest += rest;
            }

            if (extraByte) dest[0] = 0; /* append zero byte (used for strings) */

            result = insDelBuffer;
        }
        *newlenPtr = newlen/mod; /* report result length */
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

static GB_BOOL insdel_shall_be_applied_to(GBDATA *gb_data, enum insDelTarget target) {
    GB_BOOL     apply = GB_TRUE;
    const char *key   = GB_read_key_pntr(gb_data);

    if (key[0] == '_') {        // dont apply to keys starting with '_'
        switch (target) {
            case IDT_SECSTRUCT:
            case IDT_SPECIES:
                apply = GB_FALSE;
                break;
                
            case IDT_SAI:
                if (strcmp(key, "_REF") != 0) { // despite key is _REF
                    apply = GB_FALSE;
                }
                break;
        }
    }

    return apply;
}

struct insDel_params {
    char       *ali_name;       // name of alignment
    long        ali_len;        // wanted length of alignment
    long        pos;            // start position of insert/delete
    long        nchar;          // number of elements to insert/delete
    const char *delete_chars;   // characters allowed to delete (array with 256 entries, value == 0 means deletion allowed)
};



static GB_ERROR gbt_insert_character_gbd(GBDATA *gb_data, enum insDelTarget target, const struct insDel_params *params) {
    GB_ERROR error = 0;
    GB_TYPES type  = GB_read_type(gb_data);

    if (type == GB_DB) {
        GBDATA *gb_child;
        for (gb_child = GB_child(gb_data); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
            error = gbt_insert_character_gbd(gb_child, target, params);
        }
    }
    else {
        ad_assert(params->pos >= 0);
        if (type >= GB_BITS && type != GB_LINK) {
            long size = GB_read_count(gb_data);

            if (params->ali_len != size || params->nchar != 0) { /* nothing would change */
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
                                if (nchar > 0) { /* insert */
                                    if (pos<size) { /* otherwise insert pos is behind (old and too short) sequence -> dots are inserted at tail */
                                        if ((pos>0 && source[pos-1] == '.') || source[pos] == '.') { /* dot at insert position? */
                                            insert_what = '.'; /* insert dots */
                                        }
                                    }
                                }
                                else { /* delete */
                                    long    after        = pos+(-nchar); /* position after deleted part */
                                    long    p;
                                    GB_CSTR delete_chars = params->delete_chars;

                                    if (after>size) after = size;
                                    for (p = pos; p<after; p++){
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

                        default :
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

static GB_ERROR gbt_insert_character_item(GBDATA *gb_item, enum insDelTarget item_type, const struct insDel_params *params) {
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

static GB_ERROR gbt_insert_character(GBDATA *gb_item_data, const char *item_field, enum insDelTarget item_type, const struct insDel_params *params) {
    GBDATA   *gb_item;
    GB_ERROR  error      = 0;
    long      item_count = GB_number_of_subentries(gb_item_data);
    long      count      = 0;

    for (gb_item = GB_entry(gb_item_data, item_field);
         gb_item && !error;
         gb_item = GB_nextEntry(gb_item))
    {
        error = gbt_insert_character_item(gb_item, item_type, params);
        count++;
        GB_status((double)count/item_count);
    }
    return error;
}

static GB_ERROR gbt_insert_character_secstructs(GBDATA *gb_secstructs, const struct insDel_params *params) {
    GB_ERROR  error  = 0;
    GBDATA   *gb_ali = GB_entry(gb_secstructs, params->ali_name);
    if (gb_ali) {
        long    item_count = GB_number_of_subentries(gb_ali)-1;
        long    count      = 0;
        GBDATA *gb_item;

        if (item_count<1) item_count = 1;

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
            count++;
            GB_status((double)count/item_count);
        }
    }
    return error;
}

static GB_ERROR GBT_check_lengths(GBDATA *Main, const char *alignment_name) {
    GB_ERROR  error            = 0;
    GBDATA   *gb_presets       = GBT_find_or_create(Main,"presets",7);
    GBDATA   *gb_species_data  = GBT_find_or_create(Main,"species_data",7);
    GBDATA   *gb_extended_data = GBT_find_or_create(Main,"extended_data",7);
    GBDATA   *gb_secstructs    = GB_search(Main,"secedit/structs", GB_CREATE_CONTAINER);
    GBDATA   *gb_ali;

    struct insDel_params params = { 0, 0, 0, 0, 0 };

    for (gb_ali = GB_entry(gb_presets,"alignment");
         gb_ali && !error;
         gb_ali = GB_nextEntry(gb_ali))
    {
        GBDATA *gb_name = GB_find_string(gb_ali,"alignment_name",alignment_name,GB_IGNORE_CASE,down_level);

        if (gb_name) {
            GBDATA *gb_len = GB_entry(gb_ali,"alignment_len");

            params.ali_name = GB_read_string(gb_name);
            params.ali_len  = GB_read_int(gb_len);

            error             = gbt_insert_character(gb_extended_data, "extended", IDT_SAI,     &params);
            if (!error) error = gbt_insert_character(gb_species_data,  "species",  IDT_SPECIES, &params);
            if (!error) error = gbt_insert_character_secstructs(gb_secstructs, &params);

            freeset(params.ali_name, 0);
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


GB_ERROR GBT_insert_character(GBDATA *Main, char *alignment_name, long pos, long count, char *char_delete)
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
        GBDATA *gb_presets       = GBT_find_or_create(Main,"presets",7);
        GBDATA *gb_species_data  = GBT_find_or_create(Main,"species_data",7);
        GBDATA *gb_extended_data = GBT_find_or_create(Main,"extended_data",7);
        GBDATA *gb_secstructs    = GB_search(Main,"secedit/structs", GB_CREATE_CONTAINER);
        char    char_delete_list[256];

        if (strchr(char_delete,'%') ) {
            memset(char_delete_list,0,256);
        }
        else {
            int ch;
            for (ch = 0;ch<256; ch++) {
                if (char_delete) {
                    if (strchr(char_delete,ch)) char_delete_list[ch] = 0;
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
            GBDATA *gb_name = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, down_level);

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
                    struct insDel_params params = { use, len, pos, count, char_delete_list };

                    error             = gbt_insert_character(gb_species_data,   "species",  IDT_SPECIES,   &params);
                    if (!error) error = gbt_insert_character(gb_extended_data,  "extended", IDT_SAI,       &params);
                    if (!error) error = gbt_insert_character_secstructs(gb_secstructs, &params);
                }
                free(use);
            }
        }

        free_insDelBuffer();

        if (!error) GB_disable_quicksave(Main,"a lot of sequences changed");
    }
    return error;
}


/* ---------------------- */
/*      remove leafs      */


static GBT_TREE *fixDeletedSon(GBT_TREE *tree) {
    // fix tree after one son has been deleted
    // (Note: this function only works correct for trees with minimum element size!)
    GBT_TREE *delNode = tree;

    if (delNode->leftson) {
        ad_assert(!delNode->rightson);
        tree             = delNode->leftson;
        delNode->leftson = 0;
    }
    else {
        ad_assert(!delNode->leftson);
        ad_assert(delNode->rightson);
        
        tree              = delNode->rightson;
        delNode->rightson = 0;
    }

    // now tree is the new tree
    tree->father = delNode->father;

    if (delNode->remark_branch && !tree->remark_branch) { // rescue remarks if possible
        tree->remark_branch    = delNode->remark_branch;
        delNode->remark_branch = 0;
    }
    if (delNode->gb_node && !tree->gb_node) { // rescue group if possible
        tree->gb_node    = delNode->gb_node;
        delNode->gb_node = 0;
    }

    delNode->is_leaf = GB_TRUE; // don't try recursive delete

    if (delNode->father) { // not root
        GBT_delete_tree(delNode);
    }
    else { // root node
        if (delNode->tree_is_one_piece_of_memory) {
            // dont change root -> copy instead
            memcpy(delNode, tree, sizeof(GBT_TREE));
            tree = delNode;
        }
        else {
            GBT_delete_tree(delNode);
        }
    }
    return tree;
}


GBT_TREE *GBT_remove_leafs(GBT_TREE *tree, GBT_TREE_REMOVE_TYPE mode, GB_HASH *species_hash, int *removed, int *groups_removed) {
    // given 'tree' can either
    // - be linked (in this case 'species_hash' shall be NULL)
    // - be unlinked (in this case 'species_hash' has to be provided)

    if (tree->is_leaf) {
        if (tree->name) {
            GB_BOOL  deleteSelf = GB_FALSE;
            GBDATA  *gb_node;

            if (species_hash) {
                gb_node = (GBDATA*)GBS_read_hash(species_hash, tree->name);
                ad_assert(tree->gb_node == 0); // dont call linked tree with 'species_hash'!
            }
            else gb_node = tree->gb_node;

            if (gb_node) {
                if (mode & (GBT_REMOVE_MARKED|GBT_REMOVE_NOT_MARKED)) {
                    long flag = GB_read_flag(gb_node);
                    deleteSelf = (flag && (mode&GBT_REMOVE_MARKED)) || (!flag && (mode&GBT_REMOVE_NOT_MARKED));
                }
            }
            else { // zombie
                if (mode & GBT_REMOVE_DELETED) deleteSelf = GB_TRUE;
            }

            if (deleteSelf) {
                GBT_delete_tree(tree);
                (*removed)++;
                tree = 0;
            }
        }
    }
    else {
        tree->leftson  = GBT_remove_leafs(tree->leftson, mode, species_hash, removed, groups_removed);
        tree->rightson = GBT_remove_leafs(tree->rightson, mode, species_hash, removed, groups_removed);

        if (tree->leftson) {
            if (!tree->rightson) { // right son deleted
                tree = fixDeletedSon(tree);
            }
            // otherwise no son deleted
        }
        else if (tree->rightson) { // left son deleted
            tree = fixDeletedSon(tree);
        }
        else {                  // everything deleted -> delete self
            if (tree->name) (*groups_removed)++;
            tree->is_leaf = GB_TRUE;
            GBT_delete_tree(tree);
            tree          = 0;
        }
    }

    return tree;
}

/* ------------------- */
/*      free tree      */


void GBT_delete_tree(GBT_TREE *tree)
     /* frees a tree only in memory (not in the database)
        to delete the tree in Database
        just call GB_delete((GBDATA *)gb_tree);
     */
{
    free(tree->name);
    free(tree->remark_branch);

    if (!tree->is_leaf) {
        GBT_delete_tree(tree->leftson);
        GBT_delete_tree(tree->rightson);
    }
    if (!tree->tree_is_one_piece_of_memory || !tree->father) {
        free(tree);
    }
}


/********************************************************************************************
                    some tree write functions
********************************************************************************************/


GB_ERROR GBT_write_group_name(GBDATA *gb_group_name, const char *new_group_name) {
    GB_ERROR error = 0;
    size_t   len   = strlen(new_group_name);

    if (len >= GB_GROUP_NAME_MAX) {
        error = GBS_global_string("Group name '%s' too long (max %i characters)", new_group_name, GB_GROUP_NAME_MAX);
    }
    else {
        error = GB_write_string(gb_group_name, new_group_name);
    }
    return error;
}

long gbt_write_tree_nodes(GBDATA *gb_tree,GBT_TREE *node,long startid)
{
    long me;
    GB_ERROR error;
    const char *key;
    GBDATA *gb_id,*gb_name,*gb_any;
    if (node->is_leaf) return 0;
    me = startid;
    if (node->name && (strlen(node->name)) ) {
        if (!node->gb_node) {
            node->gb_node = GB_create_container(gb_tree,"node");
        }
        gb_name = GB_search(node->gb_node,"group_name",GB_STRING);
        error = GBT_write_group_name(gb_name, node->name);
        if (error) return -1;
    }
    if (node->gb_node){         /* delete not used nodes else write id */
        gb_any = GB_child(node->gb_node);
        if (gb_any) {
            key = GB_read_key_pntr(gb_any);
            if (!strcmp(key,"id")){
                gb_any = GB_nextChild(gb_any);
            }
        }

        if (gb_any){
            gb_id = GB_search(node->gb_node,"id",GB_INT);
#if defined(DEBUG) && defined(DEVEL_RALF)
            {
                int old = GB_read_int(gb_id);
                if (old != me) {
                    printf("id changed in gbt_write_tree_nodes(): old=%i new=%li (tree-node=%p; gb_node=%p)\n",
                           old, me, node, node->gb_node);
                }
            }
#endif /* DEBUG */
            error = GB_write_int(gb_id,me);
            GB_write_usr_private(node->gb_node,0);
            if (error) return -1;
        }else{
#if defined(DEBUG) && defined(DEVEL_RALF)
            {
                GBDATA *gb_id2 = GB_entry(node->gb_node, "id");
                int     id     = 0;
                if (gb_id2) id = GB_read_int(gb_id2);

                printf("deleting node w/o info: tree-node=%p; gb_node=%p prev.id=%i\n", node, node->gb_node, id);
            }
#endif /* DEBUG */
            GB_delete(node->gb_node);
            node->gb_node = 0;
        }
    }
    startid++;
    if (!node->leftson->is_leaf) {
        startid = gbt_write_tree_nodes(gb_tree,node->leftson,startid);
        if (startid<0) return startid;
    }

    if (!node->rightson->is_leaf) {
        startid = gbt_write_tree_nodes(gb_tree,node->rightson,startid);
        if (startid<0) return startid;
    }
    return startid;
}

static char *gbt_write_tree_rek_new(GBT_TREE *node, char *dest, long mode) {
    char buffer[40];        /* just real numbers */
    char    *c1;

    if ( (c1 = node->remark_branch) ) {
        int c;
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'R';
            while ( (c= *(c1++))  ) {
                if (c == 1) continue;
                *(dest++) = c;
            }
            *(dest++) = 1;
        }else{
            dest += strlen(c1) + 2;
        }
    }

    if (node->is_leaf){
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'L';
            if (node->name) strcpy(dest,node->name);
            while ( (c1= (char *)strchr(dest,1)) ) *c1 = 2;
            dest += strlen(dest);
            *(dest++) = 1;
            return dest;
        }else{
            if (node->name) return dest+1+strlen(node->name)+1; /* N name term */
            return dest+1+1;
        }
    }else{
        sprintf(buffer,"%g,%g;",node->leftlen,node->rightlen);
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'N';
            strcpy(dest,buffer);
            dest += strlen(buffer);
        }else{
            dest += strlen(buffer)+1;
        }
        dest = gbt_write_tree_rek_new(node->leftson,  dest, mode);
        dest = gbt_write_tree_rek_new(node->rightson, dest, mode);
        return dest;
    }
}

static GB_ERROR gbt_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree, int plain_only) {
    /* writes a tree to the database.

    If tree is loaded by function GBT_read_tree(..) then 'tree_name' should be zero !!!!!!
    else 'gb_tree' should be set to zero.

    to copy a tree call GB_copy((GBDATA *)dest,(GBDATA *)source);
    or set recursively all tree->gb_node variables to zero (that unlinks the tree),

    if 'plain_only' == 1 only the plain tree string is written

    */
    GB_ERROR error = 0;

    gb_assert(!plain_only || (tree_name == 0)); // if plain_only == 1 -> set tree_name to 0

    if (tree) {
        if (tree_name) {
            if (gb_tree) error = GBS_global_string("can't change name of existing tree (to '%s')", tree_name);
            else {
                error = GBT_check_tree_name(tree_name);
                if (!error) {
                    GBDATA *gb_tree_data = GB_search(gb_main, "tree_data", GB_CREATE_CONTAINER);
                    gb_tree              = GB_search(gb_tree_data, tree_name, GB_CREATE_CONTAINER);

                    if (!gb_tree) error = GB_get_error();
                }
            }
        }
        else {
            if (!gb_tree) error = "No tree name given";
        }

        ad_assert(gb_tree || error);

        if (!error) {
            if (!plain_only) {
                // mark all old style tree data for deletion
                GBDATA *gb_node;
                for (gb_node = GB_entry(gb_tree,"node"); gb_node; gb_node = GB_nextEntry(gb_node)) {
                    GB_write_usr_private(gb_node,1);
                }
            }

            // build tree-string and save to DB
            {
                char *t_size = gbt_write_tree_rek_new(tree, 0, GBT_GET_SIZE); // calc size of tree-string
                char *ctree  = (char *)GB_calloc(sizeof(char),(size_t)(t_size+1)); // allocate buffer for tree-string

                t_size = gbt_write_tree_rek_new(tree, ctree, GBT_PUT_DATA); // write into buffer
                *(t_size) = 0;

                error = GB_set_compression(gb_main,0); // no more compressions 
                if (!error) {
                    error        = GBT_write_string(gb_tree, "tree", ctree);
                    GB_ERROR err = GB_set_compression(gb_main,-1); // again allow all types of compression 
                    if (!error) error = err;
                }
                free(ctree);
            }
        }

        if (!plain_only && !error) {
            // save nodes to DB
            long size = gbt_write_tree_nodes(gb_tree,tree,0);
            
            if (size<0) error = GB_get_error();
            else {
                GBDATA *gb_node, *gb_node_next;

                error = GBT_write_int(gb_tree, "nnodes", size);
                
                for (gb_node = GB_entry(gb_tree,"node"); // delete all ghost nodes
                     gb_node && !error;
                     gb_node = gb_node_next)
                {
                    GBDATA *gbd = GB_entry(gb_node,"id");
                    gb_node_next = GB_nextEntry(gb_node);
                    if (!gbd || GB_read_usr_private(gb_node)) error = GB_delete(gb_node);
                }
            }
        }
    }

    return error;
}

GB_ERROR GBT_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree) {
    return gbt_write_tree(gb_main, gb_tree, tree_name, tree, 0);
}
GB_ERROR GBT_write_plain_tree(GBDATA *gb_main, GBDATA *gb_tree, char *tree_name, GBT_TREE *tree) {
    return gbt_write_tree(gb_main, gb_tree, tree_name, tree, 1);
}

GB_ERROR GBT_write_tree_rem(GBDATA *gb_main,const char *tree_name, const char *remark) {
    return GBT_write_string(GBT_get_tree(gb_main, tree_name), "remark", remark);
}

/********************************************************************************************
                    some tree read functions
********************************************************************************************/

GBT_TREE *gbt_read_tree_rek(char **data, long *startid, GBDATA **gb_tree_nodes, long structure_size, int size_of_tree, GB_ERROR *error)
{
    GBT_TREE    *node;
    GBDATA      *gb_group_name;
    char         c;
    char        *p1;
    static char *membase;

    gb_assert(error);
    if (*error) return NULL;

    if (structure_size>0){
        node = (GBT_TREE *)GB_calloc(1,(size_t)structure_size);
    }
    else {
        if (!startid[0]){
            membase =(char *)GB_calloc(size_of_tree+1,(size_t)(-2*structure_size)); /* because of inner nodes */
        }
        node = (GBT_TREE *)membase;
        node->tree_is_one_piece_of_memory = 1;
        membase -= structure_size;
    }

    c = *((*data)++);

    if (c=='R') {
        p1 = strchr(*data,1);
        *(p1++) = 0;
        node->remark_branch = strdup(*data);
        c = *(p1++);
        *data = p1;
    }


    if (c=='N') {
        p1 = (char *)strchr(*data,',');
        *(p1++) = 0;
        node->leftlen = GB_atof(*data);
        *data = p1;
        p1 = (char *)strchr(*data,';');
        *(p1++) = 0;
        node->rightlen = GB_atof(*data);
        *data = p1;
        if ((*startid < size_of_tree) && (node->gb_node = gb_tree_nodes[*startid])){
            gb_group_name = GB_entry(node->gb_node,"group_name");
            if (gb_group_name) {
                node->name = GB_read_string(gb_group_name);
            }
        }
        (*startid)++;
        node->leftson = gbt_read_tree_rek(data,startid,gb_tree_nodes,structure_size,size_of_tree, error);
        if (!node->leftson) {
            if (!node->tree_is_one_piece_of_memory) free((char *)node);
            return NULL;
        }
        node->rightson = gbt_read_tree_rek(data,startid,gb_tree_nodes,structure_size,size_of_tree, error);
        if (!node->rightson) {
            if (!node->tree_is_one_piece_of_memory) free((char *)node);
            return NULL;
        }
        node->leftson->father = node;
        node->rightson->father = node;
    }
    else if (c=='L') {
        node->is_leaf = GB_TRUE;
        p1            = (char *)strchr(*data,1);

        gb_assert(p1);
        gb_assert(p1[0] == 1);

        *p1        = 0;
        node->name = strdup(*data);
        *data      = p1+1;
    }
    else {
        if (!c) {
            *error = "Unexpected end of tree definition.";
        }
        else {
            *error = GBS_global_string("Can't interpret tree definition (expected 'N' or 'L' - not '%c')", c);
        }
        /* GB_internal_error("Error reading tree 362436"); */
        return NULL;
    }
    return node;
}


/** Loads a tree from the database into any user defined structure.
    make sure that the first eight members members of your
    structure looks exectly like GBT_TREE, You should send the size
    of your structure ( minimum sizeof GBT_TREE) to this
    function.

    If size < 0 then the tree is allocated as just one big piece of memery,
    which can be freed by free((char *)root_of_tree) + deleting names or
    by GBT_delete_tree.

    tree_name is the name of the tree in the db
    return NULL if any error occur
*/

static GBT_TREE *read_tree_and_size_internal(GBDATA *gb_tree, GBDATA *gb_ctree, int structure_size, int size, GB_ERROR *error) {
    GBDATA   **gb_tree_nodes;
    GBT_TREE  *node = 0;

    gb_tree_nodes = (GBDATA **)GB_calloc(sizeof(GBDATA *),(size_t)size);
    if (gb_tree) {
        GBDATA *gb_node;

        for (gb_node = GB_entry(gb_tree,"node"); gb_node && !*error; gb_node = GB_nextEntry(gb_node)) {
            long    i;
            GBDATA *gbd = GB_entry(gb_node,"id");
            if (!gbd) continue;

            /*{ GB_export_error("ERROR while reading tree '%s' 4634",tree_name);return NULL;}*/
            i = GB_read_int(gbd);
            if ( i<0 || i>= size ) {
                *error = "An inner node of the tree is corrupt";
            }
            else {
                gb_tree_nodes[i] = gb_node;
            }
        }
    }
    if (!*error) {
        char *cptr[1];
        long  startid[1];
        char *fbuf;

        startid[0] = 0;
        fbuf       = cptr[0] = GB_read_string(gb_ctree);
        node       = gbt_read_tree_rek(cptr, startid, gb_tree_nodes, structure_size,(int)size, error);
        free (fbuf);
    }

    free((char *)gb_tree_nodes);

    return node;
}

GBT_TREE *GBT_read_tree_and_size(GBDATA *gb_main,const char *tree_name, long structure_size, int *tree_size) {
    /* read a tree from DB */
    GB_ERROR error = 0;

    if (!tree_name) {
        error = "no treename given";
    }
    else {
        error = GBT_check_tree_name(tree_name);
        if (!error) {
            GBDATA *gb_tree_data = GB_search(gb_main, "tree_data", GB_CREATE_CONTAINER);
            GBDATA *gb_tree      = GB_entry(gb_tree_data, tree_name);

            if (!gb_tree) {
                error = GBS_global_string("Could not find tree '%s'", tree_name);
            }
            else {
                GBDATA *gb_nnodes = GB_entry(gb_tree, "nnodes");
                if (!gb_nnodes) {
                    error = GBS_global_string("Tree '%s' is empty", tree_name);
                }
                else {
                    long size = GB_read_int(gb_nnodes);
                    if (!size) {
                        error = GBS_global_string("Tree '%s' has zero size", tree_name);
                    }
                    else {
                        GBDATA *gb_ctree = GB_search(gb_tree, "tree", GB_FIND);
                        if (!gb_ctree) {
                            error = "Sorry - old tree format no longer supported";
                        }
                        else { /* "new" style tree */
                            GBT_TREE *tree = read_tree_and_size_internal(gb_tree, gb_ctree, structure_size, size, &error);
                            if (!error) {
                                gb_assert(tree);
                                if (tree_size) *tree_size = size; /* return size of tree */
                                return tree;
                            }

                            gb_assert(!tree);
                        }
                    }
                }
            }
        }
    }

    gb_assert(error);
    GB_export_error("Couldn't read tree '%s' (Reason: %s)", tree_name, error);
    return NULL;
}

GBT_TREE *GBT_read_tree(GBDATA *gb_main,const char *tree_name, long structure_size) {
    return GBT_read_tree_and_size(gb_main, tree_name, structure_size, 0);
}

GBT_TREE *GBT_read_plain_tree(GBDATA *gb_main, GBDATA *gb_ctree, long structure_size, GB_ERROR *error) {
    GBT_TREE *t;

    gb_assert(error && !*error); /* expect cleared error*/

    GB_push_transaction(gb_main);
    t = read_tree_and_size_internal(0, gb_ctree, structure_size, 0, error);
    GB_pop_transaction(gb_main);

    return t;
}

/********************************************************************************************
                    link the tree tips to the database
********************************************************************************************/
long GBT_count_nodes(GBT_TREE *tree){
    if ( tree->is_leaf ) {
        return 1;
    }
    return GBT_count_nodes(tree->leftson) + GBT_count_nodes(tree->rightson);
}

struct link_tree_data {
    GB_HASH *species_hash;
    GB_HASH *seen_species;      /* used to count duplicates */
    int      nodes; /* if != 0 -> update status */;
    int      counter;
    int      zombies;           /* counts zombies */
    int      duplicates;        /* counts duplicates */
};

static GB_ERROR gbt_link_tree_to_hash_rek(GBT_TREE *tree, struct link_tree_data *ltd) {
    GB_ERROR error = 0;
    if (tree->is_leaf) {
        if (ltd->nodes) { /* update status? */
            GB_status(ltd->counter/(double)ltd->nodes);
            ltd->counter++;
        }

        tree->gb_node = 0;
        if (tree->name) {
            GBDATA *gbd = (GBDATA*)GBS_read_hash(ltd->species_hash, tree->name);
            if (gbd) tree->gb_node = gbd;
            else ltd->zombies++;

            if (ltd->seen_species) {
                if (GBS_read_hash(ltd->seen_species, tree->name)) ltd->duplicates++;
                else GBS_write_hash(ltd->seen_species, tree->name, 1);
            }
        }
    }
    else {
        error = gbt_link_tree_to_hash_rek(tree->leftson, ltd);
        if (!error) error = gbt_link_tree_to_hash_rek(tree->rightson, ltd);
    }
    return error;
}

GB_ERROR GBT_link_tree_using_species_hash(GBT_TREE *tree, GB_BOOL show_status, GB_HASH *species_hash, int *zombies, int *duplicates) {
    GB_ERROR              error;
    struct link_tree_data ltd;
    long                  leafs = 0;

    if (duplicates || show_status) {
        leafs = GBT_count_nodes(tree);
    }
    
    ltd.species_hash = species_hash;
    ltd.seen_species = leafs ? GBS_create_hash(2*leafs, GB_IGNORE_CASE) : 0;
    ltd.zombies      = 0;
    ltd.duplicates   = 0;
    ltd.counter      = 0;

    if (show_status) {
        GB_status2("Relinking tree to database");
        ltd.nodes = leafs;
    }
    else {
        ltd.nodes = 0;
    }

    error = gbt_link_tree_to_hash_rek(tree, &ltd);
    if (ltd.seen_species) GBS_free_hash(ltd.seen_species);

    if (zombies) *zombies = ltd.zombies;
    if (duplicates) *duplicates = ltd.duplicates;

    return error;
}

/** Link a given tree to the database. That means that for all tips the member
    gb_node is set to the database container holding the species data.
    returns the number of zombies and duplicates in 'zombies' and 'duplicates'
*/
GB_ERROR GBT_link_tree(GBT_TREE *tree,GBDATA *gb_main,GB_BOOL show_status, int *zombies, int *duplicates)
{
    GB_HASH  *species_hash = GBT_create_species_hash(gb_main);
    GB_ERROR  error        = GBT_link_tree_using_species_hash(tree, show_status, species_hash, zombies, duplicates);

    GBS_free_hash(species_hash);

    return error;
}

/** Unlink a given tree from the database.
 */
void GBT_unlink_tree(GBT_TREE *tree)
{
    tree->gb_node = 0;
    if (!tree->is_leaf) {
        GBT_unlink_tree(tree->leftson);
        GBT_unlink_tree(tree->rightson);
    }
}


/********************************************************************************************
                    load a tree from file system
********************************************************************************************/


/* -------------------- */
/*      TreeReader      */
/* -------------------- */

typedef struct {
    char                 *tree_file_name;
    FILE                 *in;
    int                   last_character;      // may be EOF
    int                   line_cnt;
    struct GBS_strstruct *tree_comment;
    double                max_found_branchlen;
    double                max_found_bootstrap;
    GB_ERROR              error;
} TreeReader;

static char gbt_read_char(TreeReader *reader) {
    GB_BOOL done = GB_FALSE;
    int     c    = ' ';

    while (!done) {
        c = getc(reader->in);
        if (c == '\n') reader->line_cnt++;
        else if (c == ' ' || c == '\t') ; // skip
        else if (c == '[') {    // collect tree comment(s)
            if (reader->tree_comment != 0) {
                // not first comment -> do new line
                GBS_chrcat(reader->tree_comment, '\n');
            }
            c = getc(reader->in);
            while (c != ']' && c != EOF) {
                GBS_chrcat(reader->tree_comment, c);
                c = getc(reader->in);
            }
        }
        else done = GB_TRUE;
    }

    reader->last_character = c;
    return c;
}

static char gbt_get_char(TreeReader *reader) {
    int c = getc(reader->in);

    if (c == '\n') reader->line_cnt++;
    reader->last_character = c;
    return c;
}

static TreeReader *newTreeReader(FILE *input, const char *file_name) {
    TreeReader *reader = GB_calloc(1, sizeof(*reader));

    reader->tree_file_name      = strdup(file_name);
    reader->in                  = input;
    reader->tree_comment        = GBS_stropen(2048);
    reader->max_found_branchlen = -1;
    reader->max_found_bootstrap = -1;
    reader->line_cnt            = 1;
    
    gbt_read_char(reader);

    return reader;
}

static void freeTreeReader(TreeReader *reader) {
    free(reader->tree_file_name);
    if (reader->tree_comment) GBS_strforget(reader->tree_comment);
    free(reader);
}

static void setReaderError(TreeReader *reader, const char *message) {
    
    reader->error = GBS_global_string("Error reading %s:%i: %s",
                                      reader->tree_file_name,
                                      reader->line_cnt,
                                      message);
}

static char *getTreeComment(TreeReader *reader) {
    /* can only be called once. Deletes the comment from TreeReader! */
    char *comment = 0;
    if (reader->tree_comment) {
        comment = GBS_strclose(reader->tree_comment);
        reader->tree_comment = 0;
    }
    return comment;
}


/* ------------------------------------------------------------
 * The following functions assume that the "current" character
 * has already been read into 'TreeReader->last_character'
 */

static void gbt_eat_white(TreeReader *reader) {
    int c = reader->last_character;
    while ((c == ' ') || (c == '\n') || (c == '\t')){
        c = gbt_get_char(reader);
    }
}

static double gbt_read_number(TreeReader *reader) {
    char    strng[256];
    char   *s = strng;
    int     c = reader->last_character;
    double  fl;

    while (((c<='9') && (c>='0')) || (c=='.') || (c=='-') || (c=='e') || (c=='E')) {
        *(s++) = c;
        c = gbt_get_char(reader);
    }
    *s = 0;
    fl = GB_atof(strng);

    gbt_eat_white(reader);

    return fl;
}

static char *gbt_read_quoted_string(TreeReader *reader){
    /* Read in a quoted or unquoted string.
     * in quoted strings double quotes ('') are replaced by (')
     */

    char  buffer[1024];
    char *s = buffer;
    int   c = reader->last_character;

    if (c == '\'') {
        c = gbt_get_char(reader);
        while ( (c!= EOF) && (c!='\'') ) {
        gbt_lt_double_quot:
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                setReaderError(reader, GBS_global_string("Name '%s' longer than 1000 bytes", buffer));
                return NULL;
            }
            c = gbt_get_char(reader);
        }
        if (c == '\'') {
            c = gbt_read_char(reader);
            if (c == '\'') goto gbt_lt_double_quot;
        }
    }else{
        while ( c== '_') c = gbt_read_char(reader);
        while ( c== ' ') c = gbt_read_char(reader);
        while ( (c != ':') && (c!= EOF) && (c!=',') &&
                (c!=';') && (c!= ')') )
        {
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                setReaderError(reader, GBS_global_string("Name '%s' longer than 1000 bytes", buffer));
                return NULL;
            }
            c = gbt_read_char(reader);
        }
    }
    *s = 0;
    return strdup(buffer);
}

static void setBranchName(TreeReader *reader, GBT_TREE *node, char *name) {
    /* detect bootstrap values */
    /* name has to be stored in node or must be free'ed */
    
    char   *end       = 0;
    double  bootstrap = strtod(name, &end);

    if (end == name) {          // no digits -> no bootstrap
        node->name = name;
    }
    else {
        bootstrap = bootstrap*100.0 + 0.5; // needed if bootstrap values are between 0.0 and 1.0 */
        // downscaling in done later!

        if (bootstrap>reader->max_found_bootstrap) {
            reader->max_found_bootstrap = bootstrap;
        }

        assert(node->remark_branch == 0);
        node->remark_branch  = GBS_global_string_copy("%i%%", (int)bootstrap);

        if (end[0] != 0) {      // sth behind bootstrap value
            if (end[0] == ':') ++end; // ARB format for nodes with bootstraps AND node name is 'bootstrap:nodename'
            node->name = strdup(end);
        }
        free(name);
    }
}

static GB_BOOL gbt_readNameAndLength(TreeReader *reader, GBT_TREE *node, GBT_LEN *len) {
    /* reads the branch-length and -name
       '*len' should normally be initialized with DEFAULT_LENGTH_MARKER 
     * returns the branch-length in 'len' and sets the branch-name of 'node'
     * returns GB_TRUE if successful, otherwise reader->error gets set
     */

    GB_BOOL done = GB_FALSE;
    while (!done && !reader->error) {
        switch (reader->last_character) {
            case ';':
            case ',':
            case ')':
                done = GB_TRUE;
                break;
            case ':':
                gbt_read_char(reader);      /* drop ':' */
                *len = gbt_read_number(reader);
                if (*len>reader->max_found_branchlen) {
                    reader->max_found_branchlen = *len;
                }
                break;
            default: {
                char *branchName = gbt_read_quoted_string(reader);
                if (branchName) {
                    setBranchName(reader, node, branchName);
                }
                else {
                    setReaderError(reader, "Expected branch-name or one of ':;,)'");
                }
                break;
            }
        }
    }

    return !reader->error;
}

static GBT_TREE *gbt_linkedTreeNode(GBT_TREE *left, GBT_LEN leftlen, GBT_TREE *right, GBT_LEN rightlen, int structuresize) {
    GBT_TREE *node = (GBT_TREE*)GB_calloc(1, structuresize);
                                
    node->leftson  = left;
    node->leftlen  = leftlen;
    node->rightson = right;
    node->rightlen = rightlen;

    left->father  = node;
    right->father = node;

    return node;
}

static GBT_TREE *gbt_load_tree_rek(TreeReader *reader, int structuresize, GBT_LEN *nodeLen) {
    GBT_TREE *node = 0;

    if (reader->last_character == '(') {
        gbt_read_char(reader);  // drop the '('

        GBT_LEN   leftLen = DEFAULT_LENGTH_MARKER;
        GBT_TREE *left    = gbt_load_tree_rek(reader, structuresize, &leftLen);

        ad_assert(left || reader->error);

        if (left) {
            if (gbt_readNameAndLength(reader, left, &leftLen)) {
                switch (reader->last_character) {
                    case ')':               /* single node */
                        *nodeLen = leftLen;
                        node     = left;
                        left     = 0;
                        break;

                    case ',': {
                        GBT_LEN   rightLen = DEFAULT_LENGTH_MARKER;
                        GBT_TREE *right    = 0;

                        while (reader->last_character == ',' && !reader->error) {
                            if (right) { /* multi-branch */
                                GBT_TREE *pair = gbt_linkedTreeNode(left, leftLen, right, rightLen, structuresize);
                                
                                left  = pair; leftLen = 0;
                                right = 0; rightLen = DEFAULT_LENGTH_MARKER;
                            }

                            gbt_get_char(reader); /* drop ',' */
                            right = gbt_load_tree_rek(reader, structuresize, &rightLen);
                            if (right) gbt_readNameAndLength(reader, right, &rightLen);
                        }

                        if (reader->last_character == ')') {
                            node     = gbt_linkedTreeNode(left, leftLen, right, rightLen, structuresize);
                            *nodeLen = DEFAULT_LENGTH_MARKER;

                            left = 0;
                            right  = 0;

                            gbt_read_char(reader); /* drop ')' */
                        }
                        else {
                            setReaderError(reader, "Expected one of ',)'");
                        }
                        
                        free(right);

                        break;
                    }

                    default:
                        setReaderError(reader, "Expected one of ',)'");
                        break;
                }
            }
            else {
                ad_assert(reader->error);
            }

            free(left);
        }
    }
    else {                      /* single node */
        gbt_eat_white(reader);
        char *name = gbt_read_quoted_string(reader);
        if (name) {
            node          = (GBT_TREE*)GB_calloc(1, structuresize);
            node->is_leaf = GB_TRUE;
            node->name    = name;
        }
        else {
            setReaderError(reader, "Expected quoted string");
        }
    }

    ad_assert(node || reader->error);
    return node;
}



void GBT_scale_tree(GBT_TREE *tree, double length_scale, double bootstrap_scale) {
    if (tree->leftson) {
        if (tree->leftlen <= DEFAULT_LENGTH_MARKER) tree->leftlen  = DEFAULT_LENGTH;
        else                                        tree->leftlen *= length_scale;
        
        GBT_scale_tree(tree->leftson, length_scale, bootstrap_scale);
    }
    if (tree->rightson) {
        if (tree->rightlen <= DEFAULT_LENGTH_MARKER) tree->rightlen  = DEFAULT_LENGTH; 
        else                                         tree->rightlen *= length_scale;
        
        GBT_scale_tree(tree->rightson, length_scale, bootstrap_scale);
    }

    if (tree->remark_branch) {
        const char *end          = 0;
        double      bootstrap    = strtod(tree->remark_branch, (char**)&end);
        GB_BOOL     is_bootstrap = end[0] == '%' && end[1] == 0;

        freeset(tree->remark_branch, 0);

        if (is_bootstrap) {
            bootstrap = bootstrap*bootstrap_scale+0.5;
            tree->remark_branch  = GBS_global_string_copy("%i%%", (int)bootstrap);
        }
    }
}

char *GBT_newick_comment(const char *comment, GB_BOOL escape) {
    /* escape or unescape a newick tree comment
     * (']' is not allowed there)
     */

    char *result = 0;
    if (escape) {
        result = GBS_string_eval(comment, "\\\\=\\\\\\\\:[=\\\\{:]=\\\\}", NULL); // \\\\ has to be first!
    }
    else {
        result = GBS_string_eval(comment, "\\\\}=]:\\\\{=[:\\\\\\\\=\\\\", NULL); // \\\\ has to be last!
    }
    return result;
}

GBT_TREE *GBT_load_tree(const char *path, int structuresize, char **commentPtr, int allow_length_scaling, char **warningPtr) {
    /* Load a newick compatible tree from file 'path',
       structure size should be >0, see GBT_read_tree for more information
       if commentPtr != NULL -> set it to a malloc copy of all concatenated comments found in tree file
       if warningPtr != NULL -> set it to a malloc copy auto-scale-warning (if autoscaling happens)
    */

    GBT_TREE *tree  = 0;
    FILE     *input = fopen(path, "rt");
    GB_ERROR  error = 0;

    if (!input) {
        error = GBS_global_string("No such file: %s", path);
    }
    else {
        const char *name_only = strrchr(path, '/');
        if (name_only) ++name_only;
        else        name_only = path;

        TreeReader *reader      = newTreeReader(input, name_only);
        GBT_LEN     rootNodeLen = DEFAULT_LENGTH_MARKER; /* root node has no length. only used as input to gbt_load_tree_rek*/
        tree                    = gbt_load_tree_rek(reader, structuresize, &rootNodeLen);
        fclose(input);

        if (tree) {
            double bootstrap_scale = 1.0;
            double branchlen_scale = 1.0;

            if (reader->max_found_bootstrap >= 101.0) { // bootstrap values were given in percent
                bootstrap_scale = 0.01;
                if (warningPtr) {
                    *warningPtr = GBS_global_string_copy("Auto-scaling bootstrap values by factor %.2f (max. found bootstrap was %5.2f)",
                                                         bootstrap_scale, reader->max_found_bootstrap);
                }
            }
            if (reader->max_found_branchlen >= 1.01) { // assume branchlengths have range [0;100]
                if (allow_length_scaling) {
                    branchlen_scale = 0.01;
                    if (warningPtr) {
                        char *w = GBS_global_string_copy("Auto-scaling branchlengths by factor %.2f (max. found branchlength = %5.2f)",
                                                         branchlen_scale, reader->max_found_branchlen);
                        if (*warningPtr) {
                            char *w2 = GBS_global_string_copy("%s\n%s", *warningPtr, w);

                            free(*warningPtr);
                            free(w);
                            *warningPtr = w2;
                        }
                        else {
                            *warningPtr = w;
                        }
                    }
                }
            }

            GBT_scale_tree(tree, branchlen_scale, bootstrap_scale); // scale bootstraps and branchlengths

            if (commentPtr) {
                char *comment = getTreeComment(reader);

                assert(*commentPtr == 0);
                if (comment && comment[0]) {
                    char *unescaped = GBT_newick_comment(comment, GB_FALSE);
                    *commentPtr     = unescaped;
                }
                free(comment);
            }
        }

        freeTreeReader(reader);
    }

    ad_assert(tree||error);
    if (error) {
        GB_export_error("Import tree: %s", error);
        ad_assert(!tree);
    }

    return tree;
}


GBDATA *GBT_get_tree(GBDATA *gb_main, const char *tree_name) {
    /* returns the datapntr to the database structure, which is the container for the tree */
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    gb_tree = GB_entry(gb_treedata, tree_name) ;
    return gb_tree;
}

long GBT_size_of_tree(GBDATA *gb_main, const char *tree_name) {
    GBDATA *gb_tree = GBT_get_tree(gb_main,tree_name);
    GBDATA *gb_nnodes;
    if (!gb_tree) return -1;
    gb_nnodes = GB_entry(gb_tree,"nnodes");
    if (!gb_nnodes) return -1;
    return GB_read_int(gb_nnodes);
}

char *GBT_find_largest_tree(GBDATA *gb_main){
    char   *largest     = 0;
    long    maxnodes    = 0;
    GBDATA *gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    GBDATA *gb_tree;

    for (gb_tree = GB_child(gb_treedata);
         gb_tree;
         gb_tree = GB_nextChild(gb_tree))
    {
        long *nnodes = GBT_read_int(gb_tree, "nnodes");
        if (nnodes && *nnodes>maxnodes) {
            freeset(largest, GB_read_key(gb_tree));
            maxnodes = *nnodes;
        }
    }
    return largest;
}

char *GBT_find_latest_tree(GBDATA *gb_main){
    char **names = GBT_get_tree_names(gb_main);
    char *name = 0;
    char **pname;
    if (!names) return NULL;
    for (pname = names;*pname;pname++) name = *pname;
    if (name) name = strdup(name);
    GBT_free_names(names);
    return name;
}

const char *GBT_tree_info_string(GBDATA *gb_main, const char *tree_name, int maxTreeNameLen) {
    /* maxTreeNameLen shall be the max len of the longest tree name (or -1 -> do not format) */

    const char *result  = 0;
    GBDATA     *gb_tree = GBT_get_tree(gb_main,tree_name);
    
    if (!gb_tree) {
        GB_export_error("tree '%s' not found",tree_name);
    }
    else {
        GBDATA *gb_nnodes = GB_entry(gb_tree,"nnodes");
        if (!gb_nnodes) {
            GB_export_error("nnodes not found in tree '%s'",tree_name);
        }
        else {
            const char *sizeInfo = GBS_global_string("(%li:%i)", GB_read_int(gb_nnodes)+1, GB_read_security_write(gb_tree));
            GBDATA     *gb_rem   = GB_entry(gb_tree,"remark");
            int         len;

            if (maxTreeNameLen == -1) {
                result = GBS_global_string("%s %11s", tree_name, sizeInfo);
                len    = strlen(tree_name);
            }
            else {
                result = GBS_global_string("%-*s %11s", maxTreeNameLen, tree_name, sizeInfo);
                len    = maxTreeNameLen;
            }
            if (gb_rem) {
                const char *remark    = GB_read_char_pntr(gb_rem);
                const int   remarkLen = 800;
                char       *res2      = GB_give_other_buffer(remark, len+1+11+2+remarkLen+1);

                strcpy(res2, result);
                strcat(res2, "  ");
                strncat(res2, remark, remarkLen);

                result = res2;
            }
        }
    }
    return result;
}

GB_ERROR GBT_check_tree_name(const char *tree_name)
{
    GB_ERROR error;
    if ( (error = GB_check_key(tree_name)) ) return error;
    if (strncmp(tree_name,"tree_",5)){
        return GB_export_error("your treename '%s' does not begin with 'tree_'",tree_name);
    }
    return 0;
}

char **GBT_get_tree_names_and_count(GBDATA *Main, int *countPtr){
    /* returns an null terminated array of string pointers */

    int      count       = 0;
    GBDATA  *gb_treedata = GB_entry(Main,"tree_data");
    char   **erg         = 0;

    if (gb_treedata) {
        GBDATA *gb_tree;
        count = 0;

        for (gb_tree = GB_child(gb_treedata);
             gb_tree;
             gb_tree = GB_nextChild(gb_tree))
        {
            count ++;
        }

        if (count) {
            erg   = (char **)GB_calloc(sizeof(char *),(size_t)count+1);
            count = 0;

            for (gb_tree = GB_child(gb_treedata);
                 gb_tree;
                 gb_tree = GB_nextChild(gb_tree) )
            {
                erg[count] = GB_read_key(gb_tree);
                count ++;
            }
        }
    }

    *countPtr = count;
    return erg;
}

char **GBT_get_tree_names(GBDATA *Main){
    int dummy;
    return GBT_get_tree_names_and_count(Main, &dummy);
}

char *GBT_get_next_tree_name(GBDATA *gb_main, const char *tree){
    GBDATA *gb_treedata;
    GBDATA *gb_tree = 0;
    gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    if (tree){
        gb_tree = GB_entry(gb_treedata,tree);
    }
    if (gb_tree){
        gb_tree = GB_nextEntry(gb_tree);
    }else{
        gb_tree = GB_child(gb_treedata);
    }
    if (gb_tree) return GB_read_key(gb_tree);
    return NULL;
}

int gbt_sum_leafs(GBT_TREE *tree){
    if (tree->is_leaf){
        return 1;
    }
    return gbt_sum_leafs(tree->leftson) + gbt_sum_leafs(tree->rightson);
}

GB_CSTR *gbt_fill_species_names(GB_CSTR *des,GBT_TREE *tree) {
    if (tree->is_leaf){
        des[0] = tree->name;
        return des+1;
    }
    des = gbt_fill_species_names(des,tree->leftson);
    des = gbt_fill_species_names(des,tree->rightson);
    return des;
}

/* creates an array of all species names in a tree,
   the names is not strdupped !!! */

GB_CSTR *GBT_get_species_names_of_tree(GBT_TREE *tree){
    int size = gbt_sum_leafs(tree);
    GB_CSTR *result = (GB_CSTR *)GB_calloc(sizeof(char *),size +1);
#if defined(DEBUG)
    GB_CSTR *check =
#endif // DEBUG
        gbt_fill_species_names(result,tree);
    assert(check - size == result);
    return result;
}

/* search for an existing or an alternate tree */
char *GBT_existing_tree(GBDATA *Main, const char *tree) {
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    gb_treedata = GB_entry(Main,"tree_data");
    if (!gb_treedata) return NULL;
    gb_tree = GB_entry(gb_treedata,tree);
    if (gb_tree) return strdup(tree);
    gb_tree = GB_child(gb_treedata);
    if (!gb_tree) return NULL;
    return GB_read_key(gb_tree);
}

void gbt_export_tree_node_print_remove(char *str){
    int i,len;
    len = strlen (str);
    for(i=0;i<len;i++) {
        if (str[i] =='\'') str[i] ='.';
        if (str[i] =='\"') str[i] ='.';
    }
}

void gbt_export_tree_rek(GBT_TREE *tree,FILE *out){
    if (tree->is_leaf) {
        gbt_export_tree_node_print_remove(tree->name);
        fprintf(out," '%s' ",tree->name);
    }else{
        fprintf(out, "(");
        gbt_export_tree_rek(tree->leftson,out);
        fprintf(out, ":%.5f,", tree->leftlen);
        gbt_export_tree_rek(tree->rightson,out);
        fprintf(out, ":%.5f", tree->rightlen);
        fprintf(out, ")");
        if (tree->name){
            fprintf(out, "'%s'",tree->name);
        }
    }
}


GB_ERROR GBT_export_tree(GBDATA *gb_main,FILE *out,GBT_TREE *tree, GB_BOOL triple_root){
    GBUSE(gb_main);
    if(triple_root){
        GBT_TREE *one,*two,*three;
        if (tree->is_leaf){
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf && tree->rightson->is_leaf){
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf){
            one = tree->leftson;
            two = tree->rightson->leftson;
            three = tree->rightson->rightson;
        }else{
            one = tree->leftson->leftson;
            two = tree->leftson->rightson;
            three = tree->rightson;
        }
        fprintf(out, "(");
        gbt_export_tree_rek(one,out);
        fprintf(out, ":%.5f,", 1.0);
        gbt_export_tree_rek(two,out);
        fprintf(out, ":%.5f,", 1.0);
        gbt_export_tree_rek(three,out);
        fprintf(out, ":%.5f)", 1.0);
    }else{
        gbt_export_tree_rek(tree,out);
    }
    return 0;
}

/********************************************************************************************
                    species functions
********************************************************************************************/


GBDATA *GBT_find_or_create_item_rel_item_data(GBDATA *gb_item_data, const char *itemname, const char *id_field, const char *id, GB_BOOL markCreated) {
    /* Search for a item with field 'id_field' set to given 'id' (id compare is case-insensitive)
     * If item does not exist, create it.
     * Newly created items are automatically marked, if 'markCreated' is GB_TRUE
     * items may be: species, genes, SAIs, ...
     */

    GBDATA   *gb_item = 0;
    GB_ERROR  error   = 0;

    if (!gb_item_data) error = "No container";
    else {
        gb_item = GBT_find_item_rel_item_data(gb_item_data, id_field, id);
        if (!gb_item) {
            error = GB_push_transaction(gb_item_data);
            if (!error) {
                gb_item             = GB_create_container(gb_item_data, itemname); // create a new item
                if (!gb_item) error = GB_await_error();
                else {
                    error = GBT_write_string(gb_item, id_field, id); // write item identifier
                    if (!error && markCreated) error = GB_write_flag(gb_item, 1); // mark generated item
                }
            }
            error = GB_end_transaction(gb_item_data, error);
        }
    }

    if (!gb_item && !error) error = GB_await_error();
    if (error) {
        gb_item = 0;
        GB_export_error("Can't create %s '%s': %s", itemname, id, error);
    }

    return gb_item;
}

GBDATA *GBT_find_or_create_species_rel_species_data(GBDATA *gb_species_data, const char *name) {
    return GBT_find_or_create_item_rel_item_data(gb_species_data, "species", "name", name, GB_TRUE);
}

GBDATA *GBT_find_or_create_species(GBDATA *gb_main, const char *name) {
    return GBT_find_or_create_species_rel_species_data(GBT_get_species_data(gb_main), name);
}

GBDATA *GBT_find_or_create_SAI(GBDATA *gb_main,const char *name) {
    /* Search for an SAI, when SAI does not exist, create it */
    return GBT_find_or_create_item_rel_item_data(GBT_get_SAI_data(gb_main), "extended", "name", name, GB_TRUE);
}

#if defined(DEVEL_RALF)
#warning GBT_add_data is weird: name sucks and why do anything special for GB_STRING? \
    // better replace by GBT_ali_container(gb_spec, ali_name) and then use normal functions (GB_search or whatever) 
#endif /* DEVEL_RALF */

GBDATA *GBT_add_data(GBDATA *species,const char *ali_name, const char *key, GB_TYPES type)
{
    /* the same as GB_search(species, 'ali_name/key', GB_CREATE) */
    GBDATA *gb_gb;
    GBDATA *gb_data;
    if (GB_check_key(ali_name)) {
        return NULL;
    }
    if (GB_check_hkey(key)) {
        return NULL;
    }
    gb_gb = GB_entry(species,ali_name);
    if (!gb_gb) gb_gb = GB_create_container(species,ali_name);

    if (type == GB_STRING) {
        gb_data = GB_search(gb_gb, key, GB_FIND);
        if (!gb_data){
            gb_data = GB_search(gb_gb, key, GB_STRING);
            GB_write_string(gb_data,"...");
        }
    }
    else{
        gb_data = GB_search(gb_gb, key, type);
    }
    return gb_data;
}


GB_ERROR GBT_write_sequence(GBDATA *gb_data, const char *ali_name, long ali_len, const char *sequence) {
    /* writes a sequence which is generated by GBT_add_data,
     * cuts sequence after alignment len only if bases e ".-nN" */
    int slen = strlen(sequence);
    int old_char = 0;
    GB_ERROR error = 0;
    if (slen > ali_len) {
        int i;
        for (i= slen -1; i>=ali_len; i--) {
            if (!strchr("-.nN",sequence[i])) break;     /* real base after end of alignment */
        }
        i++;                            /* points to first 0 after alignment */
        if (i > ali_len){
            GBDATA *gb_main = GB_get_root(gb_data);
            ali_len = GBT_get_alignment_len(gb_main,ali_name);
            if (slen > ali_len){                /* check for modified alignment len */
                GBT_set_alignment_len(gb_main,ali_name,i);
                ali_len = i;
            }
        }
        if (slen > ali_len){
            old_char = sequence[ali_len];
            ((char*)sequence)[ali_len] = 0;
        }
    }
    error = GB_write_string(gb_data,sequence);
    if (slen> ali_len) ((char*)sequence)[ali_len] = old_char;
    return error;
}


GBDATA *GBT_gen_accession_number(GBDATA *gb_species,const char *ali_name) {
    GBDATA *gb_acc = GB_entry(gb_species,"acc");
    if (!gb_acc) {
        GBDATA *gb_data = GBT_read_sequence(gb_species,ali_name);
        if (gb_data) {                                     /* found a valid alignment */
            GB_CSTR     sequence = GB_read_char_pntr(gb_data);
            long        id       = GBS_checksum(sequence,1,".-");
            const char *acc      = GBS_global_string("ARB_%lX", id);
            GB_ERROR    error    = GBT_write_string(gb_species, "acc", acc);

            if (error) GB_export_error(error);
        }
    }
    return gb_acc;
}


int GBT_is_partial(GBDATA *gb_species, int default_value, int define_if_undef) {
    // checks whether a species has a partial or full sequence
    //
    // Note: partial sequences should not be used for tree calculations
    //
    // returns: 0 if sequence is full
    //          1 if sequence is partial
    //          -1 in case of error
    //
    // if the sequence has no 'ARB_partial' entry it returns 'default_value'
    // if 'define_if_undef' is true then create an 'ARB_partial'-entry with the default value

    int       result     = -1;
    GB_ERROR  error      = 0;
    GBDATA   *gb_partial = GB_entry(gb_species, "ARB_partial");

    if (gb_partial) {
        result = GB_read_int(gb_partial);
        if (result != 0 && result != 1) {
            error = "Illegal value for 'ARB_partial' (only 1 or 0 allowed)";
        }
    }
    else {
        if (define_if_undef) {
            error = GBT_write_int(gb_species, "ARB_partial", default_value);
        }
        result = default_value;
    }

    if (error) {
        GB_export_error(error);
        return -1;
    }
    return result;
}

/********************************************************************************************
                    some simple find procedures
********************************************************************************************/

GBDATA *GBT_find_item_rel_item_data(GBDATA *gb_item_data, const char *id_field, const char *id_value) {
    // 'gb_item_data' is a container containing items
    // 'id_field' is a field containing a unique identifier for each item (e.g. 'name' for species)
    //
    // returns a pointer to an item with 'id_field' containing 'id_value'
    // or NULL
    
    GBDATA *gb_item_id = GB_find_string(gb_item_data, id_field, id_value, GB_IGNORE_CASE, down_2_level);
    return gb_item_id ? GB_get_father(gb_item_id) : 0;
}

/* -------------------------------------------------------------------------------- */

GBDATA *GBT_get_species_data(GBDATA *gb_main) {
    return GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
}

GBDATA *GBT_first_marked_species_rel_species_data(GBDATA *gb_species_data) {
    return GB_first_marked(gb_species_data,"species");
}

GBDATA *GBT_first_marked_species(GBDATA *gb_main) {
    return GB_first_marked(GBT_get_species_data(gb_main), "species");
}
GBDATA *GBT_next_marked_species(GBDATA *gb_species) {
    gb_assert(GB_has_key(gb_species, "species"));
    return GB_next_marked(gb_species,"species");
}

GBDATA *GBT_first_species_rel_species_data(GBDATA *gb_species_data) {
    return GB_entry(gb_species_data,"species");
}
GBDATA *GBT_first_species(GBDATA *gb_main) {
    return GB_entry(GBT_get_species_data(gb_main),"species");
}

GBDATA *GBT_next_species(GBDATA *gb_species) {
    gb_assert(GB_has_key(gb_species, "species"));
    return GB_nextEntry(gb_species);
}

GBDATA *GBT_find_species_rel_species_data(GBDATA *gb_species_data,const char *name) {
    return GBT_find_item_rel_item_data(gb_species_data, "name", name);
}
GBDATA *GBT_find_species(GBDATA *gb_main, const char *name) {
    return GBT_find_item_rel_item_data(GBT_get_species_data(gb_main), "name", name);
}

/* -------------------------------------------------------------------------------- */

GBDATA *GBT_get_SAI_data(GBDATA *gb_main) {
    return GB_search(gb_main, "extended_data", GB_CREATE_CONTAINER);
}

GBDATA *GBT_first_marked_SAI_rel_SAI_data(GBDATA *gb_sai_data) {
    return GB_first_marked(gb_sai_data, "extended");
}

GBDATA *GBT_next_marked_SAI(GBDATA *gb_sai) {
    gb_assert(GB_has_key(gb_sai, "extended"));
    return GB_next_marked(gb_sai, "extended");
}

/* Search SAIs */
GBDATA *GBT_first_SAI_rel_SAI_data(GBDATA *gb_sai_data) {
    return GB_entry(gb_sai_data, "extended");
}
GBDATA *GBT_first_SAI(GBDATA *gb_main) {
    return GB_entry(GBT_get_SAI_data(gb_main),"extended");
}

GBDATA *GBT_next_SAI(GBDATA *gb_sai) {
    gb_assert(GB_has_key(gb_sai, "extended"));
    return GB_nextEntry(gb_sai);
}

GBDATA *GBT_find_SAI_rel_SAI_data(GBDATA *gb_sai_data, const char *name) {
    return GBT_find_item_rel_item_data(gb_sai_data, "name", name);
}
GBDATA *GBT_find_SAI(GBDATA *gb_main, const char *name) {
    return GBT_find_item_rel_item_data(GBT_get_SAI_data(gb_main), "name", name);
}

/* --------------------- */
/*      count items      */

long GBT_get_item_count(GBDATA *gb_parent_of_container, const char *item_container_name) {
    // returns elements stored in a container

    GBDATA *gb_item_data;
    long    count = 0;

    GB_push_transaction(gb_parent_of_container);
    gb_item_data = GB_entry(gb_parent_of_container, item_container_name);
    if (gb_item_data) count = GB_number_of_subentries(gb_item_data);
    GB_pop_transaction(gb_parent_of_container);

    return count;
}

long GBT_get_species_count(GBDATA *gb_main) {
    return GBT_get_item_count(gb_main, "species_data");
}

long GBT_get_SAI_count(GBDATA *gb_main) {
    return GBT_get_item_count(gb_main, "extended_data");
}

/* -------------------------------------------------------------------------------- */

char *GBT_create_unique_item_identifier(GBDATA *gb_item_container, const char *id_field, const char *default_id) {
    // returns an identifier not used by items in 'gb_item_container'
    // 'id_field' is the entry that is used as identifier (e.g. 'name' for species)
    // 'default_id' will be suffixed with a number to generate a unique id
    //
    // Note:
    // * The resulting id may be longer than 8 characters
    // * This function is slow, so just use in extra-ordinary situations

    GBDATA *gb_item   = GBT_find_item_rel_item_data(gb_item_container, id_field, default_id);
    char   *unique_id;

    if (!gb_item) {
        unique_id = strdup(default_id); // default_id is unused
    }
    else {
        char   *generated_id  = malloc(strlen(default_id)+20);
        size_t  min_num = 1;

#define GENERATE_ID(num) sprintf(generated_id,"%s%zi", default_id, num);
        
        GENERATE_ID(min_num);
        gb_item = GBT_find_item_rel_item_data(gb_item_container, id_field, generated_id);

        if (gb_item) {
            size_t num_items = GB_number_of_subentries(gb_item_container);
            size_t max_num   = 0;

            ad_assert(num_items); // otherwise deadlock below

            do {
                max_num += num_items;
                GENERATE_ID(max_num);
                gb_item  = GBT_find_item_rel_item_data(gb_item_container, id_field, generated_id);
            } while (gb_item && max_num >= num_items);

            if (max_num<num_items) { // overflow
                char *uid;
                generated_id[0] = 'a'+GB_random(26);
                generated_id[1] = 'a'+GB_random(26);
                generated_id[2] = 0;

                uid = GBT_create_unique_item_identifier(gb_item_container, id_field, generated_id);
                strcpy(generated_id, uid);
                free(uid);
            }
            else {
                // max_num is unused
                while ((max_num-min_num)>1) {
                    size_t mid = (min_num+max_num)/2;
                    ad_assert(mid != min_num && mid != max_num);

                    GENERATE_ID(mid);
                    gb_item = GBT_find_item_rel_item_data(gb_item_container, id_field, generated_id);

                    if (gb_item) min_num = mid;
                    else max_num = mid;
                }
                GENERATE_ID(max_num);
                ad_assert(GBT_find_item_rel_item_data(gb_item_container, id_field, generated_id) == NULL);
            }
        }
        unique_id = generated_id;

#undef GENERATE_ID
    }

    return unique_id;
}

char *GBT_create_unique_species_name(GBDATA *gb_main, const char *default_name) {
    return GBT_create_unique_item_identifier(GBT_get_species_data(gb_main), "name", default_name);
}

/********************************************************************************************
                    mark and unmark species
********************************************************************************************/
void GBT_mark_all(GBDATA *gb_main, int flag)
{
    GBDATA *gb_species;
    GB_push_transaction(gb_main);

    if (flag == 2) {
        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            GB_write_flag(gb_species,!GB_read_flag(gb_species));
        }
    }
    else {
        gb_assert(flag == 0 || flag == 1);

        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            GB_write_flag(gb_species,flag);
        }
    }
    GB_pop_transaction(gb_main);
}
void GBT_mark_all_that(GBDATA *gb_main, int flag, int (*condition)(GBDATA*, void*), void *cd)
{
    GBDATA *gb_species;
    GB_push_transaction(gb_main);

    if (flag == 2) {
        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            if (condition(gb_species, cd)) {
                GB_write_flag(gb_species,!GB_read_flag(gb_species));
            }
        }
    }
    else {
        gb_assert(flag == 0 || flag == 1);

        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            int curr_flag = GB_read_flag(gb_species);
            if (curr_flag != flag && condition(gb_species, cd)) {
                GB_write_flag(gb_species,flag);
            }
        }
    }
    GB_pop_transaction(gb_main);
}

long GBT_count_marked_species(GBDATA *gb_main)
{
    long    cnt = 0;
    GBDATA *gb_species_data;

    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GB_pop_transaction(gb_main);

    cnt = GB_number_of_marked_subentries(gb_species_data);
    return cnt;
}

char *GBT_store_marked_species(GBDATA *gb_main, int unmark_all)
{
    /* stores the currently marked species in a string
       if (unmark_all != 0) then unmark them too
    */

    void   *out = GBS_stropen(10000);
    GBDATA *gb_species;

    for (gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        GBS_strcat(out, GBT_read_name(gb_species));
        GBS_chrcat(out, ';');
        if (unmark_all) GB_write_flag(gb_species, 0);
    }

    GBS_str_cut_tail(out, 1); // remove trailing ';'
    return GBS_strclose(out);
}

NOT4PERL GB_ERROR GBT_with_stored_species(GBDATA *gb_main, const char *stored, species_callback doit, int *clientdata) {
    /* call function 'doit' with all species stored in 'stored' */

#define MAX_NAME_LEN 20
    char     name[MAX_NAME_LEN+1];
    GB_ERROR error = 0;

    while (!error) {
        char   *p   = strchr(stored, ';');
        int     len = p ? (p-stored) : (int)strlen(stored);
        GBDATA *gb_species;

        gb_assert(len <= MAX_NAME_LEN);
        memcpy(name, stored, len);
        name[len] = 0;

        gb_species = GBT_find_species(gb_main, name);
        if (gb_species) {
            error = doit(gb_species, clientdata);
        }
        else {
            error = "Some stored species where not found.";
        }

        if (!p) break;
        stored = p+1;
    }
#undef MAX_NAME_LEN
    return error;
}

static GB_ERROR restore_mark(GBDATA *gb_species, int *clientdata) {
    GBUSE(clientdata);
    GB_write_flag(gb_species, 1);
    return 0;
}

GB_ERROR GBT_restore_marked_species(GBDATA *gb_main, const char *stored_marked) {
    /* restores the species-marks to a state currently saved
       into 'stored_marked' by GBT_store_marked_species
    */

    GBT_mark_all(gb_main, 0);   /* unmark all species */
    return GBT_with_stored_species(gb_main, stored_marked, restore_mark, 0);
}

/********************************************************************************************
                    read species information
********************************************************************************************/

#if defined(DEVEL_RALF)
#warning rename GBT_read_sequence - it does not read the sequence itself
#endif /* DEVEL_RALF */
GBDATA *GBT_read_sequence(GBDATA *gb_species, const char *aliname) {
    GBDATA *gb_ali = GB_entry(gb_species, aliname);
    return gb_ali ? GB_entry(gb_ali, "data") : 0;
}

#if defined(DEVEL_RALF)
#warning rename the following functions to make the difference more obvious??
#endif /* DEVEL_RALF */
GB_CSTR GBT_read_name(GBDATA *gb_item) {
    GB_CSTR result      = GBT_read_char_pntr(gb_item, "name");
    if (!result) result = GBS_global_string("<unnamed_%s>", GB_read_key_pntr(gb_item));
    return result;
}

const char *GBT_get_name(GBDATA *gb_item) {
    GBDATA *gb_name = GB_entry(gb_item, "name");
    if (!gb_name) return 0;
    return GB_read_char_pntr(gb_name);
}

/********************************************************************************************
                    alignment procedures
********************************************************************************************/

char *GBT_get_default_alignment(GBDATA *gb_main) {
    return GBT_read_string(gb_main, "presets/use");
}

GB_ERROR GBT_set_default_alignment(GBDATA *gb_main,const char *alignment_name) {
    return GBT_write_string(gb_main, "presets/use", alignment_name);
}

/* the following function were meant to use user defined values.
 *
 * Especially for 'ECOLI' there is already a possibility to
 * specify a different reference in edit4, but there's no
 * data model in the DB for it. Consider whether it makes sense,
 * if secedit uses it as well.
 */

char *GBT_get_default_helix(GBDATA *gb_main) {
    GBUSE(gb_main);
    return strdup("HELIX");
}

char *GBT_get_default_helix_nr(GBDATA *gb_main) {
    GBUSE(gb_main);
    return strdup("HELIX_NR");
}

char *GBT_get_default_ref(GBDATA *gb_main) {
    GBUSE(gb_main);
    return strdup("ECOLI");
}


GBDATA *GBT_get_alignment(GBDATA *gb_main, const char *aliname) {
    GBDATA *gb_presets        = GB_search(gb_main, "presets", GB_CREATE_CONTAINER);
    GBDATA *gb_alignment_name = GB_find_string(gb_presets,"alignment_name",aliname,GB_IGNORE_CASE,down_2_level);
    
    if (!gb_alignment_name) {
        GB_export_error("alignment '%s' not found", aliname);
        return NULL;
    }
    return GB_get_father(gb_alignment_name);
}

#if defined(DEVEL_RALF)
#warning recode and change result type to long* ? 
#endif /* DEVEL_RALF */
long GBT_get_alignment_len(GBDATA *gb_main, const char *aliname) {
    GBDATA *gb_alignment = GBT_get_alignment(gb_main, aliname);
    return gb_alignment ? *GBT_read_int(gb_alignment, "alignment_len") : -1;
}

GB_ERROR GBT_set_alignment_len(GBDATA *gb_main, const char *aliname, long new_len) {
    GB_ERROR  error        = 0;
    GBDATA   *gb_alignment = GBT_get_alignment(gb_main, aliname);

    if (gb_alignment) {
        GB_push_my_security(gb_main);
        error             = GBT_write_int(gb_alignment, "alignment_len", new_len); /* write new len */
        if (!error) error = GBT_write_int(gb_alignment, "aligned", 0);             /* mark as unaligned */
        GB_pop_my_security(gb_main);
    }
    else error = GB_export_error("Alignment '%s' not found", aliname);
    return error;
}

int GBT_get_alignment_aligned(GBDATA *gb_main, const char *aliname) {
    GBDATA *gb_alignment = GBT_get_alignment(gb_main, aliname);
    return gb_alignment ? *GBT_read_int(gb_alignment, "aligned") : -1;
}

char *GBT_get_alignment_type_string(GBDATA *gb_main, const char *aliname) {
    char   *result       = NULL;
    GBDATA *gb_alignment = GBT_get_alignment(gb_main, aliname);
    if (gb_alignment) {
        result = GBT_read_string(gb_alignment, "alignment_type");
        gb_assert(result);
    }
    return result;
}

GB_alignment_type GBT_get_alignment_type(GBDATA *gb_main, const char *aliname) {
    char              *ali_type = GBT_get_alignment_type_string(gb_main, aliname);
    GB_alignment_type  at       = GB_AT_UNKNOWN;

    if (ali_type) {
        switch(ali_type[0]) {
            case 'r': if (strcmp(ali_type, "rna")==0) at = GB_AT_RNA; break;
            case 'd': if (strcmp(ali_type, "dna")==0) at = GB_AT_DNA; break;
            case 'a': if (strcmp(ali_type, "ami")==0) at = GB_AT_AA; break;
            case 'p': if (strcmp(ali_type, "pro")==0) at = GB_AT_AA; break;
            default: ad_assert(0); break;
        }
        free(ali_type);
    }
    return at;
}

GB_BOOL GBT_is_alignment_protein(GBDATA *gb_main,const char *alignment_name) {
    return GBT_get_alignment_type(gb_main,alignment_name) == GB_AT_AA;
}

/********************************************************************************************
                    check routines
********************************************************************************************/
GB_ERROR GBT_check_arb_file(const char *name)
     /* Checks whether the name of a file seemed to be an arb file */
     /* if == 0 it was an arb file */
{
    FILE *in;
    long i;
    char buffer[100];
    if (strchr(name,':')) return 0;
    in = fopen(name,"r");
    if (!in) return GB_export_error("Cannot find file '%s'",name);
    i = gb_read_in_long(in, 0);
    if ( (i== 0x56430176) || (i == GBTUM_MAGIC_NUMBER) || (i == GBTUM_MAGIC_REVERSED)) {
        fclose(in);
        return 0;
    }
    rewind(in);
    fgets(buffer,50,in);
    fclose(in);
    if (!strncmp(buffer,"/*ARBDB AS",10)) {
        return 0;
    };


    return GB_export_error("'%s' is not an arb file",name);
}

/********************************************************************************************
                    analyse the database
********************************************************************************************/
#define GBT_SUM_LEN 4096
/* maximum length of path */

struct DbScanner {
    GB_HASH   *hash_table;
    int        count;
    char     **result;
    GB_TYPES   type;
    char      *buffer;
};

static void gbt_scan_db_rek(GBDATA *gbd,char *prefix, int deep, struct DbScanner *scanner) {
    GB_TYPES type = GB_read_type(gbd);
    GBDATA *gb2;
    const char *key;
    int len_of_prefix;
    if (type == GB_DB) {
        len_of_prefix = strlen(prefix);
        for (gb2 = GB_child(gbd); gb2; gb2 = GB_nextChild(gb2)) {  /* find everything */
            if (deep){
                key = GB_read_key_pntr(gb2);
                sprintf(&prefix[len_of_prefix],"/%s",key);
                gbt_scan_db_rek(gb2, prefix, 1, scanner);
            }
            else {
                prefix[len_of_prefix] = 0;
                gbt_scan_db_rek(gb2, prefix, 1, scanner);
            }
        }
        prefix[len_of_prefix] = 0;
    }
    else {
        if (GB_check_hkey(prefix+1)) {
            prefix = prefix;        /* for debugging purpose */
        }
        else {
            prefix[0] = (char)type;
            GBS_incr_hash(scanner->hash_table, prefix);
        }
    }
}

static long gbs_scan_db_count(const char *key, long val, void *cd_scanner) {
    struct DbScanner *scanner = (struct DbScanner*)cd_scanner;

    scanner->count++;
    key = key;

    return val;
}

struct scan_db_insert {
    struct DbScanner *scanner;
    const char       *datapath;
};

static long gbs_scan_db_insert(const char *key, long val, void *cd_insert_data) {
    struct scan_db_insert *insert = (struct scan_db_insert *)cd_insert_data;
    char *to_insert = 0;

    if (!insert->datapath) {
        to_insert = strdup(key);
    }
    else {
        if (GBS_strscmp(insert->datapath, key+1) == 0) { // datapath matches
            to_insert    = strdup(key+strlen(insert->datapath)); // cut off prefix
            to_insert[0] = key[0]; // copy type
        }
    }

    if (to_insert) {
        struct DbScanner *scanner         = insert->scanner;
        scanner->result[scanner->count++] = to_insert;
    }

    return val;
}

static int gbs_scan_db_compare(const void *left, const void *right, void *unused){
    GBUSE(unused);
    return strcmp((GB_CSTR)left+1, (GB_CSTR)right+1);
}


char **GBT_scan_db(GBDATA *gbd, const char *datapath) {
    /* returns a NULL terminated array of 'strings':
     * - each string is the path to a node beyond gbd;
     * - every string exists only once
     * - the first character of a string is the type of the entry
     * - the strings are sorted alphabetically !!!
     *
     * if datapath != 0, only keys with prefix datapath are scanned and
     * the prefix is removed from the resulting key_names.
     */
    struct DbScanner scanner;

    scanner.hash_table = GBS_create_hash(1024, GB_MIND_CASE);
    scanner.buffer     = (char *)malloc(GBT_SUM_LEN);
    strcpy(scanner.buffer,"");
    
    gbt_scan_db_rek(gbd, scanner.buffer, 0, &scanner);

    scanner.count = 0;
    GBS_hash_do_loop(scanner.hash_table, gbs_scan_db_count, &scanner);

    scanner.result = (char **)GB_calloc(sizeof(char *),scanner.count+1);
    /* null terminated result */

    scanner.count = 0;
    struct scan_db_insert insert = { &scanner, datapath, };
    GBS_hash_do_loop(scanner.hash_table, gbs_scan_db_insert, &insert);

    GBS_free_hash(scanner.hash_table);

    GB_sort((void **)scanner.result, 0, scanner.count, gbs_scan_db_compare, 0);

    free(scanner.buffer);
    return scanner.result;
}

/********************************************************************************************
                send a message to the db server to AWAR_ERROR_MESSAGES
********************************************************************************************/

static void new_gbt_message_created_cb(GBDATA *gb_pending_messages, int *cd, GB_CB_TYPE cbt) {
    static int avoid_deadlock = 0;

    GBUSE(cd);
    GBUSE(cbt);

    if (!avoid_deadlock) {
        GBDATA *gb_msg;

        avoid_deadlock++;
        GB_push_transaction(gb_pending_messages);

        for (gb_msg = GB_entry(gb_pending_messages, "msg"); gb_msg;) {
            {
                const char *msg = GB_read_char_pntr(gb_msg);
                GB_warning("%s", msg);
            }
            {
                GBDATA *gb_next_msg = GB_nextEntry(gb_msg);
                GB_delete(gb_msg);
                gb_msg              = gb_next_msg;
            }
        }

        GB_pop_transaction(gb_pending_messages);
        avoid_deadlock--;
    }
}

void GBT_install_message_handler(GBDATA *gb_main) {
    GBDATA *gb_pending_messages;

    GB_push_transaction(gb_main);
    gb_pending_messages = GB_search(gb_main, AWAR_ERROR_CONTAINER, GB_CREATE_CONTAINER);
    GB_add_callback(gb_pending_messages, GB_CB_SON_CREATED, new_gbt_message_created_cb, 0);
    GB_pop_transaction(gb_main);

#if defined(DEBUG) && 0
    GBT_message(GB_get_root(gb_pending_messages), GBS_global_string("GBT_install_message_handler installed for gb_main=%p", gb_main));
#endif /* DEBUG */
}


void GBT_message(GBDATA *gb_main, const char *msg) {
    // When called in client(or server) this causes the DB server to show the message.
    // Message is showed via GB_warning (which uses aw_message in GUIs)
    //
    // Note: The message is not shown before the transaction ends.
    // If the transaction is aborted, the message is never shown!
    // 
    // see also : GB_warning

    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_pending_messages = GB_search(gb_main, AWAR_ERROR_CONTAINER, GB_CREATE_CONTAINER);
        GBDATA *gb_msg              = gb_pending_messages ? GB_create(gb_pending_messages, "msg", GB_STRING) : 0;

        if (!gb_msg) error = GB_await_error();
        else {
            gb_assert(msg);
            error = GB_write_string(gb_msg, msg);
        }
    }
    error = GB_end_transaction(gb_main, error);

    if (error) {
        fprintf(stderr, "GBT_message: Failed to write message '%s'\n(Reason: %s)\n", msg, error);
    }
}

/********************************************************************************************
                Rename one or many species (only one session at a time/ uses
                commit abort transaction)
********************************************************************************************/
struct gbt_renamed_struct {
    int     used_by;
    char    data[1];

};

struct gbt_rename_struct {
    GB_HASH *renamed_hash;
    GB_HASH *old_species_hash;
    GBDATA *gb_main;
    GBDATA *gb_species_data;
    int all_flag;
} gbtrst;

GB_ERROR GBT_begin_rename_session(GBDATA *gb_main, int all_flag) {
    /* starts a rename session,
     * if whole database shall be renamed, set allflag == 1
     * use GBT_abort_rename_session or GBT_commit_rename_session to end the session
     */
    
    GB_ERROR error = GB_push_transaction(gb_main);
    if (!error) {
        gbtrst.gb_main         = gb_main;
        gbtrst.gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

        if (!all_flag) { // this is meant to be used for single or few species
            int hash_size = 256;

            gbtrst.renamed_hash     = GBS_create_hash(hash_size, GB_MIND_CASE);
            gbtrst.old_species_hash = 0;
        }
        else {
            int hash_size = GBT_get_species_hash_size(gb_main);

            gbtrst.renamed_hash     = GBS_create_hash(hash_size, GB_MIND_CASE);
            gbtrst.old_species_hash = GBT_create_species_hash(gb_main);
        }
        gbtrst.all_flag = all_flag;
    }
    return error;
}
/* returns errors */
GB_ERROR GBT_rename_species(const char *oldname, const  char *newname, GB_BOOL ignore_protection)
{
    GBDATA   *gb_species;
    GBDATA   *gb_name;
    GB_ERROR  error;

    if (strcmp(oldname, newname) == 0)
        return 0;

#if defined(DEBUG) && 1
    if (isdigit(oldname[0])) {
        printf("oldname='%s' newname='%s'\n", oldname, newname);
    }
#endif

    if (gbtrst.all_flag) {
        gb_assert(gbtrst.old_species_hash);
        gb_species = (GBDATA *)GBS_read_hash(gbtrst.old_species_hash,oldname);
    }
    else {
        GBDATA *gb_found_species;

        gb_assert(gbtrst.old_species_hash == 0);
        gb_found_species = GBT_find_species_rel_species_data(gbtrst.gb_species_data, newname);
        gb_species       = GBT_find_species_rel_species_data(gbtrst.gb_species_data, oldname);

        if (gb_found_species && gb_species != gb_found_species) {
            return GB_export_error("A species named '%s' already exists.",newname);
        }
    }

    if (!gb_species) {
        return GB_export_error("Expected that a species named '%s' exists (maybe there are duplicate species, database might be corrupt)",oldname);
    }

    gb_name = GB_entry(gb_species,"name");
    if (ignore_protection) GB_push_my_security(gbtrst.gb_main);
    error   = GB_write_string(gb_name,newname);
    if (ignore_protection) GB_pop_my_security(gbtrst.gb_main);

    if (!error){
        struct gbt_renamed_struct *rns;
        if (gbtrst.old_species_hash) {
            GBS_write_hash(gbtrst.old_species_hash, oldname, 0);
        }
        rns = (struct gbt_renamed_struct *)GB_calloc(strlen(newname) + sizeof (struct gbt_renamed_struct),sizeof(char));
        strcpy(&rns->data[0],newname);
        GBS_write_hash(gbtrst.renamed_hash,oldname,(long)rns);
    }
    return error;
}

static void gbt_free_rename_session_data(void) {
    if (gbtrst.renamed_hash) {
        GBS_free_hash_free_pointer(gbtrst.renamed_hash);
        gbtrst.renamed_hash = 0;
    }
    if (gbtrst.old_species_hash) {
        GBS_free_hash(gbtrst.old_species_hash);
        gbtrst.old_species_hash = 0;
    }
}

GB_ERROR GBT_abort_rename_session(void) {
    gbt_free_rename_session_data();
    return GB_abort_transaction(gbtrst.gb_main);
}

static const char *currentTreeName = 0;

GB_ERROR gbt_rename_tree_rek(GBT_TREE *tree,int tree_index){
    char buffer[256];
    static int counter = 0;
    if (!tree) return 0;
    if (tree->is_leaf){
        if (tree->name){
            struct gbt_renamed_struct *rns = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash,tree->name);
            if (rns){
                char *newname;
                if (rns->used_by == tree_index){ /* species more than once in the tree */
                    sprintf(buffer,"%s_%i", rns->data, counter++);
                    GB_warning("Species '%s' more than once in '%s', creating zombie '%s'",
                               tree->name, currentTreeName, buffer);
                    newname = buffer;
                }
                else {
                    newname = &rns->data[0];
                }
                freedup(tree->name, newname);
                rns->used_by = tree_index;
            }
        }
    }else{
        gbt_rename_tree_rek(tree->leftson,tree_index);
        gbt_rename_tree_rek(tree->rightson,tree_index);
    }
    return 0;
}

#ifdef __cplusplus
extern "C"
#endif
GB_ERROR GBT_commit_rename_session(int (*show_status)(double gauge), int (*show_status_text)(const char *)){
    GB_ERROR error = 0;

    // rename species in trees
    {
        int tree_count;
        char **tree_names = GBT_get_tree_names_and_count(gbtrst.gb_main, &tree_count);

        if (tree_names) {
            int count;
            gb_assert(tree_count); // otherwise tree_names should be zero

            if (show_status_text) show_status_text(GBS_global_string("Renaming species in %i tree%c", tree_count, "s"[tree_count<2]));
            if (show_status) show_status(0.0);

            for (count = 0; count<tree_count; ++count) {
                char     *tname = tree_names[count];
                GBT_TREE *tree  = GBT_read_tree(gbtrst.gb_main,tname,-sizeof(GBT_TREE));

                if (tree) {
                    currentTreeName = tname; // provide tree name (used for error message)
                    gbt_rename_tree_rek(tree, count+1);
                    currentTreeName = 0;

                    GBT_write_tree(gbtrst.gb_main, 0, tname, tree);
                    GBT_delete_tree(tree);
                }
                if (show_status) show_status((double)(count+1)/tree_count);
            }
            GBT_free_names(tree_names);
        }
    }
    // rename configurations
    if (!error) {
        int config_count;
        char **config_names = GBT_get_configuration_names_and_count(gbtrst.gb_main, &config_count);

        if (config_names) {
            int count;
            gb_assert(config_count); // otherwise config_names should be zero

            if (show_status_text) show_status_text(GBS_global_string("Renaming species in %i config%c", config_count, "s"[config_count<2]));
            if (show_status) show_status(0.0);

            for (count = 0; !error && count<config_count; ++count) {
                GBT_config *config = GBT_load_configuration_data(gbtrst.gb_main, config_names[count], &error);
                if (!error) {
                    int need_save = 0;
                    int mode;

                    for (mode = 0; !error && mode<2; ++mode) {
                        char              **configStrPtr = (mode == 0 ? &config->top_area : &config->middle_area);
                        GBT_config_parser  *parser       = GBT_start_config_parser(*configStrPtr);
                        GBT_config_item    *item         = GBT_create_config_item();
                        void               *strstruct    = GBS_stropen(1000);

                        error = GBT_parse_next_config_item(parser, item);
                        while (!error && item->type != CI_END_OF_CONFIG) {
                            if (item->type == CI_SPECIES) {
                                struct gbt_renamed_struct *rns = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash, item->name);
                                if (rns) { // species was renamed
                                    freedup(item->name, rns->data);
                                    need_save = 1;
                                }
                            }
                            GBT_append_to_config_string(item, strstruct);
                            error = GBT_parse_next_config_item(parser, item);
                        }

                        if (!error) freeset(*configStrPtr, GBS_strclose(strstruct));

                        GBT_free_config_item(item);
                        GBT_free_config_parser(parser);
                    }

                    if (!error && need_save) error = GBT_save_configuration_data(config, gbtrst.gb_main, config_names[count]);
                }
                if (show_status) show_status((double)(count+1)/config_count);
            }
            GBT_free_names(config_names);
        }
    }

    // rename links in pseudo-species
    if (!error && GEN_is_genome_db(gbtrst.gb_main, -1)) {
        GBDATA *gb_pseudo;
        for (gb_pseudo = GEN_first_pseudo_species(gbtrst.gb_main);
             gb_pseudo && !error;
             gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
        {
            GBDATA *gb_origin_organism = GB_entry(gb_pseudo, "ARB_origin_species");
            if (gb_origin_organism) {
                const char                *origin = GB_read_char_pntr(gb_origin_organism);
                struct gbt_renamed_struct *rns    = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash, origin);
                if (rns) {          // species was renamed
                    const char *newname = &rns->data[0];
                    error               = GB_write_string(gb_origin_organism, newname);
                }
            }
        }
    }

    gbt_free_rename_session_data();

    error = GB_pop_transaction(gbtrst.gb_main);
    return error;
}

GBDATA **GBT_gen_species_array(GBDATA *gb_main, long *pspeccnt)
{
    GBDATA *gb_species;
    GBDATA *gb_species_data = GBT_find_or_create(gb_main,"species_data",7);
    GBDATA **result;
    *pspeccnt = 0;
    for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species)){
        (*pspeccnt) ++;
    }
    result = (GBDATA **)malloc((size_t)(sizeof(GBDATA *)* (*pspeccnt)));
    *pspeccnt = 0;
    for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species)){
        result[(*pspeccnt)++]=gb_species;
    }
    return result;
}

/* ---------------------------------------- 
 * conversion between
 *
 * - char ** (heap-allocated array of heap-allocated char*'s)
 * - one string containing several substrings separated by a separator
 *   (e.g. "name1,name2,name3")
 */

#if defined(DEVEL_RALF)
#warning search for code which is splitting strings and use GBT_split_string there
#endif /* DEVEL_RALF */

char **GBT_split_string(const char *namelist, char separator, int *countPtr) {
    // Split 'namelist' into an array of names at 'separator'.
    // Use GBT_free_names() to free it.
    // Sets 'countPtr' to the number of names found

    int         sepCount = 0;
    const char *sep      = namelist;
    while (sep) {
        sep = strchr(sep, separator);
        if (sep) {
            ++sep;
            ++sepCount;
        }
    }

    char **result = malloc((sepCount+2)*sizeof(*result)); // 3 separators -> 4 names (plus terminal NULL)
    int    count  = 0;

    for (; count < sepCount; ++count) {
        sep = strchr(namelist, separator);
        gb_assert(sep);
        
        result[count] = GB_strpartdup(namelist, sep-1);
        namelist      = sep+1;
    }

    result[count++] = strdup(namelist);
    result[count]   = NULL;

    if (countPtr) *countPtr = count;

    return result;
}

char *GBT_join_names(const char *const *names, char separator) {
    struct GBS_strstruct *out = GBS_stropen(1000);

    if (names[0]) {
        GBS_strcat(out, names[0]);
        gb_assert(strchr(names[0], separator) == 0); // otherwise you'll never be able to GBT_split_string
        int n;
        for (n = 1; names[n]; ++n) {
            GBS_chrcat(out, separator);
            GBS_strcat(out, names[n]);
            gb_assert(strchr(names[n], separator) == 0); // otherwise you'll never be able to GBT_split_string
        }
    }

    return GBS_strclose(out);
}

void GBT_free_names(char **names) {
    char **pn;
    for (pn = names; *pn;pn++) free(*pn);
    free((char *)names);
}

/* ---------------------------------------- */
/* read value from database field
 * returns 0 in case of error (use GB_await_error())
 * or when field does not exist
 *
 * otherwise GBT_read_string returns a heap copy
 * other functions return a pointer to a temporary variable (invalidated by next call)
 */

char *GBT_read_string(GBDATA *gb_container, const char *fieldpath){
    GBDATA *gbd;
    char   *result = NULL;
    
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) result = GB_read_string(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

char *GBT_read_as_string(GBDATA *gb_container, const char *fieldpath){
    GBDATA *gbd;
    char   *result = NULL;
    
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) result = GB_read_as_string(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

const char *GBT_read_char_pntr(GBDATA *gb_container, const char *fieldpath){
    GBDATA     *gbd;
    const char *result = NULL;

    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) result = GB_read_char_pntr(gbd);
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL long *GBT_read_int(GBDATA *gb_container, const char *fieldpath) {
    GBDATA *gbd;
    long   *result = NULL;
    
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) {
        static long result_var;
        result_var = GB_read_int(gbd);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL double *GBT_read_float(GBDATA *gb_container, const char *fieldpath) {
    GBDATA *gbd;
    double *result = NULL;
    
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container,fieldpath,GB_FIND);
    if (gbd) {
        static double result_var;
        result_var = GB_read_float(gbd);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

/* -------------------------------------------------------------------------------------- */
/* read value from database field or create field with default_value if missing
 * (same usage as GBT_read_XXX above)
 */

char *GBT_readOrCreate_string(GBDATA *gb_container, const char *fieldpath, const char *default_value) {
    GBDATA *gb_string;
    char   *result = NULL;

    GB_push_transaction(gb_container);
    gb_string             = GB_searchOrCreate_string(gb_container, fieldpath, default_value);
    if (gb_string) result = GB_read_string(gb_string);
    GB_pop_transaction(gb_container);
    return result;
}

const char *GBT_readOrCreate_char_pntr(GBDATA *gb_container, const char *fieldpath, const char *default_value) {
    GBDATA     *gb_string;
    const char *result = NULL;

    GB_push_transaction(gb_container);
    gb_string             = GB_searchOrCreate_string(gb_container, fieldpath, default_value);
    if (gb_string) result = GB_read_char_pntr(gb_string);
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL long *GBT_readOrCreate_int(GBDATA *gb_container, const char *fieldpath, long default_value) {
    GBDATA *gb_int;
    long   *result = NULL;

    GB_push_transaction(gb_container);
    gb_int = GB_searchOrCreate_int(gb_container, fieldpath, default_value);
    if (gb_int) {
        static long result_var;
        result_var = GB_read_int(gb_int);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

NOT4PERL double *GBT_readOrCreate_float(GBDATA *gb_container, const char *fieldpath, double default_value) {
    GBDATA *gb_float;
    double *result = NULL;

    GB_push_transaction(gb_container);
    gb_float = GB_searchOrCreate_float(gb_container, fieldpath, default_value);
    if (gb_float) {
        static double result_var;
        result_var = GB_read_float(gb_float);
        result     = &result_var;
    }
    GB_pop_transaction(gb_container);
    return result;
}

/* ------------------------------------------------------------------- */
/*      overwrite existing or create new database field                */
/*      (field must not exist twice or more - it has to be unique!!)   */

GB_ERROR GBT_write_string(GBDATA *gb_container, const char *fieldpath, const char *content) {
    GBDATA   *gbd;
    GB_ERROR  error;
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container, fieldpath, GB_STRING);
    if (!gbd) error = GB_get_error();
    else {
        error = GB_write_string(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    GB_pop_transaction(gb_container);
    return error;
}

GB_ERROR GBT_write_int(GBDATA *gb_container, const char *fieldpath, long content) {
    GBDATA   *gbd;
    GB_ERROR  error;
    GB_push_transaction(gb_container);
    gbd             = GB_search(gb_container, fieldpath, GB_INT);
    if (!gbd) error = GB_get_error();
    else {
        error = GB_write_int(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    GB_pop_transaction(gb_container);
    return error;
}

GB_ERROR GBT_write_byte(GBDATA *gb_container, const char *fieldpath, unsigned char content) {
    GBDATA   *gbd;
    GB_ERROR  error;
    GB_push_transaction(gb_container);
    gbd             = GB_search(gb_container, fieldpath, GB_BYTE);
    if (!gbd) error = GB_get_error();
    else {
        error = GB_write_byte(gbd, content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    GB_pop_transaction(gb_container);
    return error;
}


GB_ERROR GBT_write_float(GBDATA *gb_container, const char *fieldpath, double content) {
    GBDATA   *gbd;
    GB_ERROR  error;
    GB_push_transaction(gb_container);
    gbd = GB_search(gb_container, fieldpath, GB_FLOAT);
    if (!gbd) error = GB_get_error();
    else {
        error = GB_write_float(gbd,content);
        gb_assert(GB_nextEntry(gbd) == 0); // only one entry should exist (sure you want to use this function?)
    }
    GB_pop_transaction(gb_container);
    return error;
}



GBDATA *GB_test_link_follower(GBDATA *gb_main,GBDATA *gb_link,const char *link){
    GBDATA *linktarget = GB_search(gb_main,"tmp/link/string",GB_STRING);
    GBUSE(gb_link);
    GB_write_string(linktarget,GBS_global_string("Link is '%s'",link));
    return GB_get_father(linktarget);
}

/********************************************************************************************
                    SAVE & LOAD
********************************************************************************************/

/** Open a database, create an index for species and extended names,
    disable saving the database in the PT_SERVER directory */

GBDATA *GBT_open(const char *path,const char *opent,const char *disabled_path)
{
    GBDATA *gbd = GB_open(path,opent);
    GBDATA *species_data;
    GBDATA *extended_data;
    GBDATA *gb_tmp;
    long    hash_size;

    if (!gbd) return gbd;
    if (!disabled_path) disabled_path = "$(ARBHOME)/lib/pts/*";
    GB_disable_path(gbd,disabled_path);
    GB_begin_transaction(gbd);

    if (!strchr(path,':')){
        species_data = GB_search(gbd, "species_data", GB_FIND);
        if (species_data){
            hash_size = GB_number_of_subentries(species_data);
            if (hash_size < GBT_SPECIES_INDEX_SIZE) hash_size = GBT_SPECIES_INDEX_SIZE;
            GB_create_index(species_data,"name",GB_IGNORE_CASE,hash_size);

            extended_data = GBT_get_SAI_data(gbd);
            hash_size = GB_number_of_subentries(extended_data);
            if (hash_size < GBT_SAI_INDEX_SIZE) hash_size = GBT_SAI_INDEX_SIZE;
            GB_create_index(extended_data,"name",GB_IGNORE_CASE,hash_size);
        }
    }
    gb_tmp = GB_search(gbd,"tmp",GB_CREATE_CONTAINER);
    GB_set_temporary(gb_tmp);
    {               /* install link followers */
        GB_MAIN_TYPE *Main = GB_MAIN(gbd);
        Main->table_hash = GBS_create_hash(256, GB_MIND_CASE);
        GB_install_link_follower(gbd,"REF",GB_test_link_follower);
    }
    GBT_install_table_link_follower(gbd);
    GB_commit_transaction(gbd);
    return gbd;
}

/* --------------------------------------------------------------------------------
 * Remote commands
 *
 * Note: These commands may seem to be unused from inside ARB.
 * They get used, but only indirectly via the macro-function.
 *
 * Search for
 * - BIO::remote_action         (use of GBT_remote_action)
 * - BIO::remote_awar           (use of GBT_remote_awar)
 * - BIO::remote_read_awar      (use of GBT_remote_read_awar - seems unused)
 * - BIO::remote_touch_awar     (use of GBT_remote_touch_awar - seems unused)
 */


#define AWAR_REMOTE_BASE_TPL            "tmp/remote/%s/"
#define MAX_REMOTE_APPLICATION_NAME_LEN 30
#define MAX_REMOTE_AWAR_STRING_LEN      (11+MAX_REMOTE_APPLICATION_NAME_LEN+1+6+1)

struct gbt_remote_awars {
    char awar_action[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_result[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_awar[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_value[MAX_REMOTE_AWAR_STRING_LEN];
};

static void gbt_build_remote_awars(struct gbt_remote_awars *awars, const char *application) {
    int length;

    gb_assert(strlen(application) <= MAX_REMOTE_APPLICATION_NAME_LEN);

    length = sprintf(awars->awar_action, AWAR_REMOTE_BASE_TPL, application);
    gb_assert(length < (MAX_REMOTE_AWAR_STRING_LEN-6)); // Note :  6 is length of longest name appended below !

    strcpy(awars->awar_result, awars->awar_action);
    strcpy(awars->awar_awar, awars->awar_action);
    strcpy(awars->awar_value, awars->awar_action);

    strcpy(awars->awar_action+length, "action");
    strcpy(awars->awar_result+length, "result");
    strcpy(awars->awar_awar+length,   "awar");
    strcpy(awars->awar_value+length,  "value");
}

static GBDATA *gbt_remote_search_awar(GBDATA *gb_main, const char *awar_name) {
    GBDATA *gb_action;
    while (1) {
        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main, awar_name, GB_FIND);
        GB_commit_transaction(gb_main);
        if (gb_action) break;
        GB_usleep(2000);
    }
    return gb_action;
}

static GB_ERROR gbt_wait_for_remote_action(GBDATA *gb_main, GBDATA *gb_action, const char *awar_read) {
    GB_ERROR error = 0;

    /* wait to end of action */

    while (!error) {
        GB_usleep(2000);
        error = GB_begin_transaction(gb_main);
        if (!error) {
            char *ac = GB_read_string(gb_action);
            if (ac[0] == 0) { // action has been cleared from remote side
                GBDATA *gb_result = GB_search(gb_main, awar_read, GB_STRING);
                error             = GB_read_char_pntr(gb_result); // check for errors
            }
            free(ac);
        }
        error = GB_end_transaction(gb_main, error);
    }

    return error; // may be error or result
}

GB_ERROR GBT_remote_action(GBDATA *gb_main, const char *application, const char *action_name){
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_action;
    GB_ERROR                 error = NULL;

    gbt_build_remote_awars(&awars, application);
    gb_action = gbt_remote_search_awar(gb_main, awars.awar_action);

    error             = GB_begin_transaction(gb_main);
    if (!error) error = GB_write_string(gb_action, action_name); /* write command */
    error             = GB_end_transaction(gb_main, error);

    if (!error) error = gbt_wait_for_remote_action(gb_main, gb_action, awars.awar_result);
    return error;
}

GB_ERROR GBT_remote_awar(GBDATA *gb_main, const char *application, const char *awar_name, const char *value) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;
    GB_ERROR                 error = NULL;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    error = GB_begin_transaction(gb_main);
    if (!error) error = GB_write_string(gb_awar, awar_name);
    if (!error) error = GBT_write_string(gb_main, awars.awar_value, value);
    error = GB_end_transaction(gb_main, error);

    if (!error) error = gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_result);
    
    return error;
}

GB_ERROR GBT_remote_read_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;
    GB_ERROR                 error = NULL;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    error             = GB_begin_transaction(gb_main);
    if (!error) error = GB_write_string(gb_awar, awar_name);
    if (!error) error = GBT_write_string(gb_main, awars.awar_action, "AWAR_REMOTE_READ");
    error             = GB_end_transaction(gb_main, error);

    if (!error) error = gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_value);
    return error;
}

const char *GBT_remote_touch_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;
    GB_ERROR                 error = NULL;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    error             = GB_begin_transaction(gb_main);
    if (!error) error = GB_write_string(gb_awar, awar_name);
    if (!error) error = GBT_write_string(gb_main, awars.awar_action, "AWAR_REMOTE_TOUCH");
    error             = GB_end_transaction(gb_main, error);

    if (!error) error = gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_result);
    return error;
}

static const char *gb_cache_genome(GBDATA *gb_genome) {
    static GBDATA *gb_last_genome = 0;
    static char   *last_genome    = 0;

    if (gb_genome != gb_last_genome) {
        free(last_genome);

        last_genome    = GB_read_string(gb_genome);
        gb_last_genome = gb_genome;
    }

    return last_genome;
}

struct gene_part_pos {
    int            parts;       // initialized for parts
    unsigned char *certain;     // contains parts "=" chars
    char           offset[256];
};

static struct gene_part_pos *gpp = 0;

static void init_gpp(int parts) {
    if (!gpp) {
        int i;
        gpp          = malloc(sizeof(*gpp));
        gpp->certain = 0;

        for (i = 0; i<256; ++i) gpp->offset[i] = 0;

        gpp->offset[(int)'+'] = 1;
        gpp->offset[(int)'-'] = -1;
    }
    else {
        if (parts>gpp->parts) freeset(gpp->certain, 0);
    }

    if (!gpp->certain) {
        int forParts           = parts+10;
        gpp->certain           = malloc(forParts+1);
        memset(gpp->certain, '=', forParts);
        gpp->certain[forParts] = 0;
        gpp->parts             = forParts;
    }
}

static void getPartPositions(const struct GEN_position *pos, int part, size_t *startPos, size_t *stopPos) {
    // returns 'startPos' and 'stopPos' of one part of a gene
    gb_assert(part<pos->parts);
    *startPos = pos->start_pos[part]+gpp->offset[(pos->start_uncertain ? pos->start_uncertain : gpp->certain)[part]];
    *stopPos  = pos->stop_pos [part]+gpp->offset[(pos->stop_uncertain  ? pos->stop_uncertain  : gpp->certain)[part]];
}

NOT4PERL char *GBT_read_gene_sequence_and_length(GBDATA *gb_gene, GB_BOOL use_revComplement, char partSeparator, size_t *gene_length) {
    // return the sequence data of a gene
    // 
    // if use_revComplement is true -> use data from complementary strand (if complement is set for gene)
    //                    otherwise -> use data from primary strand (sort+merge parts by position)
    // 
    // if partSeparator not is 0 -> insert partSeparator between single (non-merged) parts
    // 
    // returns sequence as result (and length of sequence if 'gene_length' points to something)
    // 
    // if 'pos_certain' contains '+', start behind position (or end at position)
    //                           '-', start at position (or end before position)
    // 
    // For zero-length genes (e.g. "711^712") this function returns an empty string.

    GB_ERROR             error       = 0;
    char                *result      = 0;
    GBDATA              *gb_species  = GB_get_father(GB_get_father(gb_gene));
    struct GEN_position *pos         = GEN_read_position(gb_gene);

    if (!pos) {
        error = GB_get_error();
    }
    else {
        GBDATA        *gb_seq        = GBT_read_sequence(gb_species, "ali_genom");
        unsigned long  seq_length    = GB_read_count(gb_seq);
        int            p;
        int            parts         = pos->parts;
        int            resultlen     = 0;
        int            separatorSize = partSeparator ? 1 : 0;

        init_gpp(parts);

        // test positions and calculate overall result length
        for (p = 0; p<parts && !error; p++) {
            size_t start; // = pos->start_pos[p];
            size_t stop; // = pos->stop_pos[p];
            getPartPositions(pos, p, &start, &stop);

            if (start<1 || start>(stop+1) || stop > seq_length) { // do not reject zero-length genes (start == stop+1)
                error = GBS_global_string("Illegal gene position(s): start=%u, end=%u, seq.length=%li",
                                          start, stop, seq_length);
            }
            else {
                resultlen += stop-start+1;
            }
        }

        if (separatorSize) resultlen += (parts-1)*separatorSize;

        if (!error) {
            char T_or_U = 0;
            if (use_revComplement) {
                error = GBT_determine_T_or_U(GB_AT_DNA, &T_or_U, "reverse-complement");
            }
            else if (parts>1) {
                GEN_sortAndMergeLocationParts(pos);
                parts = pos->parts; // may have changed
            }

            if (!error) {
                const char *seq_data = gb_cache_genome(gb_seq);
                char       *resultpos;

                result    = malloc(resultlen+1);
                resultpos = result;

                if (gene_length) *gene_length = resultlen;

                for (p = 0; p<parts; ++p) {
                    size_t start; // = pos->start_pos[p];
                    size_t stop;  // = pos->stop_pos[p];

                    getPartPositions(pos, p, &start, &stop);

                    int len = stop-start+1;

                    if (separatorSize && p>0) *resultpos++ = partSeparator;

                    memcpy(resultpos, seq_data+start-1, len);
                    if (T_or_U && pos->complement[p]) {
                        GBT_reverseComplementNucSequence(resultpos, len, T_or_U);
                    }
                    resultpos += len;
                }

                resultpos[0] = 0;
            }
        }
        GEN_free_position(pos);
    }

    gb_assert(result || error);

    if (error) {
        char *id = GEN_global_gene_identifier(gb_gene, gb_species);
        error    = GB_export_error("Can't read sequence of '%s' (Reason: %s)", id, error);
        free(id);
        free(result);
        result   = 0;
    }

    return result;
}

char *GBT_read_gene_sequence(GBDATA *gb_gene, GB_BOOL use_revComplement, char partSeparator) {
    return GBT_read_gene_sequence_and_length(gb_gene, use_revComplement, partSeparator, 0);
}

/* --------------------------- */
/*      self-notification      */
/* --------------------------- */
/* provides a mechanism to notify ARB after some external tool finishes */

#define ARB_NOTIFICATIONS "tmp/notify"

/* DB structure for notifications : 
 *
 * ARB_NOTIFICATIONS/counter        GB_INT      counts used ids
 * ARB_NOTIFICATIONS/notify/id      GB_INT      id of notification
 * ARB_NOTIFICATIONS/notify/message GB_STRING   message of notification (set by callback)
 */

typedef void (*notify_cb_type)(const char *message, void *client_data);

struct NCB {
    notify_cb_type  cb;
    void           *client_data;
};

static void notify_cb(GBDATA *gb_message, int *cb_info, GB_CB_TYPE cb_type) {
    GB_ERROR error   = GB_remove_callback(gb_message, GB_CB_CHANGED|GB_CB_DELETE, notify_cb, cb_info);
    int      cb_done = 0;

    struct NCB *pending = (struct NCB*)cb_info;

    if (cb_type == GB_CB_CHANGED) {
        if (!error) {
            const char *message = GB_read_char_pntr(gb_message);
            if (message) {
                pending->cb(message, pending->client_data);
                cb_done = 1;
            }
        }

        if (!cb_done) {
            if (!error) error = GB_get_error();
            fprintf(stderr, "Notification failed (Reason: %s)\n", error);
            gb_assert(0);
        }
    }
    else { /* called from GB_remove_last_notification */
        gb_assert(cb_type == GB_CB_DELETE);
    }

    free(pending);
}

static int allocateNotificationID(GBDATA *gb_main, int *cb_info) {
    /* returns a unique notification ID
     * or 0 (use GB_get_error() in this case)
     */
    
    int      id    = 0;
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_CREATE_CONTAINER);
        
        if (gb_notify) {
            GBDATA *gb_counter = GB_searchOrCreate_int(gb_notify, "counter", 0);

            if (gb_counter) {
                int newid = GB_read_int(gb_counter) + 1; /* increment counter */
                error     = GB_write_int(gb_counter, newid);

                if (!error) {
                    /* change transaction (to never use id twice!) */
                    error             = GB_pop_transaction(gb_main);
                    if (!error) error = GB_push_transaction(gb_main);

                    if (!error) {
                        GBDATA *gb_notification = GB_create_container(gb_notify, "notify");
                        
                        if (gb_notification) {
                            error = GBT_write_int(gb_notification, "id", newid);
                            if (!error) {
                                GBDATA *gb_message = GB_searchOrCreate_string(gb_notification, "message", "");
                                
                                if (gb_message) {
                                    error = GB_add_callback(gb_message, GB_CB_CHANGED|GB_CB_DELETE, notify_cb, cb_info);
                                    if (!error) {
                                        id = newid; /* success */
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (!id) {
        if (!error) error = GB_await_error();
        error = GBS_global_string("Failed to allocate notification ID (%s)", error);
    }
    error = GB_end_transaction(gb_main, error);
    if (error) GB_export_error(error);

    return id;
}


char *GB_generate_notification(GBDATA *gb_main,
                               void (*cb)(const char *message, void *client_data),
                               const char *message, void *client_data)
{
    /* generates a call to 'arb_notify', meant to be inserted into some external system call.
     * When that call is executed, the callback instanciated here will be called.
     *
     * Tip : To return variable results from the shell skript, use the name of an environment
     *       variable in 'message' (e.g. "$RESULT")
     */

    int         id;
    char       *arb_notify_call = 0;
    struct NCB *pending         = malloc(sizeof(*pending));

    pending->cb          = cb;
    pending->client_data = client_data;

    id = allocateNotificationID(gb_main, (int*)pending);
    if (id) {
        arb_notify_call = GBS_global_string_copy("arb_notify %i \"%s\"", id, message);
    }
    else {
        free(pending);
    }

    return arb_notify_call;
}

GB_ERROR GB_remove_last_notification(GBDATA *gb_main) {
    /* aborts the last notification */
    GB_ERROR error = GB_push_transaction(gb_main);

    if (!error) {
        GBDATA *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_CREATE_CONTAINER);
        if (gb_notify) {
            GBDATA *gb_counter = GB_entry(gb_notify, "counter");
            if (gb_counter) {
                int     id    = GB_read_int(gb_counter);
                GBDATA *gb_id = GB_find_int(gb_notify, "id", id, down_2_level);

                if (!gb_id) {
                    error = GBS_global_string("No notification for ID %i", id);
                    gb_assert(0);           // GB_generate_notification() has not been called for 'id'!
                }
                else {
                    GBDATA *gb_message = GB_brother(gb_id, "message");

                    if (!gb_message) {
                        error = "Missing 'message' entry";
                    }
                    else {
                        error = GB_delete(gb_message); /* calls notify_cb */
                    }
                }
            }
            else {
                error = "No notification generated yet";
            }
        }
    }

    error = GB_end_transaction(gb_main, error);
    return error;
}

GB_ERROR GB_notify(GBDATA *gb_main, int id, const char *message) {
    /* called via 'arb_notify'
     * 'id' has to be generated by GB_generate_notification()
     * 'message' is passed to notification callback belonging to id
     */

    GB_ERROR  error     = 0;
    GBDATA   *gb_notify = GB_search(gb_main, ARB_NOTIFICATIONS, GB_FIND);

    if (!gb_notify) {
        error = "Missing notification data";
        gb_assert(0);           // GB_generate_notification() has not been called!
    }
    else {
        GBDATA *gb_id = GB_find_int(gb_notify, "id", id, down_2_level);

        if (!gb_id) {
            error = GBS_global_string("No notification for ID %i", id);
        }
        else {
            GBDATA *gb_message = GB_brother(gb_id, "message");

            if (!gb_message) {
                error = "Missing 'message' entry";
            }
            else {
                /* callback the instanciating DB client */
                error = GB_write_string(gb_message, message);
            }
        }
    }

    return error;
}


