// =============================================================== //
//                                                                 //
//   File      : aditem.cxx                                        //
//   Purpose   : item functions                                    //
//               items are e.g. species, SAIs, genes, etc          //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_local.h"

#include <arbdbt.h>
#include <arb_strbuf.h>


GBDATA *GBT_find_or_create_item_rel_item_data(GBDATA *gb_item_data, const char *itemname, const char *id_field, const char *id, bool markCreated) {
    /*! Search for an existing or create a new, named item.
     * @param gb_item_data  item container
     * @param itemname      description of itemtype (for error messages)
     * @param id_field      item-field containing ID (e.g. "name")
     * @param id            the ID itself (compare is case-insensitive)
     * @param markCreated   true -> mark item if it was created
     * @return found/created item or NULL if an error occurs (which is exported in that case)
     */

    GBDATA   *gb_item = 0;
    GB_ERROR  error   = 0;

    if (!gb_item_data) error = "Missing parent container";
    else {
        gb_item = GBT_find_item_rel_item_data(gb_item_data, id_field, id);
        if (!gb_item) {
            error = GB_push_transaction(gb_item_data);
            if (!error) {
                gb_item             = GB_create_container(gb_item_data, itemname); // create a new item
                if (!gb_item) error = GB_await_error();
                else {
                    error = GBT_write_string(gb_item, id_field, id); // write item identifier
                    if (!error && markCreated) GB_write_flag(gb_item, 1); // mark generated item
                }
            }
            error = GB_end_transaction(gb_item_data, error);
        }
    }

    if (!gb_item && !error) error = GB_await_error();
    if (error) {
        gb_item = 0;
        GB_export_errorf("Can't create %s '%s': %s", itemname, id, error);
    }

    return gb_item;
}

GBDATA *GBT_find_or_create_species_rel_species_data(GBDATA *gb_species_data, const char *name) {
    return GBT_find_or_create_item_rel_item_data(gb_species_data, "species", "name", name, true);
}

GBDATA *GBT_find_or_create_species(GBDATA *gb_main, const char *name) {
    return GBT_find_or_create_species_rel_species_data(GBT_get_species_data(gb_main), name);
}

GBDATA *GBT_find_or_create_SAI(GBDATA *gb_main, const char *name) {
    //! Search for an SAI, when SAI does not exist, create it
    return GBT_find_or_create_item_rel_item_data(GBT_get_SAI_data(gb_main), "extended", "name", name, true);
}


// ------------------------------------
//      some simple find procedures

