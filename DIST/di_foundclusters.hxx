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

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef GUI_ALIVIEW_HXX
#include <gui_aliview.hxx>
#endif


#ifndef SMARTPTR_H
#include <smartptr.h>
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

    std::string name;                               // cluster name

    ID id;                                          // unique id for this cluster (used in AWAR_CLUSTER_SELECTED)

    static ID unused_id;

    void generate_name(const ClusterTree *ct);

public:
    Cluster(ClusterTree *ct);

    ID get_ID() const { return id; }

    bool lessByOrder(const Cluster& other, ClusterOrder sortBy) {
        cl_assert(sortBy == SORT_BY_MEANDIST); // no other order impl. atm
        return mean_dist < other.mean_dist;
    }

    size_t get_member_count() const { return members.size()+1; }
    const char *description() const;

    void mark_all_members(bool mark_representative) const;
    GBDATA *get_representative() const { return representative; }
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
    UNKNOWN_CLUSTERS,
};

struct ClustersData {
    WeightedFilter    &weighted_filter;
    AW_selection_list *clusterList;
    KnownClusters      known_clusters;              // contains all known clusters
    ClusterIDs         shown;                       // clusters shown in selection list
    ClusterIDs         stored;                      // stored clusters
    ClusterOrder       order;                       // order of 'shown'
    bool               sort_needed;                 // need to sort 'shown'

    ClusterIDs *get_subset(ClusterSubset subset) {
        switch (subset) {
            case SHOWN_CLUSTERS: return &shown;
            case STORED_CLUSTERS: return &stored;
            case UNKNOWN_CLUSTERS: cl_assert(0); break;
        }
        return NULL;
    }
    ID idAtPos(int pos, ClusterSubset subset) {
        ClusterIDs *ids = get_subset(subset);
        return size_t(pos)<ids->size() ? ids->at(pos) : 0;
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

    ClusterPtr clusterAtPos(int pos, ClusterSubset subset) {
        return clusterWithID(idAtPos(pos, subset));
    }
    size_t count(ClusterSubset subset) {
        return get_subset(subset)->size();
    }

    int get_pos(ID id, ClusterSubset subset) {
        // returns -1 of not member of subset
        ClusterIDs     *ids   = get_subset(subset);
        ClusterIDsIter  begin = ids->begin();
        ClusterIDsIter  end   = ids->end();
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
