//  ==================================================================== //
//                                                                       //
//    File      : pg_main.cxx                                            //
//    Time-stamp: <Sat Sep/27/2003 16:38 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Tina Lai & Ralf Westram (coder@reallysoft.de) 2001-2003     //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef PG_SEARCH_HXX
#include "pg_search.hxx"
#endif

#include <deque>
#include <algorithm>

#include <smartptr.h>
#include <output.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "../global_defs.h"
#define SKIP_GETDATABASESTATE
#define SKIP_DECODETREENODE
#include "../common.h"

// #define SAVE_AFTER_XXX_NEW_GROUPS 10000


// bond values used:
#define AA  0
#define AC  0
#define AG  0.5
#define AU  1.1

#define CA  AC
#define CC  0
#define CG  1.5
#define CU  0

#define GA  AG
#define GC  CG
#define GG  0
#define GU  0.9

#define UA  AU
#define UC  CU
#define UG  GU
#define UU  0

using namespace std;

// globals:

GBDATA *gb_main = 0;


typedef deque<SpeciesID>     SpeciesBag;
typedef SmartPtr<SpeciesBag> SpeciesBagPtr;
typedef deque<SpeciesBagPtr> SpeciesBagStack;

//  --------------------------
//      class TreeTraversal
//  --------------------------
class TreeTraversal {
private:
    GBT_TREE        *currentSubTree;
    string           path;
    SpeciesBagPtr    species;
    SpeciesBagStack  stack;     // stores SpeciesBags of left branches

    void decentLeftmostSon() {
        int depth = 0;
        while (currentSubTree->leftson) {
            currentSubTree = currentSubTree->leftson;
            depth++;
        }
        path.append(depth, 'L');

        species = new SpeciesBag;
        species->push_back(PG_SpeciesName2SpeciesID(currentSubTree->name));
    }

public:

    TreeTraversal(GBT_TREE *rootnode) {
        currentSubTree = rootnode;
        decentLeftmostSon();
    }

    GBT_TREE *getSubTree() { return currentSubTree; }
    const string& getPath() { pg_assert(currentSubTree); return path; }
    const SpeciesBag& getSpecies() { pg_assert(currentSubTree); return *species; }

    TreeTraversal& operator++() { // prefix increment
        GBT_TREE *father = currentSubTree->father;
        if (father) {
            if (currentSubTree == father->leftson) {
                stack.push_back(species); // store species from left subtree

                currentSubTree   = father->rightson;
                *(path.rbegin()) = 'R';
                decentLeftmostSon();
            }
            else {              // we were in right branch
                // add species from left branch
                SpeciesBagPtr leftSpecies = stack.back();
                stack.pop_back();
                copy(leftSpecies->begin(), leftSpecies->end(), back_inserter(*species));

                currentSubTree = father;
                path.erase(path.length()-1);
            }
        }
        else { // at root -> done
            currentSubTree = 0;
        }

        return *this;
    }
};


static output out;

//  -------------------------------------------------------
//      static void my_output(const char *format, ...)
//  -------------------------------------------------------
// output-handler for other modules

static void my_output(const char *format, ...) {
    va_list     parg;
    va_start(parg, format);
    out.vput(format, parg);
}

// command line arguments:

//  -------------------------
//      struct Parameter
//  -------------------------
struct Parameter {
    string db_name;
    string tree_name;
    string pt_server_name;
    string db_out_name;
    string db_out_alt_name;
    string db_out_alt_name2;
    int pb_length;
};

static Parameter para;

//  -----------------------------
//      void helpArguments()
//  -----------------------------
void helpArguments() {
    fprintf(stderr,
            "\n"
            "Usage: arb_probe_group <db_name> <tree_name> <pt_server> <db_out> <pb_length>\n"
            "\n"
            "db_name        name of ARB-database to build groups for\n"
            "tree_name      name of tree to use for distance calculations\n"
            "pt_server      name of pt_server\n"
            "db_out         name of Probe-Group-database to create\n"
            "pb_length      probe length\n"
            "\n"
            //             "options: [none yet]\n"
            );
}

