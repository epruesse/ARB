/*********************************************************************************
 *  Coded by Tina Lai/Ralf Westram (coder@reallysoft.de) 2001-2003               *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef PG_SEARCH_HXX
#include "pg_search.hxx"
#endif

#ifndef __SET__
#include <set>
#endif
#include <algorithm>

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


//  --------------------------
//      class TreeTraversal
//  --------------------------
class TreeTraversal {
private:
    GBT_TREE *currentSubTree;

public:
    string              path;
    std::set<SpeciesID> species;

    TreeTraversal(GBT_TREE *rootnode) {
        currentSubTree = rootnode;
    }
    virtual ~TreeTraversal() {}

    GBT_TREE *getNextSubTree() {
        if (!currentSubTree->is_leaf) {
            currentSubTree  = currentSubTree->leftson;
            path           += "L";
        }
        else { // leaf -> go up
            while (currentSubTree==currentSubTree->father->rightson){
                currentSubTree = currentSubTree->father;
                path.erase(path.length()-1);
                if (!currentSubTree->father) return 0; // back to root position
            }
            currentSubTree        = currentSubTree->father->rightson;
            path[path.length()-1] = 'R';
        }
        return currentSubTree;

    }

    void getSpeciesNumber(GBT_TREE *tree){ //this function will return all the species number under tree
        if (!tree->is_leaf){
            getSpeciesNumber(tree->leftson);
            getSpeciesNumber(tree->rightson);
        }
        else {
            species.insert(PG_SpeciesName2SpeciesID(tree->name));
        }
    }
};

//  ---------------------
//      class output
//  ---------------------

class output {
private:
    int     indentation;
    bool    printing_points;

    inline void cr() const { fputc('\n', stdout); }

    inline void goto_indentation() {
        if (printing_points) {
            cr();
            printing_points = false;
        }
        for (int i = 0; i<indentation; ++i) fputc(' ', stdout);
    }

public:
    output() { indentation = 0; }
    virtual ~output() {}

    void indent(int howMuch) { indentation   += howMuch; }
    void unindent(int howMuch) { indentation -= howMuch; }

    void vput(const char *s, va_list argPtr) {
        goto_indentation();
        vfprintf(stdout, s, argPtr);
        cr();
    }

    void put(const char *s, ...) {
        va_list parg;
        va_start(parg, s);
        vput(s, parg);
    }

    void put()  { put(""); }

    void point() {
        if (!printing_points) {
            goto_indentation();
            printing_points = true;;
        }
        fputc('.', stdout);
        fflush(stdout);
    }
};

//  ---------------------
//      class indent
//  ---------------------
class indent {
private:
    int     indentation;
    output& out;

public:
    indent(int ind, output& out_) : indentation(ind), out(out_) {
        out.indent(indentation);
    }
    virtual ~indent() {
        out.unindent(indentation);
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
    if (!node->is_leaf) {
        if (node->name) { // write group name as remark branch
            node->remark_branch = encodeTreeNode(node->name);
        }

        GB_ERROR error    = PG_tree_change_node_info(node->leftson);
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
        GBDATA *gb_acc = GB_find(node->gb_node, "acc", 0, down_level);
        if (gb_acc) {
            acc = GB_read_string(gb_acc);
            convert_kommas_to_underscores(acc);
        }
    }

    free(node->name);
    node->name = encodeTreeNode(GBS_global_string("%i,%s,%s",
                                                  id,
                                                  fullname ? fullname : "<none>",
                                                  acc ? acc : "<none>"));

    free(acc);
    free(fullname);

    return 0;
}

static GB_ERROR openDatabases(GBDATA*& gb_main, GBDATA*& pb_main, GBDATA*& pba_main, GBDATA*& pbb_main) {
    GB_ERROR error = 0;

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
        pb_main = GB_open(name, "rwcN");//"rwch");
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
        GBDATA *stCounter    = GB_create(pba_main,"subtree_counter",GB_INT);
        if (stCounter) error = GB_write_int(stCounter,subtreeCounter);
        else    error        = GB_get_error();
    }

    if (!error) {
        GBDATA *stCounter    = GB_create(pba_main,"probe_group_counter",GB_INT);
        if (stCounter) error = GB_write_int(stCounter,probeGroupCounter);
        else    error        = GB_get_error();
    }

    if (!error) {
        GBDATA *stProbeCounter    = GB_create(pba_main,"subtree_with_probe_counter",GB_INT);
        if (stProbeCounter) error = GB_write_int(stProbeCounter,probeSubtreeCounter);
        else    error             = GB_get_error();
    }

    return error;
}

static GB_ERROR pathMappingWriteGlobals(GBDATA *pm_main, int depth) {
    GB_ERROR error = 0;
    {
        GBDATA *pm_num    = GB_create(pm_main,"species_number",GB_INT);
        if (pm_num) error = GB_write_int(pm_num,PG_NumberSpecies());
        else    error     = GB_get_error();
    }

    if (!error) {
        GBDATA *pm_depth    = GB_create(pm_main,"depth",GB_INT);
        if (pm_depth) error = GB_write_int(pm_depth,depth);
        else    error       = GB_get_error();
    }

    return error;
}

static GBT_TREE *readAndLinkTree(GBDATA *gb_main, GB_ERROR& error) {
    GB_transaction dummy(gb_main);

    out.put("Reading tree '%s'..", para.tree_name.c_str());
    GBT_TREE *gbt_tree = GBT_read_tree(gb_main,para.tree_name.c_str(),sizeof(GBT_TREE));

    if (!gbt_tree) {
        error = GB_export_error("Tree '%s' not found.", para.tree_name.c_str());
    }
    else {
        out.put("Linking tree..");
        GBT_link_tree(gbt_tree,gb_main,true);
    }

    return gbt_tree;
}

static GB_ERROR collectProbes(GBDATA *pb_main) {
    GB_ERROR error = 0;
    string pt_server_name = "localhost: "+para.pt_server_name;
    PG_init_pt_server(gb_main, pt_server_name.c_str(), my_output);

    out.put("Here we go:");

    indent i0(2, out);
    size_t group_count = 0;

    int length=para.pb_length;
    out.put("Searching probes with length %i ..", length);
    indent i1(2, out);

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
        out.put("Probes found: %i", probe_count);
        out.put("Calculating probe-matches for found probes:");
        indent i2(2, out);

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
    out.put("Overall groups found: %i", group_count);

    return error;
}

static GB_ERROR storeProbeGroupStatistic(GBDATA *pba_main, int *statistic, int stat_size) {
    GB_ERROR  error    = 0;
    GBDATA   *pba_stat = GB_create(pba_main,"probe_group_vs_species",GB_STRING);

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

static GB_ERROR findExactSubtrees(GBT_TREE *gbt_tree, GBDATA *pb_main, GBDATA *pba_main, GBDATA *pbb_main) {
    GB_ERROR       error               = 0;
    TreeTraversal  tt(gbt_tree);
    int            statistic_size      = PG_NumberSpecies();
    int            statistic[statistic_size];
    GBT_TREE      *current;
    int            count               = 1;
    int            subtreeCounter      = 0;
    int            probeSubtreeCounter = 0;
    int            probeGroupCounter   = 0;
    unsigned       depth               = 0;

    for (int i=0; i<statistic_size; ++i) statistic[i] = 0;

    GB_transaction dummya(pba_main);
    GB_transaction dummyb(pbb_main);

    GBDATA *subtree_cont     = GB_search(pba_main,"subtrees",GB_CREATE_CONTAINER);
    GBDATA *probe_group_cont = GB_search(pba_main,"probe_groups",GB_CREATE_CONTAINER);
    GBDATA *pm_main          = GB_search(pbb_main,"path_mapping",GB_CREATE_CONTAINER);

    while ((current=tt.getNextSubTree()) && !error) {
        subtreeCounter++;
        //                 out.put("%i %s %s",count,tt.path.c_str(),current->name);
        count++;
        tt.species.erase(tt.species.begin(),tt.species.end());
        tt.getSpeciesNumber(current);

        if (current->is_leaf) { // leaf
            // path mapping stuff:
            GBDATA *pm_species = GB_create_container(pm_main,"species");
            GBDATA *pm_name    = GB_create(pm_species,"name",GB_STRING);
            error              = GB_write_string(pm_name,PG_SpeciesID2SpeciesName(*tt.species.begin()).c_str());

            if (!error) {
                GBDATA *pm_path = GB_create(pm_species,"path",GB_STRING);
                error           = GB_write_string(pm_path,tt.path.c_str());

                if (tt.path.length()>depth) depth = tt.path.length();
            }
        }

        if (!error) {
            std::set<string> probes;
            {
                GB_transaction  dummy(pb_main);
                GBDATA         *pbtree = GB_search(pb_main,"group_tree",GB_FIND);
                GBDATA         *pbnode = GB_find(pbtree,"node",0,down_level);
                PG_find_probe_for_subtree(pbnode,tt.species,&probes);
            }

            // create 'subtree' for all nodes
            GBDATA *st_subtree = GB_create_container(subtree_cont,"subtree");
            GBDATA *st_path    = GB_search(st_subtree,"path",GB_STRING);
            error              = GB_write_string(st_path,tt.path.c_str());

            if (current->is_leaf) {
                GBDATA *st_member = GB_search(st_subtree, "member", GB_INT);
                error             = GB_write_int(st_member, *tt.species.begin());
            }

            if (!error && !probes.empty()) {
                statistic[tt.species.size()-1]++;

                probeSubtreeCounter++;
                probeGroupCounter++;

                // create a probe group for the subtree
                GBDATA *st_group   = GB_create_container(probe_group_cont, "probe_group");
                GBDATA *st_members = GB_search(st_group, "subtreepath", GB_STRING);

                error = GB_write_string(st_members, tt.path.c_str());
                if (!error) {
                    GBDATA *st_matches = GB_search(st_group,"probe_matches",GB_CREATE_CONTAINER);
                    for (set<string>::const_iterator j = probes.begin(); j != probes.end() && !error; ++j) {
                        GBDATA *st_probe = GB_create(st_matches,"probe",GB_STRING);
                        error            = GB_write_string(st_probe,(*j).c_str());
                    }
                }


                GBDATA *st_exact = GB_search(st_subtree, "exact", GB_INT);
                GB_write_int(st_exact, probes.size());
            }
        }
    }                           // end of getNextSubTree

    if (!error) error = probeDatabaseWriteCounters(pba_main, subtreeCounter, probeGroupCounter, probeSubtreeCounter);
    if (!error) error = pathMappingWriteGlobals(pm_main, depth);
    if (!error) error = storeProbeGroupStatistic(pba_main, statistic, statistic_size);

    return error;
}

int main(int argc,char *argv[]) {
    out.put("arb_probe_group v2.0 -- (C) 2001-2003 by Tina Lai & Ralf Westram");

    GB_ERROR  error    = scanArguments(argc, argv);
    GBDATA   *gb_main  = 0;
    GBDATA   *pb_main  = 0;
    GBDATA   *pba_main = 0;
    GBDATA   *pbb_main = 0;

    if (!error) error = openDatabases(gb_main, pb_main, pba_main, pbb_main);

    if (!error) {
        GB_install_status(PG_status); // to show progress of tree linking etc.
        PG_initSpeciesMaps(gb_main, pb_main);

        GBT_TREE *gbt_tree = readAndLinkTree(gb_main, error);
//         GB_HASH  *gb_hash  = 0;
//         if (!error) {
//             GB_transaction dummy(gb_main);
//             gb_hash = generate_GBT_TREE_hash(gbt_tree);
//         }
        if (!error) {
            GB_transaction dummy(pb_main);
            error = GBT_write_plain_tree(pb_main, pb_main, 0, gbt_tree);
        }

        if (!error) error = collectProbes(pb_main);
        if (!error) error = findExactSubtrees(gbt_tree, pb_main, pba_main, pbb_main);

        if (!error) {
            {
                GB_transaction dummy(gb_main);
                error = PG_tree_change_node_info(gbt_tree); // exchange species-names against species ids in tree
            }
            if (!error) {
                GB_transaction dummy(pba_main);
                error = GBT_write_plain_tree(pba_main, pba_main, 0, gbt_tree);
            }
            if (!error) {
                error = PG_transfer_root_string_field(pb_main, pba_main, "species_mapping"); // copy species_mapping
            }
        }

//         if (gb_hash) GBS_free_hash(gb_hash);
        if (gbt_tree) GBT_delete_tree(gbt_tree);
    }

    // save databases

    if (!error) error = save(pb_main, para.db_out_name, "complete");
    if (!error) error = save(pba_main, para.db_out_alt_name, "complete");
    if (!error) error = save(pbb_main, para.db_out_alt_name2, "complete");

    if (error) {
        fprintf(stderr, "Error in arb_probe_group: %s\n", error);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

