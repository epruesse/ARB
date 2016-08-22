// =============================================================== //
//                                                                 //
//   File      : DI_clustertree.cxx                                //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "di_clustertree.hxx"
#include <arb_progress.h>
#include <set>
#include <cmath>
#include <limits>

using namespace std;

typedef std::set<LeafRelation> SortedPairValues; // iterator runs from small to big values

// ------------------------
//      ClusterTreeRoot


ClusterTreeRoot::ClusterTreeRoot(AliView *aliview, AP_sequence *seqTemplate_, AP_FLOAT maxDistance_, size_t minClusterSize_)
    : ARB_seqtree_root(aliview, new ClusterTreeNodeFactory, seqTemplate_, false),
      maxDistance(maxDistance_),
      minClusterSize(minClusterSize_)
{}

#if defined(DEBUG)
struct ClusterStats {
    size_t clusters;
    size_t subclusters;
    size_t loadedSequences;

    ClusterStats()
        : clusters(0)
        , subclusters(0)
        , loadedSequences(0)
    {}
};

static void collectStats(ClusterTree *node, ClusterStats *stats) {
    switch (node->get_state()) {
        case CS_IS_CLUSTER: stats->clusters++; break;
        case CS_SUB_CLUSTER: stats->subclusters++; break;
        default: break;
    }
    if (node->is_leaf) {
        AP_sequence *seq = node->get_seq();
        if (seq->got_sequence()) stats->loadedSequences++;
    }
    else {
        collectStats(node->get_leftson(), stats);
        collectStats(node->get_rightson(), stats);
    }
}
#endif // DEBUG

GB_ERROR ClusterTreeRoot::find_clusters() {
    ClusterTree *root = get_root_node();

    root->init_tree();
#if defined(DEBUG)
    printf("----------------------------------------\n");
    printf("maxDistance:       %f\n", maxDistance);
    printf("minClusterSize:    %u\n", minClusterSize);
    printf("Leafs in tree:     %u\n", root->get_leaf_count());
    printf("Possible clusters: %u\n", root->get_cluster_count());
#endif // DEBUG

    arb_progress cluster_progress(root->get_cluster_count());
    cluster_progress.auto_subtitles("Cluster");

    GB_ERROR error = NULL;

    try {
        root->detect_clusters(cluster_progress);
    }
    catch (const char *msg) { error = msg; }
    catch (...) { cl_assert(0); }

    if (error) {
        // avoid further access after error
        change_root(root, NULL);
        delete root;
    }
    else {
#if defined(DEBUG)
        ClusterStats stats;
        collectStats(root, &stats);

        printf("Found clusters:::::::: %zu\n", stats.clusters);
        printf("Found subclusters::::: %zu\n", stats.subclusters);
        printf("Loaded sequences:::::: %zu (%5.2f%%)\n",
               stats.loadedSequences,
               (100.0*stats.loadedSequences)/root->get_leaf_count());

#if defined(TRACE_DIST_CALC)
        size_t distances         = root->get_calculated_distances();
        size_t leafs             = root->get_leaf_count();
        size_t existingDistances = (leafs*leafs)/2-1;

        printf("Calculated distances:: %zu (%5.2f%%)\n",
               distances,
               (100.0*distances)/existingDistances);
#endif                                              // TRACE_DIST_CALC

#endif                                              // DEBUG
    }

    return error;
}

// --------------------
//      ClusterTree

ClusterTree *ClusterTree::get_cluster(size_t num) {
    cl_assert(num < get_cluster_count());

    if (num == 0) return this;

    ClusterTree *lson = get_leftson();
    if (lson->get_cluster_count() <= num) {
        return get_rightson()->get_cluster(num - lson->get_cluster_count());
    }
    return lson->get_cluster(num);
}

void ClusterTree::init_tree() {
    cl_assert(state == CS_UNKNOWN);

#if defined(TRACE_DIST_CALC)
    calculatedDistances = 0;
#endif // TRACE_DIST_CALC

    if (get_father()) {
        depth = get_father()->get_depth()+1;
    }
    else {
        depth = 1;
        cl_assert(is_root_node());
    }

    if (is_leaf) {
        leaf_count = 1;
        clus_count = 0;
        state      = CS_TOO_SMALL;
    }
    else {
        ClusterTree *lson = get_leftson();
        ClusterTree *rson = get_rightson();

        lson->init_tree();
        rson->init_tree();

        leaf_count = lson->get_leaf_count() + rson->get_leaf_count();

        ClusterTreeRoot *troot = get_tree_root();
        if (leaf_count<troot->get_minClusterSize()) {
            state      = CS_TOO_SMALL;
            clus_count = 0;
        }
        else {
            state      = CS_MAYBE_CLUSTER;
            clus_count = lson->get_cluster_count() + rson->get_cluster_count() + 1;
            min_bases  = numeric_limits<AP_FLOAT>::infinity();
        }
    }
    
    cl_assert(state != CS_UNKNOWN);
}

