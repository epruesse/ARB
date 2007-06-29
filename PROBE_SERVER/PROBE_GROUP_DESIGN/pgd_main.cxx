#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <set>

#include <arbdb.h>
#include <arbdbt.h>

#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>
#include <output.h>

// SKIP_PROBES_TO_TARGET_CONVERSION in DEBUG mode only (for the ease of debugging)
#if defined(DEBUG)
// #define SKIP_PROBES_TO_TARGET_CONVERSION
#endif // DEBUG

#define pgd_assert(cond) arb_assert(cond)

#include "../global_defs.h"

#define NEED_decodeTreeNode
#define NEED_encodeTreeNode
#define NEED_getDatabaseState
#define NEED_setDatabaseState
#define NEED_saveProbeContainerToString
#include "../common.h"

#include "../mapping.h"
#include "../path_code.h"
#include "../read_config.h"

// #define MINTEMPERATURE 30.0
// #define MAXTEMPERATURE 100.0
// #define MINGC          50.0
// #define MAXGC          100.0
// #define MAXBOND        4.0
// #define MINPOSITION    0
// #define MAXPOSITION    10000
#define MISHIT         0
#define MINTARGETS     100
// #define DTEDGE         0.5
// #define DT             0.5
// #define SPLIT          0.5
#define CLIPRESULT     1000       // max. number of probes returned by pt-server

// ----------------------------------------

typedef string SpeciesName;
typedef string Probes;
typedef enum { TF_NONE, TF_CREATE } Treefile_Action;

struct gl_struct {
    aisc_com  *link;
    T_PT_LOCS  locs;
    T_PT_MAIN  com;
};

struct InputParameter {
    string          db_name;
    string          pt_server_name;
    string          pd_db_name;
    int             pb_length;
    Treefile_Action gen_treefile;
    string          treefile;
    string          versionumber;
};

// ----------------------------------------
// module globals

static GBDATA            *gb_main = 0; //ARB-database
static GBDATA            *pd_main = 0; //probe-design-database
static InputParameter     para;

// static float bondval[16]={0.0,0.0,0.5,1.1,
//                    0.0,0.0,1.5,0.0,
//                    0.5,1.5,0.4,0.9,
//                    1.1,0.0,0.9,0.0};


static bool  server_initialized  = false;
static char *current_server_name = 0;

static GB_HASH *path2subtree        = 0; // uses encoded paths
static long     no_of_subtrees      = 0;
static int      max_subtree_pathlen = 0;
static char    *pathbuffer          = 0;
static output   out;

// ----------------------------------------

static GB_ERROR init_path2subtree_hash(GBDATA *pd_father, long expected_no_of_subtrees) {
    GB_ERROR error = 0;
    if (!path2subtree) {
        path2subtree   = GBS_create_hash(expected_no_of_subtrees, 1);
        no_of_subtrees = 0;

        char length_buf[] = "xxxx"; // first 4 bytes of path is the pathlen

        for (GBDATA *pd_subtree = GB_find(pd_father,"subtree",0,down_level);
             pd_subtree && !error;
             pd_subtree = GB_find(pd_subtree,"subtree",0,this_level|search_next))
        {
            GBDATA *pd_path = GB_find(pd_subtree, "path", 0, down_level);
            if (!pd_path) {
                error = "Missing 'path' entry";
            }
            else {
                const char *path = GB_read_char_pntr(pd_path);
                GBS_write_hash(path2subtree, path, (long)pd_subtree);
                no_of_subtrees++;

                memcpy(length_buf, path, 4);
                int pathlen = (int)strtoul(length_buf, 0, 16);
                if (pathlen>max_subtree_pathlen) {
                    max_subtree_pathlen = pathlen;
                }
            }
        }

        if (!error) {
            if (no_of_subtrees == 0) {
                error = "No subtrees found";
            }
            else if (no_of_subtrees != expected_no_of_subtrees) {
                error = GBS_global_string("Wrong number of subtrees (found=%li, expected=%li)", no_of_subtrees, expected_no_of_subtrees);
            }
        }

        if (!error) {
            // we need 1 additional byte (pgd_add_species tries to go down from leafs as well)
            pathbuffer = (char*)GB_calloc(max_subtree_pathlen+1+1, sizeof(char));
        }
    }

    return error;
}

static const char *PGD_probe_pt_look_for_server(GBDATA *gb_main, const char *servername, GB_ERROR& error) {
    int serverNum = -1;

    for (int i=0; serverNum == -1; ++i) {
        char *aServer = GBS_ptserver_id_to_choice(i, 0);
        if (!aServer) { //  no more servers
            GB_clear_error(); // suppress error about missing entry
            break;              
        }

        if (strcmp(aServer, servername) == 0) { // wanted server
            serverNum = i;
            out.put("Found pt-server: %s", aServer);
        }
        free(aServer);
    }

    const char *result = 0;
    if (serverNum==-1){
        error = GB_export_error("'%s' is not a known pt-server.",servername);
    }
    else {
        char     *serverID = GBS_global_string_copy("ARB_PT_SERVER%i", serverNum);
        GB_ERROR  starterr = arb_look_and_start_server(AISC_MAGIC_NUMBER, serverID, gb_main);

        if (starterr) error = GB_export_error("Cannot start pt-server '%s' (Reason: %s)", servername, starterr);
        else result         = GBS_read_arb_tcp(serverID);
        
        free(serverID);
    }
    return result;
}

