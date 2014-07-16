// =============================================================== //
//                                                                 //
//   File      : DI_clusters.cxx                                   //
//   Purpose   : Detect clusters of homologous sequences in tree   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "di_clusters.hxx"
#include "di_clustertree.hxx"
#include "di_foundclusters.hxx"
#include "di_awars.hxx"

#include <AP_filter.hxx>
#include <AP_seq_protein.hxx>
#include <AP_seq_dna.hxx>

#include <awt_sel_boxes.hxx>

#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>

#include <list>

using namespace std;

#define di_assert(cond) arb_assert(cond)

// --------------
//      awars

#define AWAR_CLUSTER_PREFIX      AWAR_DIST_PREFIX "cluster/"
#define AWAR_CLUSTER_PREFIX_TEMP "/tmp/" AWAR_DIST_PREFIX

#define AWAR_CLUSTER_MAXDIST   AWAR_CLUSTER_PREFIX "maxdist"
#define AWAR_CLUSTER_MINSIZE   AWAR_CLUSTER_PREFIX "minsize"
#define AWAR_CLUSTER_AUTOMARK  AWAR_CLUSTER_PREFIX "automark"
#define AWAR_CLUSTER_MARKREP   AWAR_CLUSTER_PREFIX "markrep"
#define AWAR_CLUSTER_SELECTREP AWAR_CLUSTER_PREFIX "selrep"
#define AWAR_CLUSTER_ORDER     AWAR_CLUSTER_PREFIX "order"

#define AWAR_CLUSTER_GRP_PREFIX     AWAR_CLUSTER_PREFIX "group/"
#define AWAR_CLUSTER_GRP_PREFIX_TMP "/tmp/" AWAR_CLUSTER_GRP_PREFIX

#define AWAR_CLUSTER_GROUP_WHAT         AWAR_CLUSTER_GRP_PREFIX "all"
#define AWAR_CLUSTER_GROUP_EXISTING     AWAR_CLUSTER_GRP_PREFIX "existing"
#define AWAR_CLUSTER_GROUP_NOTFOUND     AWAR_CLUSTER_GRP_PREFIX "notfound"
#define AWAR_CLUSTER_GROUP_IDENTITY     AWAR_CLUSTER_GRP_PREFIX "identity"
#define AWAR_CLUSTER_GROUP_PREFIX       AWAR_CLUSTER_GRP_PREFIX "prefix"
#define AWAR_CLUSTER_GROUP_PREFIX_MATCH AWAR_CLUSTER_GRP_PREFIX "prefix_match"
#define AWAR_CLUSTER_GROUP_SUFFIX       AWAR_CLUSTER_GRP_PREFIX "suffix"
#define AWAR_CLUSTER_GROUP_EXAMPLE      AWAR_CLUSTER_GRP_PREFIX_TMP "example"

#define AWAR_CLUSTER_SELECTED      AWAR_CLUSTER_PREFIX_TEMP "selected" // ID of currently selected cluster (or zero)
#define AWAR_CLUSTER_RESTORE_LABEL AWAR_CLUSTER_PREFIX_TEMP "rlabel" // label of restore button

enum Group_What {
    GROUP_SELECTED,
    GROUP_LISTED,
};

enum Group_Action {
    GROUP_CREATE,
    GROUP_DELETE,
};

enum Group_NotFound {
    NOTFOUND_ABORT,
    NOTFOUND_WARN,
    NOTFOUND_IGNORE,
};

enum Group_Existing {
    EXISTING_GROUP_ABORT,
    EXISTING_GROUP_SKIP,
    EXISTING_GROUP_OVERWRITE,
    EXISTING_GROUP_APPEND_ORG,
};

// ----------------------------------------

enum AffectedClusters { ALL_CLUSTERS, SEL_CLUSTER };

static ClustersData *global_data = 0;

// ------------------------

static void di_forget_global_data(AW_window *) {
    di_assert(global_data);
    global_data->free();
    // do not delete 'global_data' itself, it will be reused when window is opened again
}

// ------------------------
//      Update contents

static void update_cluster_sellist() {
    global_data->update_cluster_selection_list();
    // @@@ update result info line
}
static void update_restore_label(AW_window *aww) {
    AW_root    *aw_root = aww->get_root();
    AW_awar    *awar    = aw_root->awar(AWAR_CLUSTER_RESTORE_LABEL);
    unsigned    size    = global_data->count(STORED_CLUSTERS);
    const char *label   = size ? GBS_global_string("Stored: %u", size) : "None stored";

    awar->write_string(label);
}
static void update_all(AW_window *aww) {
    update_cluster_sellist();
    update_restore_label(aww);
}

static void save_results_recursive(ClusterTree *subtree) {
    if (subtree->get_state() == CS_IS_CLUSTER) {
        global_data->add(new Cluster(subtree), SHOWN_CLUSTERS);
    }
    if (!subtree->is_leaf) {
        save_results_recursive(subtree->get_leftson());
        save_results_recursive(subtree->get_rightson());
    }
}
static void save_results(ClusterTreeRoot *tree) {
    global_data->clear(SHOWN_CLUSTERS);
    save_results_recursive(tree->get_root_node());
    update_cluster_sellist();
}

