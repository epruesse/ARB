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

typedef void        (*TREE_make_node_text_init)(GBDATA *gb_main);
typedef const char *(*TREE_make_node_text)     (GBDATA *gb_main, GBDATA * gbd, int mode, GBT_TREE *species, const char *tree_name);

struct TREE_node_text_gen {
    TREE_make_node_text_init init;                  // e.g. make_node_text_init() from AWT
    TREE_make_node_text      gen;                   // e.g. make_node_text_nds() from AWT

    TREE_node_text_gen(TREE_make_node_text_init init_, TREE_make_node_text gen_)
        : init(init_)
        , gen(gen_)
    {}
};



GB_ERROR TREE_write_Newick(GBDATA *gb_main, char *tree_name, const TREE_node_text_gen *node_gen, bool save_branchlengths, bool save_bootstraps, bool save_groupnames, bool pretty, char *path);
GB_ERROR TREE_write_XML(GBDATA *gb_main, const char *db_name, const char *tree_name, const TREE_node_text_gen *node_gen, bool skip_folded, const char *path);

#else
#error TreeWrite.h included twice
#endif // TREEWRITE_H
