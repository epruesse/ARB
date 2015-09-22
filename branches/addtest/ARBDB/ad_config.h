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
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#define CONFIG_DATA_PATH "configuration_data"
#define CONFIG_ITEM      "configuration"

#define DEFAULT_CONFIGURATION "default_configuration"

GBDATA *GBT_find_configuration(GBDATA *gb_main, const char *name);
GBDATA *GBT_findOrCreate_configuration(GBDATA *gb_main, const char *name);

void GBT_get_configuration_names(struct ConstStrArray& configNames, GBDATA *gb_main);

class GBT_config : virtual Noncopyable {
    char *top_area;
    char *middle_area;
    char *comment; // NULL if no comment exists
public:
    GBT_config(GBDATA *gb_main, const char *name, GB_ERROR& error);
    GBT_config() : top_area(NULL), middle_area(NULL), comment(NULL) {}
    ~GBT_config() {
        free(top_area);
        free(middle_area);
        free(comment);
    }

    static const int TOP_AREA    = 0;
    static const int MIDDLE_AREA = 1;

    bool exists() const { return top_area || middle_area; }

    const char *get_definition(int area) const {
        arb_assert(area == 0 || area == 1);
        return area ? middle_area : top_area;
    }
    void set_definition(int area, char *new_def) {
        arb_assert(area == 0 || area == 1);
        char*& Area = area ? middle_area : top_area;
        freeset(Area, new_def);
    }

    const char *get_comment() const { return comment; }
    void set_comment(const char *newComment) { freedup(comment, newComment); }

    GB_ERROR saveAsOver(GBDATA *gb_main, const char *name, const char *oldName, bool warnIfSavingDefault) const;
    GB_ERROR save(GBDATA *gb_main, const char *name, bool warnIfSavingDefault) const {
        return saveAsOver(gb_main, name, name, warnIfSavingDefault);
    }
};

enum GBT_CONFIG_ITEM_TYPE {
    CI_UNKNOWN       = 1,
    CI_GROUP         = 2,
    CI_FOLDED_GROUP  = 4,
    CI_SPECIES       = 8,
    CI_SAI           = 16,
    CI_CLOSE_GROUP   = 32,
    CI_END_OF_CONFIG = 64,
};

struct GBT_config_item : virtual Noncopyable {
    GBT_CONFIG_ITEM_TYPE  type;
    char                 *name;

    GBT_config_item() : type(CI_UNKNOWN), name(NULL) {}
    GBT_config_item(GBT_CONFIG_ITEM_TYPE type_, const char *name_) : type(type_), name(nulldup(name_)) {}
    ~GBT_config_item() { free(name); }
};

class GBT_config_parser : virtual Noncopyable {
    char *config_string;
    int   parse_pos;

    GBT_config_item item;
public:
    GBT_config_parser(const GBT_config& cfg, int area)
        : config_string(nulldup(cfg.get_definition(area))),
          parse_pos(0)
    {}
    ~GBT_config_parser() { free(config_string); }

    const GBT_config_item& nextItem(GB_ERROR& error);
};

void GBT_append_to_config_string(const GBT_config_item& item, struct GBS_strstruct *strstruct);

#if defined(DEBUG)
void GBT_test_config_parser(GBDATA *gb_main);
#endif // DEBUG

#else
#error ad_config.h included twice
#endif // AD_CONFIG_H