static void calculate_clusters(AW_window *aww) {
    GBDATA   *gb_main = global_data->get_gb_main();
    GB_ERROR  error   = NULL;

    arb_progress progress("Detecting clusters");

    // calculate ClusterTree
    ClusterTreeRoot *tree = NULL;
    {
        GB_transaction  ta(gb_main);
        AW_root        *aw_root = aww->get_root();

        {
            char    *use     = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
            AliView *aliview = global_data->weighted_filter.create_aliview(use, error);

            if (aliview) {
                AP_sequence *seq = GBT_is_alignment_protein(gb_main, use)
                    ? (AP_sequence*)new AP_sequence_protein(aliview)
                    : new AP_sequence_parsimony(aliview);

                AP_FLOAT maxDistance    = aw_root->awar(AWAR_CLUSTER_MAXDIST)->read_float();
                unsigned minClusterSize = aw_root->awar(AWAR_CLUSTER_MINSIZE)->read_int();

                tree = new ClusterTreeRoot(aliview, seq, maxDistance/100, minClusterSize);

                delete seq;
            }
            free(use);
        }

        if (!error) {
            progress.subtitle("Loading tree");
            {
                char *tree_name = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_string();
                error           = tree->loadFromDB(tree_name);
                free(tree_name);
            }

            if (!error) error = tree->linkToDB(0, 0);
        }
    }

    if (!error) {
        error = tree->find_clusters();
        if (!error) save_results(tree);
    }

    delete tree;

    if (error) aw_message(error);
}


static int with_affected_clusters_do(AW_root *aw_root, AffectedClusters affected, bool warn_if_none_affected, AW_CL cd, void (*fun)(ClusterPtr, AW_CL)) {
    // returns number of affected clusters
    int affCount = 0;
    if (affected == SEL_CLUSTER) {
        AW_awar *awar  = aw_root->awar(AWAR_CLUSTER_SELECTED);
        ID       selID = awar->read_int();

        if (selID) {
            ClusterPtr selCluster = global_data->clusterWithID(selID);
            cl_assert(!selCluster.isNull());
            fun(selCluster, cd);
            affCount++;
        }
        else if (warn_if_none_affected) {
            aw_message("No cluster is selected");
        }
    }
    else {
        unsigned shown = global_data->count(SHOWN_CLUSTERS);
        if (shown) {
            ClusterIDs     clusters(global_data->get_clusterIDs(SHOWN_CLUSTERS)); // intended copy!
            ClusterIDsIter cli_end = clusters.end();
            for (ClusterIDsIter cli = clusters.begin(); cli != cli_end; ++cli) {
                ClusterPtr cluster = global_data->clusterWithID(*cli);
                fun(cluster, cd);
                affCount++;
            }
        }
        else if (warn_if_none_affected) {
            aw_message("There are no clusters in the list");
        }
    }
    return affCount;
}

// -------------
//      mark

void Cluster::mark_all_members(bool mark_representative) const {
    DBItemSetIter sp_end = members.end();
    for (DBItemSetIter sp = members.begin(); sp != sp_end; ++sp) {
        if (mark_representative || (*sp != representative)) {
            GB_write_flag(*sp, 1);
        }
    }
}

enum {
    MARK_REPRES   = 1,
    SELECT_REPRES = 2,
};

static void mark_cluster(ClusterPtr cluster, AW_CL cl_mask) {
    cluster->mark_all_members(cl_mask & MARK_REPRES);
    if (cl_mask & SELECT_REPRES) {
        GBDATA     *gb_species = cluster->get_representative();
        const char *name       = GBT_get_name(gb_species);

        global_data->get_aw_root()->awar(AWAR_SPECIES_NAME)->write_string(name);
    }
}
static void mark_clusters(AW_window *, AffectedClusters affected, bool warn_if_none_affected) {
    AW_root *aw_root = global_data->get_aw_root();
    GBDATA  *gb_main = global_data->get_gb_main();

    GB_transaction ta(gb_main);

    bool  markRep = aw_root->awar(AWAR_CLUSTER_MARKREP)->read_int();
    bool  selRep  = aw_root->awar(AWAR_CLUSTER_SELECTREP)->read_int();
    AW_CL mask    = markRep * MARK_REPRES + selRep * SELECT_REPRES;

    GBT_mark_all(gb_main, 0);                       // unmark all
    int marked = with_affected_clusters_do(aw_root, affected, warn_if_none_affected, mask, mark_cluster);
    if (!marked || !selRep) {
        // nothing marked -> force tree refresh
        if (selRep) {
            aw_root->awar(AWAR_SPECIES_NAME)->write_string("");
        }
        else {
            aw_root->awar(AWAR_TREE_REFRESH)->touch(); // @@ hm - no effect?!
        }
    }
}

