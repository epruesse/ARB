// =============================================================== //
//                                                                 //
//   File      : gb_aci.h                                          //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_ACI_H
#define GB_ACI_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARBDBT_H
#include "arbdbt.h"
#endif

#define gb_assert(cond) arb_assert(cond)

#define GBL_MAX_ARGUMENTS 500

struct GBL {
    char *str;
};

class GBL_reference {
    GBDATA     *gb_ref;            // the database entry on which the command is applied (may be species, gene, experiment, group and maybe more)
    const char *default_tree_name; // if we have a default tree, its name is specified here (NULL otherwise)

public:
    GBL_reference(GBDATA *gbd, const char *treeName)
        : gb_ref(gbd), default_tree_name(treeName) {}

    GBDATA *get_main() const { return GB_get_root(gb_ref); }
    GBDATA *get_ref() const { return gb_ref; }
    const char *get_tree_name() const { return default_tree_name; }
};

class GBL_streams {
    int count;
    GBL content[GBL_MAX_ARGUMENTS];

    void setup() {
        count = 0;
        memset(content, 0, sizeof(content));
    }
    void cleanup() {
        for (int i = 0; i<count; ++i) {
            free(content[i].str);
        }
    }

public:
    GBL_streams() { setup(); }
    ~GBL_streams() { cleanup(); }

    void insert(char *copy) { content[count++].str = copy; }
    const char *get(int idx) const { gb_assert(idx<count); return content[idx].str; }
    int size() const { return count; }

    void erase() { cleanup(); setup(); }

    char *concatenated() const;
    void swap(GBL_streams& other);
};

struct GBL_command_arguments : public GBL_reference {
    GBL_command_arguments(GBDATA       *gbd,
                          const char   *treeName,
                          GBL_streams&  input_,
                          GBL_streams&  param_,
                          GBL_streams&  output_)
        : GBL_reference(gbd, treeName),
          input(input_),
          param(param_),
          output(output_)
    {}

    const char *command; // the name of the current command

    GBL_streams &input;
    GBL_streams &param;
    GBL_streams &output;
};

typedef GB_ERROR (*GBL_COMMAND)(GBL_command_arguments *args);

struct GBL_command_table {
    const char *command_identifier;
    GBL_COMMAND function;
};

#else
#error gb_aci.h included twice
#endif // GB_ACI_H