//  --------------------------------------------------------
//      GB_ERROR scanArguments(int argc,  char *argv[])
//  --------------------------------------------------------
GB_ERROR scanArguments(int argc,  char *argv[]) {
    GB_ERROR error = 0;

    if (argc >= 6) {
        para.db_name = argv[1];
        para.tree_name = argv[2];
        para.pt_server_name = argv[3];
        para.pb_length = atoi(argv[5]);
        string name=strcat(argv[4],argv[5]);
        para.db_out_name = name + "tmp.arb";
        para.db_out_alt_name2 = name + "tmp2.arb";
        para.db_out_alt_name = name + ".arb";
        argc -= 5; argv += 5;

#if 0
        only needed for options:

            while (!error && argc>=2) {
                string arg = argv[1];
                argc--; argv++;


            }
#endif
    }
    else {
        error = "Missing arguments";
    }

    if (error) helpArguments();
    return error;
}

//  -----------------------------------------------------------------------
//      void generate_GBT_TREE_hash_rek(GBT_TREE *node, GB_HASH *hash)
//  -----------------------------------------------------------------------
void generate_GBT_TREE_hash_rek(GBT_TREE *node, GB_HASH *hash) {
    if (node->is_leaf) {
        if (node->gb_node) {
            GBDATA *gb_name = GB_find(node->gb_node,"name",0,down_level);
            if (gb_name) {
                GBS_write_hash(hash,GB_read_char_pntr(gb_name),(long)node);
            }
        }
    }
    else {
        generate_GBT_TREE_hash_rek(node->leftson,hash);
        generate_GBT_TREE_hash_rek(node->rightson,hash);
    }
}

//  --------------------------------------------------------
//      GB_HASH *generate_GBT_TREE_hash(GBT_TREE *root)
//  --------------------------------------------------------
GB_HASH *generate_GBT_TREE_hash(GBT_TREE *root) {
    GB_HASH *hash = GBS_create_hash(GBS_SPECIES_HASH_SIZE,0);
    generate_GBT_TREE_hash_rek(root,hash);
    return hash;
}

//  -------------------------------
//      void PG_status(double)
//  -------------------------------
int PG_status(double) {
    return 0;
}

// ---------------------------------------------------------------------------------------
//      static GB_ERROR save(GBDATA *pb_main, const string& nameS, const char *state)
// ---------------------------------------------------------------------------------------
static GB_ERROR save(GBDATA *pb_main, const string& nameS, const char *state) {
    GB_ERROR error = setDatabaseState(pb_main, 0, state);

    if (!error) {
        const char *name = nameS.c_str();
        out.put("Saving %s ...", name);
        error = GB_save(pb_main, name, SAVE_MODE); // was "a"
    }

    return error;
}

//  ---------------------------------------
//      int percent(int part, int all)
//  ---------------------------------------
int percent(int part, int all) {
    if (part) {
        pg_assert(all>0);
        pg_assert(part>0);

        return int(100*(double(part)/all)+0.5);
    }
    return 0;
}

inline void convert_kommas_to_underscores(char *s) {
    if (s) {
        for (int i = 0; s[i]; ++i) {
            if (s[i] == ',') s[i] = '_';
        }
    }
}

GB_ERROR PG_tree_change_node_info(GBT_TREE *node) {
    GB_ERROR error = 0;
    if (!node->is_leaf) {
        if (node->name) { // write group name as remark branch
            node->remark_branch = encodeTreeNode(node->name);
        }

        error             = PG_tree_change_node_info(node->leftson);
        if (!error) error = PG_tree_change_node_info(node->rightson);
        return error;
    }

    // leaf
    SpeciesID id = PG_SpeciesName2SpeciesID(node->name);

    char *fullname = 0;
    char *acc      = 0;

    if (node->gb_node) {
        GBDATA *gb_fullname = GB_find(node->gb_node, "full_name", 0, down_level);
        if (gb_fullname) {
            fullname = GB_read_string(gb_fullname);
            convert_kommas_to_underscores(fullname);
        }
        else {
            error = GBS_global_string("no 'full_name' for species '%s'", node->name);
        }

        if (!error) {
            GBDATA *gb_acc = GB_find(node->gb_node, "acc", 0, down_level);
            if (gb_acc) {
                acc = GB_read_string(gb_acc);
                convert_kommas_to_underscores(acc);
            }
            else {
                error = GBS_global_string("no 'acc' for species '%s'", node->name);
            }
        }
    }
    else {
        error = "Unlinked leaf node";
    }

    if (!error) {
        free(node->name);
        node->name = encodeTreeNode(GBS_global_string("%i,%s,%s", id, fullname, acc));
    }

    free(acc);
    free(fullname);

    return error;
}

