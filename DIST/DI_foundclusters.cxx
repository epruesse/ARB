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

#include <cmath>

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

struct UseAnyTree : public ARB_tree_predicate {
    bool selects(const ARB_tree&) const { return true; }
};
    
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

    cl_assert(members.size() == ct->get_leaf_count());

    min_bases = ct->get_min_bases();
    {
        ClusterTree *root    = ct->get_root_node();
        size_t       allSize = root->get_leaf_count();
        size_t       size    = ct->get_leaf_count();
        size_t       first   = ct->relative_position_in(root);
        size_t       last    = first+size-1;

        rel_tree_pos = ((double(first)+last)/2) / allSize;
    }
    mean_dist /= distances->size();
    cl_assert(representative);
    generate_name(ct, UseAnyTree());
}

static string *get_downgroups(const ARB_countedTree *ct, const ARB_tree_predicate& keep_group_name, size_t& group_members) {
    string *result = NULL;
    if (ct->is_leaf) {
        group_members = 0;
    }
    else {
        const char *group_name = ct->group_name();

        if (group_name && keep_group_name.selects(*ct)) {
            group_members = ct->get_leaf_count();
            result        = new string(ct->name);
        }
        else {
            string *leftgroups  = get_downgroups(ct->get_leftson(), keep_group_name, group_members);
            size_t  right_members;
            string *rightgroups = get_downgroups(ct->get_rightson(), keep_group_name, right_members);

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
static const char *get_upgroup(const ARB_countedTree *ct, const ARB_tree_predicate& keep_group_name, const ARB_countedTree*& upgroupPtr) {
    const char *group_name = ct->group_name();
    char       *original   = group_name ? originalGroupName(group_name) : NULL;

    if (original) {
        static SmartCharPtr result;
        result = original;
        group_name = &*result;
        upgroupPtr = ct;
    }
    else if (group_name && keep_group_name.selects(*ct)) {
        upgroupPtr = ct;
    }
    else {
        const ARB_countedTree *father = ct->get_father();
        if (father) {
            group_name = get_upgroup(father, keep_group_name, upgroupPtr);
        }
        else {
            upgroupPtr = ct;
            group_name = "WHOLE_TREE";
        }
    }

    cl_assert(!upgroupPtr || ct->is_inside(upgroupPtr));
    cl_assert(group_name);
    return group_name;
}

void Cluster::generate_name(const ARB_countedTree *ct, const ARB_tree_predicate& keep_group_name) {
    // creates initial cluster description (using up- and down-groups and percentages) 
    cl_assert(!ct->is_leaf);

    const ARB_countedTree *upgroup         = NULL;
    const char            *upgroup_name    = get_upgroup(ct, keep_group_name, upgroup);
    size_t                 upgroup_members = upgroup->get_leaf_count(); 

    size_t cluster_members = ct->get_leaf_count();

    cl_assert(upgroup_members>0); // if no group, whole tree should have been returned

    if (upgroup_members == cluster_members) { // use original or existing name
        name = upgroup_name;
    }
    else {
        size_t    downgroup_members;
        string   *downgroup_names = get_downgroups(ct, keep_group_name, downgroup_members);
        AP_FLOAT up_rel = (AP_FLOAT)cluster_members/upgroup_members;   // used for: xx% of upgroup (big is good)
        AP_FLOAT  down_rel        = (AP_FLOAT)downgroup_members/cluster_members; // used for: downgroup(s) + (1-xx)% outgroup (big is good)

        if (up_rel>down_rel) { // prefer up_percent
            name = string(GBS_global_string("%4.1f%% of %s", up_rel*100.0, upgroup_name));
        }
        else {
            if (downgroup_members == cluster_members) {
                name = downgroup_names->c_str();
            }
            else {
                name = string(GBS_global_string("%s + %4.1f%% outgroup", downgroup_names->c_str(), (1-down_rel)*100.0));
            }
        }

        delete downgroup_names;
    }
}

string Cluster::build_group_name(const ARB_countedTree *ct, const ARB_tree_predicate& keep_group_name) {
    // similar to generate_name()
    // - contains relative position in up-group (instead of percentages)

    cl_assert(!ct->is_leaf);

    const ARB_countedTree *upgroup      = NULL;
    const char            *upgroup_name = get_upgroup(ct, keep_group_name, upgroup);

    size_t upgroup_members = upgroup->get_leaf_count();
    size_t cluster_members = ct->get_leaf_count();

    const char *group_name;
    if (upgroup_members == cluster_members) {
        group_name = upgroup_name;
    }
    else {
        size_t cluster_pos1 = ct->relative_position_in(upgroup)+1; // range [1..upgroup_members]
        size_t cluster_posN = cluster_pos1+cluster_members-1;

        group_name = GBS_global_string("%zu-%zu/%zu_%s", cluster_pos1, cluster_posN, upgroup_members, upgroup_name);
    }
    return group_name;
}

// --------------------------
//      DescriptionFormat

class DescriptionFormat {
    size_t   max_count;
    AP_FLOAT max_dist;
    AP_FLOAT max_minBases;

    mutable char *format_string;

    static long calc_digits(long val) {
        long digits;
        if (val>0) digits      = log10(val)+1;
        else if (val<0) digits = calc_digits(-val);
        else digits            = 1;

        cl_assert(digits>0);
        
        return digits;
    }

    static char *make_format(size_t val) {
        long digits = calc_digits(val);
        return GBS_global_string_copy("%%%lizu", digits);
    }
    static char *make_format(AP_FLOAT val) {
        long digits   = calc_digits(long(val));
        long afterdot = digits <=3 ? 5-digits-1 : 0;
        return GBS_global_string_copy("%%%li.%lif", digits, afterdot);
    }

public:

    DescriptionFormat()
        : max_count(0)
        , max_dist(0.0)
        , max_minBases(0.0)
        , format_string(NULL)
    {}

    void announce(size_t count, AP_FLOAT maxDist, AP_FLOAT minBases) {
        max_count    = std::max(max_count, count);
        max_dist     = std::max(max_dist, maxDist);
        max_minBases = std::max(max_minBases, minBases);

        freenull(format_string);
    }

    const char *get_format() const {
        if (!format_string) {
            // create format string for description
            char *count_part = make_format(max_count);
            char *dist_part  = make_format(max_dist);
            char *bases_part = make_format(max_minBases);

            format_string = GBS_global_string_copy("%s  %s [%s-%s]  %s  %%s",
                                                   count_part,
                                                   dist_part, dist_part, dist_part,
                                                   bases_part);

            free(bases_part);
            free(dist_part);
            free(count_part);
        }
        return format_string;
    }
};



void Cluster::scan_description_widths(DescriptionFormat& format) const {
    AP_FLOAT max_of_all_dists = std::max(max_dist, std::max(min_dist, mean_dist));
    format.announce(get_member_count(), max_of_all_dists*100.0, min_bases);
}

const char *Cluster::description(const DescriptionFormat *format) const {
    const char *format_string;
    if (format) format_string = format->get_format();
    else        format_string = "%zu  %.3f [%.3f-%.3f]  %.1f  %s";

    return GBS_global_string(format_string,
                             get_member_count(),
                             mean_dist*100.0,
                             min_dist*100.0,
                             max_dist*100.0,
                             min_bases,
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
    ClusterOrder         primary_order;
    ClusterOrder         secondary_order;
public:
    SortClusterIDsBy(const KnownClusters& known_, ClusterOrder primary, ClusterOrder secondary)
        : known(known_)
        , primary_order(primary)
        , secondary_order(secondary)
    {}

     bool operator()(ID id1, ID id2) const {
         ClusterPtr cl1 = known.find(id1)->second;
         ClusterPtr cl2 = known.find(id2)->second;

         cl_assert(!cl1.isNull() && !cl2.isNull());

         bool less = cl1->lessByOrder(*cl2, primary_order);
         if (!less) {
             bool equal = !cl2->lessByOrder(*cl1, primary_order);
             if (equal) {
                 less = cl1->lessByOrder(*cl2, secondary_order);
             }
         }
         return less;
     }
};

void ClustersData::update_cluster_selection_list(AW_window *aww) {
    cl_assert(clusterList);
    aww->clear_selection_list(clusterList);

    if (shown.empty()) {
        aww->insert_default_selection(clusterList, "<No clusters detected>", ID(0));
    }
    else {
        {
            SortClusterIDsBy sortBy(known_clusters, criteria[0], criteria[1]);
            sort(shown.begin(), shown.end(), sortBy);
        }
        {
            ClusterIDsIter    cl_end = shown.end();
            DescriptionFormat format;

            for (int pass = 1; pass <= 2; ++pass) {
                for (ClusterIDsIter cl = shown.begin(); cl != cl_end; ++cl) {
                    ID         id      = *cl;
                    ClusterPtr cluster = clusterWithID(id);

                    if (pass == 1) {
                        cluster->scan_description_widths(format);
                    }
                    else {
                        aww->insert_selection(clusterList, cluster->description(&format), cluster->get_ID());
                    }
                }
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
    update_cluster_selection_list(aww);
    known_clusters.clear();
}

char *originalGroupName(const char *groupname) {
    char *original = NULL;
    const char *was = strstr(groupname, " {was:");
    const char *closing = strchr(groupname, '}');
    if (was && closing) {
        original = GB_strpartdup(was+6, closing-1);
        if (original[0]) {
            char *sub = originalGroupName(original);
            if (sub) freeset(original, sub);
        }
        else {
            freenull(original); // empty -> invalid
        }
    }
    return original;
}



