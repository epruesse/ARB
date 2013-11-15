// ============================================================ //
//                                                              //
//   File      : TreeWrite.h                                    //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   www.arb-home.de                                            //
//                                                              //
// ============================================================ //

#ifndef TREEWRITE_H
#define TREEWRITE_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef NDS_H
#include <nds.h>
#endif

typedef void        (*TREE_make_node_text_init)(GBDATA *gb_main);
typedef const char *(*TREE_make_node_text)     (GBDATA *gb_main, GBDATA * gbd, enum NDS_Type mode, GBT_TREE *species, const char *tree_name);

struct TREE_node_text_gen {
    TREE_make_node_text_init init;                  // e.g. make_node_text_init() from AWT
    TREE_make_node_text      gen;                   // e.g. make_node_text_nds() from AWT

    TREE_node_text_gen(TREE_make_node_text_init init_, TREE_make_node_text gen_)
        : init(init_)
        , gen(gen_)
    {}
};

enum TREE_node_quoting {
    TREE_DISALLOW_QUOTES = 0, // don't use quotes
    TREE_SINGLE_QUOTES   = 1, // use single quotes
    TREE_DOUBLE_QUOTES   = 2, // use double quotes

    TREE_FORCE_QUOTES  = 4, // force usage of quotes
    TREE_FORCE_REPLACE = 8, // replace all problematic characters (default is to replace quotes in quoted labels)
};

GB_ERROR TREE_write_Newick(GBDATA *gb_main, const char *tree_name, const TREE_node_text_gen *node_gen, bool save_branchlengths, bool save_bootstraps, bool save_groupnames, bool pretty, TREE_node_quoting quoteMode, const char *path);
GB_ERROR TREE_write_XML(GBDATA *gb_main, const char *db_name, const char *tree_name, const TREE_node_text_gen *node_gen, bool skip_folded, const char *path);
GB_ERROR TREE_export_tree(GBDATA *gb_main, FILE *out, GBT_TREE *tree, bool triple_root, bool export_branchlens, bool use_double_quotes);

#else
#error TreeWrite.h included twice
#endif // TREEWRITE_H