static GB_ERROR openDatabases(GBDATA*& gb_main, GBDATA*& pb_main, GBDATA*& pba_main, GBDATA*& pbb_main) {
    GB_ERROR error = 0;

    out.put("Opening databases..");
    indent i(out);

    // open arb-database:
    {
        const char *name = para.db_name.c_str();

        out.put("Opening ARB-database '%s'..", name);
        gb_main = GBT_open(name, "rwt", 0); // eigentlich "rwh"
        if (!gb_main) {
            error = GB_get_error();
            if (!error) error = GB_export_error("Can't open database '%s'", name);
        }
    }

    // open probe-group-database:
    if (!error) {
        const char *name = para.db_out_name.c_str();

        out.put("Opening probe-group-database '%s'..", name);
        pb_main = GB_open(name, "wcN");//"rwch");
        if (!pb_main) {
            error             = GB_get_error();
            if (!error) error = GB_export_error("Can't open database '%s'", name);
        }
        if (!error) error = setDatabaseState(pb_main, "probe_group_db", "empty");
    }

    //open path-mapping-database:
    if (!error) {
        const char *name = para.db_out_alt_name2.c_str();

        out.put("Opening path-mapping-database '%s'..",name);
        pbb_main = GB_open(name,"wch");
        if (!pbb_main){
            error            = GB_get_error();
            if(!error) error = GB_export_error("Can't open database '%s'..",name);
        }
        if (!error) error = setDatabaseState(pbb_main, "probe_group_mapping_db", "empty");
    }

    //open probe-group-subtree-result-database:
    if (!error) {
        const char *name = para.db_out_alt_name.c_str();

        out.put("Opening probe-group-subtree-result-database '%s'..",name);
        pba_main = GB_open(name,"wch");
        if (!pba_main){
            error            = GB_get_error();
            if(!error) error = GB_export_error("Can't open database '%s'",name);
        }
        if (!error) error = setDatabaseState(pba_main, "probe_group_subtree_db", "empty");
    }

    return error;
}

static GB_ERROR probeDatabaseWriteCounters(GBDATA *pba_main, int subtreeCounter, int probeGroupCounter, int probeSubtreeCounter) {
    GB_ERROR error = 0;
    GB_transaction dummy(pba_main);

    {
        GBDATA *stCounter    = GB_search(pba_main,"subtree_counter",GB_INT);
        if (stCounter) error = GB_write_int(stCounter,subtreeCounter);
        else    error        = GB_get_error();
    }

    if (!error) {
        GBDATA *stCounter    = GB_search(pba_main,"probe_group_counter",GB_INT);
        if (stCounter) error = GB_write_int(stCounter,probeGroupCounter);
        else    error        = GB_get_error();
    }

    if (!error) {
        GBDATA *stProbeCounter    = GB_search(pba_main,"subtree_with_probe_counter",GB_INT);
        if (stProbeCounter) error = GB_write_int(stProbeCounter,probeSubtreeCounter);
        else    error             = GB_get_error();
    }

    return error;
}

static GB_ERROR pathMappingWriteGlobals(GBDATA *pm_main, int depth) {
    GB_ERROR error = 0;
    {
        GBDATA *pm_num    = GB_search(pm_main,"species_number",GB_INT);
        if (pm_num) error = GB_write_int(pm_num,PG_NumberSpecies());
        else    error     = GB_get_error();
    }

    if (!error) {
        GBDATA *pm_depth    = GB_search(pm_main,"depth",GB_INT);
        if (pm_depth) error = GB_write_int(pm_depth,depth);
        else    error       = GB_get_error();
    }

    return error;
}

