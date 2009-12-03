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
#include <AP_seq_simple_pro.hxx>
#include <AP_seq_dna.hxx>

#include <awt_sel_boxes.hxx>

#include <aw_window.hxx>
#include <aw_awars.hxx>

#include <climits>


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

#define AWAR_CLUSTER_SELECTED      AWAR_CLUSTER_PREFIX_TEMP "selected" // ID of currently selected cluster (or zero) 
#define AWAR_CLUSTER_RESTORE_LABEL AWAR_CLUSTER_PREFIX_TEMP "rlabel" // label of restore button

void DI_create_cluster_awars(AW_root *aw_root, AW_default def, AW_default db) {
    aw_root->awar_float(AWAR_CLUSTER_MAXDIST, 3.0, def)->set_minmax(0.0, 100.0);
    aw_root->awar_int(AWAR_CLUSTER_MINSIZE, 7, def)->set_minmax(2, INT_MAX);
    aw_root->awar_int(AWAR_CLUSTER_AUTOMARK, 1, def);
    aw_root->awar_int(AWAR_CLUSTER_MARKREP, 0, def);
    aw_root->awar_int(AWAR_CLUSTER_SELECTREP, 1, def);
    aw_root->awar_int(AWAR_CLUSTER_SELECTED, 0, def);
    aw_root->awar_string(AWAR_CLUSTER_RESTORE_LABEL, "None stored", def);

    aw_root->awar_int(AWAR_TREE_REFRESH, 0, db);
}

// ----------------------------------------

enum AffectedClusters { ALL_CLUSTERS, SEL_CLUSTER };

static ClustersData *global_data = 0;

// ------------------------

static void di_forget_global_data(AW_window *aww) {
    di_assert(global_data);
    global_data->free(aww);
    // do not delete 'global_data' itself, it will be reused when window is opened again
}

// ------------------------
//      Update contents

