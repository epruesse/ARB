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

#include "../global_defs.h"
#include "../common.h"
#include "../mapping.h"

#define MINTEMPERATURE 30.0
#define MAXTEMPERATURE 100.0
#define MINGC          50.0
#define MAXGC          100.0
#define MAXBOND        4.0
#define MINPOSITION    0
#define MAXPOSITION    10000
#define MISHIT         0
#define MINTARGETS     100
#define DTEDGE         0.5
#define DT             0.5
#define SPLIT          0.5
#define CLIPRESULT     40       //??? length of output

typedef string SpeciesName;
typedef string Probes;

GBDATA *gb_main = 0;            //ARB-database
GBDATA *pd_main = 0;            //probe-design-database

typedef enum { TF_NONE, TF_CREATE, TF_EXTEND } Treefile_Action;

struct InputParameter{
    string          db_name;
    string          pt_server_name;
    string          pd_db_name;
    int             pb_length;
    Treefile_Action gen_treefile;
    string          treefile;
    string          versionumber;
};

static InputParameter para;

float bondval[16]={0.0,0.0,0.5,1.1,
                   0.0,0.0,1.5,0.0,
                   0.5,1.5,0.4,0.9,
                   1.1,0.0,0.9,0.0};

struct gl_struct {
    aisc_com  *link;
    T_PT_LOCS  locs;
    T_PT_MAIN  com;
};


static bool server_initialized=false;
static char *current_server_name=0;

static char *probe_pt_look_for_server(GBDATA *gb_main,const char *servername,GB_ERROR& error){
    int serverid = -1;

    for (int i=0;i<1000;++i){
        char *aServer=GBS_ptserver_id_to_choice(i);
        if (aServer){
            if(strcmp(aServer,servername)==0){
                serverid=i;
                printf("Found pt-server: %s\n",aServer);
                free(aServer);
                break;
            }
            free(aServer);
        }
    }
    if (serverid==-1){
        error = GB_export_error("'%s' is not a known pt-server.",servername);
        return 0;
    }

    char choice[256];
    sprintf(choice,"ARB_PT_SERVER%li",(long)serverid);
    GB_ERROR starterr = arb_look_and_start_server(AISC_MAGIC_NUMBER, choice, gb_main);
    if (starterr) {
        error = GB_export_error("Cannot start pt-server '%s' (Reason: %s)", servername, starterr);
        return 0;
    }
    return GBS_read_arb_tcp(choice);
}

GB_ERROR PG_init_pt_server(GBDATA *gb_main, const char *servername) {
    GB_ERROR error = 0;
    if (server_initialized) {
        error = "pt-server initialized twice";
    }
    else {
        printf("Search a free running pt-server..\n");
        current_server_name            = (char *)probe_pt_look_for_server(gb_main,servername,error);
        if (!error) server_initialized = true;
    }
    return error;
}

void PG_exit_pt_server(void) {
    free(current_server_name);
    current_server_name = 0;
    server_initialized  = false;
}