GBDATA *GBT_find_item_rel_item_data(GBDATA *gb_item_data, const char *id_field, const char *id_value) {
    /*! search for items starting at item container
     *
     * @param 'gb_item_data' is a container containing items
     * @param 'id_field' is a field containing a unique identifier for each item (e.g. 'name' for species)
     * @param 'id_value' expected value of 'id_field'
     *
     * @return a pointer to an item with 'id_field' containing 'id_value'
     * or NULL (in this case an error MAY be exported)
     *
     * Note: If you expect the item to exist, use GBT_expect_item_rel_item_data()
     */

    GBDATA *gb_item_id = GB_find_string(gb_item_data, id_field, id_value, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    return gb_item_id ? GB_get_father(gb_item_id) : 0;
}

static GBDATA *GBT_expect_item_rel_item_data(GBDATA *gb_item_data, const char *id_field, const char *id_value) {
    //! like GBT_find_item_rel_item_data(), but also exports an error if the item is not present

    GBDATA *gb_found = GBT_find_item_rel_item_data(gb_item_data, id_field, id_value);
    if (!gb_found && !GB_have_error()) { // item simply not exists
        GBDATA     *gb_any   = GB_find(gb_item_data, id_field, SEARCH_GRANDCHILD);
        const char *itemname = gb_any ? GB_read_key_pntr(GB_get_father(gb_any)) : "<item>";
        GB_export_errorf("Could not find %s with %s '%s'", itemname, id_field, id_value);
    }
    return gb_found;
}

// --------------------------------------------------------------------------------

GBDATA *GBT_get_species_data(GBDATA *gb_main) {
    return GBT_find_or_create(gb_main, "species_data", 7);
}

GBDATA *GBT_first_marked_species_rel_species_data(GBDATA *gb_species_data) {
    return GB_first_marked(gb_species_data, "species");
}

GBDATA *GBT_first_marked_species(GBDATA *gb_main) {
    return GB_first_marked(GBT_get_species_data(gb_main), "species");
}
GBDATA *GBT_next_marked_species(GBDATA *gb_species) {
    gb_assert(GB_has_key(gb_species, "species"));
    return GB_next_marked(gb_species, "species");
}

GBDATA *GBT_first_species_rel_species_data(GBDATA *gb_species_data) {
    return GB_entry(gb_species_data, "species");
}
GBDATA *GBT_first_species(GBDATA *gb_main) {
    return GB_entry(GBT_get_species_data(gb_main), "species");
}

GBDATA *GBT_next_species(GBDATA *gb_species) {
    gb_assert(GB_has_key(gb_species, "species"));
    return GB_nextEntry(gb_species);
}

GBDATA *GBT_find_species_rel_species_data(GBDATA *gb_species_data, const char *name) {
    return GBT_find_item_rel_item_data(gb_species_data, "name", name);
}
GBDATA *GBT_find_species(GBDATA *gb_main, const char *name) {
    // Search for a species.
    // Return found species or NULL (in this case an error MAY be exported).
    //
    // Note: If you expect the species to exists, use GBT_expect_species!
    return GBT_find_item_rel_item_data(GBT_get_species_data(gb_main), "name", name);
}

GBDATA *GBT_expect_species(GBDATA *gb_main, const char *name) {
    // Returns an existing species or
    // NULL (in that case an error is exported)
    return GBT_expect_item_rel_item_data(GBT_get_species_data(gb_main), "name", name);
}

// --------------------------------------------------------------------------------

GBDATA *GBT_get_SAI_data(GBDATA *gb_main) {
    return GBT_find_or_create(gb_main, "extended_data", 7);
}

// Search SAIs
GBDATA *GBT_first_SAI_rel_SAI_data(GBDATA *gb_sai_data) {
    return GB_entry(gb_sai_data, "extended");
}
GBDATA *GBT_first_SAI(GBDATA *gb_main) {
    return GB_entry(GBT_get_SAI_data(gb_main), "extended");
}

GBDATA *GBT_next_SAI(GBDATA *gb_sai) {
    gb_assert(GB_has_key(gb_sai, "extended"));
    return GB_nextEntry(gb_sai);
}

GBDATA *GBT_find_SAI_rel_SAI_data(GBDATA *gb_sai_data, const char *name) {
    return GBT_find_item_rel_item_data(gb_sai_data, "name", name);
}
GBDATA *GBT_find_SAI(GBDATA *gb_main, const char *name) {
    // Search for a SAI.
    // Return found SAI or NULL (in this case an error MAY be exported).
    //
    // Note: If you expect the SAI to exist, use GBT_expect_SAI!
    return GBT_find_item_rel_item_data(GBT_get_SAI_data(gb_main), "name", name);
}

GBDATA *GBT_expect_SAI(GBDATA *gb_main, const char *name) {
    // Returns an existing SAI or
    // NULL (in that case an error is exported)
    return GBT_expect_item_rel_item_data(GBT_get_SAI_data(gb_main), "name", name);
}

// ---------------------
//      count items

static long GBT_get_item_count(GBDATA *gb_parent_of_container, const char *item_container_name) {
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

// --------------------------------------------------------------------------------

static char *GBT_create_unique_item_identifier(GBDATA *gb_item_container, const char *id_field, const char *default_id) {
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
        unique_id = ARB_strdup(default_id); // default_id is unused
    }
    else {
        char   *generated_id  = (char*)ARB_alloc(strlen(default_id)+20);
        size_t  min_num = 1;

#define GENERATE_ID(num) sprintf(generated_id, "%s%zu", default_id, num);

        GENERATE_ID(min_num);
        gb_item = GBT_find_item_rel_item_data(gb_item_container, id_field, generated_id);

        if (gb_item) {
            size_t num_items = GB_number_of_subentries(gb_item_container);
            size_t max_num   = 0;

            gb_assert(num_items); // otherwise deadlock below

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
                    gb_assert(mid != min_num && mid != max_num);

                    GENERATE_ID(mid);
                    gb_item = GBT_find_item_rel_item_data(gb_item_container, id_field, generated_id);

                    if (gb_item) min_num = mid;
                    else max_num = mid;
                }
                GENERATE_ID(max_num);
                gb_assert(GBT_find_item_rel_item_data(gb_item_container, id_field, generated_id) == NULL);
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


// --------------------------------
//      mark and unmark species

void GBT_mark_all(GBDATA *gb_main, int flag) {
    // flag == 0 -> unmark
    // flag == 1 -> mark
    // flag == 2 -> invert

    GBDATA *gb_species;
    GB_push_transaction(gb_main);

    if (flag == 2) {
        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species))
        {
            GB_write_flag(gb_species, !GB_read_flag(gb_species));
        }
    }
    else {
        gb_assert(flag == 0 || flag == 1);

        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species))
        {
            GB_write_flag(gb_species, flag);
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
             gb_species = GBT_next_species(gb_species))
        {
            if (condition(gb_species, cd)) {
                GB_write_flag(gb_species, !GB_read_flag(gb_species));
            }
        }
    }
    else {
        gb_assert(flag == 0 || flag == 1);

        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species))
        {
            int curr_flag = GB_read_flag(gb_species);
            if (curr_flag != flag && condition(gb_species, cd)) {
                GB_write_flag(gb_species, flag);
            }
        }
    }
    GB_pop_transaction(gb_main);
}

