// =============================================================== //
//                                                                 //
//   File      : ad_config.cxx                                     //
//   Purpose   : editor configurations (aka species selections)    //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2005       //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_local.h"

#include <ad_config.h>
#include <arbdbt.h>

#include <arb_strbuf.h>
#include <arb_defs.h>
#include <arb_strarray.h>

void GBT_get_configuration_names(ConstStrArray& configNames, GBDATA *gb_main) {
    /* returns existing configurations in 'configNames'
     * Note: automatically generates names for configs w/o legal name.
     */

    GB_transaction  ta(gb_main);
    GBDATA         *gb_config_data = GB_search(gb_main, CONFIG_DATA_PATH, GB_CREATE_CONTAINER);

    if (gb_config_data) {
        int unnamed_count = 0;

        configNames.reserve(GB_number_of_subentries(gb_config_data));
        
        for (GBDATA *gb_config = GB_entry(gb_config_data, CONFIG_ITEM);
             gb_config;
             gb_config = GB_nextEntry(gb_config))
        {
            const char *name = GBT_read_char_pntr(gb_config, "name");

            if (!name || name[0] == 0) { // no name or empty name
                char     *new_name = GBS_global_string_copy("<unnamed%i>", ++unnamed_count);
                GB_ERROR  error    = GBT_write_string(gb_config, "name", new_name);

                if (error) {
                    GB_warningf("Failed to rename unnamed configuration to '%s'", new_name);
                    freenull(new_name);
                    name = NULL;
                }
                else {
                    name = GBT_read_char_pntr(gb_config, "name");
                }
            }

            if (name) configNames.put(name);
        }
    }
}

GBDATA *GBT_find_configuration(GBDATA *gb_main, const char *name) {
    GBDATA *gb_config_data = GB_search(gb_main, CONFIG_DATA_PATH, GB_DB);
    GBDATA *gb_config_name = GB_find_string(gb_config_data, "name", name, GB_IGNORE_CASE, SEARCH_GRANDCHILD);
    return gb_config_name ? GB_get_father(gb_config_name) : 0;
}

GBDATA *GBT_findOrCreate_configuration(GBDATA *gb_main, const char *name) {
    GBDATA *gb_config = GBT_find_configuration(gb_main, name);
    if (!gb_config) {
        GBDATA *gb_config_data = GB_search(gb_main, CONFIG_DATA_PATH, GB_DB);

        gb_config = GB_create_container(gb_config_data, CONFIG_ITEM); // create new container
        if (gb_config) {
            GB_ERROR error = GBT_write_string(gb_config, "name", name);
            if (error) GB_export_error(error);
        }
    }
    return gb_config;
}

GBT_config::GBT_config(GBDATA *gb_main, const char *name, GB_ERROR& error) {
    GB_transaction  ta(gb_main);
    GBDATA         *gb_config = GBT_find_configuration(gb_main, name);

    error = NULL;
    if (!gb_config) {
        error       = GBS_global_string("No such configuration '%s'", name);
        top_area    = NULL;
        middle_area = NULL;
        comment     = NULL;
    }
    else {
        top_area    = GBT_read_string(gb_config, "top_area");
        middle_area = GBT_read_string(gb_config, "middle_area");

        if (!top_area || !middle_area) {
            error = GBS_global_string("Configuration '%s' is corrupted (Reason: %s)", name, GB_await_error());
        }

        comment = GBT_read_string(gb_config, "comment");
    }
}

GB_ERROR GBT_config::saveAsOver(GBDATA *gb_main, const char *name, const char *oldName, bool warnIfSavingDefault) const {
    /*! save config as 'name' (overwriting config 'oldName')
     * if 'warnIfSavingDefault' is true, saving DEFAULT_CONFIGURATION raises a warning
     */
    GB_ERROR error = 0;

    GB_push_transaction(gb_main);

    GBDATA *gb_config = GBT_findOrCreate_configuration(gb_main, oldName);
    if (!gb_config) {
        error = GBS_global_string("Can't create configuration '%s' (Reason: %s)", oldName, GB_await_error());
    }
    else {
        if (strcmp(name, oldName) != 0) error = GBT_write_string(gb_config, "name",    name);

        error             = GBT_write_string(gb_config, "top_area",    top_area);
        if (!error) error = GBT_write_string(gb_config, "middle_area", middle_area);

        if (!error) {
            if (comment && comment[0]) { // non-empty
                error = GBT_write_string(gb_config, "comment", comment);
            }
            else {
                GBDATA *gb_comment = GB_entry(gb_config, "comment");
                if (gb_comment) error = GB_delete(gb_comment); // delete field if comment empty
            }
        }

        if (error) error = GBS_global_string("%s (in configuration '%s')", error, name);
    }

    if (warnIfSavingDefault && strcmp(name, DEFAULT_CONFIGURATION) == 0) {
        GBT_message(gb_main, "Note: You saved the '" DEFAULT_CONFIGURATION "'.\nStarting ARB_EDIT4 will probably overwrite it!");
    }

    return GB_end_transaction(gb_main, error);
}