static int init_local_com_struct(struct gl_struct& pd_gl)
{
    const char *user;
    if (!(user = getenv("USER"))) user = "unknown user";

    /* @@@ use finger, date and whoami */
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
    T_PT_PDC  pdc;
    GB_ERROR error;
    struct gl_struct my_pd_gl;
    PT_server_connection(){
        error = 0;
        memset(&my_pd_gl, 0, sizeof(my_pd_gl));

    my_pd_gl.link=(aisc_com *)aisc_open(current_server_name,&my_pd_gl.com,AISC_MAGIC_NUMBER);

    if(!my_pd_gl.link) error = "can't contact pt_server (unknown reason\n";
    else if(init_local_com_struct(my_pd_gl)) error = "can't contact pt_server (connection refused)\n";
    else{
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
void helpArguments(){
    fprintf(stderr,
            "\n"
            "Usage: arb_probe_group_design <db_name> <pt_server> <db_out> <probe_length> [(-c|-x) <treefile> <versionumber>]\n"
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
//      GB_ERROR scanArguments(int argc,char *argv[])
//  --------------------------------------------------------
GB_ERROR scanArguments(int argc,char *argv[]){
    GB_ERROR error = 0;

    if (argc == 5 || argc == 8) {
        para.db_name        = argv[1];
        para.pt_server_name = string("localhost: ")+argv[2];
        para.pd_db_name     = string(argv[3])+argv[4];
        para.pb_length      = atoi(argv[4]);
        if (argc == 8) {
            if      (strcmp(argv[5], "-c") == 0) para.gen_treefile = TF_CREATE;
            else if (strcmp(argv[5], "-x") == 0) para.gen_treefile = TF_EXTEND;
            else error = GBS_global_string("'-c' or '-x' expected, '%s' found", argv[5]);

            para.treefile     = string(argv[6]);
            para.versionumber = string(argv[7]);
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

void pgd_init_species(GBDATA *subtree,set<SpeciesName> *species){
    GBDATA *grp_species=GB_find(subtree,"species",0,down_level);
    if(grp_species){
        GBDATA *name=GB_find(grp_species,"name",0,down_level);
        do{
            (*species).insert(GB_read_string(name));
        }while((name=GB_find(name,"name",0,this_level|search_next)));
    }
}

char *pgd_get_the_names(set<SpeciesName> *species,bytestring &bs,bytestring &checksum){
    GBDATA *gb_species;
    GBDATA *gb_name;
    GBDATA *gb_data;

    void *names = GBS_stropen(1024);
    void *checksums = GBS_stropen(1024);

    GB_begin_transaction(gb_main);
    char *use=GBT_get_default_alignment(gb_main);

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

    bs.data = GBS_strclose(names, 0);
    bs.size = strlen(bs.data)+1;

    checksum.data = GBS_strclose(checksums, 0);
    checksum.size = strlen(checksum.data)+1;

    GB_commit_transaction(gb_main);
    return 0;
}

int pgd_probe_design_send_data(){
    if (aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                 PDC_DTEDGE,DTEDGE*100.0,
                 PDC_DT,DT*100.0,
                 PDC_SPLIT,SPLIT,
                 PDC_CLIPRESULT,CLIPRESULT,
                 0)) return 1;
    for (int i=0;i<16;i++) {
        if (aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                     PT_INDEX,i,
                     PDC_BONDVAL,bondval[i],
                     0)) return 1;
    }
    return 0;
}

GB_ERROR probe_design_event(set<SpeciesName> *species,set<Probes> *probe_list)
{
    T_PT_TPROBE  tprobe;        //long
    bytestring   bs;
    bytestring   check;
    char        *match_info;
    const char  *error = 0;

    // validate sequence of the species
    error = pgd_get_the_names(species,bs,check);

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
                delete unknown_names.data;
            }
        }
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

    return error;
}

GB_ERROR PGD_export_tree(GBT_TREE *node, FILE *out) {
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
        if (node->name) { // groupname
            fprintf(out, "'%s'", node->name);
        }
    }
    return error;
}

GB_ERROR PGD_decodeBranchNames(GBT_TREE *node) {
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
            const string& shortname = PM_ID2name(atoi(newName));
            if (shortname.empty()) {
                error = GBS_global_string("illegal id in '%s'", newName);
            }
            else {
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
GB_ERROR PGD_encodeBranchNames(GBT_TREE *node) {
    if (node->is_leaf) {
        if (!node->name) return "node w/o name";
        char *newName = encodeTreeNode(node->name);
        free(node->name);
        node->name    = newName;
        return 0;
    }

    GB_ERROR error    = PGD_encodeBranchNames(node->leftson);
    if (!error) error = PGD_encodeBranchNames(node->rightson);
    return error;
}

GBT_TREE *PGD_find_subtree_by_path(GBT_TREE *node, const char *path) {
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

int main(int argc,char *argv[]) {
    printf("arb_probe_group_design v1.0 -- (C) 2001-2003 by Tina Lai & Ralf Westram\n");

    //Check and init Parameters:
    GB_ERROR error = scanArguments(argc, argv);

    //Open arb database:
    if (!error) {
        const char *db_name = para.db_name.c_str();
        printf("Opening ARB-database '%s'...",db_name);

        // @@@ FIXME:  I think this programm should be able to run w/o the main database
        // it only uses the db to open the pt-server connection and does
        // some unneeded species checking..

        gb_main = GBT_open(db_name,"rwt",0);
        if (!gb_main) {
            error            = GB_get_error();
            if(!error) error = GB_export_error("Can't open database '%s'",db_name);
        }
    }

    //Open probe design database:
    if(!error){
        string      pd_nameS = para.pd_db_name+".arb";
        const char *pd_name  = pd_nameS.c_str();
        printf("Opening probe-group-design-database '%s'...\n" ,pd_name);

        pd_main = GB_open(pd_name,"rwcN");
        if (!pd_main) {
            error            = GB_get_error();
            if(!error) error = GB_export_error("Can't open database '%s'",pd_name);
        }
        else {
            error = checkDatabaseType(pd_main, pd_name, "probe_group_subtree_db", "complete");
        }
    }

    if (!error) {
        error = PM_initSpeciesMaps(pd_main);
    }

    if (!error) {
        GB_begin_transaction(pd_main);
        GBDATA *pd_container = GB_find(pd_main,"subtree_with_probe",0,down_level);
        if (!pd_container) {
            error = GB_export_error("Can't find container (subtree_with_probe)!");
        }
        else {
            if (!GB_find(pd_container,"subtree",0,down_level)) {
                error = GB_export_error("Can't find container (subtree)!");
            }
            else {
                //initialize connection
                PG_init_pt_server(gb_main,para.pt_server_name.c_str());
                my_server = new PT_server_connection();
                error     = my_server->get_error();

                if (!error) {
                    //initialize parameters
                    aisc_create(my_server->get_pd_gl().link,PT_LOCS,my_server->get_pd_gl().locs,
                                LOCS_PROBE_DESIGN_CONFIG,PT_PDC,&my_server->pdc,
                                PDC_PROBELENGTH,para.pb_length,
                                PDC_MINTEMP,MINTEMPERATURE,
                                PDC_MAXTEMP,MAXTEMPERATURE,
                                PDC_MINGC,MINGC/100.0,
                                PDC_MAXGC,MAXGC/100.0,
                                PDC_MAXBOND,MAXBOND,
                                0);
                    aisc_put(my_server->get_pd_gl().link,PT_PDC,my_server->pdc,
                             PDC_MINPOS,MINPOSITION,
                             PDC_MAXPOS,MAXPOSITION,
                             PDC_MISHIT,MISHIT,
                             PDC_MINTARGETS,MINTARGETS/100.0,
                             0);

                    if (pgd_probe_design_send_data()) {
                        error = "Connection to PT_SERVER lost (1)";
                    }
                }

                if (!error) {
                    // count substrees
                    long subtrees   = 0;
                    for (GBDATA *pd_subtree = GB_find(pd_container,"subtree",0,down_level);
                         pd_subtree;
                         pd_subtree = GB_find(pd_subtree,"subtree",0,this_level|search_next))
                    {
                        ++subtrees;
                    }

                    printf("Designing probes for %li subtrees:\n", subtrees);

                    // design probes
                    long count      = 0;
                    for (GBDATA *pd_subtree = GB_find(pd_container,"subtree",0,down_level);
                         pd_subtree;
                         pd_subtree = GB_find(pd_subtree,"subtree",0,this_level|search_next))
                    {
                        ++count;
                        if (count%60) fputc('.', stdout);
                        else printf(". %i%%\n", int((double(count)/subtrees)*100+0.5));

                        // erase existing data
                        GBDATA *pbgrp = GB_find(pd_subtree,"probe_group_design",0,down_level);
                        if(pbgrp) GB_delete(pbgrp);

                        // get species
                        std::set<SpeciesName> species; //set of species for probe design
                        std::set<Probes>      probe; //set of probes from the design
                        pgd_init_species(pd_subtree,&species);

                        error = probe_design_event(&species,&probe); // call probe_design

                        if (!error && !probe.empty()) {
                            // store the result
                            GBDATA *pgd_pbgrp = GB_search(pd_subtree,"probe_group_design",GB_CREATE_CONTAINER);
                            for (set<Probes>::const_iterator i=probe.begin();
                                 i!=probe.end() && !error;
                                 ++i)
                            {
                                GBDATA *pgd_probe = GB_create(pgd_pbgrp,"probe",GB_STRING);
                                error             = GB_write_string(pgd_probe,(*i).c_str());
                            }
                        }
                    }

                    if (error) fputc('\n', stdout);
                    else {
                        printf(" %i%%\n", int((double(count)/subtrees)*100+0.5));
                        printf("\nMarking common probes..\n");

                        GBT_TREE *tree = 0;
                        if (para.gen_treefile != TF_NONE) { // create/extend tree ?
                            GBDATA *pd_tree = GB_find(pd_main, "tree", 0, down_level);
                            if (!pd_tree) {
                                error = "'tree' missing";
                            }
                            else {
                                tree = GBT_read_plain_tree(pd_main, pd_tree, sizeof(*tree));
                                PGD_decodeBranchNames(tree);
                            }
                        }

                        // check whether probes common in probe_group and probe_group_design
                        count           = 0;
                        for (GBDATA *pd_subtree = GB_find(pd_container,"subtree",0,down_level);
                             pd_subtree && !error;
                             pd_subtree = GB_find(pd_subtree,"subtree",0,this_level|search_next))
                        {
                            ++count;
                            if (count%60) fputc('.', stdout);
                            else printf(". %i%%\n", int((double(count)/subtrees)*100+0.5));

                            GBDATA *pd_probe_group        = GB_find(pd_subtree, "probe_group", 0, down_level);
                            GBDATA *pd_probe_group_design = GB_find(pd_subtree, "probe_group_design", 0, down_level);
                            GBDATA *pd_probe_group_common = 0;

                            if (pd_probe_group && pd_probe_group_design) {
                                int probes_found = 0;

                                for (GBDATA *pd_probe1 = GB_find(pd_probe_group, "probe", 0, down_level);
                                     pd_probe1; )
                                {
                                    GBDATA     *pd_next_probe1 = GB_find(pd_probe1, "probe", 0, this_level|search_next);
                                    const char *probe_string   = GB_read_char_pntr(pd_probe1);
                                    GBDATA     *pd_probe2      = GB_find(pd_probe_group_design, 0, probe_string, down_level);

                                    if (pd_probe2) { // 'probe_group_design' also contains the probe
                                        // -> put the common probe into 'probe_group_common'
                                        if (!pd_probe_group_common) {
                                            pd_probe_group_common = GB_search(pd_subtree, "probe_group_common", GB_CREATE_CONTAINER);
                                            if (!pd_probe_group_common) error = GB_get_error();
                                        }

                                        if (!error) {
                                            GBDATA *pd_common1     = GB_create(pd_probe_group_common, "probe", GB_STRING);
                                            if (!pd_common1) error = GB_get_error();
                                            else    error          = GB_write_string(pd_common1, probe_string);

                                            probes_found++;

                                            // and delete probe from 'probe_group' and 'probe_group_design'
                                            if (!error) error = GB_delete(pd_probe2);
                                            if (!error) error = GB_delete(pd_probe1);
                                        }
                                    }

                                    pd_probe1 = pd_next_probe1;
                                }

                                // add number of found probes to nodename
                                if (probes_found && !error) {
                                    GBDATA     *pd_path      = GB_find(pd_subtree, "path", 0, down_level);
                                    const char *path         = GB_read_char_pntr(pd_path);
                                    GBT_TREE   *current_node = PGD_find_subtree_by_path(tree, path);

                                    if (!current_node) {
                                        error = GBS_global_string("Can't find current node (%s)", path);
                                    }
                                    else {
                                        if (current_node->name) {
                                            char *newName      = GBS_global_string_copy("%s,p=%i", current_node->name, probes_found);
                                            free(current_node->name);
                                            current_node->name = newName;
                                        }
                                        else {
                                            current_node->name = GBS_global_string_copy("p=%i", probes_found);
                                        }
                                    }
                                }
                            }
                            else {
#if defined(DEBUG)
                                GBDATA     *pd_path = GB_find(pd_subtree, "path", 0, down_level);
                                const char *path    = GB_read_char_pntr(pd_path);
                                const char *error_message;

                                if (pd_probe_group) {
                                    error_message = "Container 'probe_group_design' not found";
                                }
                                else {
                                    if (pd_probe_group_design) error_message = "Container 'probe_group' not found";
                                    else error_message                       = "Containers 'probe_group' and 'probe_group_design' not found";
                                }

                                fprintf(stderr, "\n%s (in subtree '%s')\n", error_message, path);
#endif // DEBUG
                            }
                        }

                        if (error) fputc('\n', stdout);
                        else printf(" %i%%\n", int((double(count)/subtrees)*100+0.5));

                        if (tree && !error) {
                            PGD_encodeBranchNames(tree);

                            const char *treefilename = para.treefile.c_str();
                            FILE       *out          = fopen(treefilename, "wt");

                            fprintf(out, "[version=ARB_PS_TREE_%s\n]\n", para.versionumber.c_str());
                            error = PGD_export_tree(tree, out);
                            fputc('\n', out);
                            fclose(out);

                            if (error) unlink(treefilename);
                        }
                    }
                }

                // clean up pt-server connection
                aisc_close(my_server->get_pd_gl().link);
                my_server->get_pd_gl().link = 0;
                PG_exit_pt_server();
            }
        }

        // adjust database type
        if (!error) error = setDatabaseState(pd_main, "probe_group_design_db", "complete");

        if (error) {
            GB_abort_transaction(pd_main);
        }
        else {
            GB_commit_transaction(pd_main);
            string      savenameS = para.pd_db_name+"_design.arb";
            const char *savename  = savenameS.c_str();
            printf("Saving %s ..\n", savename);
            error                 = GB_save(pd_main, savename, SAVE_MODE);
        }
    }

    if (error) {
        fprintf(stderr, "Error in probe_group_design: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