static GBT_TREE *readAndLinkTree(GBDATA *gb_main, GB_ERROR& error) {
    GB_transaction dummy(gb_main);

    out.put("Reading tree '%s'..", para.tree_name.c_str());
    GBT_TREE *gbt_tree = GBT_read_tree(gb_main, para.tree_name.c_str(),sizeof(GBT_TREE));

    if (!gbt_tree) {
        error = GB_export_error("Tree '%s' not found.", para.tree_name.c_str());
    }
    else {
        indent i(out);
        out.put("Linking tree..");
        GBT_link_tree(gbt_tree,gb_main,true);
    }

    return gbt_tree;
}

static GB_ERROR collectProbes(GBDATA *pb_main) {
    GB_ERROR error = 0;
    string pt_server_name = "localhost: "+para.pt_server_name;
    PG_init_pt_server(gb_main, pt_server_name.c_str(), my_output);

    size_t group_count = 0;

    int length = para.pb_length;
    out.put("Searching probes with length %i ..", length);

    size_t probe_count = 0;
    PG_init_find_probes(length, error);
    if (!error) {
        const char *probe;
        while ((probe = PG_find_next_probe(error)) && !error) {
            probe_count++;
        }
        PG_exit_find_probes();
    }

    if (!error) {
        indent i(out);
        out.put("Probes found: %i", probe_count);
    }

    if (!error) {
        out.put("Calculating probe-matches for found probes:");
        out.setMaxPoints(probe_count);
        indent i1(out);

        PG_init_find_probes(length, error);
        if (!error) {
            const char *probe;
            size_t probe_count2 = 0;
            PG_probe_match_para pm_para =
                {
                    // bonds:
                    { AA, AC, AG, AU, CA, CC, CG, CU, GA, GC, GG, GU, UA, UC, UG, UU},
                    0.5, 0.5, 0.5
                };

            while ((probe = PG_find_next_probe(error)) && !error) {
                probe_count2++;
                PG_Group group;
                error = PG_probe_match(group, pm_para, probe);

                if (!error) {
                    if (group.empty()) {
                        static bool warned = false;
                        if (!warned) {
                            out.put("Warning: pt-server seems to be out-of-date");
                            warned = true;
                        }
                    }
                    else {
                        if (group.size()>0) {
                            GB_transaction  pb_dummy(pb_main);
                            bool            created;
                            int             numSpecies;
                            GBDATA         *pb_group = group.groupEntry(pb_main, true, created, &numSpecies);

                            PG_add_probe(pb_group, probe);

                            if (created) {
                                out.point();
                                ++group_count;
                            }
                        }
                    }
                }
            }

            PG_exit_find_probes();
        }
    }

//     GB_begin_transaction(pb_main);
//     gb_status = GB_search(pb_main,"status",GB_CREATE_CONTAINER);
//     GBDATA *gb_probesMatched = GB_search(gb_status,"probes_matched",GB_INT);
//     error = GB_write_int(gb_probesMatched,1);
//     if (error) GB_abort_transaction(pb_main);
//     else GB_commit_transaction(pb_main);

    PG_exit_pt_server();
    out.put("Overall groups found: %i (of %i possible groups)", group_count, probe_count);

    return error;
}

static GB_ERROR storeProbeGroupStatistic(GBDATA *pba_main, int *statistic, int stat_size) {
    GB_ERROR  error    = 0;
    GBDATA   *pba_stat = GB_search(pba_main,"probe_group_vs_species",GB_STRING);

    if (!pba_stat) {
        error = GB_get_error();
    }
    else {
        string data;
        for(int i=0; i<stat_size; i++){
            if (statistic[i]) {
                char buffer[20];
                sprintf(buffer,"%i,%i;",i+1,statistic[i]);
                data += buffer;
            }
        }
        error = GB_write_string(pba_stat,data.c_str());
    }

    return error;
}

