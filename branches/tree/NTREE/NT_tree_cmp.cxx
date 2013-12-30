// =============================================================== //
//                                                                 //
//   File      : NT_tree_cmp.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_tree_cmp.h"
#include "NT_local.h"

#include <AP_Tree.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <string>

using namespace std;

AWT_species_set_root::AWT_species_set_root(GBDATA *gb_maini, long nspeciesi, arb_progress *progress_) {
    memset((char *)this, 0, sizeof(*this));
    gb_main  = gb_maini;
    nspecies = nspeciesi;
    progress = progress_;
    sets     = (AWT_species_set **)GB_calloc(sizeof(AWT_species_set *), (size_t)(nspecies*2+1));

    int i;
    for (i=0; i<256; i++) {
        int j = i;
        int count = 0;
        while (j) {             // count bits
            if (j&1) count ++;
            j = j>>1;
        }
        diff_bits[i] = count;
    }
    species_hash = GBS_create_hash(nspecies, GB_IGNORE_CASE);
    species_counter = 1;
}

AWT_species_set_root::~AWT_species_set_root() {
    int i;
    for (i=0; i<nsets; i++) {
        delete sets[i];
    }
    delete sets;
}

void AWT_species_set_root::add(const char *species_name) {
    if (GBS_read_hash(species_hash, species_name)) {
        aw_message(GBS_global_string("Warning: Species '%s' is found more than once in tree", species_name));
    }
    else {
        GBS_write_hash(species_hash, species_name, species_counter++);
    }
}

void AWT_species_set_root::add(AWT_species_set *set) {
    nt_assert(nsets<nspecies*2);
    sets[nsets++] = set;
}

AWT_species_set *AWT_species_set_root::search(AWT_species_set *set, long *best_co) {
    AWT_species_set *result = 0;
    long i;
    long best_cost = 0x7fffffff;
    unsigned char *sbs = set->bitstring;
    for (i=nsets-1; i>=0; i--) {
        long j;
        long sum = 0;
        unsigned char *rsb = sets[i]->bitstring;
        for (j=nspecies/8 -1 + 1; j>=0; j--) {
            sum += diff_bits[(sbs[j]) ^ (rsb[j])];
        }
        if (sum > nspecies/2) sum = nspecies - sum; // take always the minimum
        if (sum < best_cost) {
            best_cost = sum;
            result = sets[i];
        }
    }
    *best_co = best_cost;
    return result;
}

int AWT_species_set_root::search(AWT_species_set *set, FILE *log_file) {
    long net_cost;
    double best_cost;
    AWT_species_set *bs = this->search(set, &net_cost);
    best_cost = net_cost + set->unfound_species_count * 0.0001;
    if (best_cost < bs->best_cost) {
        bs->best_cost = best_cost;
        bs->best_node = set->node;
    }
    if (log_file) {
        if (net_cost) {
            fprintf(log_file, "Group '%s' not placed optimal (%li errors)\n",
                    set->node->name,
                    net_cost);
        }
    }
    return net_cost;
}

AWT_species_set::AWT_species_set(AP_tree *nodei, AWT_species_set_root *ssr, char *species_name) {
    memset((char *)this, 0, sizeof(*this));
    bitstring = (unsigned char *)GB_calloc(sizeof(char), size_t(ssr->nspecies/8)+sizeof(long)+1);
    long species_index = GBS_read_hash(ssr->species_hash, species_name);
    if (species_index) {
        bitstring[species_index/8] |= 1 << (species_index % 8);
    }
    else {
        unfound_species_count = 1;
    }
    this->node = nodei;
    best_cost = 0x7fffffff;
}

AWT_species_set::AWT_species_set(AP_tree *nodei, AWT_species_set_root *ssr, AWT_species_set *l, AWT_species_set *r) {
    memset((char *)this, 0, sizeof(*this));
    bitstring = (unsigned char *)GB_calloc(sizeof(char), size_t(ssr->nspecies/8)+5);
    long *lbits = (long *)l->bitstring;
    long *rbits = (long *)r->bitstring;
    long *dest = (long *)bitstring;
    for (long j = ssr->nspecies/8/sizeof(long) - 1 + 1; j>=0; j--) {
        dest[j] = lbits[j] | rbits[j];
    }
    unfound_species_count = l->unfound_species_count + r->unfound_species_count;
    this->node = nodei;
    best_cost = 0x7fffffff;
}

AWT_species_set::~AWT_species_set() {
    free(bitstring);
}