static void update_cluster_sellist(AW_window *aww) {
    global_data->update_selection_list(aww);
    // @@@ update result info line
}
static void update_restore_label(AW_window *aww) {
    AW_root    *aw_root = aww->get_root();
    AW_awar    *awar    = aw_root->awar(AWAR_CLUSTER_RESTORE_LABEL);
    size_t      size    = global_data->count(STORED_CLUSTERS);
    const char *label   = size ? GBS_global_string("Stored: %zu", size) : "None stored";

    awar->write_string(label);
}
static void update_all(AW_window *aww) {
    update_cluster_sellist(aww);
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
static void save_results(AW_window *aww, ClusterTreeRoot *tree) {
    global_data->clear(SHOWN_CLUSTERS);
    save_results_recursive(tree->get_root_node());
    update_cluster_sellist(aww);
}

static void calculate_clusters(AW_window *aww) {
    GBDATA   *gb_main = global_data->get_gb_main();
    GB_ERROR  error   = NULL;

    // calculate ClusterTree
    ClusterTreeRoot *tree = NULL;
    {
        GB_transaction  ta(gb_main);
        AW_root        *aw_root = aww->get_root();

        {
            char    *use     = aw_root->awar(AWAR_DIST_ALIGNMENT)->read_string();
            AliView *aliview = global_data->weighted_filter.create_aliview(use);

            AP_sequence *seq = GBT_is_alignment_protein(gb_main,use)
                ? (AP_sequence*)new AP_sequence_simple_protein(aliview)
                : new AP_sequence_parsimony(aliview);

            AP_FLOAT maxDistance    = aw_root->awar(AWAR_CLUSTER_MAXDIST)->read_float();
            size_t   minClusterSize = aw_root->awar(AWAR_CLUSTER_MINSIZE)->read_int();

            tree = new ClusterTreeRoot(aliview, seq, maxDistance/100, minClusterSize);

            delete seq;
            free(use);
        }

        aw_openstatus("Loading tree");
        {
            char *tree_name = aw_root->awar(AWAR_DIST_TREE_CURR_NAME)->read_string();
            error           = tree->loadFromDB(tree_name);
            free(tree_name);
        }

        if (!error) error = tree->linkToDB(0, 0);
        aw_closestatus();
    }

    if (!error) {
        error = tree->find_clusters();
        if (!error) save_results(aww, tree);
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
            cl_assert(!selCluster.Null());
            fun(selCluster, cd);
            affCount++;
        }
        else if (warn_if_none_affected) {
            aw_message("No cluster is selected");
        }
    }
    else {
        size_t shown = global_data->count(SHOWN_CLUSTERS);
        if (shown) {
            for (int i = shown-1; i>0; --i) {
                ClusterPtr cluster = global_data->clusterAtPos(i, SHOWN_CLUSTERS);
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
    SpeciesSetIter sp_end = members.end();
    for (SpeciesSetIter sp = members.begin(); sp != sp_end; ++sp) {
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
static void mark_clusters(AW_window *, AW_CL cl_affected, AW_CL cl_warn_if_none_affected) {
    AffectedClusters  affected = AffectedClusters(cl_affected);
    AW_root          *aw_root  = global_data->get_aw_root();
    GBDATA           *gb_main  = global_data->get_gb_main();
    GB_transaction    ta(gb_main);

    bool  markRep = aw_root->awar(AWAR_CLUSTER_MARKREP)->read_int();
    bool  selRep  = aw_root->awar(AWAR_CLUSTER_SELECTREP)->read_int();
    AW_CL mask    = markRep * MARK_REPRES + selRep * SELECT_REPRES;

    GBT_mark_all(gb_main, 0);                       // unmark all
    int marked = with_affected_clusters_do(aw_root, affected, cl_warn_if_none_affected, mask, mark_cluster);
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

// --------------
//      group

static void group_cluster(AW_window *aww) {
    cl_assert(0); // not impl
}

// ---------------
//      delete

static void delete_selected_cluster(ClusterPtr cluster, AW_CL) {
    int pos    = global_data->get_pos(cluster, SHOWN_CLUSTERS);
    int nextId = global_data->idAtPos(pos+1, SHOWN_CLUSTERS);
    select_cluster(nextId);
    global_data->remove(cluster, SHOWN_CLUSTERS);
}
static void delete_clusters(AW_window *aww, AW_CL cl_affected) {
    AffectedClusters affected = AffectedClusters(cl_affected);

    switch (affected) {
        case SEL_CLUSTER:
            with_affected_clusters_do(aww->get_root(), affected, true, cl_affected, delete_selected_cluster);
            break;
        case ALL_CLUSTERS:
            select_cluster(0);
            global_data->clear(SHOWN_CLUSTERS);
            break;
    }

    update_cluster_sellist(aww);
}

// ----------------------
//      store/restore

static void store_selected_cluster(ClusterPtr cluster, AW_CL) {
    int pos    = global_data->get_pos(cluster, SHOWN_CLUSTERS);
    int nextId = global_data->idAtPos(pos+1, SHOWN_CLUSTERS);
    
    select_cluster(nextId);
    global_data->store(cluster->get_ID());
}
static void store_clusters(AW_window *aww, AW_CL cl_affected) {
    AffectedClusters affected = AffectedClusters(cl_affected);

    switch (affected) {
        case SEL_CLUSTER:
            with_affected_clusters_do(aww->get_root(), affected, true, cl_affected, store_selected_cluster);
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

static void save_clusters(AW_window *aww) {
    cl_assert(0); // not impl
}
static void load_clusters(AW_window *aww) {
    cl_assert(0); // not impl
}

// ---------------------------------
//      cluster detection window

AW_window *DI_create_cluster_detection_window(AW_root *aw_root, AW_CL cl_weightedFilter) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        cl_assert(!global_data);
        global_data = new ClustersData(*(WeightedFilter*)cl_weightedFilter);

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
        aws->callback(AW_POPUP_HELP, (AW_CL)"di_clusters.hlp");
        aws->create_button("HELP", "HELP");

        aws->at("max_dist");
        aws->create_input_field(AWAR_CLUSTER_MAXDIST, 12);

        aws->at("min_size");
        aws->create_input_field(AWAR_CLUSTER_MINSIZE, 5);

        aws->at("calculate");
        aws->callback(calculate_clusters);
        aws->create_autosize_button("CALC", "Detect clusters");

        aws->button_length(20);
        aws->at("tree_name");
        aws->create_button("TREE", AWAR_DIST_TREE_CURR_NAME);

        // -------------------
        //      lower area

        aws->button_length(18);

        // column 1
        
        aws->at("mark_all"); aws->callback(mark_clusters, ALL_CLUSTERS, true); aws->create_button("MARKALL", "Mark all");
        
        aws->at("auto_mark");  aws->create_toggle(AWAR_CLUSTER_AUTOMARK);
        aws->at("mark_rep");   aws->create_toggle(AWAR_CLUSTER_MARKREP);
        aws->at("select_rep"); aws->create_toggle(AWAR_CLUSTER_SELECTREP);
        
        aws->at("mark"); aws->callback(mark_clusters, SEL_CLUSTER, true); aws->create_button("MARK", "Mark cluster");

        // column 2

        aws->sens_mask(AWM_DISABLED);
        aws->at("group"); aws->callback(group_cluster); aws->create_button("GROUP", "Create group");
        aws->sens_mask(AWM_ALL);
        
        // store/restore

        aws->at("store_all"); aws->callback(store_clusters,  ALL_CLUSTERS); aws->create_button("STOREALL", "Store all");
        aws->at("store")    ; aws->callback(store_clusters,  SEL_CLUSTER);  aws->create_button("STORESEL", "Store selected");
        aws->at("restore")  ; aws->callback(restore_clusters);              aws->create_button("RESTORE",  AWAR_CLUSTER_RESTORE_LABEL);
        aws->at("swap")     ; aws->callback(swap_clusters);                 aws->create_button("Swap",     "Swap stored");

        // column 4

        aws->at("clear")    ; aws->callback(delete_clusters, ALL_CLUSTERS); aws->create_button("CLEAR",    "Clear list");
        aws->at("delete")   ; aws->callback(delete_clusters, SEL_CLUSTER) ; aws->create_button("DEL",      "Delete selected");
        aws->sens_mask(AWM_DISABLED);
        aws->at("save")   ; aws->callback(save_clusters)   ; aws->create_button("SAVE",    "Save list");
        aws->at("load")   ; aws->callback(load_clusters)   ; aws->create_button("LOAD",    "Load list");
        aws->sens_mask(AWM_ALL);


        // --------------------
        //      clusterlist

        aws->at("cluster_list");
        global_data->clusterList = aws->create_selection_list(AWAR_CLUSTER_SELECTED, "Found clusters");
        update_cluster_sellist(aws);

        aw_root->awar(AWAR_CLUSTER_SELECTED)->add_callback(select_cluster_cb);
    }

    return aws;
}





