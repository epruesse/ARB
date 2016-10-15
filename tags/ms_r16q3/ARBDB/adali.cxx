// =============================================================== //
//                                                                 //
//   File      : adali.cxx                                         //
//   Purpose   : alignments                                        //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <adGene.h>
#include <arb_strarray.h>

#include "gb_local.h"

static long check_for_species_without_data(const char *species_name, long value, void *counterPtr) {
    if (value == 1) {
        long cnt = *((long*)counterPtr);
        if (cnt<40) {
            GB_warningf("Species '%s' has no data in any alignment", species_name);
        }
        *((long*)counterPtr) = cnt+1;
    }
    return value; // new hash value
}

GBDATA *GBT_get_presets(GBDATA *gb_main) {
    return GBT_find_or_create(gb_main, "presets", 7);
}

int GBT_count_alignments(GBDATA *gb_main) {
    int     count      = 0;
    GBDATA *gb_presets = GBT_get_presets(gb_main);
    for (GBDATA *gb_ali = GB_entry(gb_presets, "alignment");
         gb_ali;
         gb_ali = GB_nextEntry(gb_ali))
    {
        ++count;
    }
    return count;
}

static GB_ERROR GBT_check_alignment(GBDATA *gb_main, GBDATA *preset_alignment, GB_HASH *species_name_hash) {
    /* check
     * - whether alignment has correct size and
     * - whether all data is present.
     *
     * Sets the security deletes and writes.
     *
     * If 'species_name_hash' is not NULL,
     * - it initially has to contain value == 1 for each existing species.
     * - afterwards it will contain value  == 2 for each species where an alignment has been found.
     */

    GBDATA *gb_species_data  = GBT_get_species_data(gb_main);
    GBDATA *gb_extended_data = GBT_get_SAI_data(gb_main);

    GB_ERROR  error      = 0;
    char     *ali_name   = GBT_read_string(preset_alignment, "alignment_name");
    if (!ali_name) error = "Alignment w/o 'alignment_name'";

    if (!error) {
        long    security_write = -1;
        long    stored_ali_len = -1;
        long    found_ali_len  = -1;
        long    aligned        = 1;
        GBDATA *gb_ali_len     = 0;

        {
            GBDATA *gb_ali_wsec = GB_entry(preset_alignment, "alignment_write_security");
            if (!gb_ali_wsec) {
                error = "has no 'alignment_write_security' entry";
            }
            else {
                security_write = GB_read_int(gb_ali_wsec);
            }
        }


        if (!error) {
            gb_ali_len = GB_entry(preset_alignment, "alignment_len");
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
                GBDATA     *gb_name        = GB_entry(gb_species, "name");
                const char *name           = 0;
                int         alignment_seen = 0;

                if (!gb_name) {
                    // fatal: name is missing -> create a unique name
                    char *unique = GBT_create_unique_species_name(gb_main, "autoname.");
                    error        = GBT_write_string(gb_species, "name", unique);

                    if (!error) {
                        gb_name = GB_entry(gb_species, "name");
                        GBS_write_hash(species_name_hash, unique, 1); // not seen before
                        GB_warningf("Seen unnamed species (gave name '%s')", unique);
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

                if (!error) {
                    GB_push_my_security(gb_name);

                    error             = GB_write_security_delete(gb_name, 7);
                    if (!error) error = GB_write_security_write(gb_name, 6);

                    if (!error) {
                        GBDATA *gb_ali = GB_entry(gb_species, ali_name);
                        if (gb_ali) {
                            GBDATA *gb_data = GB_entry(gb_ali, "data");
                            if (!gb_data) {
                                error = GBT_write_string(gb_ali, "data", "Error: entry 'data' was missing and therefore was filled with this text.");
                                GB_warningf("No '%s/data' entry for species '%s' (has been filled with dummy data)", ali_name, name);
                            }
                            else {
                                if (GB_read_type(gb_data) != GB_STRING) {
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

                                    error = GB_write_security_delete(gb_data, 7);

                                    if (!alignment_seen && species_name_hash) { // mark as seen
                                        GBS_write_hash(species_name_hash, name, 2); // 2 means "species has data in at least 1 alignment"
                                    }
                                }
                            }
                        }
                    }

                    if (!error) error = GB_write_security_delete(gb_species, security_write);

                    GB_pop_my_security(gb_name);
                }
            }
        }

        if (!error) {
            GBDATA *gb_sai;
            for (gb_sai = GBT_first_SAI_rel_SAI_data(gb_extended_data);
                 gb_sai && !error;
                 gb_sai = GBT_next_SAI(gb_sai))
            {
                GBDATA *gb_sai_name = GB_entry(gb_sai, "name");
                GBDATA *gb_ali;

                if (!gb_sai_name) continue;

                GB_write_security_delete(gb_sai_name, 7);

                gb_ali = GB_entry(gb_sai, ali_name);
                if (gb_ali) {
                    GBDATA *gb_sai_data;
                    for (gb_sai_data = GB_child(gb_ali);
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
            error = GBS_global_string("Error checking alignment '%s':\n%s\n",  ali_name, error);
        }
    }

    free(ali_name);

    return error;
}

GB_ERROR GBT_check_data(GBDATA *Main, const char *alignment_name) {
    /* alignment_name
     *  == 0     -> check all existing alignments
     * otherwise -> check only one alignment
     */

    GB_ERROR  error             = 0;
    GBDATA   *gb_sd             = GBT_get_species_data(Main);
    GBDATA   *gb_presets        = GBT_get_presets(Main);
    GB_HASH  *species_name_hash = 0;

    // create rest of main containers
    GBT_get_SAI_data(Main);
    GBT_get_tree_data(Main);

    if (alignment_name) {
        GBDATA *gb_ali_name = GB_find_string(gb_presets, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
        if (!gb_ali_name) {
            error = GBS_global_string("Alignment '%s' does not exist - it can't be checked.", alignment_name);
        }
    }

    if (!error) {
        // check whether we have an default alignment
        GBDATA *gb_use = GB_entry(gb_presets, "use");
        if (!gb_use) {
            // if we have no default alignment -> look for any alignment
            GBDATA *gb_ali_name = GB_find_string(gb_presets, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

            error = gb_ali_name ?
                GBT_write_string(gb_presets, "use", GB_read_char_pntr(gb_ali_name)) :
                "No alignment defined";
        }
    }

    if (!alignment_name && !error) {
        // if all alignments are checked -> use species_name_hash to detect duplicated species and species w/o data
        long    duplicates = 0;
        species_name_hash  = GBS_create_hash(GBT_get_species_count(Main), GB_IGNORE_CASE);

        if (!error) {
            for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_sd);
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
                                      "   'Search For Equal Fields and Mark Duplicates'\n"
                                      "in ARB_NTREE search tool, save DB and restart ARB."
                                      , duplicates);
        }
    }

    if (!error) {
        for (GBDATA *gb_ali = GB_entry(gb_presets, "alignment");
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
                GB_warningf("Found %li species without alignment data (only some were listed)", counter);
            }
        }

        GBS_free_hash(species_name_hash);
    }

    return error;
}

void GBT_get_alignment_names(ConstStrArray& names, GBDATA *gbd) {
    /* Get names of existing alignments from database.
     *
     * Returns: array of strings, the last element is NULL
     * (Note: use GBT_free_names() to free result)
     */

    GBDATA *presets = GBT_get_presets(gbd);
    for (GBDATA *ali = GB_entry(presets, "alignment"); ali; ali = GB_nextEntry(ali)) {
        GBDATA *name = GB_entry(ali, "alignment_name");
        names.put(name ? GB_read_char_pntr(name) : "<unnamed alignment>");
    }
}

static char *gbt_nonexisting_alignment(GBDATA *gbMain) {
    char  *ali_other = 0;
    int    counter;

    for (counter = 1; !ali_other; ++counter) {
        ali_other = GBS_global_string_copy("ali_x%i", counter);
        if (GBT_get_alignment(gbMain, ali_other) != 0) freenull(ali_other); // exists -> continue
    }

    return ali_other;
}

GB_ERROR GBT_check_alignment_name(const char *alignment_name)
{
    GB_ERROR error;
    if ((error = GB_check_key(alignment_name))) return error;
    if (strncmp(alignment_name, "ali_", 4)) {
        return GB_export_errorf("your alignment_name '%s' must start with 'ali_'",
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
    /* create alignment
     *
     * returns pointer to alignment or
     * NULL (in this case an error has been exported)
     */
    GB_ERROR  error      = NULL;
    GBDATA   *gb_presets = GBT_get_presets(gbd);
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
            GBDATA *gb_name = GB_find_string(gb_presets, "alignment_name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

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
        gb_assert(error);
        GB_export_errorf("in GBT_create_alignment: %s", error);
    }
#if defined(DEBUG)
    else gb_assert(!error);
#endif // DEBUG

    return result;
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
                gb_new             = GB_create_container(gb_item, dest);
                if (!gb_new) error = GB_await_error();
                else error         = GB_copy(gb_new, gb_ali);
            }
        }
        if (dele) error = GB_delete(gb_ali);
    }

    if (error && gb_item) {
        error = GBS_global_string("%s\n(while renaming alignment for %s '%s')", error, item_name, GBT_read_name(gb_item));
    }

    return error;
}

GB_ERROR GBT_rename_alignment(GBDATA *gbMain, const char *source, const char *dest, int copy, int dele)
{
    /* if copy == 1 then create a copy
     * if dele == 1 then delete old
     */

    GB_ERROR  error            = 0;
    int       is_case_error    = 0;
    GBDATA   *gb_presets       = GBT_get_presets(gbMain);
    GBDATA   *gb_species_data  = GBT_get_species_data(gbMain);
    GBDATA   *gb_extended_data = GBT_get_SAI_data(gbMain);

    if (!gb_presets || !gb_species_data || !gb_extended_data) error = GB_await_error();

    // create copy and/or delete old alignment description
    if (!error) {
        GBDATA *gb_old_alignment = GBT_get_alignment(gbMain, source);

        if (!gb_old_alignment) {
            error = GB_await_error();
        }
        else {
            if (copy) {
                GBDATA *gbh = GBT_get_alignment(gbMain, dest);
                if (gbh) {
                    error         = GBS_global_string("destination alignment '%s' already exists", dest);
                    is_case_error = (strcasecmp(source, dest) == 0); // test for case-only difference
                }
                else {
                    GB_clear_error();
                    error = GBT_check_alignment_name(dest);
                    if (!error) {
                        GBDATA *gb_new_alignment = GB_create_container(gb_presets, "alignment");
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

    // change default alignment
    if (!error && dele && copy) {
        error = GBT_write_string(gb_presets, "use", dest);
    }

    // copy and/or delete alignment entries in species
    if (!error) {
        error = gbt_rename_alignment_of_item(gb_species_data, "Species", "species", source, dest, copy, dele);
    }

    // copy and/or delete alignment entries in SAIs
    if (!error) {
        error = gbt_rename_alignment_of_item(gb_extended_data, "SAI", "extended", source, dest, copy, dele);
    }

    if (is_case_error) {
        // alignments source and dest only differ in case
        char *ali_other = gbt_nonexisting_alignment(gbMain);
        gb_assert(copy);

        printf("Renaming alignment '%s' -> '%s' -> '%s' (to avoid case-problem)\n", source, ali_other, dest);

        error             = GBT_rename_alignment(gbMain, source, ali_other, 1, dele);
        if (!error) error = GBT_rename_alignment(gbMain, ali_other, dest, 1, 1);

        free(ali_other);
    }

    return error;
}

// -----------------------------------------
//      alignment related item functions

NOT4PERL GBDATA *GBT_add_data(GBDATA *species, const char *ali_name, const char *key, GB_TYPES type) {
    // goes to header: __ATTR__DEPRECATED_TODO("better use GBT_create_sequence_data()")

    /* replace this function by GBT_create_sequence_data
     * the same as GB_search(species, 'ali_name/key', GB_CREATE)
     *
     * Note: The behavior is weird, cause it does sth special for GB_STRING (write default content "...")
     *
     * returns create database entry (or NULL; exports an error in this case)
     */

    GB_ERROR error = GB_check_key(ali_name);
    if (error) {
        error = GBS_global_string("Invalid alignment name '%s' (Reason: %s)", ali_name, error);
    }
    else {
        error = GB_check_hkey(key);
        if (error) {
            error = GBS_global_string("Invalid field name '%s' (Reason: %s)", key, error);
        }
    }

    GBDATA *gb_data = NULL;
    if (error) {
        GB_export_error(error);
    }
    else {
        GBDATA *gb_gb     = GB_entry(species, ali_name);
        if (!gb_gb) gb_gb = GB_create_container(species, ali_name);

        if (gb_gb) {
            if (type == GB_STRING) {
                gb_data = GB_search(gb_gb, key, GB_FIND);
                if (!gb_data) {
                    gb_data = GB_search(gb_gb, key, GB_STRING);
                    GB_write_string(gb_data, "...");
                }
            }
            else {
                gb_data = GB_search(gb_gb, key, type);
            }
        }
    }
    return gb_data;
}

NOT4PERL GBDATA *GBT_create_sequence_data(GBDATA *species, const char *ali_name, const char *key, GB_TYPES type, int security_write) {
    GBDATA *gb_data = GBT_add_data(species, ali_name, key, type);
    if (gb_data) {
        GB_ERROR error = GB_write_security_write(gb_data, security_write);
        if (error) {
            GB_export_error(error);
            gb_data = 0;
        }
    }
    return gb_data;
}

GBDATA *GBT_gen_accession_number(GBDATA *gb_species, const char *ali_name) {
    GBDATA *gb_acc = GB_entry(gb_species, "acc");
    if (!gb_acc) {
        GBDATA *gb_data = GBT_find_sequence(gb_species, ali_name);
        if (gb_data) {                                     // found a valid alignment
            GB_CSTR     sequence = GB_read_char_pntr(gb_data);
            long        id       = GBS_checksum(sequence, 1, ".-");
            const char *acc      = GBS_global_string("ARB_%lX", id);
            GB_ERROR    error    = GBT_write_string(gb_species, "acc", acc);

            if (error) GB_export_error(error);
        }
    }
    return gb_acc;
}


int GBT_is_partial(GBDATA *gb_species, int default_value, bool define_if_undef) {
    // checks whether a species has a partial or full sequence
    //
    // Note: partial sequences should not be used for tree calculations
    //
    // returns: 0 if sequence is full
    //          1 if sequence is partial
    //          -1 in case of error (which is exported in this case)
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

GBDATA *GBT_find_sequence(GBDATA *gb_species, const char *aliname) {
    GBDATA *gb_ali = GB_entry(gb_species, aliname);
    return gb_ali ? GB_entry(gb_ali, "data") : 0;
}

char *GBT_get_default_alignment(GBDATA *gb_main) {
    gb_assert(!GB_have_error()); // illegal to enter this function when an error is exported!
    return GBT_read_string(gb_main, GB_DEFAULT_ALIGNMENT);
}

GB_ERROR GBT_set_default_alignment(GBDATA *gb_main, const char *alignment_name) {
    return GBT_write_string(gb_main, GB_DEFAULT_ALIGNMENT, alignment_name);
}

GBDATA *GBT_get_alignment(GBDATA *gb_main, const char *aliname) {
    /*! @return global alignment container for alignment 'aliname' or
     * NULL if alignment not found (error exported in that case)
     */
    if (!aliname) {
        GB_export_error("no alignment given");
        return NULL;
    }

    GBDATA *gb_presets        = GBT_get_presets(gb_main);
    GBDATA *gb_alignment_name = GB_find_string(gb_presets, "alignment_name", aliname, GB_IGNORE_CASE, SEARCH_GRANDCHILD);

    if (!gb_alignment_name) {
        GB_export_errorf("alignment '%s' not found", aliname);
        return NULL;
    }
    return GB_get_father(gb_alignment_name);
}

#if defined(WARN_TODO)
#warning recode and change result type to long* ?
#endif
long GBT_get_alignment_len(GBDATA *gb_main, const char *aliname) {
    /*! @return length of alignment 'aliname' or
     * -1 if alignment not found (error exported in that case)
     */
    GBDATA *gb_alignment = GBT_get_alignment(gb_main, aliname);
    return gb_alignment ? *GBT_read_int(gb_alignment, "alignment_len") : -1;
}

GB_ERROR GBT_set_alignment_len(GBDATA *gb_main, const char *aliname, long new_len) {
    GB_ERROR  error        = 0;
    GBDATA   *gb_alignment = GBT_get_alignment(gb_main, aliname);

    if (gb_alignment) {
        GB_push_my_security(gb_main);
        error             = GBT_write_int(gb_alignment, "alignment_len", new_len); // write new len
        if (!error) error = GBT_write_int(gb_alignment, "aligned", 0);             // mark as unaligned
        GB_pop_my_security(gb_main);
    }
    else error = GB_export_errorf("Alignment '%s' not found", aliname);
    return error;
}

char *GBT_get_alignment_type_string(GBDATA *gb_main, const char *aliname) {
    /*! @return type-string of alignment 'aliname' or
     * NULL if alignment not found (error exported in that case)
     */
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
        switch (ali_type[0]) {
            case 'r': if (strcmp(ali_type, "rna")==0) at = GB_AT_RNA; break;
            case 'd': if (strcmp(ali_type, "dna")==0) at = GB_AT_DNA; break;
            case 'a': if (strcmp(ali_type, "ami")==0) at = GB_AT_AA; break;
            case 'p': if (strcmp(ali_type, "pro")==0) at = GB_AT_AA; break;
            default: gb_assert(0); break;
        }
        free(ali_type);
    }
    return at;
}

bool GBT_is_alignment_protein(GBDATA *gb_main, const char *alignment_name) {
    return GBT_get_alignment_type(gb_main, alignment_name) == GB_AT_AA;
}

// -----------------------
//      gene sequence

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

static gene_part_pos *gpp = 0;

static void init_gpp(int parts) {
    if (!gpp) {
        int i;
        ARB_alloc(gpp, 1);
        gpp->certain = 0;

        for (i = 0; i<256; ++i) gpp->offset[i] = 0;

        gpp->offset[(int)'+'] = 1;
        gpp->offset[(int)'-'] = -1;
    }
    else {
        if (parts>gpp->parts) freenull(gpp->certain);
    }

    if (!gpp->certain) {
        int forParts           = parts+10;
        ARB_alloc(gpp->certain, forParts+1);
        memset(gpp->certain, '=', forParts);
        gpp->certain[forParts] = 0;
        gpp->parts             = forParts;
    }
}

static void getPartPositions(const GEN_position *pos, int part, size_t *startPos, size_t *stopPos) {
    // returns 'startPos' and 'stopPos' of one part of a gene
    gb_assert(part<pos->parts);
    *startPos = pos->start_pos[part]+gpp->offset[(pos->start_uncertain ? pos->start_uncertain : gpp->certain)[part]];
    *stopPos  = pos->stop_pos [part]+gpp->offset[(pos->stop_uncertain  ? pos->stop_uncertain  : gpp->certain)[part]];
}

NOT4PERL char *GBT_read_gene_sequence_and_length(GBDATA *gb_gene, bool use_revComplement, char partSeparator, size_t *gene_length) {
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

    GB_ERROR      error      = 0;
    char         *result     = 0;
    GBDATA       *gb_species = GB_get_grandfather(gb_gene);
    GEN_position *pos        = GEN_read_position(gb_gene);

    if (!pos) error = GB_await_error();
    else {
        GBDATA        *gb_seq        = GBT_find_sequence(gb_species, "ali_genom");
        unsigned long  seq_length    = GB_read_count(gb_seq);
        int            p;
        int            parts         = pos->parts;
        int            resultlen     = 0;
        int            separatorSize = partSeparator ? 1 : 0;

        init_gpp(parts);

        // test positions and calculate overall result length
        for (p = 0; p<parts && !error; p++) {
            size_t start;
            size_t stop;
            getPartPositions(pos, p, &start, &stop);

            if (start<1 || start>(stop+1) || stop > seq_length) { // do not reject zero-length genes (start == stop+1)
                error = GBS_global_string("Illegal gene position(s): start=%zu, end=%zu, seq.length=%li",
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

                ARB_alloc(result, resultlen+1);
                resultpos = result;

                if (gene_length) *gene_length = resultlen;

                for (p = 0; p<parts; ++p) {
                    size_t start;
                    size_t stop;
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
        error    = GB_export_errorf("Can't read sequence of '%s' (Reason: %s)", id, error);
        free(id);
        free(result);
        result   = 0;
    }

    return result;
}

char *GBT_read_gene_sequence(GBDATA *gb_gene, bool use_revComplement, char partSeparator) {
    return GBT_read_gene_sequence_and_length(gb_gene, use_revComplement, partSeparator, 0);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

void TEST_alignment() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("TEST_prot.arb", "r");

    {
        GB_transaction ta(gb_main);

        TEST_EXPECT_EQUAL(GBT_count_alignments(gb_main), 2);

        char *def_ali_name = GBT_get_default_alignment(gb_main);
        TEST_EXPECT_EQUAL(def_ali_name, "ali_tuf_dna");

        {
            ConstStrArray names;
            GBT_get_alignment_names(names, gb_main);
            {
                char *joined = GBT_join_strings(names, '*');
                TEST_EXPECT_EQUAL(joined, "ali_tuf_pro*ali_tuf_dna");
                free(joined);
            }

            for (int i = 0; names[i]; ++i) {
                long len = GBT_get_alignment_len(gb_main, names[i]);
                TEST_EXPECT_EQUAL(len, !i ? 487 : 1462);

                char *type_name = GBT_get_alignment_type_string(gb_main, names[i]);
                TEST_EXPECT_EQUAL(type_name, !i ? "ami" : "dna");
                free(type_name);

                GB_alignment_type type = GBT_get_alignment_type(gb_main, names[i]);
                TEST_EXPECT_EQUAL(type, !i ? GB_AT_AA : GB_AT_DNA);
                TEST_EXPECT_EQUAL(GBT_is_alignment_protein(gb_main, names[i]), !i);
            }
        }

        // test functions called with aliname==NULL
        TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GBT_get_alignment(gb_main, NULL), "no alignment");
        TEST_EXPECT_EQUAL(GBT_get_alignment_len(gb_main, NULL), -1);
        TEST_EXPECT_CONTAINS(GB_await_error(), "no alignment");
        TEST_EXPECT_NORESULT__ERROREXPORTED_CONTAINS(GBT_get_alignment_type_string(gb_main, NULL), "no alignment");

        free(def_ali_name);
    }

    GB_close(gb_main);
}
TEST_PUBLISH(TEST_alignment);

#endif // UNIT_TESTS
