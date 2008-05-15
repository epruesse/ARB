/* ==================================================================== */
/*                                                                      */
/*   File      : ad_config.c                                            */
/*   Purpose   : handle editor configurations                           */
/*   Time-stamp: <Tue May/24/2005 18:38 MET Coder@ReallySoft.de>        */
/*                                                                      */
/*                                                                      */
/* Coded by Ralf Westram (coder@reallysoft.de) in May 2005              */
/* Copyright Department of Microbiology (Technical University Munich)   */
/*                                                                      */
/* Visit our web site at: www.arb-home.de                               */ 
/*                                                                      */
/* ==================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbdb.h"
#include "arbdbt.h"
#include "ad_config.h"

/********************************************************************************************
                    editor configurations
********************************************************************************************/

char **GBT_get_configuration_names_and_count(GBDATA *gb_main, int *countPtr) {
    /* returns an null terminated array of string pointers */
    GBDATA  *gb_configuration_data;
    int      count       = 0;
    char   **configNames = 0;

    GB_push_transaction(gb_main);

    gb_configuration_data = GB_search(gb_main,AWAR_CONFIG_DATA,GB_DB);
    if (gb_configuration_data) {
        GBDATA *gb_config;

#if defined(DEVEL_RALF)
#warning auto-rename unnamed configurations here
#endif /* DEVEL_RALF */

        for (gb_config = GB_find(gb_configuration_data, AWAR_CONFIG, 0, down_level);
             gb_config;
             gb_config = GB_find(gb_config, AWAR_CONFIG, 0, this_level|search_next))
        {
            count++;
        }

        if (count) {
            configNames = (char **)GB_calloc(sizeof(char *),(size_t)count+1);
            count       = 0;

            for (gb_config = GB_find(gb_configuration_data, AWAR_CONFIG, 0, down_level);
                 gb_config;
                 gb_config = GB_find(gb_config, AWAR_CONFIG, 0, this_level|search_next))
            {
                GBDATA *gb_name = GB_find(gb_config, "name", 0, down_level);
                if (gb_name) {
                    configNames[count++] = GB_read_string(gb_name);
                }
            }
        }
    }
    
    GB_pop_transaction(gb_main);
    *countPtr = count;

    return configNames;
}

char **GBT_get_configuration_names(GBDATA *gb_main) {
    int dummy;
    return GBT_get_configuration_names_and_count(gb_main, &dummy);
}

GBDATA *GBT_find_configuration(GBDATA *gb_main,const char *name){
    GBDATA *gb_configuration_data = GB_search(gb_main,AWAR_CONFIG_DATA,GB_DB);
    GBDATA *gb_configuration_name;
    gb_configuration_name = GB_find(gb_configuration_data,"name",name,down_2_level);
    if (!gb_configuration_name) return 0;
    return GB_get_father(gb_configuration_name);
}

GBDATA *GBT_create_configuration(GBDATA *gb_main, const char *name){
    GBDATA *gb_configuration_data;
    GBDATA *gb_configuration;
    GBDATA *gb_name;
    gb_configuration = GBT_find_configuration(gb_main,name);
    if (gb_configuration) return gb_configuration;
    gb_configuration_data = GB_search(gb_main,AWAR_CONFIG_DATA,GB_DB);
    gb_configuration = GB_create_container(gb_configuration_data,AWAR_CONFIG);
    gb_name = GBT_search_string(gb_configuration,"name",name);
    return gb_configuration;
}