static void select_cluster_cb(AW_root *aw_root) {
    bool auto_mark = aw_root->awar(AWAR_CLUSTER_AUTOMARK)->read_int();
    if (auto_mark) mark_clusters(NULL, SEL_CLUSTER, false);
}

static void select_cluster(ID id) {
    global_data->get_aw_root()->awar(AWAR_CLUSTER_SELECTED)->write_int(id);
}


// -------------------
//      Sort order


static void sort_order_changed_cb(AW_root *aw_root) {
    ClusterOrder order = (ClusterOrder)aw_root->awar(AWAR_CLUSTER_ORDER)->read_int();
    global_data->changeSortOrder(order);
    update_cluster_sellist();
}

// --------------
//      group

class GroupTree;
typedef map<string, GroupTree*> Species2Tip;

class GroupTree : public ARB_countedTree {
    unsigned leaf_count;   // total number of leafs in subtree
    unsigned tagged_count; // tagged leafs

    void update_tag_counters();
public:

    explicit GroupTree(ARB_seqtree_root *root)
        : ARB_countedTree(root),
          leaf_count(0),
          tagged_count(0)
    {}

    // ARB_countedTree interface
    virtual void init_tree() OVERRIDE { update_leaf_counters(); }
    // ARB_countedTree interface end

    DEFINE_TREE_RELATIVES_ACCESSORS(GroupTree);

    void map_species2tip(Species2Tip& mapping);

    unsigned get_leaf_count() const OVERRIDE { return leaf_count; }
    unsigned update_leaf_counters();

    void tag_leaf() {
        di_assert(is_leaf);
        tagged_count = 1;
        get_father()->update_tag_counters();
    }
    unsigned get_tagged_count() const { return tagged_count; }
    void clear_tags();

    double tagged_rate() const { return double(get_tagged_count())/get_leaf_count(); }
};

struct GroupTreeNodeFactory : public RootedTreeNodeFactory {
    virtual RootedTree *makeNode(TreeRoot *root) const {
        return new GroupTree(DOWNCAST(ARB_seqtree_root*, root));
    }
};

unsigned GroupTree::update_leaf_counters() {
    if (is_leaf) leaf_count = 1;
    else leaf_count = get_leftson()->update_leaf_counters() + get_rightson()->update_leaf_counters();
    return leaf_count;
}

void GroupTree::clear_tags() {
    if (!is_leaf && tagged_count) {
        get_leftson()->clear_tags();
        get_rightson()->clear_tags();
    }
    tagged_count = 0;
}

void GroupTree::map_species2tip(Species2Tip& mapping) {
    if (is_leaf) {
        if (name) mapping[name] = this;
    }
    else {
        get_leftson()->map_species2tip(mapping);
        get_rightson()->map_species2tip(mapping);
    }
}

void GroupTree::update_tag_counters() {
    di_assert(!is_leaf);
    GroupTree *node = this;
    while (node) {
        node->tagged_count = node->get_leftson()->get_tagged_count() + node->get_rightson()->get_tagged_count();
        node               = node->get_father();
    }
}

struct GroupChanges {
    unsigned created;
    unsigned skipped;
    unsigned overwritten;
    unsigned deleted;
    unsigned restored;

    void clear() { created = skipped = overwritten = deleted = restored = 0; }
    bool exist() const { return created||overwritten||deleted||restored; }

    void show_message() {
        string msg;

        if (created)     msg += GBS_global_string("%u created  ",     created);
        if (overwritten) msg += GBS_global_string("%u overwritten  ", overwritten);
        if (skipped)     msg += GBS_global_string("%u skipped  ",     skipped);
        if (deleted)     msg += GBS_global_string("%u deleted  ",     deleted);
        if (restored)    msg += GBS_global_string("%u restored  ",    restored);

        if (!msg.empty()) {
            msg = string("Group changes: ")+msg;
            aw_message(msg.c_str());
        }
    }
    
    GroupChanges() { clear(); }
};


// ---------------------
//      GroupBuilder

typedef map<GroupTree*, ClusterPtr> Group2Cluster;

class GroupBuilder : virtual Noncopyable {
    GBDATA           *gb_main;
    string            tree_name;
    ARB_seqtree_root *tree_root;
    Group_Action      action;                       // create or delete ?
    Species2Tip       species2tip;                  // map speciesName -> leaf
    ARB_ERROR         error;
    ClusterPtr        bad_cluster;                  // error occurred here (is set)
    Group_Existing    existing;
    unsigned          existing_count;               // counts existing groups
    Group_NotFound    notfound;
    double            matchRatio;                   // needed identity of subtree and cluster
    double            maxDist;                      // max. Distance used for calculation
    string            cluster_prefix;               // prefix for cluster name
    string            cluster_suffix_def;           // suffix-definition for cluster name
    GroupChanges      changes;                      // count tree modifications
    bool              del_match_prefixes;           // only delete groups, where prefix matches

