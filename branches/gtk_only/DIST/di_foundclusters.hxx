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
#ifndef SMARTPTR_H
#include <smartptr.h>
#endif


#ifndef _GLIBCXX_SET
#include <set>
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif
#ifndef _GLIBCXX_ALGORITHM
#include <algorithm>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif
#ifndef DBITEM_SET_H
#include <dbitem_set.h>
#endif

#define cl_assert(cond) arb_assert(cond)

class  ClusterTree;
class  ARB_tree_predicate;
struct ARB_countedTree;
class  AW_selection_list;
class  AW_window;

// ---------------------
//      Cluster

typedef int32_t ID;

enum ClusterOrder {
    UNSORTED             = 0, 
    SORT_BY_MEANDIST     = 1,
    SORT_BY_MIN_BASES    = 2,
    SORT_BY_CLUSTERSIZE  = 3,
    SORT_BY_TREEPOSITION = 4,
    SORT_BY_MIN_DIST     = 5,
    SORT_BY_MAX_DIST     = 6,

    SORT_REVERSE = 1<<3,                              // bitflag!
};

class DisplayFormat;

class Cluster : virtual Noncopyable {
    double min_dist;                                // min. distance between species inside Cluster
    double max_dist;                                // dito, but max.
    double mean_dist;                               // dito, but mean

    double min_bases;                               // min bases used for sequence distance
    double rel_tree_pos;                            // relative position in tree [0.0 .. 1.0]

    GBDATA    *representative;                      // cluster member with lowest mean distance
    DBItemSet  members;                             // all members (including representative)

    std::string  desc;                              // cluster description
    std::string *next_desc;                         // proposed new description (call accept_proposed() to accept it)

    ID id;                                          // unique id for this cluster (used in AWAR_CLUSTER_SELECTED)

    static ID unused_id;

public:
    Cluster(ClusterTree *ct);
    ~Cluster() { delete next_desc; }

    ID get_ID() const { return id; }

    size_t get_member_count() const { return members.size(); }
    void scan_display_widths(DisplayFormat& format) const;
    const char *get_list_display(const DisplayFormat *format) const;

    const DBItemSet& get_members() const { return members; }

    void mark_all_members(bool mark_representative) const;
    GBDATA *get_representative() const { return representative; }

    std::string create_description(const ARB_countedTree *ct);
    std::string get_upgroup_info(const ARB_countedTree *ct, const ARB_tree_predicate& keep_group_name);
    double get_mean_distance() const { return mean_dist; }

    void propose_description(const std::string& newDesc) {
        delete next_desc;
        next_desc = new std::string(newDesc);
    }
    void update_description(const ARB_countedTree *ct) {
        propose_description(create_description(ct));
    }
    void accept_proposed(bool accept) {
        if (accept && next_desc) {
            desc = *next_desc;
        }

        delete next_desc;
        next_desc = NULL;
    }

private:
    bool lessByOrder_forward(const Cluster& other, ClusterOrder sortBy) const {
        bool less = false;
        switch (sortBy) {
            case UNSORTED:              break;
            case SORT_BY_MEANDIST:      less = mean_dist < other.mean_dist; break;
            case SORT_BY_MIN_BASES:     less = min_bases < other.min_bases; break;
            case SORT_BY_CLUSTERSIZE:   less = members.size() < other.members.size(); break;
            case SORT_BY_TREEPOSITION:  less = rel_tree_pos < other.rel_tree_pos;  break;
            case SORT_BY_MIN_DIST:      less = min_dist < other.min_dist; break;
            case SORT_BY_MAX_DIST:      less = max_dist < other.max_dist; break;
                
            case SORT_REVERSE:
                cl_assert(0);
                break;
        }
        return less;
    }
public:
    bool lessByOrder(const Cluster& other, ClusterOrder sortBy) const {
        bool less;
        if (sortBy&SORT_REVERSE) {
            less = other.lessByOrder_forward(*this, ClusterOrder(sortBy^SORT_REVERSE));
        }
        else {
            less = lessByOrder_forward(other, sortBy);
        }
        return less;
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

struct ClustersData : virtual Noncopyable {
    WeightedFilter    &weighted_filter;
    AW_selection_list *clusterList;
    KnownClusters      known_clusters;              // contains all known clusters
    ClusterIDs         shown;                       // clusters shown in selection list
    ClusterIDs         stored;                      // stored clusters
    ClusterOrder       criteria[2];                 // order of 'shown'
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
        , sort_needed(true)
    {
        criteria[0] = SORT_BY_MEANDIST;
        criteria[1] = UNSORTED;
    }

    void changeSortOrder(ClusterOrder newOrder) {
        if (newOrder == SORT_REVERSE) { // toggle reverse
            criteria[0] = ClusterOrder(criteria[0]^SORT_REVERSE);
        }
        else if (newOrder != criteria[0]) {
            criteria[1] = criteria[0];
            criteria[0] = newOrder;
        }
        sort_needed  = true;
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

    void update_cluster_selection_list();

    GBDATA *get_gb_main() const { return weighted_filter.get_gb_main(); }
    AW_root *get_aw_root() const { return weighted_filter.get_aw_root(); }

    void free();
};

char *originalGroupName(const char *groupname);

#else
#error di_foundclusters.hxx included twice
#endif // DI_FOUNDCLUSTERS_HXX
