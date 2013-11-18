// ==================================================================== //
//                                                                      //
//   File      : ad_config.h                                            //
//   Purpose   : Read/write editor configurations                       //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in May 2005              //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#ifndef AD_CONFIG_H
#define AD_CONFIG_H

#ifndef ARBDB_BASE_H
#include "arbdb_base.h"
#endif

#define CONFIG_DATA_PATH "configuration_data"
#define CONFIG_ITEM      "configuration"

GBDATA *GBT_find_configuration(GBDATA *gb_main, const char *name);
GBDATA *GBT_create_configuration(GBDATA *gb_main, const char *name);

void GBT_get_configuration_names(struct ConstStrArray& configNames, GBDATA *gb_main);

struct GBT_config {
    char *top_area;
    char *middle_area;
};

GBT_config *GBT_load_configuration_data(GBDATA *gb_main, const char *name, GB_ERROR *error);
void        GBT_free_configuration_data(GBT_config *data);
GB_ERROR    GBT_save_configuration_data(GBT_config *data, GBDATA *gb_main, const char *name);

enum GBT_CONFIG_ITEM_TYPE {
    CI_UNKNOWN       = 1,
    CI_GROUP         = 2,
    CI_FOLDED_GROUP  = 4,
    CI_SPECIES       = 8,
    CI_SAI           = 16,
    CI_CLOSE_GROUP   = 32,
    CI_END_OF_CONFIG = 64,
};

struct GBT_config_item {
    GBT_CONFIG_ITEM_TYPE  type;
    char                 *name;
};

struct GBT_config_parser {
    char *config_string;
    int   parse_pos;
};

GBT_config_parser *GBT_start_config_parser(const char *config_string);
void               GBT_free_config_parser(GBT_config_parser *parser);

GB_ERROR         GBT_parse_next_config_item(GBT_config_parser *parser, GBT_config_item *item);
void             GBT_append_to_config_string(const GBT_config_item *item, struct GBS_strstruct *strstruct);

GBT_config_item *GBT_create_config_item();
void             GBT_free_config_item(GBT_config_item *item);

#if defined(DEBUG)
void GBT_test_config_parser(GBDATA *gb_main);
#endif // DEBUG

#else
#error ad_config.h included twice
#endif // AD_CONFIG_H

