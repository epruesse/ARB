// ================================================================ //
//                                                                  //
//   File      : DI_foundclusters.cxx                               //
//   Purpose   : Store results of cluster detection                 //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2009   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "di_foundclusters.hxx"
#include "di_clustertree.hxx"

#include <aw_window.hxx>


using namespace std;

ID Cluster::unused_id = 1;

static AP_FLOAT calc_mean_dist_to(const ClusterTree *distance_to, const LeafRelations *distances) {
    size_t   count = 0;
    AP_FLOAT sum   = 0.0;

    LeafRelationCIter dist_end = distances->end();
    for (LeafRelationCIter dist = distances->begin(); dist != dist_end; ++dist) {
        const TwoLeafs& leafs = dist->first;

        if (leafs.first() == distance_to || leafs.second() == distance_to) {
            count++;
            sum += dist->second;
        }
    }

    cl_assert(count);                               // 'distance_to' does not occur in 'distances'

    return sum/count;
}

Cluster::Cluster(ClusterTree *ct)
    : next_name(NULL)
{
    cl_assert(ct->get_state() == CS_IS_CLUSTER);

    id = unused_id++;

    const LeafRelations *distances = ct->get_sequence_dists();
    cl_assert(distances);

    LeafRelationCIter dist     = distances->begin();
    LeafRelationCIter dist_end = distances->end();

    cl_assert(dist != dist_end); // oops - empty cluster

    min_dist  = max_dist = dist->second;
    mean_dist = 0.0;

    AP_FLOAT min_mean_dist = 9999999999.0;
    representative         = NULL;

    for (; dist != dist_end; ++dist) {
        const TwoLeafs& leafs = dist->first;
        AP_FLOAT        di    = dist->second;

        if (di<min_dist) min_dist = di;
        if (di>max_dist) max_dist = di;

        mean_dist += di;

        for (int i = 0; i<2; ++i) {
            const ClusterTree *member     = i ? leafs.first() : leafs.second();
            GBDATA            *gb_species = member->gb_node;

            cl_assert(gb_species);
            if (members.find(gb_species) == members.end()) { // not in set
                members.insert(gb_species);
                AP_FLOAT mean_dist_to_member = calc_mean_dist_to(member, distances);

                if (mean_dist_to_member<min_mean_dist) {
                    representative = gb_species;
                    min_mean_dist  = mean_dist_to_member;
                }
            }
        }

    }

    mean_dist /= distances->size();
    cl_assert(representative);
    generate_name(ct);
}

static string *get_downgroups(const ARB_countedTree *ct, size_t& group_members) {
    string *result = NULL;
    if (ct->is_leaf) {
        group_members = 0;
    }
    else {
        if (ct->gb_node) {
            group_members = ct->get_leaf_count();
            result        = new string(ct->name);
        }
        else {
            string *leftgroups  = get_downgroups(ct->get_leftson(), group_members);
            size_t  right_members;
            string *rightgroups = get_downgroups(ct->get_rightson(), right_members);

            group_members += right_members;

            if (leftgroups || rightgroups) {
                if (!leftgroups)       { result = rightgroups; rightgroups = NULL; }
                else if (!rightgroups) { result = leftgroups; leftgroups = NULL; }
                else                     result = new string(*leftgroups+"+"+*rightgroups);
                
                delete leftgroups;
                delete rightgroups;
            }
        }
    }
    return result;
}
static const char *get_upgroup(const ARB_countedTree *ct, const ARB_countedTree*& upgroupPtr) {
    const char *name = NULL;
    if (ct->gb_node) {
        upgroupPtr = ct;
        name    = ct->name;
    }
    else {
        const ARB_countedTree *father = ct->get_father();
        if (father) {
            name = get_upgroup(father, upgroupPtr);
        }
        else {
            upgroupPtr = ct;
            name       = "WHOLE_TREE";
        }
    }

    cl_assert(name);
    return name;
}

void Cluster::generate_name(const ARB_countedTree *ct) {
    cl_assert(!ct->is_leaf);

    if (ct->gb_node) {                              // cluster root is a group
        name = ct->name;
    }
    else {
        size_t                 downgroup_members;
        string                *downgroup_names = get_downgroups(ct, downgroup_members);
        const ARB_countedTree *upgroup         = NULL;
        const char            *upgroup_name    = get_upgroup(ct, upgroup);
        size_t                 upgroup_members = upgroup->get_leaf_count();

        size_t cluster_members = ct->get_leaf_count();

        cl_assert(upgroup_members>0); // if no group, whole tree should have been returned

        AP_FLOAT up_rel   = (AP_FLOAT)cluster_members/upgroup_members; // used for: xx% of upgroup (big is good)
        AP_FLOAT down_rel = (AP_FLOAT)downgroup_members/cluster_members; // used for: downgroup(s) + (1-xx)% outgroup (big is good)

        if (up_rel>down_rel) { // prefer up_percent
            name = string(GBS_global_string("%4.1f%% of %s", up_rel*100.0, upgroup_name));
        }
        else {
            name = string(GBS_global_string("%s + %4.1f%% outgroup", downgroup_names->c_str(), (1-down_rel)*100.0));
        }

        delete downgroup_names;
    }
}