static GB_ERROR PG_init_pt_server(GBDATA *gb_main, const char *servername) {
    GB_ERROR error = 0;
    if (server_initialized) {
        error = "pt-server initialized twice";
    }
    else {
        out.put("Search a free running pt-server..");
        indent i(out);
        current_server_name            = GB_strdup(PGD_probe_pt_look_for_server(gb_main,servername,error));
        if (!error) server_initialized = true;
    }
    return error;
}

static void PG_exit_pt_server(void) {
    free(current_server_name);
    current_server_name = 0;
    server_initialized  = false;
}

static int init_local_com_struct(struct gl_struct& pd_gl)
{
    const char *user = GB_getenvUSER();

    if( aisc_create(pd_gl.link, PT_MAIN, pd_gl.com,
                    MAIN_LOCS, PT_LOCS, &pd_gl.locs,
                    LOCS_USER, user,
                    NULL)){
        return 1;
    }
    return 0;
}

class PT_server_connection {
    //private:
    //    T_PT_PDC  pdc;
    //    GB_ERROR error;
    //    struct gl_struct my_pd_gl;

public:
    T_PT_PDC         pdc;
    GB_ERROR         error;
    struct gl_struct my_pd_gl;
    
    PT_server_connection() {
        error = 0;
        memset(&my_pd_gl, 0, sizeof(my_pd_gl));

        my_pd_gl.link = aisc_open(current_server_name,&my_pd_gl.com,AISC_MAGIC_NUMBER);

        if (!my_pd_gl.link) {
            error = "can't contact pt_server (unknown reason\n";
        }
        else if(init_local_com_struct(my_pd_gl)) {
            error = "can't contact pt_server (connection refused)\n";
        }
        else {
            aisc_create(my_pd_gl.link,PT_LOCS, my_pd_gl.locs,
                        LOCS_PROBE_DESIGN_CONFIG, PT_PDC, &pdc,
                        0);
        }
    }
    virtual ~PT_server_connection() {
        if (my_pd_gl.link) aisc_close(my_pd_gl.link);
        delete error;
    }

    struct gl_struct& get_pd_gl() {return my_pd_gl;}
    GB_ERROR get_error() const {return error;}
    T_PT_PDC& get_pdc() {return pdc;}
};

static PT_server_connection *my_server;

//  -----------------------------
//      void helpArguments()
//  -----------------------------
static void helpArguments(){
    fprintf(stderr,
            "\n"
            "Usage: arb_probe_group_design <db_name> <pt_server> <db_out> <probe_length> [<treefile> <versionumber>]\n"
            "\n"
            "db_name        name of ARB-database to build groups for\n"
            "pt_server      name of pt_server\n"
            "db_out         name of probe-design-database\n"
            "probe_lengh    length of the probe (15-20)\n"
            "treefile       treefile for probe_server [-c=create, -x=extend]\n"
            "versionumber   should be current date in seconds till epoch\n"
            "\n"
            );
}

//  --------------------------------------------------------
//      GB_ERROR pgd_scanArguments(int argc,char *argv[])
//  --------------------------------------------------------
static GB_ERROR pgd_scanArguments(int argc,char *argv[]){
    GB_ERROR error = 0;

    if (argc == 5 || argc == 7) {
        para.db_name        = argv[1];
        para.pt_server_name = string("localhost: ")+argv[2];
        para.pd_db_name     = string(argv[3])+argv[4];
        para.pb_length      = atoi(argv[4]);
        if (argc == 7) {
            para.gen_treefile = TF_CREATE;
            para.treefile     = string(argv[5]);
            para.versionumber = string(argv[6]);
        }
        else {
            para.gen_treefile = TF_NONE;
        }
    }
    else {
        helpArguments();
        error = "Wrong number of arguments";
    }

    return error;
}

static GB_ERROR pgd_add_species(int len, set<SpeciesName> *species) {
    // uses the global buffer 'pathbuffer'

    GB_ERROR  error = 0;
    GBDATA   *pd_subtree;
    {
        const char *encoded_path = encodePath(pathbuffer, len, error);
        if (!error) {
            pd_subtree = (GBDATA*)GBS_read_hash(path2subtree, encoded_path);
        }
    }

    if (!error) {
        if (pd_subtree) {
            pathbuffer[len]   = 'L';
            pathbuffer[len+1] = 0;

            if (pgd_add_species(len+1, species)) {
                // error occurred -> we are a leaf
                pathbuffer[len] = 0;

                GBDATA *gb_member = GB_find(pd_subtree, "member", 0, down_level);
                if (!gb_member) {
                    return GBS_global_string("'member' expected in '%s'", pathbuffer);
                }

                const string& name = PM_ID2name(GB_read_int(gb_member), error);
                if (!error) species->insert(name.c_str());
            }
            else { // inner node
                pathbuffer[len]   = 'R';
                pathbuffer[len+1] = 0;

                error = pgd_add_species(len+1, species);
            }
        }
        else {
            error = GBS_global_string("illegal path '%s'", pathbuffer);
        }
    }

    return error;
}