AWT_species_set *AWT_species_set_root::move_tree_2_ssr(AP_tree *node) {
    AWT_species_set *ss;
    if (node->is_leaf) {
        this->add(node->name);
        ss = new AWT_species_set(node, this, node->name);
    }
    else {
        AWT_species_set *ls = move_tree_2_ssr(node->get_leftson());
        AWT_species_set *rs = move_tree_2_ssr(node->get_rightson());
        ss = new AWT_species_set(node, this, ls, rs);
        this->add(ss);
    }
    return ss;
}

AWT_species_set *AWT_species_set_root::find_best_matches_info(AP_tree *tree_source, FILE *log, bool compare_node_info) {
    /* Go through all node of the source tree and search for the best
     * matching node in dest_tree (meaning searching ssr->sets)
     * If a match is found, set ssr->sets to this match)
     */
    AWT_species_set *ss = NULL;
    if (tree_source->is_leaf) {
        ss = new AWT_species_set(tree_source, this, tree_source->name);
    }
    else {
        AWT_species_set *ls =      find_best_matches_info(tree_source->get_leftson(),  log, compare_node_info);
        AWT_species_set *rs = ls ? find_best_matches_info(tree_source->get_rightson(), log, compare_node_info) : NULL;

        if (rs) {
            ss = new AWT_species_set(tree_source, this, ls, rs); // Generate new bitstring
            if (compare_node_info) {
                int mismatches = this->search(ss, log); // Search optimal position
                freenull(ss->node->remark_branch);
                if (mismatches) {
                    // the #-sign is important (otherwise TREE_write_Newick will not work correctly; interference with bootstrap values!)
                    ss->node->remark_branch = GBS_global_string_copy("# %i", mismatches);
                }
            }
            else {
                if (tree_source->name) {
                    this->search(ss, log);      // Search optimal position
                }
            }
        }
        delete rs;
        delete ls;
    }
    if (ss) {
        progress->inc();
        if (progress->aborted()) {
            delete ss;
            ss = NULL;
        }
    }
    return ss;                  // return bitstring for this node
}

GB_ERROR AWT_species_set_root::copy_node_information(FILE *log, bool delete_old_nodes, bool nodes_with_marked_only) {
    GB_ERROR error = NULL;

    if (log) fputs("\nDetailed group changes:\n\n", log);

    for (long j=this->nsets-1; j>=0 && !error; j--) {
        AWT_species_set *cset = this->sets[j];

        char *old_group_name  = NULL;
        bool  insert_new_node = cset->best_node && cset->best_node->name;

        if (nodes_with_marked_only && insert_new_node) {
            if (!cset->node->contains_marked_species()) insert_new_node = false;
        }

        if (cset->node->gb_node && (delete_old_nodes || insert_new_node)) { // There is already a node, delete old
            if (cset->node->name == 0) {
                GBDATA *gb_name = GB_entry(cset->node->gb_node, "group_name");
                if (gb_name) {
                    cset->node->name = GB_read_string(gb_name);
                }
                else {
                    cset->node->name = strdup("<gb_node w/o name>");
                }
            }

            old_group_name = strdup(cset->node->name); // store old_group_name to rename new inserted node

            error = GB_delete(cset->node->gb_node);
            if (!error) {
                cset->node->gb_node = 0;
                cset->node->name    = 0;
            }
        }

        if (!error) {
            if (insert_new_node) {
                cset->node->gb_node = GB_create_container(cset->node->get_tree_root()->get_gb_tree(), "node");
                error = GB_copy(cset->node->gb_node, cset->best_node->gb_node);
                if (!error) {
                    GBDATA *gb_name = GB_entry(cset->node->gb_node, "group_name");
                    nt_assert(gb_name);
                    if (gb_name) {
                        char *best_group_name = GB_read_string(gb_name);
                        if (old_group_name) {
                            if (!delete_old_nodes) {
                                if (strcmp(old_group_name, best_group_name) != 0) { // old and new name differ
                                    char *new_group_name = GBS_global_string_copy("%s [was: %s]", best_group_name, old_group_name);
                                    GB_write_string(gb_name, new_group_name);
                                    if (log) fprintf(log, "Destination group '%s' overwritten by '%s'\n", old_group_name, new_group_name);
                                    free(new_group_name);
                                }
                                else {
                                    if (log) fprintf(log, "Group '%s' remains unchanged\n", old_group_name);
                                }
                            }
                            else {
                                if (log) {
                                    if (strcmp(old_group_name, best_group_name) == 0) {
                                        fprintf(log, "Group '%s' remains unchanged\n", old_group_name);
                                    }
                                    else {
                                        fprintf(log, "Destination group '%s' overwritten by '%s'\n", old_group_name, best_group_name);
                                    }
                                }
                            }
                        }
                        else {
                            if (log) fprintf(log, "Group '%s' inserted\n", best_group_name);
                        }
                        free(best_group_name);
                    }
                }
            }
            else {
                if (old_group_name && log) fprintf(log, "Destination group '%s' removed\n", old_group_name);
            }
        }
        free(old_group_name);
    }
    return error;
}