void ClusterTree::detect_clusters(arb_progress& progress) {
    if (state == CS_MAYBE_CLUSTER) {
        cl_assert(!is_leaf);

        ClusterTree *lson = get_leftson();
        ClusterTree *rson = get_rightson();

        lson->detect_clusters(progress);
        rson->detect_clusters(progress);

#if defined(TRACE_DIST_CALC)
        calculatedDistances += get_leftson()->calculatedDistances;
        calculatedDistances += get_rightson()->calculatedDistances;
#endif // TRACE_DIST_CALC

        if (lson->get_state() == CS_NO_CLUSTER || rson->get_state() == CS_NO_CLUSTER) {
            // one son is no cluster -> can't be cluster myself
            state = CS_NO_CLUSTER;
        }
        else {
            state = CS_IS_CLUSTER;                  // assume this is a cluster

            // Get set of branches (sorted by branch distance)

            get_branch_dists();                     // calculates branchDists
            SortedPairValues pairsByBranchDistance; // biggest branch distances at end
            {
                LeafRelationCIter bd_end = branchDists->end();
                for (LeafRelationCIter bd = branchDists->begin(); bd != bd_end; ++bd) {
                    pairsByBranchDistance.insert(LeafRelation(bd->first, bd->second));
                }
            }

            cl_assert(pairsByBranchDistance.size() == possible_relations());

            // calculate real sequence distance
            // * stop when distance > maxDistance is detected
            // * calculate longest branchdistance first
            //   (Assumption is: big branch distance <=> big sequence distance)

            AP_FLOAT maxDistance       = get_tree_root()->get_maxDistance();
            AP_FLOAT worstDistanceSeen = -1.0;

            cl_assert(!sequenceDists);
            sequenceDists = new LeafRelations;

            SortedPairValues::const_reverse_iterator pair_end = pairsByBranchDistance.rend();
            for (SortedPairValues::const_reverse_iterator pair = pairsByBranchDistance.rbegin(); pair != pair_end; ++pair) {
                AP_FLOAT realDist = get_seqDist(pair->get_pair());

                if (realDist>worstDistanceSeen) {
                    worstDistanceSeen = realDist;

                    delete worstKnownDistance;
                    worstKnownDistance = new TwoLeafs(pair->get_pair());

                    if (realDist>maxDistance) {     // big distance found -> not a cluster
                        state = CS_NO_CLUSTER;
                        break;
                    }
                }
            }

#if defined(TRACE_DIST_CALC)
            calculatedDistances += sequenceDists->size();
#endif // TRACE_DIST_CALC

            if (state == CS_IS_CLUSTER) {
#if defined(DEBUG)
                // test whether all distances have been calculated
                cl_assert(!is_leaf);
                size_t knownDistances = sequenceDists->size() +
                    get_leftson()->known_seqDists() +
                    get_rightson()->known_seqDists();

                cl_assert(knownDistances == possible_relations());
#endif // DEBUG

                get_leftson()->state  = CS_SUB_CLUSTER;
                get_rightson()->state = CS_SUB_CLUSTER;
            }

        }

        cl_assert(state == CS_NO_CLUSTER || state == CS_IS_CLUSTER); // detection failure

        lson->oblivion(false);
        rson->oblivion(false);

        progress.inc();
        if (progress.aborted()) throw "aborted on userrequest";
    }

    cl_assert(state != CS_MAYBE_CLUSTER);
}

void ClusterTree::oblivion(bool forgetDistances) {
    delete branchDepths;
    branchDepths = NULL;

    delete branchDists;
    branchDists = NULL;

    if (state == CS_NO_CLUSTER) {
        if (forgetDistances) {
            delete sequenceDists;
            sequenceDists = NULL;
        }
        forgetDistances = true;
    }
    else {
        cl_assert(state & (CS_IS_CLUSTER|CS_SUB_CLUSTER|CS_TOO_SMALL));
    }

    if (!is_leaf) {
        get_leftson()->oblivion(forgetDistances);
        get_rightson()->oblivion(forgetDistances);
    }
}