static GB_ERROR pgd_init_species(GBDATA *pd_probe_group, set<SpeciesName> *species) {
    GB_ERROR  error       = 0;
    GBDATA   *pd_group_id = GB_find(pd_probe_group, "id", 0, down_level);

    if (!pd_group_id) {
        error = "entry 'id' expected";
    }
    else {
        const char *group_id = GB_read_char_pntr(pd_group_id);
        if (group_id[0] == 'p') { // probe group hits a subtree
            // here group_id is 'pxxx' where xxx is the encoded path
            const char *decoded_path = decodePath(group_id+1, error);
            if (!error) {
                int len = strlen(decoded_path);
                pgd_assert(len <= max_subtree_pathlen);
                memcpy(pathbuffer, decoded_path, len+1);
                error   = pgd_add_species(len, species);
            }
        }
        else if (group_id[0] == 'g') { // subtree independent probe-group
            GBDATA *pd_members = GB_find(pd_probe_group, "members", 0, down_level);
            if (pd_members) {
                const char *members = GB_read_char_pntr(pd_members);
                const char *number   = members;

                while (number && !error) {
                    SpeciesID     id   = atoi(number);
                    const string& name = PM_ID2name(id, error);

                    if (!error) {
                        species->insert(name.c_str());

                        const char *komma  = strchr(number, ',');
                        if (komma) number  = komma+1;
                        else        number = 0;
                    }
                }
            }
            else {
                error = GBS_global_string("'members' expected in independent group '%s'", group_id);
            }
        }
        else {
            error = GBS_global_string("Illegal group_id '%s'", group_id);
        }
    }

    return error;
}

static char *pgd_get_the_names(set<SpeciesName> *species, bytestring &bs, bytestring &checksum, const char *use) {
    GBDATA *gb_species;
    GBDATA *gb_name;
    GBDATA *gb_data;

    void *names = GBS_stropen(1024);
    void *checksums = GBS_stropen(1024);

    GB_begin_transaction(gb_main);
//     char *use = GBT_get_default_alignment(gb_main);

    gb_species=GB_search(gb_main,"species_data",GB_FIND);
    if(!gb_species) return 0;

    for(set<SpeciesName>::const_iterator i=(*species).begin();i!=(*species).end();++i){
        gb_name=GB_find(gb_species,"name",(*i).c_str(),down_2_level);
        if(gb_name){
            gb_data=GB_find(gb_name,use,0,this_level);
            if(gb_data) gb_data=GB_find(gb_data,"data",0,down_level);
            if(!gb_data) return (char*)GB_export_error("Species '%s' has no sequence '%s'",GB_read_char_pntr(gb_name),use);
            GBS_intcat(checksums, GBS_checksum(GB_read_char_pntr(gb_data), 1, ".-"));
            GBS_strcat(names, GB_read_char_pntr(gb_name));
            GBS_chrcat(checksums, '#');
            GBS_chrcat(names, '#');
        }
    }

    bs.data = GBS_strclose(names);
    bs.size = strlen(bs.data)+1;

    checksum.data = GBS_strclose(checksums);
    checksum.size = strlen(checksum.data)+1;

    GB_commit_transaction(gb_main);
    return 0;
}

static int pgd_probe_design_send_data(const probe_config_data& probe_config) {
    if (aisc_put(my_server->get_pd_gl().link,
                 PT_PDC,            my_server->pdc,
                 PDC_DTEDGE,        probe_config.dtedge*100.0,
                 PDC_DT,            probe_config.dt*100.0,
                 PDC_SPLIT,         probe_config.split,
                 PDC_CLIPRESULT,    CLIPRESULT,
                 0))
        return 1;

    for (int i=0;i<16;i++) {
        if (aisc_put(my_server->get_pd_gl().link,
                     PT_PDC,        my_server->pdc,
                     PT_INDEX,      i,
                     PDC_BONDVAL,   probe_config.bonds[i],
                     0))
            return 1;
    }
    return 0;
}

