/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
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

#include "../global_defs.h"

#define SAVE_AFTER_XXX_NEW_GROUPS 10000


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

#include <arbdbt.h>

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
    string path;
    std::set<SpeciesID> species;
    TreeTraversal(GBT_TREE *rootnode) {
        currentSubTree = rootnode;
    }
    virtual ~TreeTraversal() {}

    GBT_TREE *getNextSubTree() {
        if(!currentSubTree->is_leaf) {
            currentSubTree=currentSubTree->leftson;
            path+="L";
        }
        else{
            while(currentSubTree==currentSubTree->father->rightson){
                currentSubTree=currentSubTree->father;
                path.erase(path.length()-1);
                if(!currentSubTree->father) return 0; //back to root position
            }
            currentSubTree=currentSubTree->father->rightson;
            path[path.length()-1]='R';
        }
        //    if(currentSubTree->is_leaf) return getNextSubTree();
        return currentSubTree;

    }

    void getSpeciesNumber(GBT_TREE *tree){ //this function will return all the species number under tree
        if(!tree->is_leaf){
            getSpeciesNumber(tree->leftson);
            getSpeciesNumber(tree->rightson);
        }
        else{
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

//  ----------------------------------------------
//      static GB_ERROR save(GBDATA *pb_main)
//  ----------------------------------------------
static GB_ERROR save(GBDATA *pb_main) {
    const char  *name  = para.db_out_name.c_str();
    GB_ERROR     error = 0;

    out.put("Saving %s ...", name);
    error = GB_save(pb_main, name, SAVE_MODE); // was "a"

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

//  ------------------------------------------
//      Int Main(Int Argc,  Char *Argv[])
//  ------------------------------------------
int main(int argc,char *argv[]) {

    GBDATA  *gb_main = 0;
    GBDATA  *pb_main = 0;
    GBDATA      *pba_main = 0;
    GBDATA      *pbb_main = 0;
    GBDATA      *gb_status = 0;
    GBDATA      *gb_probesMatched = 0;

    out.put();
    out.put("arb_probe_group v2.0 -- (C) 2001-2003 by Tina Lai & Ralf Westram");

    GB_ERROR error = scanArguments(argc, argv);

    // open arb-database:
    if (!error) {
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
    }

    //open path-mapping-database:
    if (!error) {
        const char *name=para.db_out_alt_name2.c_str();

        out.put("Opening path-mapping-database '%s'..",name);
        pbb_main=GB_open(name,"wch");
        if(!pbb_main){
            error=GB_get_error();
            if(!error) error=GB_export_error("Can't open database '%s'..",name);
        }
    }

    //open probe-group-subtree-result-database:
    if (!error) {
        const char *name=para.db_out_alt_name.c_str();

        out.put("Opening probe-group-subtree-result-database '%s'..",name);
        pba_main=GB_open(name,"wch");
        if(!pba_main){
            error=GB_get_error();
            if(!error) error=GB_export_error("Can't open database '%s'",name);
        }
    }

    if (!error) {
        GB_install_status(PG_status);
        PG_initSpeciesMaps(gb_main, pb_main);

        int statistic[PG_NumberSpecies()];
        for(int i=0;i<PG_NumberSpecies();i++)
            statistic[i]=0;

        bool status=false;

        {
            GB_transaction dummy(pb_main);
            gb_status = GB_find(pb_main,"status",0,down_level);
            if (gb_status) {
                gb_probesMatched=GB_find(gb_status,"probes_matched",0,down_level);
                status = (GB_read_int(gb_probesMatched) == 1);
            }
        }

        GBT_TREE *gbt_tree = 0;
        GB_HASH  *gb_hash  = 0;

        {
            GB_transaction dummy(gb_main);

            out.put("Reading tree '%s'..", para.tree_name.c_str());
            gbt_tree=GBT_read_tree(gb_main,para.tree_name.c_str(),sizeof(GBT_TREE));

            if (!gbt_tree) {
                error = GB_export_error("Tree '%s' not found.", para.tree_name.c_str());
            }
            else {
                out.put("Linking tree..");
                GBT_link_tree(gbt_tree,gb_main,true);
                gb_hash = generate_GBT_TREE_hash(gbt_tree);
            }
        }

        if (!status &&!error) {
            {
                GB_transaction dummy(gb_main);
                GB_begin_transaction(pb_main);
                error = GBT_write_tree(gb_main,pb_main,0,gbt_tree);
                if (error) GB_abort_transaction(pb_main);
                else GB_commit_transaction(pb_main);
            }

            if (!error) {
                string pt_server_name = "localhost: "+para.pt_server_name;
                PG_init_pt_server(gb_main, pt_server_name.c_str(), my_output);

                out.put("Here we go:");

                indent i0(2, out);
                size_t group_count = 0;
                size_t last_save_group_count = 0;

                //             for (int length = 15; !error && length <= 20; ++length) {
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
                                        bool created;
                                        int numSpecies;
                                        GBDATA *pb_group = group.groupEntry(pb_main, true, created, &numSpecies);
                                        //if(numSpecies>0)
                                        //statistic[numSpecies-1]++;
                                        /*GBDATA *pb_probe = */ PG_add_probe(pb_group, probe);

                                        if (created) {
                                            out.point();
                                            //out.put("New group matched by %-10s (matches: %4i)", probe, group.size());
                                            ++group_count;
                                            size_t since_last_save = group_count-last_save_group_count;

                                            if (since_last_save >= SAVE_AFTER_XXX_NEW_GROUPS)  {
                                                out.put("Groups found: %6i  Probes matched: %6i  (%i%% of probes with length %i)",
                                                        group_count,
                                                        probe_count2,
                                                        percent(probe_count2, probe_count),
                                                        length);

                                                last_save_group_count = group_count;
                                                error = save(pb_main);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        PG_exit_find_probes();
                    }
                }
                //             }

                GB_begin_transaction(pb_main);
                gb_status = GB_search(pb_main,"status",GB_CREATE_CONTAINER);
                gb_probesMatched = GB_search(gb_status,"probes_matched",GB_INT);
                error = GB_write_int(gb_probesMatched,1);
                if (error) GB_abort_transaction(pb_main);
                else GB_commit_transaction(pb_main);

                PG_exit_pt_server();
                out.put("Overall groups found: %i", group_count);
            }

            // save last state:
            if (!error) error = save(pb_main);
        }

        if (!error) {
            // Building a tree with probes

            TreeTraversal tt(gbt_tree);

            GBT_TREE *current;
            int       count               = 1;
            int       subtreeCounter      = 0;
            int       probeSubtreeCounter = 0;
            int       depth               = 0;

            GB_begin_transaction(pba_main);
            GB_begin_transaction(pbb_main);

            GBDATA *st_main = GB_search(pba_main,"subtree_with_probe",GB_CREATE_CONTAINER);
            GBDATA *pm_main = GB_search(pbb_main,"path_mapping",GB_CREATE_CONTAINER);

            while((current=tt.getNextSubTree())&&!error) {
                subtreeCounter++;
                //                 out.put("%i %s %s",count,tt.path.c_str(),current->name);
                count++;
                tt.species.erase(tt.species.begin(),tt.species.end());
                tt.getSpeciesNumber(current);
                if(current->is_leaf) { //leaf
                    GBDATA *pm_species = GB_create_container(pm_main,"species");
                    GBDATA *pm_name    = GB_create(pm_species,"name",GB_STRING);
                    error              = GB_write_string(pm_name,PG_SpeciesID2SpeciesName(*tt.species.begin()).c_str());

                    if (!error) {
                        GBDATA *pm_path = GB_create(pm_species,"path",GB_STRING);
                        error           = GB_write_string(pm_path,tt.path.c_str());
                    }

                    if(error) GB_abort_transaction(pbb_main);
                    if(tt.path.length()>depth) depth = tt.path.length();
                }
                std::set<string> probes;
                {
                    GB_transaction dummy(pb_main);
                    GBDATA *pbtree=GB_search(pb_main,"group_tree",GB_FIND);
                    GBDATA *pbnode=GB_find(pbtree,"node",0,down_level);
                    PG_find_probe_for_subtree(pbnode,tt.species,&probes);
                }
                if(!probes.empty()){
                    statistic[tt.species.size()-1]++;
                    probeSubtreeCounter++;
                    GBDATA *st_subtree=GB_create_container(st_main,"subtree");
                    GBDATA *st_path=GB_search(st_subtree,"path",GB_STRING);
                    error=GB_write_string(st_path,tt.path.c_str());
                    GBDATA *st_species=GB_search(st_subtree,"species",GB_CREATE_CONTAINER);
                    for(set<SpeciesID>::const_iterator i=tt.species.begin();i!=tt.species.end();++i){
                        GBDATA *st_name=GB_create(st_species,"name",GB_STRING);
                        error=GB_write_string(st_name,PG_SpeciesID2SpeciesName(*i).c_str());
                    }

                    GBDATA *st_group=GB_search(st_subtree,"probe_group",GB_CREATE_CONTAINER);
                    for(set<string>::const_iterator j=probes.begin();j!=probes.end();++j){
                        GBDATA *st_probe=GB_create(st_group,"probe",GB_STRING);
                        error=GB_write_string(st_probe,(*j).c_str());
                    }
                }
            }

            if (!error) {
                GBDATA *pm_num    = GB_create(pm_main,"species_number",GB_INT);
                if (pm_num) error = GB_write_int(pm_num,PG_NumberSpecies());
                else    error     = GB_get_error();
            }

            if (!error) {
                GBDATA *pm_depth    = GB_create(pm_main,"depth",GB_INT);
                if (pm_depth) error = GB_write_int(pm_depth,depth);
                else    error       = GB_get_error();
            }

            if (error) GB_abort_transaction(pbb_main);
            else GB_commit_transaction(pbb_main);

            if (!error) {
                GBDATA *stCounter    = GB_create(pba_main,"subtree_counter",GB_INT);
                if (stCounter) error = GB_write_int(stCounter,subtreeCounter);
                else    error        = GB_get_error();

                if (!error) {
                    GBDATA *stProbeCounter    = GB_create(pba_main,"subtree_with_probe_counter",GB_INT);
                    if (stProbeCounter) error = GB_write_int(stProbeCounter,probeSubtreeCounter);
                    else    error             = GB_get_error();
                }

                if (!error) {
                    GBDATA *stat = GB_create(pba_main,"probe_group_vs_species",GB_STRING);
                    string  data;
                    for(int i=0; i<PG_NumberSpecies(); i++){
                        char buffer[20];
                        sprintf(buffer,"%i,%i;",i+1,statistic[i]);
                        data += buffer;
                    }
                    error = GB_write_string(stat,data.c_str());
                }

                if (error) GB_abort_transaction(pba_main);
                else GB_commit_transaction(pba_main);

                if (!error) {
                    out.put("Saving %s ...",para.db_out_alt_name.c_str());
                    error = GB_save(pba_main,para.db_out_alt_name.c_str(), SAVE_MODE); // was "a"
                    out.put("Saving %s ...",para.db_out_alt_name2.c_str());
                    error = GB_save(pbb_main,para.db_out_alt_name2.c_str(), SAVE_MODE); // was "a"
                }
            }
        }
        if (gb_hash) GBS_free_hash(gb_hash);
        if (gbt_tree) GBT_delete_tree(gbt_tree);
    }

    if (error) {
        fprintf(stderr, "Error: %s\n", error);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