    GroupTree *find_group_position(GroupTree *subtree, unsigned cluster_size);

public:
    GroupBuilder(GBDATA *gb_main_, Group_Action action_)
        : gb_main(gb_main_),
          tree_root(NULL),
          action(action_),
          error(NULL),
          existing_count(0)
    {
        AW_root *awr = global_data->get_aw_root();

        tree_name          = awr->awar(AWAR_DIST_TREE_CURR_NAME)->read_char_pntr();
        existing           = (Group_Existing)awr->awar(AWAR_CLUSTER_GROUP_EXISTING)->read_int();
        notfound           = (Group_NotFound)awr->awar(AWAR_CLUSTER_GROUP_NOTFOUND)->read_int();
        del_match_prefixes = awr->awar(AWAR_CLUSTER_GROUP_PREFIX_MATCH)->read_int();
        matchRatio         = awr->awar(AWAR_CLUSTER_GROUP_IDENTITY)->read_int()/100.0;
        maxDist            = awr->awar(AWAR_CLUSTER_MAXDIST)->read_float();
        cluster_prefix     = awr->awar(AWAR_CLUSTER_GROUP_PREFIX)->read_char_pntr();
        cluster_suffix_def = awr->awar(AWAR_CLUSTER_GROUP_SUFFIX)->read_char_pntr();
    }
    ~GroupBuilder() {
        delete tree_root;
    }

    ARB_ERROR get_error() const { return error; }
    ClusterPtr get_bad_cluster() const { return bad_cluster; }
    Group_Existing with_existing() const { return existing; }
    unsigned get_existing_count() const { return existing_count; }
    double get_max_distance() const { return maxDist; }

    ARB_ERROR save_modified_tree();
    void      load_tree();

    DEFINE_DOWNCAST_ACCESSORS(GroupTree, get_root_node, tree_root->get_root_node());

    GroupTree *find_best_matching_subtree(ClusterPtr cluster);
    void update_group(ClusterPtr cluster); // create or delete group for cluster
    string generate_group_name(ClusterPtr cluster, const GroupTree *group_node);

    bool matches_current_prefix(const char *groupname) const {
        return strstr(groupname, cluster_prefix.c_str()) == groupname;
    }

    bool shall_delete_group(const char *name) const {
        return !del_match_prefixes || matches_current_prefix(name);
    }
};

void GroupBuilder::load_tree() {
    di_assert(!tree_root);

    tree_root = new ARB_seqtree_root(new AliView(gb_main), new GroupTreeNodeFactory, NULL, false);
    error     = tree_root->loadFromDB(tree_name.c_str());

    if (error) {
        delete tree_root;
        tree_root = NULL;
    }
    else {
        changes.clear();

        GroupTree *tree = get_root_node();
        tree->update_leaf_counters();
        tree->map_species2tip(species2tip);
    }
}
ARB_ERROR GroupBuilder::save_modified_tree() {
    di_assert(!error);
    if (changes.exist()) {
        di_assert(tree_root);
        error = tree_root->saveToDB();

        AW_root *awr = global_data->get_aw_root();
        awr->awar(AWAR_TREE_REFRESH)->touch();

        if (!error) {
            changes.show_message();
            changes.clear();
        }
    }
    return error;
}

GroupTree *GroupBuilder::find_group_position(GroupTree *subtree, unsigned cluster_size) {
    // searches for best group in subtree matching the cluster

    GroupTree *groupPos = NULL;
    if (subtree->get_tagged_count() == cluster_size) {
        groupPos                = find_group_position(subtree->get_leftson(), cluster_size);
        if (!groupPos) groupPos = find_group_position(subtree->get_rightson(), cluster_size);

        if (!groupPos) {                            // consider 'subtree'
            if (subtree->tagged_rate() >= matchRatio) {
                groupPos = subtree;
            }
        }
    }
    return groupPos;
}

class HasntCurrentClusterPrefix : public ARB_tree_predicate {
    const GroupBuilder& builder;
public:
    HasntCurrentClusterPrefix(const GroupBuilder& builder_) : builder(builder_) {}
    bool selects(const ARB_seqtree& tree) const OVERRIDE {
        const char *groupname        = tree.get_group_name();
        bool        hasClusterPrefix = groupname && builder.matches_current_prefix(groupname);
        return !hasClusterPrefix;
    }
};

string concatenate_name_parts(const list<string>& namepart) {
    string concat;
    for (list<string>::const_iterator p = namepart.begin(); p != namepart.end(); ++p) {
        if (!p->empty()) concat += '_'+*p;
    }
    return concat.erase(0, 1);
}

struct UseTreeRoot : public ARB_tree_predicate {
    bool selects(const ARB_seqtree& tree) const OVERRIDE { return tree.is_root_node(); }
};