static string PG_SpeciesBag_Content(const SpeciesBag& species) {
    string result;
    for (SpeciesBag::const_iterator s = species.begin(); s != species.end(); ++s) {
        char buffer[100];
        sprintf(buffer, "%i,", *s);
        result += buffer;
    }
    result.erase(result.length()-1);
    return result;
}

static GB_ERROR findExactSubtrees(GBT_TREE *gbt_tree, GBDATA *pb_main, GBDATA *pba_main, GBDATA *pbb_main) {
    GB_ERROR      error               = 0;
    TreeTraversal tt(gbt_tree);
    int           statistic_size      = PG_NumberSpecies();
    int           statistic[statistic_size];
    int           count               = 1;
    int           probeSubtreeCounter = 0;
    int           probeGroupCounter   = 0;
    unsigned      depth               = 0;

    out.put("Extracting groups for subtrees:");
    indent i(out);

    for (int i=0; i<statistic_size; ++i) statistic[i] = 0;

    GB_transaction dummya(pba_main);
    GB_transaction dummyb(pbb_main);

    GBDATA *subtree_cont     = GB_search(pba_main,"subtrees",GB_CREATE_CONTAINER);
    GBDATA *probe_group_cont = GB_search(pba_main,"probe_groups",GB_CREATE_CONTAINER);
    GBDATA *pm_main          = GB_search(pbb_main,"path_mapping",GB_CREATE_CONTAINER);

    int subtreeCounter = 0;
    for (GBT_TREE *current = tt.getSubTree();
         current && !error;
         current       = (++tt).getSubTree())
    {
        subtreeCounter++;
        count++;

        const SpeciesBag& species = tt.getSpecies();
//         out.put("subtree '%s' = %s", tt.getPath().c_str(), PG_SpeciesBag_Content(species).c_str());

        if (current->is_leaf) { // leaf
            // path mapping stuff:
            GBDATA *pm_species = GB_create_container(pm_main,"species");
            GBDATA *pm_name    = GB_search(pm_species,"name",GB_STRING);
            error              = GB_write_string(pm_name,PG_SpeciesID2SpeciesName(species.front()).c_str());

            if (!error) {
                GBDATA *pm_path = GB_search(pm_species,"path",GB_STRING);
                error           = GB_write_string(pm_path,tt.getPath().c_str());

                size_t path_len           = tt.getPath().length();
                if (path_len>depth) depth = path_len;
            }
        }

        if (!error) {
            deque<string> probes;

            {
                GB_transaction  dummy(pb_main);
                GBDATA         *pbtree = GB_search(pb_main,"group_tree",GB_FIND);
                GBDATA         *pbnode = GB_find(pbtree,"node",0,down_level);
                PG_find_probe_for_subtree(pbnode, species, probes);
            }

            // create 'subtree' for all nodes
            GBDATA *st_subtree = GB_create_container(subtree_cont,"subtree");
            GBDATA *st_path    = GB_search(st_subtree,"path",GB_STRING);
            error              = GB_write_string(st_path,tt.getPath().c_str());

            if (current->is_leaf) {
                GBDATA *st_member = GB_search(st_subtree, "member", GB_INT);
                error             = GB_write_int(st_member, species.front());
            }

            if (!error && !probes.empty()) {
                statistic[species.size()-1]++;

                probeSubtreeCounter++;
                probeGroupCounter++;
                out.point();

                // create a probe group for the subtree
                GBDATA *st_group   = GB_create_container(probe_group_cont, "probe_group");
                GBDATA *st_members = GB_search(st_group, "subtreepath", GB_STRING);

                error = GB_write_string(st_members, tt.getPath().c_str());
                if (!error) {
                    GBDATA *st_matches = GB_search(st_group,"probe_matches",GB_CREATE_CONTAINER);
                    for (deque<string>::const_iterator j = probes.begin(); j != probes.end() && !error; ++j) {
                        GBDATA *st_probe = GB_create(st_matches,"probe",GB_STRING);
                        error            = GB_write_string(st_probe, j->c_str());
                    }
                }


                GBDATA *st_exact = GB_search(st_subtree, "exact", GB_INT);
                GB_write_int(st_exact, probes.size());
            }
        }
    } // end of traversal

    if (!error) error = probeDatabaseWriteCounters(pba_main, subtreeCounter, probeGroupCounter, probeSubtreeCounter);
    if (!error) error = pathMappingWriteGlobals(pm_main, depth);
    if (!error) error = storeProbeGroupStatistic(pba_main, statistic, statistic_size);

    out.put("Number of Subtrees:         %i", subtreeCounter);
    out.put("Subtrees with exact probes: %i (=%i%%)", probeSubtreeCounter, int(double(probeSubtreeCounter)/subtreeCounter*100+.5));

    return error;
}