string Cluster::build_group_name(const ARB_countedTree *ct) {
    // similar to generate_name()
    // - contains relative position in up-group (instead of percentages)

    string group_name;

    cl_assert(!ct->is_leaf);
    if (ct->gb_node) {                              // cluster root is a group
        group_name = ct->name;
    }
    else {
        const ARB_countedTree *upgroup         = NULL;
        const char            *upgroup_name    = get_upgroup(ct, upgroup);
        size_t                 upgroup_members = upgroup->get_leaf_count();
        size_t                 cluster_members = ct->get_leaf_count();

        size_t cluster_pos1 = ct->relative_position_in(upgroup)+1; // range [1..upgroup_members]
        size_t cluster_posN = cluster_pos1+cluster_members-1;

        group_name = GBS_global_string("%i-%i/%i_%s", cluster_pos1, cluster_posN, upgroup_members, upgroup_name);
    }
    return group_name;
}

const char *Cluster::description() const {
    return GBS_global_string("%3zu  dist: %5.3f [%5.3f - %5.3f]  %s",
                             get_member_count(),
                             mean_dist*100.0, 
                             min_dist*100.0, 
                             max_dist*100.0,
                             name.c_str());
}


// ---------------------
//      ClustersData

void ClustersData::add(ClusterPtr clus, ClusterSubset subset) {
    known_clusters[clus->get_ID()] = clus;
    get_subset(subset).push_back(clus->get_ID());
}

void ClustersData::remove(ClusterPtr clus, ClusterSubset subset) {
    int pos = get_pos(clus, subset);
    if (pos != -1) {
        ClusterIDs&           ids      = get_subset(subset);
        ClusterIDs::iterator  toRemove = ids.begin();
        advance(toRemove, pos);
        ids.erase(toRemove);
        known_clusters.erase(clus->get_ID());
    }
}

void ClustersData::clear(ClusterSubset subset) {
    ClusterIDs&     ids    = get_subset(subset);
    ClusterIDsIter  id_end = ids.end();
    for (ClusterIDsIter  id = ids.begin(); id != id_end; ++id) {
        known_clusters.erase(*id);
    }
    ids.clear();
}

void ClustersData::store(ID id) {
    ClusterPtr toStore = clusterWithID(id);
    remove(toStore, SHOWN_CLUSTERS);
    add(toStore, STORED_CLUSTERS);
}

void ClustersData::store_all() {
    copy(shown.begin(), shown.end(), back_insert_iterator<ClusterIDs>(stored));
    shown.clear();
    sort_needed = false;
}

void ClustersData::restore_all() {
    copy(stored.begin(), stored.end(), back_insert_iterator<ClusterIDs>(shown));
    stored.clear();
    sort_needed = true;
}

void ClustersData::swap_all() {
    swap(stored, shown);
    sort_needed = true;
}


class SortClusterIDsBy {
    const KnownClusters& known;
    ClusterOrder         order;
public:
    SortClusterIDsBy(const KnownClusters& known_, ClusterOrder order_)
        : known(known_)
        , order(order_)
    {}

     bool operator()(ID id1, ID id2) const {
         ClusterPtr cl1 = known.find(id1)->second;
         ClusterPtr cl2 = known.find(id2)->second;

         cl_assert(!cl1.Null() && !cl2.Null());

         return cl1->lessByOrder(*cl2, order);
     }
};

void ClustersData::update_selection_list(AW_window *aww) {
    cl_assert(clusterList);
    aww->clear_selection_list(clusterList);

    if (shown.empty()) {
        aww->insert_default_selection(clusterList, "<No clusters detected>", ID(0));
    }
    else {
        {
            SortClusterIDsBy sortBy(known_clusters, order);
            sort(shown.begin(), shown.end(), sortBy);
        }
        {
            ClusterIDsIter cl     = shown.begin();
            ClusterIDsIter cl_end = shown.end();

            for (; cl != cl_end; ++cl) {
                ID         id      = *cl;
                ClusterPtr cluster = clusterWithID(id);

                aww->insert_selection(clusterList, cluster->description(), cluster->get_ID());
            }
            aww->insert_default_selection(clusterList, "<select no cluster>", ID(0));
        }
    }
    aww->update_selection_list(clusterList);
}

void ClustersData::free(AW_window *aww) {
    // delete calculated/stored data

    shown.clear();
    stored.clear();
    update_selection_list(aww);
    known_clusters.clear();
}



