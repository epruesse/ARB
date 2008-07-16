#ifndef awt_tree_cmp_hxx_included
#define awt_tree_cmp_hxx_included

class AP_tree;
class AWT_species_set;

class AWT_species_set_root {
    long              species_counter;
    long              nsets;
    AWT_species_set **sets;
    int               diff_bits[256];
    
public:
    long     mstatus;
    long     status;            // temporary variables for status
    long     nspecies;
    GBDATA  *gb_main;
    GB_HASH *species_hash;

    // REAL PUBLIC
    AWT_species_set_root(GBDATA *gb_main,long nspecies);
    ~AWT_species_set_root();
    
    void             add(const char *species_name);                                                // max nspecies
    void             add(AWT_species_set *set);                                                    // max 2 * nspecies !!!
    AWT_species_set *search(AWT_species_set *set,long *best_cost);
    int              search(AWT_species_set *set,FILE *log_file);                                  // set's best_cost & best_node
    GB_ERROR         copy_node_infos(FILE *log, AW_BOOL delete_old_nodes, AW_BOOL nodes_with_marked_only);
    AWT_species_set *find_best_matches_info(AP_tree *tree_source,FILE *log,AW_BOOL setinner_node);
    AWT_species_set *move_tree_2_ssr(AP_tree *node);
};


class AWT_species_set {
public:
    unsigned char *bitstring;
    int            unfound_species_count;
    double         best_cost;
    AP_tree       *best_node;
    AP_tree       *node;
    // REAL PUBLIC:

    AWT_species_set(AP_tree *node,AWT_species_set_root *ssr,char *species_name);
    AWT_species_set(AP_tree *node,AWT_species_set_root *ssr,AWT_species_set *l,AWT_species_set *r); // or of two subsets
    ~AWT_species_set();
};


GB_ERROR AWT_move_info(GBDATA *gb_main, const char *tree_source,const char *tree_dest,const char *log_file, AW_BOOL compare_node_info, AW_BOOL overwrite_old_nodes, AW_BOOL nodes_with_marked_only);


#endif