void ClusterTree::calc_branch_depths() {
    cl_assert(!branchDepths);
    branchDepths = new NodeValues;
    if (is_leaf) {
        (*branchDepths)[this] = 0.0;
    }
    else {
        for (int s = 0; s<2; s++) {
            const NodeValues *sonDepths = s ? get_leftson()->get_branch_depths() : get_rightson()->get_branch_depths();
            AP_FLOAT          lenToSon  = s ? leftlen : rightlen;

            NodeValues::const_iterator sd_end = sonDepths->end();
            for (NodeValues::const_iterator sd = sonDepths->begin(); sd != sd_end; ++sd) {
                (*branchDepths)[sd->first] = sd->second+lenToSon;
            }
        }
    }
}

void ClusterTree::calc_branch_dists() {
    cl_assert(!branchDists);
    if (!is_leaf) {
        branchDists = new LeafRelations;

        for (int s = 0; s<2; s++) {
            ClusterTree         *son      = s ? get_leftson() : get_rightson();
            const LeafRelations *sonDists = son->get_branch_dists();

            cl_assert(sonDists || son->is_leaf);
            if (sonDists) branchDists->insert(sonDists->begin(), sonDists->end());
        }

        const NodeValues *ldepths = get_leftson()->get_branch_depths();
        const NodeValues *rdepths = get_rightson()->get_branch_depths();

        NodeValues::const_iterator l_end = ldepths->end();
        NodeValues::const_iterator r_end = rdepths->end();

        for (NodeValues::const_iterator ld = ldepths->begin(); ld != l_end; ++ld) {
            AP_FLOAT llen = ld->second+leftlen;
            for (NodeValues::const_iterator rd = rdepths->begin(); rd != r_end; ++rd) {
                AP_FLOAT rlen = rd->second+rightlen;

                (*branchDists)[TwoLeafs(ld->first, rd->first)] = llen+rlen;
            }
        }

        cl_assert(branchDists->size() == possible_relations());
    }
}

AP_FLOAT ClusterTree::get_seqDist(const TwoLeafs& pair) {
    // searches or calculates the distance between the sequences of leafs in 'pair'

    const AP_FLOAT *found = has_seqDist(pair);
    if (!found) {
        const ClusterTree *commonFather = pair.first()->commonFatherWith(pair.second());

        while (commonFather != this) {
            found = commonFather->has_seqDist(pair);
            if (found) break;

            commonFather = commonFather->get_father();
            cl_assert(commonFather); // first commonFather was not inside this!
        }
    }

    if (!found) {
        // calculate the distance

        const AP_sequence *seq1 = pair.first()->get_seq();
        const AP_sequence *seq2 = pair.second()->get_seq();

        seq1->lazy_load_sequence();
        seq2->lazy_load_sequence();

        {
            AP_sequence *ancestor     = seq1->dup();
            AP_FLOAT     mutations    = ancestor->combine(seq1, seq2);
            AP_FLOAT     wbc1         = seq1->weighted_base_count();
            AP_FLOAT     wbc2         = seq2->weighted_base_count();
            AP_FLOAT     minBaseCount = wbc1 < wbc2 ? wbc1 : wbc2;
            AP_FLOAT     dist;

            if (minBaseCount>0) {
                dist = mutations / minBaseCount;
                if (minBaseCount < min_bases) min_bases = minBaseCount;
            }
            else { // got at least one empty sequence -> no distance
                dist      = 0.0;
                min_bases = minBaseCount;
            }

#if defined(DEBUG) && 0
            const char *name1 = pair.first()->name;
            const char *name2 = pair.second()->name;
            printf("%s <-?-> %s: mutations=%5.2f wbc1=%5.2f wbc2=%5.2f mbc=%5.2f dist=%5.2f\n",
                   name1, name2, mutations, wbc1, wbc2, minBaseCount, dist);
#endif // DEBUG

            cl_assert(!is_nan_or_inf(dist));
            
            (*sequenceDists)[pair] = dist;
            delete ancestor;
        }

        found = has_seqDist(pair);
        cl_assert(found);
    }

    cl_assert(found);
    return *found;
}

const AP_FLOAT *ClusterTree::has_seqDist(const TwoLeafs& pair) const {
    if (sequenceDists) {
        LeafRelationCIter found = sequenceDists->find(pair);
        if (found != sequenceDists->end()) {
            return &(found->second);
        }
    }
    return NULL;
}

const ClusterTree *ClusterTree::commonFatherWith(const ClusterTree *other) const {
    int depth_diff = get_depth() - other->get_depth();
    if (depth_diff) {
        if (depth_diff<0) { // this is less deep than other
            return other->get_father()->commonFatherWith(this);
        }
        else { // other is less deep than this
            return get_father()->commonFatherWith(other);
        }
    }
    else { // both at same depth
        if (this == other) { // common father reached ?
            return this;
        }
        else { // otherwise check fathers
            return get_father()->commonFatherWith(other->get_father());
        }
    }
}