GBT_config *GBT_load_configuration_data(GBDATA *gb_main, const char *name, GB_ERROR *error) {
    GBT_config *config = 0;
    GBDATA     *gb_configuration;

    GB_push_transaction(gb_main);

    *error           = 0;
    gb_configuration = GBT_find_configuration(gb_main, name);

    if (!gb_configuration) {
        *error = GBS_global_string("No such configuration '%s'", name);
    }
    else {
        GBDATA *gb_top    = GB_search(gb_configuration, "top_area", GB_FIND);
        GBDATA *gb_middle = GB_search(gb_configuration, "middle_area", GB_FIND);

        if (gb_top && gb_middle) {
            config              = GB_calloc(1, sizeof(*config));
            config->top_area    = GB_read_string(gb_top);
            config->middle_area = GB_read_string(gb_middle);

            if (!(config->top_area && config->middle_area)) {
                GBT_free_configuration_data(config);
                config = 0;
            }
        }
        if (!config) *error = GBS_global_string("Configuration '%s' is corrupted", name);
    }

    if (*error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return config;
}

static GB_ERROR gbt_save_entry(GBDATA *gb_configuration, const char *subentry, const char *content) {
    GB_ERROR  error       = 0;
    GBDATA   *gb_subentry = GB_search(gb_configuration, subentry, GB_STRING);

    if (gb_subentry) error = GB_write_string(gb_subentry, content);
    else error             = GBS_global_string("Can't create field '%s'", subentry);
    
    return error;
}

GB_ERROR GBT_save_configuration_data(GBT_config *config, GBDATA *gb_main, const char *name) {
    GB_ERROR  error = 0;
    GBDATA   *gb_configuration;

    GB_push_transaction(gb_main);
    
    gb_configuration = GBT_create_configuration(gb_main, name);
    if (!gb_configuration) {
        error = GBS_global_string("Can't create configuration '%s' (Reason: %s)", name, GB_get_error());
    }
    else {
        error             = gbt_save_entry(gb_configuration, "top_area", config->top_area);
        if (!error) error = gbt_save_entry(gb_configuration, "middle_area", config->middle_area);
        if (error) error  = GBS_global_string("%s (in configuration '%s')", error, name);
    }

    if (error) GB_abort_transaction(gb_main);
    else GB_pop_transaction(gb_main);

    return error;
}

void GBT_free_configuration_data(GBT_config *data) {
    free(data->top_area);
    free(data->middle_area);
    free(data);
}

GBT_config_parser *GBT_start_config_parser(const char *config_string) {
    GBT_config_parser *parser = GB_calloc(1, sizeof(*parser));

    parser->config_string = GB_strdup(config_string);
    parser->parse_pos     = 0;

    return parser;
}

GBT_config_item *GBT_create_config_item() {
    GBT_config_item *item = GB_calloc(1, sizeof(*item));;
    item->type            = CI_UNKNOWN;
    item->name            = 0;
    return item;
}

void GBT_free_config_item(GBT_config_item *item) {
    free(item->name);
    free(item);
}

GB_ERROR GBT_parse_next_config_item(GBT_config_parser *parser, GBT_config_item *item) {
    /* the passed 'item' gets filled with parsed data from the config string */
    GB_ERROR error = 0;

    const char *str = parser->config_string;
    int         pos = parser->parse_pos;

    free(item->name);
    item->name = 0;
    item->type = CI_END_OF_CONFIG;

    if (str[pos]) {             /* if not at 0-byte */
        char label = str[pos+1];
        item->type = CI_UNKNOWN;

        switch (label) {
            case 'L': item->type = CI_SPECIES; break;
            case 'S': item->type = CI_SAI; break;
            case 'F': item->type = CI_FOLDED_GROUP; break;
            case 'G': item->type = CI_GROUP; break;
            case 'E': item->type = CI_CLOSE_GROUP; break;
            default : item->type = CI_UNKNOWN; break;
        }

        switch (item->type) {
            case CI_CLOSE_GROUP:
                pos += 2;
                break;

            case CI_UNKNOWN:
                error = GBS_global_string_copy("Unknown flag '%c'", label);
                break;

            default : {
                const char *start_of_name = str+pos+2;
                const char *behind_name   = strchr(start_of_name, '\1');

                if (!behind_name) behind_name = strchr(start_of_name, '\0'); /* eos */
                gb_assert(behind_name);

                item->name = GB_calloc(1, behind_name-start_of_name+1);
                memcpy(item->name, start_of_name, behind_name-start_of_name);
                pos       = behind_name-str;
                break;
            }
        }

        if (error) { /* stop parser */
            char *end_of_config = strchr(str+pos, '\0');
            pos                 = end_of_config-str;
            gb_assert(str[pos] == 0);
        }

        parser->parse_pos = pos;
    }
    return error;
}

void GBT_append_to_config_string(const GBT_config_item *item, void *strstruct) {
    /* strstruct has to be created by GBS_stropen() */

    if (item->type & (CI_UNKNOWN|CI_END_OF_CONFIG)) {
        GB_CORE;
    }
    else {
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

                default : GB_CORE; break;
            }
            prefix[1] = label;
            GBS_strcat(strstruct, prefix);
            GBS_strcat(strstruct, item->name);
        }
    }
}


void GBT_free_config_parser(GBT_config_parser *parser) {
    free(parser->config_string);
    free(parser);
}

#if defined(DEBUG) && 0 
void GBT_test_config_parser(GBDATA *gb_main) {
    char **config_names = GBT_get_configuration_names(gb_main);
    if (config_names) {
        int count;
        for (count = 0; config_names[count]; ++count) {
            char       *config_name = config_names[count];
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
                    void              *new_config      = GBS_stropen(1000);
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
        GBT_free_names(config_names);
    }
}
#endif /* DEBUG */



