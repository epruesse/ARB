// ================================================================ //
//                                                                  //
//   File      : di_foundclusters.hxx                               //
//   Purpose   : Store results of cluster detection                 //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2009   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef DI_FOUNDCLUSTERS_HXX
#define DI_FOUNDCLUSTERS_HXX

#ifndef GUI_ALIVIEW_HXX
#include <gui_aliview.hxx>
#endif

#ifndef _CPP_SET
#include <set>
#endif
#ifndef _CPP_MAP
#include <map>
#endif
#ifndef _CPP_VECTOR
#include <vector>
#endif
#ifndef _CPP_ALGORITHM
#include <algorithm>
#endif
#ifndef _CPP_STRING
#include <string>
#endif

#define cl_assert(cond) arb_assert(cond)

class ClusterTree;
class ARB_countedTree;
class AW_selection_list;
class AW_window;

// ---------------------
//      Cluster

typedef std::set<GBDATA*>          SpeciesSet;
typedef SpeciesSet::const_iterator SpeciesSetIter;

typedef long ID;

enum ClusterOrder { SORT_BY_MEANDIST };

class Cluster {
    double min_dist;                                // min. distance between species inside Cluster
    double max_dist;                                // dito, but max.
    double mean_dist;                               // dito, but mean

    GBDATA     *representative;                     // cluster member with lowest mean distance
    SpeciesSet  members;                            // all members (including representative)

    std::string  name;                              // cluster name
    std::string *next_name;                         // proposed new name (call accept_name() to accept it)

    ID id;                                          // unique id for this cluster (used in AWAR_CLUSTER_SELECTED)

    static ID unused_id;

public:
    Cluster(ClusterTree *ct);
    ~Cluster() { delete next_name; }

    ID get_ID() const { return id; }

    bool lessByOrder(const Cluster& other, ClusterOrder sortBy) {
        cl_assert(sortBy == SORT_BY_MEANDIST); // no other order impl. atm
        return mean_dist < other.mean_dist;
    }

    size_t get_member_count() const { return members.size(); }
    const char *description() const;

    const SpeciesSet& get_members() const { return members; }

    void mark_all_members(bool mark_representative) const;
    GBDATA *get_representative() const { return representative; }

    void        generate_name(const ARB_countedTree *ct);
    std::string build_group_name(const ARB_countedTree *ct);

    void propose_name(const std::string& newName) {
        delete next_name;
        next_name = new std::string(newName);
    }
    void accept_name(bool accept) {
        if (accept && next_name) name = *next_name;

        delete next_name;
        next_name = NULL;
    }
};

typedef SmartPtr<Cluster>             ClusterPtr;
typedef std::map<ID, ClusterPtr>      KnownClusters;
typedef KnownClusters::const_iterator KnownClustersIter;
typedef std::vector<ID>               ClusterIDs;
typedef ClusterIDs::const_iterator    ClusterIDsIter;

// --------------------
//      global data

enum ClusterSubset {
    STORED_CLUSTERS,
    SHOWN_CLUSTERS,
};

struct ClustersData {
    WeightedFilter    &weighted_filter;
    AW_selection_list *clusterList;
    KnownClusters      known_clusters;              // contains all known clusters
    ClusterIDs         shown;                       // clusters shown in selection list
    ClusterIDs         stored;                      // stored clusters
    ClusterOrder       order;                       // order of 'shown'
    bool               sort_needed;                 // need to sort 'shown'

    ClusterIDs& get_subset(ClusterSubset subset) {
        if (subset == SHOWN_CLUSTERS) {
            // @@@ sort here if needed
            return shown;
        }
        return stored;
    }
    ID idAtPos(int pos, ClusterSubset subset) {
        ClusterIDs& ids = get_subset(subset);
        return size_t(pos)<ids.size() ? ids.at(pos) : 0;
    }

public:

    ClustersData(WeightedFilter& weighted_filter_)
        : weighted_filter(weighted_filter_)
        , clusterList(0)
        , order(SORT_BY_MEANDIST)
        , sort_needed(true)
    {}

    void changeSortOrder(ClusterOrder newOrder) {
        if (order != newOrder) {
            order       = newOrder;
            sort_needed = true;
        }
    }

    ClusterPtr clusterWithID(ID id) const {
        KnownClustersIter found = known_clusters.find(id);
        return found == known_clusters.end() ? ClusterPtr() : found->second;
    }

    ClusterPtr clusterAtPos(int pos, ClusterSubset subset) { return clusterWithID(idAtPos(pos, subset)); }
    size_t count(ClusterSubset subset) { return get_subset(subset).size(); }
    const ClusterIDs& get_clusterIDs(ClusterSubset subset) { return get_subset(subset); }

    int get_pos(ID id, ClusterSubset subset) {
        // returns -1 of not member of subset
        ClusterIDs&     ids   = get_subset(subset);
        ClusterIDsIter  begin = ids.begin();
        ClusterIDsIter  end   = ids.end();
        ClusterIDsIter  found = find(begin, end, id);

        return found == end ? -1 : distance(begin, found);
    }
    int get_pos(ClusterPtr cluster, ClusterSubset subset) { return get_pos(cluster->get_ID(), subset); }

    void add(ClusterPtr clus, ClusterSubset subset);
    void remove(ClusterPtr clus, ClusterSubset subset);
    void clear(ClusterSubset subset);

    void store(ID id);

    void store_all();
    void restore_all();
    void swap_all();

    void update_selection_list(AW_window *aww);

    GBDATA *get_gb_main() const { return weighted_filter.get_gb_main(); }
    AW_root *get_aw_root() const { return weighted_filter.get_aw_root(); }

    void free(AW_window *aww);
};

#else
#error di_foundclusters.hxx included twice
#endif // DI_FOUNDCLUSTERS_HXX
