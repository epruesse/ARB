// =============================================================== //
//                                                                 //
//   File      : NT_tree_cmp.h                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NT_TREE_CMP_H
#define NT_TREE_CMP_H

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif


class AP_tree;
class AWT_species_set;
class arb_progress;

class AWT_species_set_root : virtual Noncopyable {
    long              species_counter;
    long              nsets;
    AWT_species_set **sets;
    int               diff_bits[256];

    arb_progress *progress;

public:
    long     nspecies;
    GBDATA  *gb_main;
    GB_HASH *species_hash;

    // REAL PUBLIC
    AWT_species_set_root(GBDATA *gb_main, long nspecies, arb_progress *progress_);
    ~AWT_species_set_root();

    void             add(const char *species_name);                                                // max nspecies
    void             add(AWT_species_set *set);                                                    // max 2 * nspecies !!!
    AWT_species_set *search_best_match(const AWT_species_set *set, long& best_cost);
    int              search_and_remember_best_match_and_log_errors(const AWT_species_set *set, FILE *log_file);
    GB_ERROR         copy_node_information(FILE *log, bool delete_old_nodes, bool nodes_with_marked_only);
    AWT_species_set *find_best_matches_info(AP_tree *tree_source, FILE *log, bool setinner_node);
    AWT_species_set *move_tree_2_ssr(AP_tree *node);

    void finish(GB_ERROR& error);
};


struct AWT_species_set : virtual Noncopyable {
    unsigned char *bitstring;
    int            unfound_species_count;
    double         best_cost;
    AP_tree       *best_node;
    AP_tree       *node;
    // REAL PUBLIC:

    AWT_species_set(AP_tree *node, AWT_species_set_root *ssr, char *species_name);
    AWT_species_set(AP_tree *node, AWT_species_set_root *ssr, AWT_species_set *l, AWT_species_set *r); // or of two subsets
    ~AWT_species_set();
};

enum TreeInfoMode {
    TREE_INFO_COPY,    // overwrites existing info
    TREE_INFO_COMPARE, // compare node info
    TREE_INFO_ADD,     // doesn't overwrite
};

GB_ERROR AWT_move_info(GBDATA *gb_main, const char *tree_source, const char *tree_dest, const char *log_file, TreeInfoMode mode, bool nodes_with_marked_only);

#else
#error NT_tree_cmp.h included twice
#endif // NT_TREE_CMP_H
