#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>

#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

#include <iostream>

#include "probe_functions.hxx"
#include "chip.hxx"

#define MAX_SPECIES 999999 // max species returned by pt-server (probe match)
using namespace std;

static void my_print(const char *, ...) {
    chip_assert(0); // PG_init_pt_server should install another handler
}
// ================================================================================

static bool 	 server_initialized  		   = false;;
static char 	*current_server_name 		   = 0;
static void 	 (*print)(const char *format, ...) = my_print;

static GB_ERROR connection_lost      = "connection to pt-server lost";
static GB_ERROR cant_contact_unknown = "can't contact pt-server (unknown reason)";
static GB_ERROR cant_contact_refused = "can't contact pt-server (connection refused)";

static GB_ERROR PG_init_pt_server_not_called 	 = "programmers error: pt-server not contacted (PG_init_pt_server not called)";
static GB_ERROR PG_init_find_probes_called_twice = "programmers error: PG_init_find_probes called twice";

using namespace std;


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

//  ---------------------------------------------
//      static char *pd_ptid_to_choice(int i)
//  ---------------------------------------------
static char *pd_ptid_to_choice(int i){
    char search_for[256];
    char choice[256];
    char	*fr;
    char *file;
    char *server;
    char empty[] = "";
    sprintf(search_for,"ARB_PT_SERVER%i",i);

    server = GBS_read_arb_tcp(search_for);
    if (!server) return 0;
    fr = server;
    file = server;				/* i got the machine name of the server */
    if (*file) file += strlen(file)+1;	/* now i got the command string */
    if (*file) file += strlen(file)+1;	/* now i got the file */
    if (strrchr(file,'/')) file = strrchr(file,'/')-1;
    if (!(server = strtok(server,":"))) server = empty;
    sprintf(choice,"%-8s: %s",server,file+2);
    delete fr;

    return strdup(choice);
}

//  --------------------------------------------------------------------------------------
//      static char *probe_pt_look_for_server(const char *servername, GB_ERROR& error)
//  --------------------------------------------------------------------------------------
static char *probe_pt_look_for_server(GBDATA *gb_main, const char *servername, GB_ERROR& error)
{
    int serverid = -1;

    for (int i=0;i<1000; ++i) {
        char *aServer = pd_ptid_to_choice(i);
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

	    if (!my_pd_gl.link) 			error = cant_contact_unknown;
	    else if (init_local_com_struct(my_pd_gl)) 	error = cant_contact_refused;
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
static bool pg_init_probe_match(T_PT_PDC pdc, struct gl_struct& pd_gl, const PG_probe_match_para& para) {
    int 	i;
    char 	buffer[256];

    if (aisc_put(pd_gl.link, PT_PDC, pdc,
                 PDC_DTEDGE, 		para.dtedge,
                 PDC_DT, 		para.dt,
                 PDC_SPLIT, 		para.split,
                 0)) return false;

    for (i=0;i<16;i++) {
        sprintf(buffer,"probe_design/bonds/pos%i",i);
        if (aisc_put(pd_gl.link,PT_PDC, pdc,
                     PT_INDEX,		i,
                     PDC_BONDVAL, 	para.bondval[i],
                     0) ) return false;
    }

    return true;
}
//  -----------------------------------------------------------------------------------------------------
//      GB_ERROR PG_probe_match(euer_container& g, const PG_probe_match_para& para, const char *for_probe)
//  -----------------------------------------------------------------------------------------------------
//  calls probe-match for 'for_probe' and inserts all matching species into PG_Group 'g'

GB_ERROR PG_probe_match(euer_container& g, const PG_probe_match_para& para, const char *for_probe) {
    static PT_server_connection *my_server = 0;
    GB_ERROR 			 error 	   = 0;

    if (!my_server) {
        my_server = new PT_server_connection();
        error 	  = my_server->get_error();

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

    //     char 		*match_info, *match_name;
    //     GBDATA 		*gb_species_data = 0;
    //     GBDATA 		*gb_species;
    //     int 		 show_status 	 = 0;
    //     GBT_TREE* 		 p	 = 0;
//     T_PT_PDC& 		 pdc   = my_server->get_pdc();

    struct gl_struct& 	 pd_gl = my_server->get_pd_gl();

    // @@@ soll eigentlich auch reverse-complement gesucht werden??

    // start probe-match:
    if (aisc_nput(pd_gl.link,
		  PT_LOCS, 			pd_gl.locs,
                  LOCS_MATCH_REVERSED,		0,
                  LOCS_MATCH_SORT_BY,		0,
                  LOCS_MATCH_COMPLEMENT, 	0,
                  LOCS_MATCH_MAX_MISMATCHES,	0, 	// no mismatches
                  LOCS_MATCH_MAX_SPECIES, 	MAX_SPECIES,
                  LOCS_SEARCHMATCH,		for_probe,
                  0))
    {
        error = connection_lost;
    }

    if (!error) {
        // read_results:
        T_PT_MATCHLIST 	 match_list;
        long 		 match_list_cnt;
        char		*locs_error = 0;
        bytestring 	 bs;

        bs.data = 0;

        if (aisc_get( pd_gl.link, PT_LOCS, pd_gl.locs,
                      LOCS_MATCH_LIST,		&match_list,
                      LOCS_MATCH_LIST_CNT,	&match_list_cnt,
                      LOCS_MATCH_STRING,	&bs,
                      LOCS_ERROR,		&locs_error,
                      0))
        {
            error = connection_lost;
        }

        if (!error && locs_error && locs_error[0]) {
            static char *err;

            if (err) free(err);
            err   = locs_error;
            error = err;
        }

        if (!error) {
            char toksep[2]     = { 1, 0 };
            char 	*hinfo = strtok(bs.data, toksep);

            if (hinfo) {
                while (1) {
                    char 	*match_name = strtok(0, toksep); if (!match_name) break;
                    char 	*match_info = strtok(0, toksep); if (!match_info) break;

                    // @@@ hier Namen in container einfuegen
                }
            }
        }

        free(bs.data);
    }

    return 0;
}





