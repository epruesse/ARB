/*********************************************************************************
 *  Coded by Tina Lai/Ralf Westram (coder@reallysoft.de) 2001-2003               *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#include <string>
#include <deque>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>

#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

#ifndef PG_SEARCH_HXX
#include "pg_search.hxx"
#endif

#define MAX_SPECIES 999999 // max species returned by pt-server (probe match)

using namespace std;

static void my_print(const char *, ...) {
    pg_assert(0); // PG_init_pt_server should install another handler
}

// ================================================================================

static bool      server_initialized            = false;
static char     *current_server_name           = 0;
static void      (*print)(const char *format, ...)  = my_print;

static GB_ERROR connection_lost      = "connection to pt-server lost";
static GB_ERROR cant_contact_unknown = "can't contact pt-server (unknown reason)";
static GB_ERROR cant_contact_refused = "can't contact pt-server (connection refused)";

static GB_ERROR PG_init_pt_server_not_called     = "programmers error: pt-server not contacted (PG_init_pt_server not called)";
static GB_ERROR PG_init_find_probes_called_twice = "programmers error: PG_init_find_probes called twice";


// ================================================================================

//  -------------------------
//      struct gl_struct
//  -------------------------
struct gl_struct {
    aisc_com  *link;
    T_PT_LOCS  locs;
    T_PT_MAIN  com;
};

// ================================================================================

//  -----------------------------------------------------------------
//      static int init_local_com_struct(struct gl_struct& pd_gl)
//  -----------------------------------------------------------------
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

//  --------------------------------------------------------------------------------------
//      static char *probe_pt_look_for_server(const char *servername, GB_ERROR& error)
//  --------------------------------------------------------------------------------------
static char *probe_pt_look_for_server(GBDATA *gb_main, const char *servername, GB_ERROR& error)
{
    int serverid = -1;

    for (int i=0;i<1000; ++i) {
        char *aServer = GBS_ptserver_id_to_choice(i);
        if (aServer) {
            //printf("server='%s'\n",aServer);
            if (strcmp(aServer, servername)==0) {
                serverid = i;
                print("Found pt-server: %s", aServer);
                free(aServer);
                break;
            }
            free(aServer);
        }
    }
    if (serverid==-1) {
        error = GB_export_error("'%s' is not a known pt-server.", servername);
        return 0;
    }
    char choice[256];
    sprintf(choice, "ARB_PT_SERVER%li", (long)serverid);
    error = arb_look_and_start_server(AISC_MAGIC_NUMBER, choice, gb_main);
    if (error) {
        print("error: cannot start pt-server");
        return 0;
    }
    return GBS_read_arb_tcp(choice);
}
// ================================================================================

//  -----------------------------------------------------------------------------------------------------------------------------
//      GB_ERROR PG_init_pt_server(GBDATA *gb_main, const char *servername, void (*print_function)(const char *format, ...))
//  -----------------------------------------------------------------------------------------------------------------------------
GB_ERROR PG_init_pt_server(GBDATA *gb_main, const char *servername, void (*print_function)(const char *format, ...)) {
    if (server_initialized) return "pt-server is already initialized";

    GB_ERROR    error = 0;
    print         = print_function;

    print("Search a free running pt-server..");
    current_server_name = (char *)probe_pt_look_for_server(gb_main, servername, error);
    pg_assert(error || current_server_name);
    server_initialized  = true;;
    return error;
}

//  -------------------------------------
//      void PG_exit_pt_server(void)
//  -------------------------------------
void PG_exit_pt_server(void) {
    free(current_server_name);
    current_server_name = 0;
    server_initialized  = false;
}

// ================================================================================

//  ------------------------------
//      struct FindProbesData
//  ------------------------------
struct FindProbesData {
    struct gl_struct gl;
    T_PT_PEP pep;
    int length, numget, restart;
};

static struct FindProbesData fpd;
static bool initialized = false;

//  -------------------------------------------------------------------------------------------------------
//      bool PG_init_find_probes(GBDATA *gb_main, const char *servername, int length, GB_ERROR& error)
//  -------------------------------------------------------------------------------------------------------
bool PG_init_find_probes(int length, GB_ERROR& error) {
    if (initialized) {
        error = PG_init_find_probes_called_twice;
        return false;
    }
    if (!server_initialized) {
        error = PG_init_pt_server_not_called;
        return false;
    }

    error = 0;
    memset(&fpd, 0, sizeof(fpd));

    fpd.gl.link = (aisc_com*)aisc_open(current_server_name, &fpd.gl.com, AISC_MAGIC_NUMBER);

    if (!fpd.gl.link)               error = cant_contact_unknown;
    else if (init_local_com_struct(fpd.gl))     error = cant_contact_refused;
    else {
        aisc_create(fpd.gl.link, PT_LOCS, fpd.gl.locs, LOCS_PROBE_FIND_CONFIG, PT_PEP, &fpd.pep, 0);

        fpd.length = length;
        fpd.numget = 30;
        fpd.restart = 1;

        bool ok = aisc_put(fpd.gl.link, PT_PEP, fpd.pep,
                           PEP_PLENGTH, length, // length of wanted probes
                           PEP_NUMGET, 10, // no of probes in result
                           PEP_RESTART, 1, // find from beginning
                           0)==0;

        if (!ok) error = connection_lost;
    }

    initialized = (error==0);
    return initialized;
}

//  -----------------------------------
//      bool PG_exit_find_probes()
//  -----------------------------------
bool PG_exit_find_probes() {
    pg_assert(initialized); // forgot to call PG_init_find_probes
    initialized = false;
    if (fpd.gl.link) {
        aisc_close(fpd.gl.link);
    }
    return true;
}

//  ------------------------------------------------------------------------
//      static const char *PG_find_next_probe_internal(GB_ERROR& error)
//  ------------------------------------------------------------------------
static const char *PG_find_next_probe_internal(GB_ERROR& error) {
    pg_assert(initialized);     // forgot to call PG_init_find_probes
    error      = 0;
    bool    ok = aisc_put(fpd.gl.link, PT_PEP, fpd.pep, PEP_FIND_PROBES, 0, 0) == 0;

    if (ok) {
        fpd.restart = 0;

        static char *result;

        if (result) { free(result); result = 0; }

        ok = aisc_get(fpd.gl.link, PT_PEP, fpd.pep,
                      PEP_RESULT, &result,
                      0)==0;

        if (ok) {
            if (result && result[0]) {
                for(int j=0;result[j];++j) {
                    switch (result[j]) {
                        case 2: result[j] = 'A'; break;
                        case 3: result[j] = 'C'; break;
                        case 4: result[j] = 'G'; break;
                        case 5: result[j] = 'U'; break;
                        case ';': result[j] = ';'; break;
                        default:
                            printf("Illegal value (%i) in result ('%s')\n", int(result[j]), result);
                            pg_assert(0);
                            break;
                    }
                }

                return result; // found some probes!
            }

            return 0;
        }
    }

    if (!ok) error = connection_lost;
    return 0;
}

//  --------------------------------------------------------
//      const char *PG_find_next_probe(GB_ERROR& error)
//  --------------------------------------------------------
const char *PG_find_next_probe(GB_ERROR& error) {
    static const char *result;
    static const char *result_ptr;

    if (!result) {
        result = PG_find_next_probe_internal(error);
        result_ptr = result;

        if (!result) return 0; // got all probes
    }

    static char *this_result;
    if (this_result) {
        free(this_result);
        this_result = 0;
    }

    char *spunkt = strchr(result_ptr, ';');
    if (spunkt) {
        int len = spunkt-result_ptr;
        this_result = (char*)malloc(len+1);
        memcpy(this_result, result_ptr, len);
        this_result[len] = 0;
        result_ptr = spunkt+1;
    }
    else {
        this_result = strdup(result_ptr);
        result = result_ptr = 0;
    }

    return this_result;
}

// ================================================================================

// --------------------------------------------------------------------------------
//     class PT_server_connection
// --------------------------------------------------------------------------------
class PT_server_connection {
private:
    T_PT_PDC  pdc;
    GB_ERROR error;
    struct gl_struct my_pd_gl;

public:
    PT_server_connection() {
        error = 0;
        memset(&my_pd_gl, 0, sizeof(my_pd_gl));

        if (!server_initialized) {
            error = PG_init_pt_server_not_called;
        }
        else {
            my_pd_gl.link = (aisc_com *)aisc_open(current_server_name, &my_pd_gl.com,AISC_MAGIC_NUMBER);

            if (!my_pd_gl.link)             error = cant_contact_unknown;
            else if (init_local_com_struct(my_pd_gl))   error = cant_contact_refused;
            else {
                aisc_create(my_pd_gl.link,PT_LOCS, my_pd_gl.locs,
                            LOCS_PROBE_DESIGN_CONFIG, PT_PDC, &pdc,
                            0);
            }
        }
    }
    virtual ~PT_server_connection() {
        if (my_pd_gl.link) aisc_close(my_pd_gl.link);
        delete error;
    }

    struct gl_struct& get_pd_gl() { return my_pd_gl; }
    GB_ERROR get_error() const { return error; }
    T_PT_PDC& get_pdc() { return pdc; }
};


// ================================================================================

//  ----------------------------------------------------------------------------------------------------------------
//      static bool pg_init_probe_match(T_PT_PDC pdc, struct gl_struct& pd_gl, const PG_probe_match_para& para)
//  ----------------------------------------------------------------------------------------------------------------
// static bool pg_init_probe_match(T_PT_PDC pdc, struct gl_struct& pd_gl, const PG_probe_match_para& para) {
static bool pg_init_probe_match(T_PT_PDC pdc, struct gl_struct& pd_gl, const probe_config_data& para) {
    int     i;
    char    buffer[256];

    if (aisc_put(pd_gl.link,
                 PT_PDC,     pdc,
                 PDC_DTEDGE, para.dtedge,
                 PDC_DT,     para.dt,
                 PDC_SPLIT,  para.split,
                 0
                 )) return false;

    for (i=0;i<16;i++) {
        sprintf(buffer,"probe_design/bonds/pos%i",i);
        if (aisc_put(pd_gl.link,
                     PT_PDC,        pdc,
                     PT_INDEX,      i,
                     PDC_BONDVAL,   para.bonds[i],
                     0
                     )) return false;
    }

    return true;
}

//  -----------------------------------------------------------------------------------------------------
//      GB_ERROR PG_probe_match(PG_Group& group, const PG_probe_match_para& para, const char *for_probe)
//  -----------------------------------------------------------------------------------------------------
//  calls probe-match for 'for_probe' and inserts all matching species into PG_Group 'g'

GB_ERROR PG_probe_match(PG_Group& group, const probe_config_data& para, const char *for_probe) {
    static PT_server_connection *my_server = 0;
    GB_ERROR             error     = 0;

    if (!my_server) {
        my_server = new PT_server_connection();
        error     = my_server->get_error();

        if (!error &&
            !pg_init_probe_match(my_server->get_pdc(), my_server->get_pd_gl(), para)) {
            error = connection_lost;
        }

        if (error) {
            delete my_server;
            my_server = 0;
            return error;
        }
    }

    //     char         *match_info, *match_name;
    //     GBDATA       *gb_species_data = 0;
    //     GBDATA       *gb_species;
    //     int       show_status     = 0;
    //     GBT_TREE*         p   = 0;
    //     T_PT_PDC&         pdc   = my_server->get_pdc();

    struct gl_struct&    pd_gl = my_server->get_pd_gl();

    // @@@ soll eigentlich auch reverse-complement gesucht werden??

    // start probe-match:
    if (aisc_nput(pd_gl.link,
                  PT_LOCS,                      pd_gl.locs,
                  LOCS_MATCH_REVERSED,          0,
                  LOCS_MATCH_SORT_BY,           0,
                  LOCS_MATCH_COMPLEMENT,        0, //checked!!
                  LOCS_MATCH_MAX_MISMATCHES,    0, // no mismatches
                  LOCS_MATCH_MAX_SPECIES,       MAX_SPECIES,
                  LOCS_SEARCHMATCH,             for_probe,
                  0))
    {
        error = connection_lost;
    }

    if (!error) {
        // read_results:
        T_PT_MATCHLIST  match_list;
        long            match_list_cnt;
        char           *locs_error = 0;
        bytestring      bs;

        bs.data = 0;

        if (aisc_get( pd_gl.link,
                      PT_LOCS,              pd_gl.locs,
                      LOCS_MATCH_LIST,      &match_list,
                      LOCS_MATCH_LIST_CNT,  &match_list_cnt,
                      LOCS_MATCH_STRING,    &bs,
                      LOCS_ERROR,           &locs_error,
                      0))
        {
            error = connection_lost;
        }

        if (!error && locs_error && locs_error[0]) {
            static char *err = 0;

            if (err) free(err);
            err        = locs_error;
            locs_error = 0;

            error = err;
        }

        if (!error) {
            char toksep[2] = { 1, 0 };
            char *hinfo = strtok(bs.data, toksep);

            if (hinfo) {
                while (1) {
                    char *match_name = strtok(0, toksep); if (!match_name) break;
                    char *match_info = strtok(0, toksep); if (!match_info) break;

                    group.add(match_name);
                }
            }
        }

        free(bs.data);
        free(locs_error);
    }

    return 0;
}

typedef SpeciesBag::const_iterator SpeciesBagIter;

GBDATA *PG_find_probe_group_for_species(GBDATA *node, const SpeciesBag& species) {
    SpeciesBagIter i;
    for (i=species.begin(); i != species.end() && node; ++i) {
        node           = GB_find(node, "num", (const char *)&*i, down_2_level);
        if (node) node = GB_get_father(node);
    }

    if (node) node = GB_find(node, "group", 0, down_level);
    return node;
}

// static void dumpSpecies(SpeciesBagIter start, SpeciesBagIter end) {
//     while (start != end) {
//         fprintf(stdout, "%i ", *start);
//         ++start;
//     }
// }

// static void dumpBestCover(SpeciesBagIter start, SpeciesBagIter end, int allowed_non_hits, int left_non_hits) {
//     fprintf(stdout, "Found cover for ");
//     dumpSpecies(start, end);
//     fprintf(stdout, "allowed_non_hits=%i left_non_hits=%i\n", allowed_non_hits, left_non_hits);
// }

static GBDATA *best_covering_probe_group(GBDATA *pb_node, SpeciesBagIter start, SpeciesBagIter end, int allowed_non_hits, int& left_non_hits) {
    GBDATA *pb_best_group = 0;

//     fprintf(stdout, "Searching cover for ");
//     dumpSpecies(start, end);
//     fprintf(stdout, " (allowed_non_hits=%i)\n", allowed_non_hits);

    pg_assert(allowed_non_hits >= 0);

    if (start == end) {         // all specied found or skipped
        pb_best_group = GB_find(pb_node, "group", 0, down_level);
        left_non_hits = allowed_non_hits;
    }
    else {
        // try direct path
        int            max_left_non_hits = -1;
        SpeciesBagIter next              = start;
        ++next;

        {
            SpeciesID  id      = *start;
            GBDATA    *pb_num = GB_find(pb_node, "num", (const char *)&id, down_2_level);

            if (pb_num) { // yes there may be such a group
                GBDATA *pb_subnode = GB_get_father(pb_num);
                int     unused_non_hits;
                GBDATA *pb_group   = best_covering_probe_group(pb_subnode, next, end, allowed_non_hits, unused_non_hits);

                if (pb_group) {
//                     dumpBestCover(next, end, allowed_non_hits, unused_non_hits);

                    pb_best_group     = pb_group;
                    max_left_non_hits = unused_non_hits;
                }
            }
        }

        if (allowed_non_hits) { // try to find group w/o current species
            int     unused_non_hits;
            GBDATA *pb_group = 0;

            if (max_left_non_hits == -1) {
                pb_group = best_covering_probe_group(pb_node, next, end, allowed_non_hits-1, unused_non_hits);
            }
            else {
                // we already found a group with 'max_left_non_hits' left hits
                // try to find better group only
                if (allowed_non_hits > max_left_non_hits) {
                    pb_group         = best_covering_probe_group(pb_node, next, end, allowed_non_hits-(max_left_non_hits+1), unused_non_hits);
                    unused_non_hits += (max_left_non_hits+1);
                }
            }

//             if (pb_group) dumpBestCover(next, end, allowed_non_hits, unused_non_hits);

            if (pb_group && unused_non_hits>max_left_non_hits) {
                pb_best_group  = pb_group;
                max_left_non_hits = unused_non_hits;
            }
        }

        left_non_hits = max_left_non_hits;
    }

    return pb_best_group;
}

GBDATA *PG_find_best_covering_probe_group_for_species(GBDATA *pb_rootNode, const SpeciesBag& species, int /*min_non_matched*/, int max_non_matched, int& groupsize) {
    int     unused_non_matched;
    GBDATA *pb_group = best_covering_probe_group(pb_rootNode, species.begin(), species.end(), max_non_matched, unused_non_matched);

    if (pb_group) {
        int non_matched = max_non_matched-unused_non_matched;
        pg_assert(non_matched >= 0); // otherwise an exact group exists and we should not be here!
        groupsize       = species.size()-non_matched;
    }
    else {
        groupsize = 0;
    }

    return pb_group;
}


// return the id number stored in num under node
SpeciesID PG_get_id(GBDATA *node){
    GBDATA *pg_node = GB_find(node,"num",0,down_level);
    return GB_read_int(pg_node);
}