static void unlinkTreeAndFreeNodeInfo(GBT_TREE *tree) {
    tree->gb_node = 0;
    free(tree->name);
    tree->name    = 0;

    if (!tree->is_leaf) {
        unlinkTreeAndFreeNodeInfo(tree->leftson);
        unlinkTreeAndFreeNodeInfo(tree->rightson);
    }
}

static GBT_TREE *PG_find_subtree_by_path(GBT_TREE *node, const char *path) {
    const char *restPath = path;

    while (node && !node->is_leaf) {
        char go = restPath[0];

        if (go == 'L') {
            node = node->leftson;
            restPath++;
        }
        else if (go == 'R') {
            node = node->rightson;
            restPath++;
        }
        else if (go) {
            GB_export_error("Illegal character '%c' in path (%s)", go, path);
            node = 0;
        }
        else { // end of path reached
             break;
        }
    }


    // leaf reached
    if (node && restPath[0]) {
        GB_export_error("Leaf reached. Cannot follow rest (%s) of path (%s)", restPath, path);
        node = 0;
    }

    return node;
}

static GB_ERROR linkTreeToSubtrees(GBT_TREE *tree, GBDATA *pba_main) {
    GB_ERROR error = 0;
    GB_transaction dummy(pba_main);

    unlinkTreeAndFreeNodeInfo(tree);

    GBDATA *pba_subtrees = GB_find(pba_main, "subtrees", 0, down_level);
    if (!pba_subtrees) {
        error = "cannot find 'subtrees'";
    }
    else {
        for (GBDATA *pba_subtree = GB_find(pba_subtrees, "subtree", 0, down_level);
             pba_subtree && !error;
             pba_subtree = GB_find(pba_subtree, "subtree", 0, this_level|search_next))
        {
            GBDATA *pba_path = GB_find(pba_subtree, "path", 0, down_level);
            if (!pba_path) {
                error = "cannot find 'path'";
            }
            else {
                const char *path = GB_read_char_pntr(pba_path);
                GBT_TREE   *node = PG_find_subtree_by_path(tree, path);
                if (!node) {
                    error = GB_get_error();
                }
                else {
                    if (node->gb_node) {
                        error = GBS_global_string("duplicated subtree '%s'", path);
                    }
                    else {
                        node->gb_node = pba_subtree; // link to subtree
                    }
                }
            }
        }
    }

    return error;
}

static int insertChildCounters(GBT_TREE *node, GB_ERROR& error) {
    if (node->is_leaf) {
        return 1;
    }

    int leftChilds = insertChildCounters(node->leftson, error);
    if (!error) {
        int rightChilds = insertChildCounters(node->rightson, error);
        if (!error) {
            int     myChilds     = leftChilds+rightChilds;
            GBDATA *pb_subtree   = node->gb_node;
            GBDATA *pb_speccount = GB_search(pb_subtree, "speccount", GB_INT);

            error = GB_write_int(pb_speccount, myChilds);
            return myChilds;
        }
    }

    return -1; // error
}