long GBT_count_marked_species(GBDATA *gb_main) {
    GB_transaction ta(gb_main);
    return GB_number_of_marked_subentries(GBT_get_species_data(gb_main));
}

char *GBT_store_marked_species(GBDATA *gb_main, bool unmark_all) {
    /*! stores the currently marked species in a string
     * @param gb_main    database
     * @param unmark_all if true -> unmark species
     * @return ';'-separated list of species names
     *
     * Note: a faster (but less robust) way to temporarily store species marks,
     *       is to use the flag GB_USERFLAG_WASMARKED together with GB_write_user_flag
     */
    GBS_strstruct *out = GBS_stropen(10000);
    GBDATA        *gb_species;

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
    /*! call a function with each species of a list
     * @param gb_main    database
     * @param stored     ';'-separated list of species names
     * @param doit       function to call with each species in 'stored'
     * @param clientdata is passed to 'doit'
     * @return error if sth goes wrong (or if 'doit' reports error)
     */

#define MAX_NAME_LEN 20
    char     name[MAX_NAME_LEN+1];
    GB_ERROR error = 0;

    while (!error) {
        const char   *p   = strchr(stored, ';');
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

static GB_ERROR restore_mark(GBDATA *gb_species, int *) {
    GB_write_flag(gb_species, 1);
    return 0;
}

GB_ERROR GBT_restore_marked_species(GBDATA *gb_main, const char *stored_marked) {
    /*! restores marked species.
     * @param gb_main       database
     * @param stored_marked contains a ';'-separated list of species names to mark (as returned by GBT_store_marked_species)
     * @return error if sth goes wrong
     */
    GBT_mark_all(gb_main, 0);   // unmark all species
    return GBT_with_stored_species(gb_main, stored_marked, restore_mark, 0);
}

// ---------------------------------
//      read species information

#if defined(WARN_TODO)
#warning rename the following functions to make the difference more obvious??
#endif
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

GBDATA **GBT_gen_species_array(GBDATA *gb_main, long *pspeccnt)
{
    GBDATA *gb_species;
    GBDATA *gb_species_data = GBT_get_species_data(gb_main);
    GBDATA **result;
    *pspeccnt = 0;
    for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species)) {
        (*pspeccnt) ++;
    }
    result = (GBDATA **)ARB_alloc((size_t)(sizeof(*result)*(*pspeccnt))); // @@@ fails if no species present
    *pspeccnt = 0;
    for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species)) {
        result[(*pspeccnt)++]=gb_species;
    }
    return result;
}

