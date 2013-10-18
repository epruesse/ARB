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

#ifndef ARBDBT_H
#include "arbdbt.h"
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#define gb_assert(cond) arb_assert(cond)

typedef SmartMallocPtr(char) GBL;

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
    std::vector<GBL> content;

public:
    void insert(char *copy) { content.push_back(copy); }
    void insert(GBL smartie) { content.push_back(smartie); }
    const char *get(int idx) const { gb_assert(idx<size()); return &*content[idx]; }
    GBL get_smart(int idx) const { gb_assert(idx<size()); return content[idx]; }
    int size() const { return content.size(); }

    void erase() { content.clear(); }

    char *concatenated() const;
    void swap(GBL_streams& other) { std::swap(content, other.content); }
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