string GroupBuilder::generate_group_name(ClusterPtr cluster, const GroupTree *group_node) {
    list<string> namepart;
    namepart.push_back(cluster_prefix);

    string  orgname_suffix;
    if (existing == EXISTING_GROUP_APPEND_ORG) {
        char *old_name = group_node->name;
        if (old_name) {
            char *original = originalGroupName(old_name);
            if (!original && !matches_current_prefix(old_name)) {
                original = strdup(old_name); // use existing name as original name
            }
            if (original) {
                orgname_suffix = string(" {was:")+original+"}";
                free(original);
            }
        }
    }

    string text;
    for (int i = 0; cluster_suffix_def[i]; ++i) {
        if (cluster_suffix_def[i] == '%') {
            ++i;
            if (cluster_suffix_def[i] == '%') {
                text += '%';
            }
            else {
                string expanded;
                switch(cluster_suffix_def[i]) {
                    case 'p': expanded = cluster->get_upgroup_info(group_node, HasntCurrentClusterPrefix(*this)); break;
                    case 'P': expanded = cluster->get_upgroup_info(group_node, UseAnyTree()); break;
                    case 't': expanded = cluster->get_upgroup_info(group_node, UseTreeRoot()); break;
                    case 'd': expanded = GBS_global_string("~%.3f", cluster->get_mean_distance()); break;
                    case 'D': expanded = GBS_global_string("<=%.1f%%", get_max_distance()); break;
                    case 'e': expanded = group_node->name ? group_node->name : ""; break;
                    case 'o': {
                        int matchRate = int(group_node->tagged_rate()*100+0.5);
                        if (matchRate<100) expanded = GBS_global_string("%i%%_of", matchRate);
                        break;
                    }
                    default:
                        text += '%';
                        text += cluster_suffix_def[i];
                        break;
                }

                if (!expanded.empty()) {
                    namepart.push_back(text);
                    text = "";
                    namepart.push_back(expanded);
                }
            }
        }
        else {
            text += cluster_suffix_def[i];
        }
    }
    namepart.push_back(text);

    return concatenate_name_parts(namepart)+orgname_suffix;
}

GroupTree *GroupBuilder::find_best_matching_subtree(ClusterPtr cluster) {
    GroupTree *group_node = NULL;
    if (!error) {
        if (!tree_root) load_tree();
        if (!error) {
            const DBItemSet& members = cluster->get_members();

            // mark cluster members in tree
            {
                GB_transaction ta(gb_main);
                DBItemSetIter sp_end = members.end();
                for (DBItemSetIter sp = members.begin(); sp != sp_end && !error; ++sp) {
                    const char *name = GBT_get_name(*sp);
                    di_assert(name);
                    if (name) {
                        Species2Tip::const_iterator found = species2tip.find(name);
                        if (found == species2tip.end()) {
                            error = GBS_global_string("Species '%s' is not in '%s'", name, tree_name.c_str());
                        }
                        else {
                            GroupTree *leaf = found->second;
                            leaf->tag_leaf();
                        }
                    }
                }
            }

            if (!error) {
                // top-down search for best matching node
                group_node = find_group_position(get_root_node(), cluster->get_member_count());
            }
        }
    }
    return group_node;
}

void GroupBuilder::update_group(ClusterPtr cluster) {
    if (!error) {
        GroupTree *group_node = find_best_matching_subtree(cluster);
        if (!error) {
            if (!group_node) { // no matching subtree found
                switch (notfound) {
                    case NOTFOUND_WARN:
                    case NOTFOUND_ABORT: {
                        const char *msg = GBS_global_string("Could not find matching subtree for cluster '%s'", cluster->get_list_display(NULL));
                        if (notfound == NOTFOUND_ABORT) error = msg;
                        else aw_message(msg);
                        break;
                    }
                    case NOTFOUND_IGNORE: break; // silently ignore
                }
            }
            else { // found subtree for group
                switch (action) {
                    case GROUP_CREATE: {
                        char *old_name = group_node->name;

                        if (old_name && existing == EXISTING_GROUP_ABORT) {
                            error = GBS_global_string("Existing group '%s' is in the way", old_name);
                        }
                        else {
                            if (old_name && existing == EXISTING_GROUP_SKIP) {
                                changes.skipped++;
                            }
                            else {
                                string new_name = generate_group_name(cluster, group_node);

                                if (old_name) changes.overwritten++; else changes.created++;

                                free(old_name);
                                group_node->name = strdup(new_name.c_str());

                                // @@@ DRY that.. it's spread everywhere through libs :(
                                if (!group_node->gb_node) {
                                    GBDATA *gb_tree = group_node->get_tree_root()->get_gb_tree();
                                    GB_transaction ta(gb_tree);
                                    GBDATA *gb_node = GB_create_container(gb_tree, "node");
                                    if (!gb_node) {
                                        error = GB_await_error();
                                    }
                                    else {
                                        error = GBT_write_int(gb_node, "id", 0);
                                    }

                                    if (!error) {
                                        group_node->gb_node = gb_node;
                                    }
                                }
                                if (group_node->gb_node && !error) {
                                    GB_transaction ta(group_node->gb_node);
                                    error = GBT_write_string(group_node->gb_node, "group_name", group_node->name);
                                }

                                cluster->update_description(group_node); // change list display
                            }
                        }
                        break;
                    }
                    case GROUP_DELETE: {
                        if (group_node->name && shall_delete_group(group_node->name)) {
                            char *original = originalGroupName(group_node->name);

                            if (original) {
                                freeset(group_node->name, original); // restore original name
                                if (group_node->gb_node) {
                                    error = GBT_write_string(group_node->gb_node, "group_name", group_node->name);
                                }
                                changes.restored++;
                            }
                            else {
                                freenull(group_node->name);
                                group_node->gb_node = NULL; // forget ref to group data (@@@ need to delete group data from DB ? )
                                changes.deleted++;
                            }

                            cluster->update_description(group_node); // change list display
                        }
                        break;
                    }
                }
            }
        }

        if (error) bad_cluster = cluster;
        get_root_node()->clear_tags();
    }
}