void AWT_species_set_root::finish(GB_ERROR& error) {
    if (!error) error = progress->error_if_aborted();
    progress->done();
}

GB_ERROR AWT_move_info(GBDATA *gb_main, const char *tree_source, const char *tree_dest, const char *log_file, TreeInfoMode mode, bool nodes_with_marked_only) {
    GB_ERROR  error = 0;
    FILE     *log   = 0;

    nt_assert(contradicted(mode == TREE_INFO_COMPARE, log_file));

    if (mode == TREE_INFO_COMPARE) {
        // info is written into 'tree_source'
        // (but we want to modify destination tree - like 'mode node info' does)
        std::swap(tree_source, tree_dest);
    }

    if (log_file) {
        nt_assert(mode == TREE_INFO_COPY || mode == TREE_INFO_ADD);
        log = fopen(log_file, "w");

        fprintf(log,
                "LOGFILE: %s node info\n"
                "\n"
                "     Source tree '%s'\n"
                "Destination tree '%s'\n"
                "\n",
                mode == TREE_INFO_COPY ? "Copying" : "Adding",
                tree_source, tree_dest);
    }

    GB_begin_transaction(gb_main);

    AP_tree_root rsource(new AliView(gb_main), new AP_TreeNodeFactory, NULL, false);
    AP_tree_root rdest  (new AliView(gb_main), new AP_TreeNodeFactory, NULL, false);

    arb_progress progress("Comparing Topologies");

    error             = rsource.loadFromDB(tree_source);
    if (!error) error = rsource.linkToDB(0, 0);
    if (!error) {
        error             = rdest.loadFromDB(tree_dest);
        if (!error) error = rdest.linkToDB(0, 0);
        if (!error) {
            AP_tree *source = rsource.get_root_node();
            AP_tree *dest   = rdest.get_root_node();

            long nspecies     = dest->count_leafs();
            long source_leafs = source->count_leafs();
            long source_nodes = source_leafs*2-1;

            arb_progress compare_progress(source_nodes);
            compare_progress.subtitle("Comparing both trees");

            AWT_species_set_root *ssr = new AWT_species_set_root(gb_main, nspecies, &compare_progress);

            ssr->move_tree_2_ssr(dest);

            if (source_leafs < 3) error = GB_export_error("Destination tree has less than 3 species");
            else {
                AWT_species_set *root_setl =             ssr->find_best_matches_info(source->get_leftson(),  log, mode == TREE_INFO_COMPARE);
                AWT_species_set *root_setr = root_setl ? ssr->find_best_matches_info(source->get_rightson(), log, mode == TREE_INFO_COMPARE) : NULL;

                if (root_setr) {
                    if (mode != TREE_INFO_COMPARE) {
                        compare_progress.subtitle("Copying node information");
                        ssr->copy_node_information(log, mode == TREE_INFO_COPY, nodes_with_marked_only);
                    }

                    long             dummy         = 0;
                    AWT_species_set *new_root_setl = ssr->search(root_setl, &dummy);
                    AWT_species_set *new_root_setr = ssr->search(root_setr, &dummy);
                    AP_tree         *new_rootl     = new_root_setl->node;
                    AP_tree         *new_rootr     = new_root_setr->node;

                    new_rootl->set_root(); // set root correctly
                    new_rootr->set_root(); // set root correctly

                    compare_progress.subtitle("Saving trees");

                    AP_tree *root = new_rootr->get_root_node();


                    error             = GBT_overwrite_tree(rdest.get_gb_tree(), root);
                    if (!error) error = GBT_overwrite_tree(rsource.get_gb_tree(), source);

                    if (!error) {
                        char *entry;
                        if (mode == TREE_INFO_COMPARE) {
                            entry = GBS_global_string_copy("Compared topology with %s", tree_source);
                        }
                        else {
                            const char *copiedOrAdded = mode == TREE_INFO_COPY ? "Copied" : "Added";

                            entry = GBS_global_string_copy("%s node info %sfrom %s",
                                                           copiedOrAdded,
                                                           nodes_with_marked_only ? "of marked " : "",
                                                           tree_source);
                        }
                        GBT_log_to_tree_remark(rdest.get_gb_tree(), entry);
                        free(entry);
                    }
                }

                delete root_setl;
                delete root_setr;
            }

            ssr->finish(error);
            delete ssr;
        }
    }

    if (log) {
        if (error) fprintf(log, "\nError: %s\n", error);       // write error to log as well
        fclose(log);
    }

    return GB_end_transaction(gb_main, error);
}