static GB_ERROR propagateExactSubtrees(GBT_TREE *node, int& hits, const char*& id) {
    GB_ERROR  error      = 0;
    GBDATA   *pb_subtree = node->gb_node;
    GBDATA   *pb_exact   = GB_find(pb_subtree, "exact", 0, down_level);

    if (pb_exact) {             // have an exact probe_group here
        if (node->is_leaf) {
            hits = 1;
        }
        else {
            GBDATA *pb_speccount = GB_find(pb_subtree, "speccount", 0, down_level);
            pg_assert(pb_speccount); // all inner nodes must have 'speccount'
            hits                 = GB_read_int(pb_speccount);
        }

        GBDATA *gb_path = GB_find(pb_subtree, "path", 0, down_level);
        id              = GB_read_char_pntr(gb_path);
    }
    else {                      // no exact probe_group -> find best Groups
        if (node->is_leaf) {
            hits = 0;
            id   = 0;
        }
        else {
            int         leftHits, rightHits;
            const char *leftId, *rightId;

            error             = propagateExactSubtrees(node->leftson, leftHits, leftId);
            if (!error) error = propagateExactSubtrees(node->rightson, rightHits, rightId);

            if (!error) {
                if (leftHits>rightHits) {
                    hits = leftHits;
                    id   = leftId;
                }
                else {
                    hits = rightHits;
                    id   = rightId;
                }

                if (hits) { // we have a subgroup
                    GBDATA *pb_coverage = GB_search(pb_subtree, "coverage", GB_INT);
                    error               = GB_write_int(pb_coverage, hits);

                    if (!error) {
                        GBDATA *pb_coverage_id = GB_search(pb_subtree, "coverage_id", GB_STRING);
                        error                  = GB_write_string(pb_coverage_id, id);
                    }
                }
            }
        }
    }

    return error;
}

int main(int argc,char *argv[]) {
    out.put("arb_probe_group v2.0 -- (C) 2001-2003 by Tina Lai & Ralf Westram");
    GB_ERROR error = scanArguments(argc, argv);

    {
        indent i(out);

        GBDATA   *gb_main  = 0;
        GBDATA   *pb_main  = 0;
        GBDATA   *pba_main = 0;
        GBDATA   *pbb_main = 0;

        if (!error) error = openDatabases(gb_main, pb_main, pba_main, pbb_main);

        if (!error) {
            GB_install_status(PG_status); // to show progress of tree linking etc.
            PG_initSpeciesMaps(gb_main, pb_main);

            GBT_TREE *gbt_tree   = readAndLinkTree(gb_main, error);
            // GB_HASH  *gb_hash = 0;
            // if (!error) {
            // GB_transaction dummy(gb_main);
            // gb_hash = generate_GBT_TREE_hash(gbt_tree);
            // }
            if (!error) {
                GB_transaction dummy(pb_main);
                error = GBT_write_plain_tree(pb_main, pb_main, 0, gbt_tree);
            }

            if (!error) error = collectProbes(pb_main);
            if (!error) error = findExactSubtrees(gbt_tree, pb_main, pba_main, pbb_main);
            if (!error) {
                GB_transaction dummy(gb_main);
                error = PG_tree_change_node_info(gbt_tree); // exchange species-names against species ids in tree
                if (!error) {
                    GB_transaction dummy(pba_main);
                    error = GBT_write_plain_tree(pba_main, pba_main, 0, gbt_tree);
                }
            }
            if (!error) error = PG_transfer_root_string_field(pb_main, pba_main, "species_mapping"); // copy species_mapping

            if (!error) {
                GB_transaction dummy(pba_main);

                // From here gbt_tree is not linked to species.
                // Instead it is linked to subtree containers of pba_main!

                error = linkTreeToSubtrees(gbt_tree, pba_main);
                if (!error) insertChildCounters(gbt_tree, error);
                if (!error) {
                    int         dummy1;
                    const char *dummy2;
                    error = propagateExactSubtrees(gbt_tree, dummy1, dummy2);
                }
            }

            // if (gb_hash) GBS_free_hash(gb_hash);
            if (gbt_tree) GBT_delete_tree(gbt_tree);
        }

        // save databases

        {
            out.put("Saving databases..");
            indent i(out);
            if (!error) error = save(pb_main, para.db_out_name, "complete");
            if (!error) error = save(pba_main, para.db_out_alt_name, "complete");
            if (!error) error = save(pbb_main, para.db_out_alt_name2, "complete");
        }
    }

    if (error) {
        out.put("Error in arb_probe_group: %s\n", error);
        return EXIT_FAILURE;
    }

    out.put("arb_probe_group done.");
    return EXIT_SUCCESS;
}