const GBT_config_item& GBT_config_parser::nextItem(GB_ERROR& error) {
    error = 0;

    freenull(item.name);
    item.type = CI_END_OF_CONFIG;

    if (config_string[parse_pos]) {             // if not at 0-byte
        char label = config_string[parse_pos+1];
        item.type = CI_UNKNOWN;

        switch (label) {
            case 'L': item.type = CI_SPECIES;      break;
            case 'S': item.type = CI_SAI;          break;
            case 'F': item.type = CI_FOLDED_GROUP; break;
            case 'G': item.type = CI_GROUP;        break;
            case 'E': item.type = CI_CLOSE_GROUP;  break;
            default:  item.type = CI_UNKNOWN;      break;
        }

        if (item.type == CI_CLOSE_GROUP) {
            parse_pos += 2;
        }
        else {
            const char *start_of_name = config_string+parse_pos+2;
            const char *behind_name   = strchr(start_of_name, '\1');

            if (!behind_name) behind_name = strchr(start_of_name, '\0'); // eos
            gb_assert(behind_name);

            char *data = GB_strpartdup(start_of_name, behind_name-1);
            if (item.type == CI_UNKNOWN) {
                error = GBS_global_string_copy("Unknown flag '%c' (followed by '%s')", label, data);
                free(data);
            }
            else {
                item.name = data;
                parse_pos = behind_name-config_string;
            }
        }

        if (error) { // stop parser
            const char *end_of_config = strchr(config_string+parse_pos, '\0');
            parse_pos                 = end_of_config-config_string;
            gb_assert(config_string[parse_pos] == 0);
        }
    }

    return item;
}

void GBT_append_to_config_string(const GBT_config_item& item, struct GBS_strstruct *strstruct) {
    // strstruct has to be created by GBS_stropen()

    gb_assert((item.type & (CI_UNKNOWN|CI_END_OF_CONFIG)) == 0);

    char prefix[] = "\1?";
    if (item.type == CI_CLOSE_GROUP) {
        prefix[1] = 'E';
        GBS_strcat(strstruct, prefix);
    }
    else {
        char label = 0;
        switch (item.type) {
            case CI_SPECIES:      label = 'L'; break;
            case CI_SAI:          label = 'S'; break;
            case CI_GROUP:        label = 'G'; break;
            case CI_FOLDED_GROUP: label = 'F'; break;

            default: gb_assert(0); break;
        }
        prefix[1] = label;
        GBS_strcat(strstruct, prefix);
        GBS_strcat(strstruct, item.name);
    }
}

#if defined(DEBUG) 
void GBT_test_config_parser(GBDATA *gb_main) {
    ConstStrArray config_names;
    GBT_get_configuration_names(config_names, gb_main);
    if (!config_names.empty()) {
        int count;
        for (count = 0; config_names[count]; ++count) {
            const char *config_name = config_names[count];
            GB_ERROR    error       = 0;

            printf("Testing configuration '%s':\n", config_name);
            GBT_config config(gb_main, config_name, error);
            if (error) {
                printf("* Error loading config: %s\n", error);
            }
            else {
                int area;

                gb_assert(!error);
                printf("* Successfully loaded\n");

                for (area = 0; area<2 && !error; ++area) {
                    const char        *area_name       = area ? "middle_area" : "top_area";
                    const char        *area_config_def = config.get_definition(area);
                    GBT_config_parser  parser(config, area);
                    GBS_strstruct     *new_config      = GBS_stropen(1000);

                    printf("* Created parser for '%s'\n", area_name);

                    while (1) {
                        const GBT_config_item& item = parser.nextItem(error);
                        if (error || item.type == CI_END_OF_CONFIG) break;

                        printf("  - %i %s\n", item.type, item.name ? item.name : "[close group]");
                        GBT_append_to_config_string(item, new_config);
                    }

                    char *new_config_str = GBS_strclose(new_config);

                    if (error) printf("* Parser error: %s\n", error);
                    else {
                        if (strcmp(area_config_def, new_config_str) == 0) {
                            printf("* Re-Created config is identical to original\n");
                        }
                        else {
                            printf("* Re-Created config is NOT identical to original:\n"
                                   "  - original : '%s'\n"
                                   "  - recreated: '%s'\n",
                                   area_config_def,
                                   new_config_str);
                        }
                    }

                    free(new_config_str);
                }
            }
        }
    }
}
#endif // DEBUG

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

void TEST_GBT_get_configuration_names() {
    GB_shell  shell;
    GBDATA   *gb_main = GB_open("nosuch.arb", "c");

    {
        GB_transaction ta(gb_main);

        const char *configs[] = { "arb", "BASIC", "Check it", "dummy" };
        for (size_t i = 0; i<ARRAY_ELEMS(configs); ++i) {
            TEST_EXPECT_RESULT__NOERROREXPORTED(GBT_findOrCreate_configuration(gb_main, configs[i]));
        }

        ConstStrArray cnames;
        GBT_get_configuration_names(cnames, gb_main);

        TEST_EXPECT_EQUAL(cnames.size(), 4U);

        char *joined = GBT_join_strings(cnames, '*');
        TEST_EXPECT_EQUAL(joined, "arb*BASIC*Check it*dummy");
        free(joined);
    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS


