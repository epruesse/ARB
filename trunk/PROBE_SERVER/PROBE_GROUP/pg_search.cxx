/*********************************************************************************
 *  Coded by Tina Lai/Ralf Westram (coder@reallysoft.de) 2001-2004               *
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
    const char *user = GB_getenvUSER();

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
//     int length, numget, restart;
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

//         fpd.length = length;
//         fpd.numget = 30;
//         fpd.restart = 1;

        bool ok = aisc_put(fpd.gl.link, PT_PEP, fpd.pep,
                           PEP_PLENGTH, long(length), // length of wanted probes
                           PEP_NUMGET, long(40), // no of probes in result
                           PEP_RESTART, long(1), // find from beginning
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
        static char *result;
        if (result) { free(result); result = 0; }

        ok = aisc_get(fpd.gl.link, PT_PEP, fpd.pep, PEP_RESULT, &result, 0)==0;

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
        result     = PG_find_next_probe_internal(error);
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

GB_ERROR PG_probe_match(PG_Group& group, const probe_config_data& para, const char *for_probe) {
    //  calls probe-match for 'for_probe' and inserts all matching species into PG_Group 'group'
    
    static PT_server_connection *my_server = 0;
    GB_ERROR                     error     = 0;

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

static GBDATA *PG_find_probe_group_for_species(GBDATA *pb_tree_or_path, SpeciesBagIter start, SpeciesBagIter end, size_t size) {
    for (GBDATA *pb_path = GB_find(pb_tree_or_path, "path", 0, down_level);
         pb_path;
         pb_path = GB_find(pb_path, "path", 0, this_level|search_next))
    {
        GBDATA *pb_members = GB_find(pb_path, "members", 0, down_level);
        pg_assert(pb_members);

        const char     *members = GB_read_char_pntr(pb_members);
        SpeciesBagIter  last_match;
        const char     *mismatch;
        size_t          same    = PG_match_path(members, start, end, last_match, mismatch);

        if (same) {
            if (mismatch) { // "members" contain species NOT contained in 'species'
                // -> searched group does not exist
                return 0;
            }

            if (same == size) { // perfect match
                GBDATA *pb_probes = GB_find(pb_path, "probes", 0, down_level);
                return pb_probes;
            }

            // all species in "members" were in 'species'
            ++last_match;
            return PG_find_probe_group_for_species(pb_path, last_match, end, size-same); // search for rest
        }
    }

    return 0;
}

GBDATA *PG_find_probe_group_for_species(GBDATA *pb_tree_or_path, const SpeciesBag& species) {
    return PG_find_probe_group_for_species(pb_tree_or_path, species.begin(), species.end(), species.size());
}

static bool path_is_subset_of(const char *path, SpeciesBagIter start, SpeciesBagIter end, int allowed_mismatches,
                              SpeciesBagIter& nextToMatch, int& unused_mismatches, int& matched)
{
    int used_mismatches = 0;

    pg_assert(start != end);
    int curr_member = *start;
    matched         = 0;

    for (int path_member = atoi(path); path_member >= 0; path_member = path ? atoi(path) : -1) {
        const char *comma = strchr(path, ',');
        path              = comma ? comma+1 : 0;

        while (curr_member != path_member) {
            if (path_member<curr_member) return false; // 'path_member' is not member of SpeciesBag
            if (used_mismatches >= allowed_mismatches) return false; // too many mismatches

            ++used_mismatches;  // count as mismatch
            if (++start == end) return false;
            curr_member = *start;
        }

        matched++;

        if (++start == end) {
            if (!path) break; // end of bag AND end of path
            return false;
        }

        curr_member = *start;
    }

    pg_assert(used_mismatches <= allowed_mismatches);
    unused_mismatches = allowed_mismatches-used_mismatches;
    nextToMatch       = start;

    return true;
}

static GBDATA *best_covering_probe_group(GBDATA *pb_tree_or_path, SpeciesBagIter start, SpeciesBagIter end, int size, int allowed_mismatches, int& used_mismatches) {
    pg_assert(allowed_mismatches >= 0);

    if (start == end) {
        used_mismatches = 0;
        return GB_find(pb_tree_or_path, "probes", 0, down_level);
    }

    int     min_used_mismatches = INT_MAX;
    GBDATA *best_covering_group = 0;

    // test all existing paths
    for (GBDATA *pb_path = GB_find(pb_tree_or_path, "path", 0, down_level);
         pb_path;
         pb_path = GB_find(pb_path, "path", 0, this_level|search_next))
    {
        GBDATA         *pb_members = GB_find(pb_path, "members", 0, down_level);
        pg_assert(pb_members);
        const char     *members    = GB_read_char_pntr(pb_members);
        SpeciesBagIter  nextToMatch;
        int             curr_unused_mismatches;
        int             matched_members;

        if (path_is_subset_of(members, start, end, allowed_mismatches, nextToMatch, curr_unused_mismatches, matched_members)) {
            int     sub_allowed_mismatches = curr_unused_mismatches;
            int     sub_used_mismatches;
            GBDATA *pb_best_sub_group      = best_covering_probe_group(pb_path, nextToMatch, end, size-matched_members, sub_allowed_mismatches, sub_used_mismatches);

            if (pb_best_sub_group) {
                pg_assert(sub_used_mismatches >= 0 && sub_used_mismatches <= sub_allowed_mismatches);

                int used_mismatches = sub_used_mismatches + (allowed_mismatches-curr_unused_mismatches); 
                pg_assert(used_mismatches >= 0 && used_mismatches <= allowed_mismatches);

                if (used_mismatches<min_used_mismatches) {
                    min_used_mismatches = used_mismatches;
                    best_covering_group = pb_best_sub_group;

                    if (allowed_mismatches>min_used_mismatches) {
                        allowed_mismatches = min_used_mismatches; // restrict further searches
                    }
                }
            }
        }

    }

    used_mismatches = min_used_mismatches;
    return best_covering_group;
}

GBDATA *PG_find_best_covering_probe_group_for_species(GBDATA *pb_rootNode, const SpeciesBag& species, int /*min_non_matched*/, int max_non_matched, int& groupsize) {
    int     used_non_matched;
    GBDATA *pb_group = best_covering_probe_group(pb_rootNode, species.begin(), species.end(), species.size(), max_non_matched, used_non_matched);

    if (pb_group) {
        pg_assert(used_non_matched >= 0 && used_non_matched <= max_non_matched); // otherwise an exact group exists and we should not be here!
        groupsize = species.size() - used_non_matched;
    }
    else {
        groupsize = 0;
    }

    return pb_group;
}