static void update_example(AW_root *aw_root) {
    ID     selID = aw_root->awar(AWAR_CLUSTER_SELECTED)->read_int();
    string value;

    if (selID) {
        ClusterPtr selCluster = global_data->clusterWithID(selID);
        cl_assert(!selCluster.isNull());

        GroupBuilder  builder(global_data->get_gb_main(), GROUP_CREATE);
        GroupTree    *group_node = builder.find_best_matching_subtree(selCluster);

        GB_ERROR error = builder.get_error().deliver();
        
        if (error)           value = GBS_global_string("<error: %s>", error);
        else if (group_node) value = builder.generate_group_name(selCluster, group_node);
        else                 value = "<no matching subtree found>";
    }
    else value = "<no cluster selected>";
    aw_root->awar(AWAR_CLUSTER_GROUP_EXAMPLE)->write_string(value.c_str());
}

static void update_cluster_group(ClusterPtr cluster, AW_CL cl_groupBuilder) {
    GroupBuilder *groupBuilder = (GroupBuilder*)cl_groupBuilder;
    if (!groupBuilder->get_error()) {
        groupBuilder->update_group(cluster);
    }
}

static void accept_proposed_names(ClusterPtr cluster, AW_CL cl_accept) {
    bool accept(cl_accept);
    cluster->accept_proposed(accept);
}

static void group_clusters(AW_window *, Group_Action action) {
    AW_root          *aw_root  = global_data->get_aw_root();
    Group_What        what     = (Group_What)aw_root->awar(AWAR_CLUSTER_GROUP_WHAT)->read_int();
    AffectedClusters  affected = what == GROUP_LISTED ? ALL_CLUSTERS : SEL_CLUSTER;

    GroupBuilder groupBuilder(global_data->get_gb_main(), action);

    GB_transaction ta(global_data->get_gb_main());
    with_affected_clusters_do(aw_root, affected, true, (AW_CL)&groupBuilder, update_cluster_group);

    ARB_ERROR error = groupBuilder.get_error();
    if (error) {
        ClusterPtr bad = groupBuilder.get_bad_cluster();
        if (!bad.isNull()) {
            select_cluster(bad->get_ID());
            aw_message("Problematic cluster has been highlighted");
        }
    }
    else {
        error = groupBuilder.save_modified_tree();
    }

    error = ta.close(error);

    bool accept = !error;
    aw_message_if(error);
    // careful! the following code will invalidate error, so don't use below

    with_affected_clusters_do(aw_root, affected, false, (AW_CL)accept, accept_proposed_names); // just affects display
    global_data->update_cluster_selection_list();
}

