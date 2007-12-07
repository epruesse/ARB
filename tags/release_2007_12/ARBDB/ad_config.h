// ==================================================================== //
//                                                                      //
//   File      : ad_config.h                                            //
//   Purpose   : Read/write editor configurations                       //
//   Time-stamp: <Mon May/23/2005 09:25 MET Coder@ReallySoft.de>        //
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

#ifdef __cplusplus
extern "C" {
#endif

#define AWAR_CONFIG_DATA "configuration_data"
#define AWAR_CONFIG      "configuration"

    GBDATA *GBT_find_configuration(GBDATA *gb_main,const char *name);
    GBDATA *GBT_create_configuration(GBDATA *gb_main, const char *name);

    char **GBT_get_configuration_names(GBDATA *gb_main);
    char **GBT_get_configuration_names_and_count(GBDATA *gb_main, int *countPtr);


    typedef struct s_gbt_config {
        char *top_area;
        char *middle_area;
    } GBT_config;

    GBT_config *GBT_load_configuration_data(GBDATA *gb_main, const char *name, GB_ERROR *error);

    GB_ERROR GBT_save_configuration_data(GBT_config *data, GBDATA *gb_main, const char *name);
    void     GBT_free_configuration_data(GBT_config *data);


    typedef enum {
        CI_UNKNOWN       = 1,
        CI_GROUP         = 2,
        CI_FOLDED_GROUP  = 4,
        CI_SPECIES       = 8,
        CI_SAI           = 16,
        CI_CLOSE_GROUP   = 32,
        CI_END_OF_CONFIG = 64,
    } gbt_config_item_type;

    typedef struct s_gbt_config_item {
        gbt_config_item_type  type;
        char                 *name;
    } GBT_config_item;

    typedef struct s_gbt_config_parser {
        char *config_string;
        int   parse_pos;
    } GBT_config_parser;

    GBT_config_parser *GBT_start_config_parser(const char *config_string);
    void               GBT_free_config_parser(GBT_config_parser *parser);

    GB_ERROR         GBT_parse_next_config_item(GBT_config_parser *parser, GBT_config_item *item);
    void             GBT_append_to_config_string(const GBT_config_item *item, void *strstruct);

    GBT_config_item *GBT_create_config_item();
    void             GBT_free_config_item(GBT_config_item *item);

#if defined(DEBUG)
        void GBT_test_config_parser(GBDATA *gb_main);
#endif // DEBUG

#ifdef __cplusplus
}
#endif

#else
#error ad_config.h included twice
#endif // AD_CONFIG_H