static GB_ERROR PGD_probe_design_event(set<SpeciesName> *species,set<Probes> *probe_list, const char *use)
{
    T_PT_TPROBE  tprobe;        //long
    bytestring   bs;
    bytestring   check;
    char        *match_info;
    const char  *error = 0;

    // validate sequence of the species
    error = pgd_get_the_names(species,bs,check, use);

    if (!error) {
        aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                 PDC_NAMES,&bs,
                 PDC_CHECKSUMS,&check,
                 0);

        // validate PT server (Get unknown names)
        bytestring unknown_names;
        if (aisc_get(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                     PDC_UNKNOWN_NAMES,&unknown_names, 0))
        {
            error = "Connection to PT_SERVER lost (2)";
        }

        if (!error) {
            char *unames = unknown_names.data;
            if (unknown_names.size>1) {
                error = GB_export_error("Your PT server is not up to date or wrongly chosen\n"
                                        "The following names are new to it:\n"
                                        "%s" , unames);
            }
        }
        free(unknown_names.data);
    }

    if (!error) {
        // calling the design
        aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                 PDC_GO,0, 0);

        //result
        // printf("Read the results from the server");
        {
            char *locs_error = 0;
            if (aisc_get(my_server->get_pd_gl().link,PT_LOCS,my_server->get_pd_gl().locs,
                         LOCS_ERROR,&locs_error, 0))
            {
                error = "Connection to PT_SERVER lost (3)";
            }
            else {
                if (locs_error && locs_error[0]) {
                    error = GB_export_error(locs_error);
                }
                free(locs_error);
            }
        }
    }

    if (!error) {
        aisc_get(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                 PDC_TPROBE,&tprobe, 0);
    }

    if (!error) {
        while (tprobe) {
            long tprobe_next;
            if(aisc_get(my_server->get_pd_gl().link,PT_TPROBE,tprobe,
                        TPROBE_NEXT,&tprobe_next,
                        TPROBE_INFO,&match_info, 0))
                break;

            tprobe = tprobe_next;

            char *probe,*space;
            probe = strpbrk(match_info,"acgtuACGTU");

            if (probe) {
                space = strchr(probe,' ');
            }

            if (probe && space) {
                *space = 0;
                probe  = strdup(probe);
                *space = ' ';
            }
            else{
                probe = strdup("");
            }
            (*probe_list).insert(probe);
            free(probe);
            free(match_info);
        }
    }

    free(bs.data);
    free(check.data);

    return error;
}

static GB_ERROR PGD_export_tree(GBT_TREE *node, FILE *out) {
    if (node->is_leaf) {
        if (!node->name) return "leaf w/o name";
        fprintf(out, "'%s'", node->name);
        return 0;
    }

    fprintf(out, "(");
    GB_ERROR error = PGD_export_tree(node->leftson, out);
    fprintf(out, ":%.5f,\n", node->leftlen);
    if (!error) {
        error = PGD_export_tree(node->rightson, out);
        fprintf(out, ":%.5f)", node->rightlen);
        if (node->name) { // node info
            fprintf(out, "'%s'", node->name);
        }
        else {
            fprintf(out, "''"); // save empty nodo-info (needed by pgd_tree_merge)
        }
    }
    return error;
}

static GB_ERROR PGD_decodeBranchNames(GBT_TREE *node) {
    GB_ERROR error = 0;
    if (node->is_leaf) {
        if (!node->name) return "node w/o name";
        char *newName = decodeTreeNode(node->name, error);
        if (error) return error;

        char *afterNum = strchr(newName, ',');
        if (!afterNum) {
            error = GBS_global_string("komma expected in node name '%s'", newName);
        }
        else {
            const string& shortname = PM_ID2name(atoi(newName), error);
            if (!error) {
                char *fullname = afterNum+1;
                char *acc      = strchr(fullname, ',');
                if (!acc) {
                    error = GBS_global_string("2nd komma expected in '%s'", newName);
                }
                else {
                    *acc++ = 0;

                    char *newName2 = GBS_global_string_copy("n=%s,f=%s,a=%s", shortname.c_str(), fullname, acc);

                    free(newName);
                    newName = newName2;
                }
            }
        }

        if (error) {
            free(newName);
            return error;
        }

        free(node->name);
        node->name = newName;
        return 0;
    }

    // not leaf:
    if (node->remark_branch) { // if remark_branch then it is a group name
        char *groupName = decodeTreeNode(node->remark_branch, error);
        if (error) return error;

        char *newName = GBS_global_string_copy("g=%s", groupName);
        free(groupName);

        free(node->remark_branch);
        node->remark_branch = 0;

        free(node->name);
        node->name = newName;
    }

    error             = PGD_decodeBranchNames(node->leftson);
    if (!error) error = PGD_decodeBranchNames(node->rightson);
    return error;
}
static GB_ERROR PGD_encodeBranchNames(GBT_TREE *node) {
    if (node->name) { // all nodes
        char *newName = encodeTreeNode(node->name);
        free(node->name);
        node->name    = newName;
    }

    if (node->is_leaf) {
        if (!node->name) return "node w/o name";
        return 0;
    }

    GB_ERROR error    = PGD_encodeBranchNames(node->leftson);
    if (!error) error = PGD_encodeBranchNames(node->rightson);
    return error;
}