static void popup_group_clusters_window(AW_window *aw_clusterList) {
    static AW_window_simple *aws = 0;

    if (!aws) {
        AW_root *aw_root = aw_clusterList->get_root();

        aws = new AW_window_simple;
        aws->init(aw_root, "cluster_groups", "Cluster groups");

        aws->auto_space(10, 10);

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");
        aws->callback(makeHelpCallback("cluster_group.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at_newline();

        aws->label("For");
        aws->create_option_menu(AWAR_CLUSTER_GROUP_WHAT, true);
        aws->insert_option        ("selected cluster", "s", GROUP_SELECTED);
        aws->insert_default_option("listed clusters",  "l", GROUP_LISTED);
        aws->update_option_menu();

        aws->at_newline();

        aws->label("with a min. cluster/subtree identity (%) of");
        aws->create_input_field(AWAR_CLUSTER_GROUP_IDENTITY, 4);

        aws->at_newline();

        aws->label("-> if no matching subtree found");
        aws->create_option_menu(AWAR_CLUSTER_GROUP_NOTFOUND, true);
        aws->insert_default_option("abort",          "a", NOTFOUND_ABORT);
        aws->insert_option        ("warn",           "w", NOTFOUND_WARN);
        aws->insert_option        ("ignore",         "i", NOTFOUND_IGNORE);
        aws->update_option_menu();

        aws->at_newline();

        aws->callback(makeWindowCallback(group_clusters, GROUP_CREATE));
        aws->create_autosize_button("CREATE_GROUPS", "create groups!");

        aws->label("If group exists");
        aws->create_option_menu(AWAR_CLUSTER_GROUP_EXISTING, true);
        aws->insert_default_option("abort",                "a", EXISTING_GROUP_ABORT);
        aws->insert_option        ("skip",                 "s", EXISTING_GROUP_SKIP);
        aws->insert_option        ("overwrite (caution!)", "o", EXISTING_GROUP_OVERWRITE);
        aws->insert_option        ("append original",      "p", EXISTING_GROUP_APPEND_ORG);
        aws->update_option_menu();

        aws->at_newline();

        aws->callback(makeWindowCallback(group_clusters, GROUP_DELETE));
        aws->create_autosize_button("DELETE_GROUPS", "delete groups!");

        aws->create_text_toggle(AWAR_CLUSTER_GROUP_PREFIX_MATCH, "(all)", "(where prefix matches)", 30);

        aws->at_newline(); aws->label("Name prefix:"); aws->create_input_field(AWAR_CLUSTER_GROUP_PREFIX, 20);
        aws->at_newline(); aws->label("Name suffix:"); aws->create_input_field(AWAR_CLUSTER_GROUP_SUFFIX, 20);

        aws->at_newline();
        aws->button_length(60);
        aws->label("=>"); 
        aws->create_button(NULL, AWAR_CLUSTER_GROUP_EXAMPLE);

        aws->window_fit();
    }

    aws->activate();
}

// ---------------
//      delete

static void delete_selected_cluster(ClusterPtr cluster, AW_CL) {
    int pos    = global_data->get_pos(cluster, SHOWN_CLUSTERS);
    int nextId = global_data->idAtPos(pos+1, SHOWN_CLUSTERS);
    select_cluster(nextId);
    global_data->remove(cluster, SHOWN_CLUSTERS);
}
static void delete_clusters(AW_window *aww, AffectedClusters affected) {
    switch (affected) {
        case SEL_CLUSTER:
            with_affected_clusters_do(aww->get_root(), affected, true, affected, delete_selected_cluster);
            break;
        case ALL_CLUSTERS:
            select_cluster(0);
            global_data->clear(SHOWN_CLUSTERS);
            break;
    }

    update_cluster_sellist();
}

// ----------------------
//      store/restore

static void store_selected_cluster(ClusterPtr cluster, AW_CL) {
    int pos    = global_data->get_pos(cluster, SHOWN_CLUSTERS);
    int nextId = global_data->idAtPos(pos+1, SHOWN_CLUSTERS);

    select_cluster(nextId);
    global_data->store(cluster->get_ID());
}
static void store_clusters(AW_window *aww, AffectedClusters affected) {
    switch (affected) {
        case SEL_CLUSTER:
            with_affected_clusters_do(aww->get_root(), affected, true, affected, store_selected_cluster);
            break;
        case ALL_CLUSTERS:
            select_cluster(0);
            global_data->store_all();
            break;
    }

    update_all(aww);
}


static void restore_clusters(AW_window *aww) {
    global_data->restore_all();
    update_all(aww);
}
static void swap_clusters(AW_window *aww) {
    global_data->swap_all();
    select_cluster(0);
    update_all(aww);
}

// ------------------
//      save/load

#if defined(WARN_TODO)
#warning "implement save/load clusters"
#endif
static void save_clusters(AW_window *) {
    aw_message("save_clusters not implemented");
}
static void load_clusters(AW_window *) {
    aw_message("load_clusters not implemented");
}

// ---------------------------------
//      cluster detection window

AW_window *DI_create_cluster_detection_window(AW_root *aw_root, WeightedFilter *weightedFilter) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        cl_assert(!global_data);
        global_data = new ClustersData(*weightedFilter);

        aws = new AW_window_simple;
        aws->init(aw_root, "DETECT_CLUSTERS", "Detect clusters in tree");
        aws->load_xfig("di_clusters.fig");

        aws->on_hide(di_forget_global_data);

        // -------------------
        //      upper area

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE");

        aws->at("help");
        aws->callback(makeHelpCallback("di_clusters.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("max_dist");
        aws->d_callback(makeWindowCallback(calculate_clusters));
        aws->create_input_field(AWAR_CLUSTER_MAXDIST, 12);

        aws->at("min_size");
        aws->d_callback(makeWindowCallback(calculate_clusters));
        aws->create_input_field(AWAR_CLUSTER_MINSIZE, 5);

        aws->at("calculate");
        aws->callback(calculate_clusters);
        aws->create_autosize_button("CALC", "Detect clusters");

        aws->button_length(20);
        aws->at("tree_name");
        aws->create_button(NULL, AWAR_DIST_TREE_CURR_NAME);

        // -------------------
        //      lower area

        aws->button_length(18);

        // column 1

        aws->at("mark_all"); aws->callback(makeWindowCallback(mark_clusters, ALL_CLUSTERS, true)); aws->create_button("MARKALL", "Mark all");

        aws->at("auto_mark");  aws->create_toggle(AWAR_CLUSTER_AUTOMARK);
        aws->at("mark_rep");   aws->create_toggle(AWAR_CLUSTER_MARKREP);
        aws->at("select_rep"); aws->create_toggle(AWAR_CLUSTER_SELECTREP);

        aws->at("mark"); aws->callback(makeWindowCallback(mark_clusters, SEL_CLUSTER, true)); aws->create_button("MARK", "Mark cluster");

        // column 2

        aws->at("group"); aws->callback(popup_group_clusters_window); aws->create_button("GROUP", "Cluster groups..");

        aws->at("sort");
        aws->create_option_menu(AWAR_CLUSTER_ORDER, true);
        aws->insert_default_option("by mean distance",  "d", SORT_BY_MEANDIST);
        aws->insert_option        ("by min bases used", "b", SORT_BY_MIN_BASES);
        aws->insert_option        ("by size",           "s", SORT_BY_CLUSTERSIZE);
        aws->insert_option        ("by tree position",  "p", SORT_BY_TREEPOSITION);
        aws->insert_option        ("by min distance",   "i", SORT_BY_MIN_DIST);
        aws->insert_option        ("by max distance",   "x", SORT_BY_MAX_DIST);
        aws->insert_option        ("reverse",           "r", SORT_REVERSE);
        aws->update_option_menu();

        // store/restore

        aws->at("store_all"); aws->callback(makeWindowCallback(store_clusters, ALL_CLUSTERS)); aws->create_button("STOREALL", "Store all");
        aws->at("store");     aws->callback(makeWindowCallback(store_clusters, SEL_CLUSTER));  aws->create_button("STORESEL", "Store selected");
        aws->at("restore");   aws->callback(restore_clusters);                                 aws->create_button("RESTORE",  AWAR_CLUSTER_RESTORE_LABEL);
        aws->at("swap");      aws->callback(swap_clusters);                                    aws->create_button("Swap",     "Swap stored");

        // column 4

        aws->at("clear");  aws->callback(makeWindowCallback(delete_clusters, ALL_CLUSTERS)); aws->create_button("CLEAR", "Clear list");
        aws->at("delete"); aws->callback(makeWindowCallback(delete_clusters, SEL_CLUSTER));  aws->create_button("DEL",   "Delete selected");
        aws->sens_mask(AWM_DISABLED);
        aws->at("save"); aws->callback(save_clusters); aws->create_button("SAVE", "Save list");
        aws->at("load"); aws->callback(load_clusters); aws->create_button("LOAD", "Load list");
        aws->sens_mask(AWM_ALL);


        // --------------------
        //      clusterlist

        aws->at("cluster_list");
        global_data->clusterList = aws->create_selection_list(AWAR_CLUSTER_SELECTED, true);
        update_cluster_sellist();

        aw_root->awar(AWAR_CLUSTER_SELECTED)->add_callback(select_cluster_cb);
        aw_root->awar(AWAR_CLUSTER_ORDER)->add_callback(sort_order_changed_cb);
        sort_order_changed_cb(aw_root);
    }

    return aws;
}

void DI_create_cluster_awars(AW_root *aw_root, AW_default def, AW_default db) {
    aw_root->awar_float(AWAR_CLUSTER_MAXDIST,   3.0, def)->set_minmax(0.0, 100.0);
    aw_root->awar_int  (AWAR_CLUSTER_MINSIZE,   7,   def)->set_minmax(2, INT_MAX);
    aw_root->awar_int  (AWAR_CLUSTER_AUTOMARK,  1,   def);
    aw_root->awar_int  (AWAR_CLUSTER_MARKREP,   0,   def);
    aw_root->awar_int  (AWAR_CLUSTER_SELECTREP, 1,   def);

    aw_root->awar_int   (AWAR_CLUSTER_ORDER,         SORT_BY_MEANDIST, def);
    aw_root->awar_string(AWAR_CLUSTER_RESTORE_LABEL, "None stored",    def);

    aw_root->awar_int(AWAR_CLUSTER_GROUP_WHAT,     GROUP_LISTED,   def);
    aw_root->awar_int(AWAR_CLUSTER_GROUP_NOTFOUND, NOTFOUND_ABORT, def);

    aw_root->awar_int   (AWAR_CLUSTER_GROUP_IDENTITY,     100, def)->set_minmax(1, 100);
    aw_root->awar_int   (AWAR_CLUSTER_GROUP_PREFIX_MATCH, 1,   def);
    aw_root->awar_string(AWAR_CLUSTER_GROUP_EXAMPLE,      "",  def);

    aw_root->awar_int   (AWAR_CLUSTER_SELECTED,       0,                    def)->add_callback(update_example);
    aw_root->awar_int   (AWAR_CLUSTER_GROUP_EXISTING, EXISTING_GROUP_ABORT, def)->add_callback(update_example);
    aw_root->awar_string(AWAR_CLUSTER_GROUP_PREFIX,   "cluster",            def)->add_callback(update_example);
    aw_root->awar_string(AWAR_CLUSTER_GROUP_SUFFIX,   "%o%p",               def)->add_callback(update_example);

    aw_root->awar_int(AWAR_TREE_REFRESH, 0, db);

    update_example(aw_root);
}

