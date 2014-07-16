// =============================================================== //
//                                                                 //
//   File      : ad_config.cxx                                     //
//   Purpose   : editor configurations                             //
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

GBDATA *GBT_create_configuration(GBDATA *gb_main, const char *name) {
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

void GBT_free_configuration_data(GBT_config *data) {
    free(data->top_area);
    free(data->middle_area);
    free(data);
}

GBT_config *GBT_load_configuration_data(GBDATA *gb_main, const char *name, GB_ERROR *error) {
    GBT_config *config = 0;

    *error            = GB_push_transaction(gb_main);
    GBDATA *gb_config = GBT_find_configuration(gb_main, name);

    if (!gb_config) {
        *error = GBS_global_string("No such configuration '%s'", name);
    }
    else {
        config              = (GBT_config*)GB_calloc(1, sizeof(*config));
        config->top_area    = GBT_read_string(gb_config, "top_area");
        config->middle_area = GBT_read_string(gb_config, "middle_area");

        if (!config->top_area || !config->middle_area) {
            GBT_free_configuration_data(config);
            config = 0;
            *error = GBS_global_string("Configuration '%s' is corrupted (Reason: %s)",
                                       name, GB_await_error());
        }
    }

    *error = GB_end_transaction(gb_main, *error);
    return config;
}

GB_ERROR GBT_save_configuration_data(GBT_config *config, GBDATA *gb_main, const char *name) {
    GB_ERROR  error = 0;
    GBDATA   *gb_config;

    GB_push_transaction(gb_main);

    gb_config = GBT_create_configuration(gb_main, name);
    if (!gb_config) {
        error = GBS_global_string("Can't create configuration '%s' (Reason: %s)", name, GB_await_error());
    }
    else {
        error             = GBT_write_string(gb_config, "top_area", config->top_area);
        if (!error) error = GBT_write_string(gb_config, "middle_area", config->middle_area);

        if (error) error = GBS_global_string("%s (in configuration '%s')", error, name);
    }

    return GB_end_transaction(gb_main, error);
}

GBT_config_parser *GBT_start_config_parser(const char *config_string) {
    GBT_config_parser *parser = (GBT_config_parser*)GB_calloc(1, sizeof(*parser));

    parser->config_string = nulldup(config_string);
    parser->parse_pos     = 0;

    return parser;
}

GBT_config_item *GBT_create_config_item() {
    GBT_config_item *item = (GBT_config_item*)GB_calloc(1, sizeof(*item));
    item->type            = CI_UNKNOWN;
    item->name            = 0;
    return item;
}

void GBT_free_config_item(GBT_config_item *item) {
    free(item->name);
    free(item);
}

GB_ERROR GBT_parse_next_config_item(GBT_config_parser *parser, GBT_config_item *item) {
    // the passed 'item' gets filled with parsed data from the config string
    GB_ERROR error = 0;

    const char *str = parser->config_string;
    int         pos = parser->parse_pos;

    freenull(item->name);
    item->type = CI_END_OF_CONFIG;

    if (str[pos]) {             // if not at 0-byte
        char label = str[pos+1];
        item->type = CI_UNKNOWN;

        switch (label) {
            case 'L': item->type = CI_SPECIES; break;
            case 'S': item->type = CI_SAI; break;
            case 'F': item->type = CI_FOLDED_GROUP; break;
            case 'G': item->type = CI_GROUP; break;
            case 'E': item->type = CI_CLOSE_GROUP; break;
            default: item->type = CI_UNKNOWN; break;
        }

        if (item->type == CI_CLOSE_GROUP) {
            pos += 2;
        }
        else {
            const char *start_of_name = str+pos+2;
            const char *behind_name   = strchr(start_of_name, '\1');

            if (!behind_name) behind_name = strchr(start_of_name, '\0'); // eos
            gb_assert(behind_name);

            char *data = GB_strpartdup(start_of_name, behind_name-1);
            if (item->type == CI_UNKNOWN) {
                error = GBS_global_string_copy("Unknown flag '%c' (followed by '%s')", label, data);
                free(data);
            }
            else {
                item->name = data;
                pos        = behind_name-str;
            }
        }

        if (error) { // stop parser
            const char *end_of_config = strchr(str+pos, '\0');
            pos                 = end_of_config-str;
            gb_assert(str[pos] == 0);
        }

        parser->parse_pos = pos;
    }
    return error;
}

void GBT_append_to_config_string(const GBT_config_item *item, GBS_strstruct *strstruct) {
    // strstruct has to be created by GBS_stropen()

    gb_assert((item->type & (CI_UNKNOWN|CI_END_OF_CONFIG)) == 0);

    char prefix[] = "\1?";
    if (item->type == CI_CLOSE_GROUP) {
        prefix[1] = 'E';
        GBS_strcat(strstruct, prefix);
    }
    else {
        char label = 0;
        switch (item->type) {
            case CI_SPECIES:      label = 'L'; break;
            case CI_SAI:          label = 'S'; break;
            case CI_GROUP:        label = 'G'; break;
            case CI_FOLDED_GROUP: label = 'F'; break;

            default: gb_assert(0); break;
        }
        prefix[1] = label;
        GBS_strcat(strstruct, prefix);
        GBS_strcat(strstruct, item->name);
    }
}


void GBT_free_config_parser(GBT_config_parser *parser) {
    free(parser->config_string);
    free(parser);
}

#if defined(DEBUG) 
void GBT_test_config_parser(GBDATA *gb_main) {
    ConstStrArray config_names;
    GBT_get_configuration_names(config_names, gb_main);
    if (!config_names.empty()) {
        int count;
        for (count = 0; config_names[count]; ++count) {
            const char *config_name = config_names[count];
            GBT_config *config;
            GB_ERROR    error       = 0;

            printf("Testing configuration '%s':\n", config_name);
            config = GBT_load_configuration_data(gb_main, config_name, &error);
            if (!config) {
                gb_assert(error);
                printf("* Error loading config: %s\n", error);
            }
            else {
                int area;

                gb_assert(!error);
                printf("* Successfully loaded\n");

                for (area = 0; area<2 && !error; ++area) {
                    const char        *area_name       = area ? "middle_area" : "top_area";
                    const char        *area_config_def = area ? config->middle_area : config->top_area;
                    GBT_config_parser *parser          = GBT_start_config_parser(area_config_def);
                    GBT_config_item   *item            = GBT_create_config_item();
                    GBS_strstruct     *new_config      = GBS_stropen(1000);
                    char              *new_config_str;

                    gb_assert(parser);
                    printf("* Created parser for '%s'\n", area_name);

                    while (1) {
                        error = GBT_parse_next_config_item(parser, item);
                        if (error || item->type == CI_END_OF_CONFIG) break;

                        printf("  - %i %s\n", item->type, item->name ? item->name : "[close group]");
                        GBT_append_to_config_string(item, new_config);
                    }

                    GBT_free_config_item(item);
                    new_config_str = GBS_strclose(new_config);

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

                    GBT_free_config_parser(parser);
                    free(new_config_str);
                }
            }
            GBT_free_configuration_data(config);
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
            TEST_EXPECT_RESULT__NOERROREXPORTED(GBT_create_configuration(gb_main, configs[i]));
        }

        ConstStrArray cnames;
        GBT_get_configuration_names(cnames, gb_main);

        TEST_EXPECT_EQUAL(cnames.size(), 4U);

        char *joined = GBT_join_names(cnames, '*');
        TEST_EXPECT_EQUAL(joined, "arb*BASIC*Check it*dummy");
        free(joined);
    }

    GB_close(gb_main);
}

#endif // UNIT_TESTS