static GBT_TREE *PGD_find_subtree_by_path(GBT_TREE *node, const char *path) {
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

static GB_ERROR pgd_openDatabases() {
    GB_ERROR error = 0;
    indent   i(out);

    //Open arb database:
    const char *db_name = para.db_name.c_str();
    out.put("Opening ARB-database '%s' ..",db_name);

    // @@@ FIXME:  I think this program should be able to run w/o the main database
    // it only uses the db to open the pt-server connection and does
    // some unneeded species checking..

    gb_main = GBT_open(db_name,"rwt",0);
    if (!gb_main) {
        error             = GB_get_error();
        if (!error) error = GB_export_error("Can't open database '%s'",db_name);
    }

    if (!error) {
        //Open probe design database:
        string      pd_nameS = para.pd_db_name+".arb";
        const char *pd_name  = pd_nameS.c_str();
        out.put("Opening probe-group-design-database '%s' .." ,pd_name);

        pd_main = GB_open(pd_name,"rwcN");
        if (!pd_main) {
            error            = GB_get_error();
            if(!error) error = GB_export_error("Can't open database '%s'",pd_name);
        }
        else {
            error = checkDatabaseType(pd_main, pd_name, "probe_group_subtree_db", "complete");
        }
    }

    return error;
}

static GB_ERROR saveDatabase() {
    string      savenameS = para.pd_db_name+"_design.arb";
    const char *savename  = savenameS.c_str();
    out.put("Saving %s ..", savename);
    return GB_save(pd_main, savename, SAVE_MODE);
}

class ConnectServer {
    bool     pt_server_initialized;
    bool     aisc_created;
    GB_ERROR error;

public:
    ConnectServer(const probe_config_data& probe_config) {
        PG_init_pt_server(gb_main,para.pt_server_name.c_str());
        my_server = new PT_server_connection();
        error     = my_server->get_error();

        if (error) {
            pt_server_initialized = false;
        }
        else {
            pt_server_initialized = true;
            //initialize parameters
            aisc_create(my_server->get_pd_gl().link,
                        PT_LOCS,                    my_server->get_pd_gl().locs,
                        LOCS_PROBE_DESIGN_CONFIG,
                        PT_PDC,                     &my_server->pdc,
                        PDC_PROBELENGTH,            long(para.pb_length),
                        PDC_MINTEMP,                probe_config.min_temperature,
                        PDC_MAXTEMP,                probe_config.max_temperature,
                        PDC_MINGC,                  probe_config.min_gccontent/100.0,
                        PDC_MAXGC,                  probe_config.max_gccontent/100.0,
                        PDC_MAXBOND,                double(probe_config.max_hairpin_bonds),
                        0);

            aisc_put(my_server->get_pd_gl().link,
                     PT_PDC,            my_server->pdc,
                     PDC_MINPOS,        long(probe_config.ecoli_min_pos),
                     PDC_MAXPOS,        long(probe_config.ecoli_max_pos),
                     PDC_MISHIT,        long(MISHIT),
                     PDC_MINTARGETS,    double(MINTARGETS/100.0),
                     0);

            if (pgd_probe_design_send_data(probe_config)) {
                error        = "Connection to PT_SERVER lost (1)";
                aisc_created = false;
            }
            else {
                aisc_created = true;
            }
        }
    }

    ~ConnectServer() {
        if (pt_server_initialized) {
            if (aisc_created) {
                aisc_close(my_server->get_pd_gl().link);
                my_server->get_pd_gl().link = 0;
            }
            PG_exit_pt_server();
        }
    }

    GB_ERROR getError() const { return error; }
};

static GB_ERROR loadProbeSetFromString(std::set<Probes>& probes, GBDATA *gb_father,
                                       const char *entry_name, bool error_if_missing)
{
    GB_ERROR  error    = 0;
    GBDATA   *gb_entry = GB_find(gb_father, entry_name, 0, down_level);

    if (gb_entry) {
        char *probe_string = GB_read_string(gb_entry);
        if (!probe_string) {
            error = GB_get_error();
            pgd_assert(error);
        }
        else {
            for (const char *tok = strtok(probe_string, ",");
                 tok;
                 tok = strtok(0, ","))
            {
                probes.insert(tok);
            }
        }
    }
    else if (error_if_missing) {
        error = GBS_global_string("no such entry '%s'", entry_name);
    }

    return error;
}

static GB_ERROR designProbesForGroup(GBDATA *pd_probe_group, const char *use) {
    {
        GBDATA *pd_pgd = GB_find(pd_probe_group,"designed_probes",0,down_level);
        if (pd_pgd) GB_delete(pd_pgd); // erase existing data
    }

#if defined(DEBUG) && 0
    {
        GBDATA     *pd_id = GB_find(pd_probe_group, "id", 0, down_level);
        const char *id    = GB_read_char_pntr(pd_id);

        if (strcmp(id, "g15_8") == 0) {
            fprintf(stderr, "group '%s' reached\n", id);
        }
    }
#endif // DEBUG

    bool     skip_design = true; // @@@ if true -> design is not performed!
    GB_ERROR error       = 0;

    if (skip_design) {
        // move 'matched_probes' -> 'common_probes' (fake design)
        GBDATA *pd_matched_probes     = GB_find(pd_probe_group, "matched_probes", 0, down_level);
        if (!pd_matched_probes) error = GB_get_error();
        else {
            GBDATA *pd_common_probes = GB_create(pd_probe_group, "common_probes", GB_STRING);
            if (!pd_common_probes) error = GB_get_error();
            else {
                error             = GB_copy(pd_common_probes, pd_matched_probes);
                if (!error) error = GB_delete(pd_matched_probes);
            }
        }

    }
    else {
        // get species
        std::set<SpeciesName> species; //set of species for probe design
        GB_ERROR              error = pgd_init_species(pd_probe_group, &species);

        if (!error && species.empty()) error = "No species detected for probe group.";
        if (!error) {
            std::set<Probes> designedProbes; //set of probes from the design
            error = PGD_probe_design_event(&species, &designedProbes, use); // call probe_design

            size_t both_count = 0;

            if (!error && !designedProbes.empty()) {
                std::set<Probes> matchedProbes;
                error = loadProbeSetFromString(matchedProbes, pd_probe_group, "matched_probes", true);


                if (!error) {
                    std::set<Probes> commonProbes;

                    for (set<Probes>::iterator i=designedProbes.begin(); i != designedProbes.end(); ) {
                        set<Probes>::const_iterator found = matchedProbes.find(*i);
                        if (found != matchedProbes.end()) {
                            // probe was found by probe-design and probe-match
                            commonProbes.insert(*i);
                            matchedProbes.erase(found);
                            designedProbes.erase(i++);
                        }
                        else {
                            // probe was only found by probe-design
                            ++i;
                        }
                    }

                    // write designed-only, matched-only and common probe sets

                    if (!designedProbes.empty()) {
                        error = saveProbeContainerToString<set<Probes> >(pd_probe_group, "designed_probes", false, designedProbes.begin(), designedProbes.end());
                    }

                    if (!error) {
                        if (matchedProbes.empty()) {
                            GBDATA *gb_matched_probes = GB_find(pd_probe_group, "matched_probes", 0, down_level);
                            if (gb_matched_probes) {
                                error = GB_delete(gb_matched_probes);
                            }
                        }
                        else {
                            error = saveProbeContainerToString<set<Probes> >(pd_probe_group, "matched_probes", true, matchedProbes.begin(), matchedProbes.end());
                        }
                    }

                    if (!commonProbes.empty()) {
                        both_count = commonProbes.size();
                        error      = saveProbeContainerToString<set<Probes> >(pd_probe_group, "common_probes", false, commonProbes.begin(), commonProbes.end());
                    }
                }
            }

            // update 'exact' entry for real subtrees
            if (!error) {
                GBDATA *pd_group_id = GB_find(pd_probe_group, "id", 0, down_level);
                if (pd_group_id) {
                    const char *group_id     = GB_read_char_pntr(pd_group_id);
                    if (group_id[0] == 'p') { // if this is a subtree-group -> update number of found probes
                        const char *encoded_path = group_id+1;
                        GBDATA     *pd_subtree   = (GBDATA*)GBS_read_hash(path2subtree, encoded_path);

                        if (pd_subtree) {
                            GBDATA *pd_exact = GB_find(pd_subtree, "exact", 0, down_level);
                            if (!both_count) {
                                if (pd_exact) error = GB_delete(pd_exact);
                            }
                            else {
                                if (!pd_exact) pd_exact = GB_create(pd_subtree, "exact", GB_INT);
                                if (!error) error       = GB_write_int(pd_exact, both_count);
                            }
                        }
                        else {
                            error = GBS_global_string("couldn't update number of exact matches (subtree '%s' not found)", encoded_path);
                        }
                    }
                }
            }

        }
    }

    return error;
}

static GB_ERROR designProbes(const probe_config_data &probe_config) {
    GB_ERROR error = 0;
    GB_begin_transaction(pd_main);

    GBDATA *pd_subtree_cont    = GB_find(pd_main,"subtrees",0,down_level);
    GBDATA *pd_probegroup_cont = GB_find(pd_main,"probe_groups",0,down_level);

    if (!pd_subtree_cont) error         = "Can't find container 'subtrees'";
    else if (!pd_probegroup_cont) error = "Can't find container 'probe_groups'";
    else {
        GBDATA *pd_subtree_counter = GB_find(pd_main, "subtree_counter", 0, down_level);
        if (!pd_subtree_counter) {
            error = "Can't find 'subtree_counter'";
        }
        else {
            long subtrees = GB_read_int(pd_subtree_counter);
            error         = init_path2subtree_hash(pd_subtree_cont, subtrees); // initialize path cache
        }
    }

    long probe_group_counter;
    if (!error) {
        GBDATA *pd_probe_group_counter = GB_find(pd_main, "probe_group_counter", 0, down_level);
        if (!pd_probe_group_counter) error = "Can't find 'probe_group_counter'";
        else {
            probe_group_counter = GB_read_int(pd_probe_group_counter);
        }
    }

    if (!error) {
        ConnectServer connect(probe_config);  // initialize pt-server connection
        error = connect.getError();
        if (!error) {
            out.put("Designing probes for %li used probe groups:", probe_group_counter);
            indent i(out);
            out.setMaxPoints(probe_group_counter);

            // design probes
            long count = 0;

            char *use;
            {
                GB_transaction dummy(gb_main);
                use = GBT_get_default_alignment(gb_main);
            }

            for (GBDATA *pd_probe_group = GB_find(pd_probegroup_cont,"probe_group",0,down_level);
                 pd_probe_group && !error;
                 pd_probe_group = GB_find(pd_probe_group,"probe_group",0,this_level|search_next))
            {
                ++count;
                out.point();

//                 if (count%60) fputc('.', stdout);
//                 else printf(". %i%%\n", int((double(count)/probe_group_counter)*100+0.5));

                error = designProbesForGroup(pd_probe_group, use);
            }

            free(use);

//             if (error) fputc('\n', stdout);
//             else printf(" %i%%\n", int((double(count)/probe_group_counter)*100+0.5));
        }
    }

    if (error) GB_abort_transaction(pd_main);
    else GB_commit_transaction(pd_main);

    return error;
}

static char *appendNodeInfo(char *oldInfo, const char *token, int value) {
    if (oldInfo == 0) {
        return GBS_global_string_copy("%s=%i", token, value);
    }
    else {
        char *newInfo = GBS_global_string_copy("%s,%s=%i", oldInfo, token, value);
        free(oldInfo);
        return newInfo;
    }
}

static GB_ERROR setNodeInfo(GBDATA *pd_subtree, GBT_TREE *node) {
//     int exact_matches    = 0;
//     int max_coverage     = 100;
//     int min_nongrouphits = 0;
    int speccount;

    GBDATA *pd_speccount = GB_find(pd_subtree, "speccount", 0, down_level);
    if (pd_speccount) { // inner node
        speccount = GB_read_int(pd_speccount);
    }
    else { // leaf
        speccount = 1;
    }

    GBDATA *pd_exact = GB_find(pd_subtree, "exact", 0, down_level);
    if (pd_exact) { // has exact probes
        int exact_matches = GB_read_int(pd_exact);
        pgd_assert(exact_matches != 0);
        node->name        = appendNodeInfo(node->name, "em", exact_matches);
    }
    else {
        GBDATA *pd_coverage = GB_find(pd_subtree, "coverage", 0, down_level);
        if (pd_coverage) {
            int covered_species = GB_read_int(pd_coverage);
            int max_coverage    = int(double(covered_species)/speccount*100.0+.5);
            if (max_coverage == 100 && covered_species != speccount) {
                max_coverage = 99; // avoid "false" 100% coverage
            }
            else if (max_coverage == 0 && covered_species != 0) {
                max_coverage = 1; // avoid "false" 0% coverage
            }
            node->name = appendNodeInfo(node->name, "mc", max_coverage);
        }


        // @@@ FIXME: handle "ng=" (aka non group hits)
    }

    return 0;
}

static GB_ERROR saveTreefile() {
    GB_ERROR error = 0;

    if (para.gen_treefile == TF_CREATE) { // create treefile ?
        GB_transaction dummy(pd_main);

        // load the tree
        GBT_TREE *tree    = 0;
        GBDATA   *pd_tree = GB_find(pd_main, "tree", 0, down_level);
        if (!pd_tree) {
            error = "'tree' missing";
        }
        else {
            tree = GBT_read_plain_tree(pd_main, pd_tree, sizeof(*tree), &error);
            if (error) {
                pgd_assert(!tree);
                error = GBS_global_string("Error reading tree: %s", error);
            }
            else {
                pgd_assert(!error);
                PGD_decodeBranchNames(tree);
            }
        }

        GBDATA *pd_subtree_cont = GB_find(pd_main, "subtrees", 0, down_level);
        if (!pd_subtree_cont) error = "Can't find container 'subtrees'";

        for (GBDATA *pd_subtree = GB_find(pd_subtree_cont,"subtree",0,down_level);
             pd_subtree && !error;
             pd_subtree = GB_find(pd_subtree,"subtree",0,this_level|search_next))
        {
            GBDATA *pd_path = GB_find(pd_subtree, "path", 0, down_level);
            if (!pd_path) {
                error = "no 'path' in 'subtree'";
            }
            else {
                const char *path         = GB_read_char_pntr(pd_path);
                const char *decoded_path = decodePath(path, error);
                if (!error) {
                    GBT_TREE *node = PGD_find_subtree_by_path(tree, decoded_path);
                    if (!node) {
                        error = GBS_global_string("cannot find node '%s'", decoded_path);
                    }
                    else {
                        error = setNodeInfo(pd_subtree, node);
                    }
                }
            }
        }

        if (!error) {
            // save the tree
            PGD_encodeBranchNames(tree);

            const char *treefilename = para.treefile.c_str();
            out.put("Saving %s ..", treefilename);

            FILE *out = fopen(treefilename, "wt");
            if (!out) {
                error = GBS_global_string("Can't write '%s'", treefilename);
            }
            else {
                fprintf(out, "[version=ARB_PS_TREE_%s\n]\n", para.versionumber.c_str());
                error = PGD_export_tree(tree, out);
                fputc('\n', out);
                fclose(out);
            }

            if (error) unlink(treefilename);
        }
    }

    return error;
}

static GB_ERROR convertTargetsToProbesContainer(GBDATA *pd_probe_group, const char *subgroupname, GB_alignment_type ali_type) {
    GBDATA *pd_sub_cont = GB_find(pd_probe_group, subgroupname, 0, down_level);
    if (!pd_sub_cont) return 0; //  some exist some not

    char     T_or_U;
    GB_ERROR error        = GBT_determine_T_or_U(ali_type, &T_or_U, "reverse-complement");
    bool     probes_found = false;

    for (GBDATA *pd_probe = GB_find(pd_sub_cont, "probe", 0, down_level);
         pd_probe && !error;
         pd_probe = GB_find(pd_probe, "probe", 0, this_level|search_next))
    {
        probes_found = true;
        char *probe  = GB_read_string(pd_probe);
        GBT_reverseComplementNucSequence(probe, GB_read_count(pd_probe), T_or_U);
        GB_write_string(pd_probe, probe);
        free(probe);
    }

    if (!error && !probes_found) {
        error = GBS_global_string("Empty '%s' (no probes)", subgroupname);
    }

    return error;
}

static GB_ERROR convertTargetsToProbes(GBDATA *pd_main) {
    // till here the database contains probe-targets (not the probes themselves)

    GB_begin_transaction(pd_main);
    out.put("Converting targets to probes ..");

    GB_ERROR  error              = 0;
    GBDATA   *pd_probegroup_cont = GB_find(pd_main,"probe_groups",0,down_level);

    if (!pd_probegroup_cont) {
        error = "Can't find container 'probe_groups'";
    }
    else {
        GBDATA *pd_ali_type = GB_find(pd_main, "alignment_type", 0, down_level);
        if (!pd_ali_type) {
            error = "'alignment_type' not found";
        }
        else {
            GB_alignment_type ali_type = (GB_alignment_type)GB_read_int(pd_ali_type);

            for (GBDATA *pd_probe_group = GB_find(pd_probegroup_cont,"probe_group",0,down_level);
                 pd_probe_group && !error;
                 pd_probe_group = GB_find(pd_probe_group,"probe_group",0,this_level|search_next))
            {
                error             = convertTargetsToProbesContainer(pd_probe_group, "probe_group_design", ali_type);
                if (!error) error = convertTargetsToProbesContainer(pd_probe_group, "probe_matches", ali_type);
                if (!error) error = convertTargetsToProbesContainer(pd_probe_group, "probe_group_common", ali_type);
            }
        }
    }

    if (!error) GB_commit_transaction(pd_main);
    else GB_abort_transaction(pd_main);

    return error;
}

int main(int argc,char *argv[]) {
    out.put("arb_probe_group_design v1.0 -- (C) 2001-2003 by Tina Lai & Ralf Westram");
    GB_ERROR error = 0;
    {
        indent            ig(out);
        probe_config_data probe_config;
        {
            Config config("probe_parameters.conf", probe_config);
            error = config.getError();
        }

        if (!error) error = pgd_scanArguments(argc, argv); // Check and init Parameters
        if (!error) error = pgd_openDatabases();
        if (!error) error = PM_initSpeciesMaps(pd_main);
        if (!error) {
            initDecodeTable();
            error = designProbes(probe_config);
        }
#if defined(SKIP_PROBES_TO_TARGET_CONVERSION)
#warning probe to target conversion skipped for debugging
#else
        if (!error) error = convertTargetsToProbes(pd_main);
#endif // SKIP_PROBES_TO_TARGET_CONVERSION
        if (!error) {
            out.put("Saving ..");
            indent i(out);
            error = saveTreefile();
            if (!error) {
                error = setDatabaseState(pd_main, "probe_group_design_db", "complete"); // adjust database type
                if (!error) error = saveDatabase();
            }
        }
    }

    if (error) {
        fprintf(stderr, "Error in probe_group_design: %s\n", error);
        return EXIT_FAILURE;
    }
    out.put("arb_probe_group_design done.");
    return EXIT_SUCCESS;
}

