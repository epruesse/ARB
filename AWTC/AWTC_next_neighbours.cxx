#include <arbdbt.h>

#include <servercntrl.h>
#include <PT_com.h>
#include <client.h>
#include <aw_window.hxx>

#include <awtc_next_neighbours.hxx>

#include <cstdio>
#include <climits>


AWTC_FIND_FAMILY_MEMBER::AWTC_FIND_FAMILY_MEMBER() {
    matches = 0;
    name = 0;
    next = 0;
}

AWTC_FIND_FAMILY_MEMBER::~AWTC_FIND_FAMILY_MEMBER() {
    free(name);
}

void awtc_ff_message(const char *msg) {
    GB_warning(msg);
}

void AWTC_FIND_FAMILY::delete_family_list() {
    AWTC_FIND_FAMILY_MEMBER *fl, *fln;
    for (fl = family_list; fl;  fl = fln) {
         fln = fl->next;
         delete fl;
    }
    family_list = 0;
}

GB_ERROR AWTC_FIND_FAMILY::init_communication()
{
    const char *user = "Find Family";

    // connect PT server
    if (aisc_create(link, PT_MAIN, com,
                    MAIN_LOCS, PT_LOCS, &locs,
                    LOCS_USER, user,
                    NULL)) {
        return GB_export_error("Cannot initialize communication");
    }
    return 0;
}


GB_ERROR AWTC_FIND_FAMILY::open(char *servername) {
    GB_ERROR error = 0;
    if (arb_look_and_start_server(AISC_MAGIC_NUMBER, servername, gb_main)) {
        error = "Cannot contact PT  server";
    }
    else {
        const char *socketid = GBS_read_arb_tcp(servername);
        if (!socketid) error = GB_await_error();
        else {
            link = (aisc_com *)aisc_open(socketid, &com, AISC_MAGIC_NUMBER);
            if (!link) error = "Cannot contact PT server [1]";
            else if (init_communication()) error = "Cannot contact PT server [2]";
        }
    }
    return error;
}

void AWTC_FIND_FAMILY::close()
{
   if (link) aisc_close(link);
   link = 0;
}

AWTC_FIND_FAMILY::AWTC_FIND_FAMILY(GBDATA *gb_maini)
{
    memset((char *)this, 0, sizeof(*this));
    this->gb_main = gb_maini;
}

AWTC_FIND_FAMILY::~AWTC_FIND_FAMILY()
{
    delete_family_list();
    close();
}

GB_ERROR AWTC_FIND_FAMILY::retrieve_family(char *sequence, int oligo_len, int mismatches, bool fast_flag, bool rel_matches, FF_complement compl_mode, int max_results) {
    T_PT_FAMILYLIST f_list;
    char *compressed_sequence = GB_command_interpreter(gb_main, sequence, "|keep(acgtunACGTUN)", 0, 0);

    bytestring bs;
    bs.data = compressed_sequence;
    bs.size = strlen(bs.data)+1;

    delete_family_list();
    /* Start find_family() at the PT_SERVER
     *
     * Here we have to make a loop, until the match count of the
     * first member is big enough
     */

    if (aisc_put(link, PT_LOCS, locs,
                 LOCS_FF_PROBE_LEN,             oligo_len,        // oligo length (12 hardcoded till July 2008)
                 LOCS_FF_MISMATCH_NUMBER,       mismatches,       // number of mismatches (0 hardcoded till July 2008)
                 LOCS_FF_FIND_TYPE,             int(fast_flag),   // 0: complete search, 1: quick search (only search oligos starting with 'A')
                 LOCS_FF_SORT_TYPE,             int(rel_matches), // 0: matches, 1: relative matches (0 hardcoded till July 2008)
                 LOCS_FF_SORT_MAX,              max_results,      // speed up family sorting (only sort retrieved results)
                 LOCS_FF_COMPLEMENT,            compl_mode,       // any combination of: 1 = forward, 2 = reverse, 4 = reverse-complement, 8 = complement (1 hardcoded in PT-Server till July 2008)
                 LOCS_FF_FIND_FAMILY,           &bs,
                 NULL))
    {
        return GB_export_error  ("Communication Error (2)");
    }

    // Read family list
    aisc_get(link, PT_LOCS, locs,
             LOCS_FF_FAMILY_LIST, &f_list,
             LOCS_FF_FAMILY_LIST_SIZE, &real_hits,
             NULL);

    hits_truncated = false;
    if (max_results<1) max_results = INT_MAX;

    AWTC_FIND_FAMILY_MEMBER *tail = NULL;
    while (f_list) {
        if (max_results == 0) {
            hits_truncated = true;
            break;
        }
        max_results--;

        AWTC_FIND_FAMILY_MEMBER *fl = new AWTC_FIND_FAMILY_MEMBER();

        (tail ? tail->next : family_list) = fl;
        tail                              = fl;
        fl->next                          = NULL;

        aisc_get(link, PT_FAMILYLIST, f_list,
                 FAMILYLIST_NAME,        &fl->name,
                 FAMILYLIST_MATCHES,     &fl->matches,
                 FAMILYLIST_REL_MATCHES, &fl->rel_matches,
                 FAMILYLIST_NEXT,        &f_list,
                 NULL);
    }

    free(compressed_sequence);
    return 0;
}

void AWTC_FIND_FAMILY::print() {
    AWTC_FIND_FAMILY_MEMBER *fl;
    for (fl = family_list; fl;  fl = fl->next) {
        printf("%s %li\n", fl->name, fl->matches);
    }
}

GB_ERROR AWTC_FIND_FAMILY::findFamily(int server_id, char *sequence, int oligo_len, int mismatches, bool fast_flag, bool rel_matches, FF_complement compl_mode, int max_results) {
    // searches the PT-server for related species.
    //
    // relation-score is calculated by fragmenting the sequence into oligos of length 'oligo_len' and
    // then summarizing the number of hits.
    //
    // 'mismatches'  = the number of allowed mismatches
    // 'fast_flag'   = 0 -> do complete search, 1 -> search only oligos starting with 'A'
    // 'rel_matches' = 0 -> score is number of oligo-hits, 1 -> score is relative to longer sequence (target or source) * 10
    //
    // 'max_results' limits the length of the generated result list (low scores deleted first)
    //               if < 1 -> don't limit

    char     *buffer = GBS_global_string_copy("ARB_PT_SERVER%i", server_id);
    GB_ERROR  error  = open(buffer);

    if (!error) {
        error = init_communication();
        if (!error) {
            error = retrieve_family(sequence, oligo_len, mismatches, fast_flag, rel_matches, compl_mode, max_results);
        }
    }
    close();
    return error;
}


void AWTC_create_common_next_neighbour_vars(AW_root *aw_root) {
    static bool created = false;
    if (!created) {
        aw_root->awar_int(AWAR_NN_OLIGO_LEN,   12);
        aw_root->awar_int(AWAR_NN_MISMATCHES,  0);
        aw_root->awar_int(AWAR_NN_FAST_MODE,   0);
        aw_root->awar_int(AWAR_NN_REL_MATCHES, 1);

        created = true;
    }
}

void AWTC_create_common_next_neighbour_fields(AW_window *aws) {
    // used in several figs: ad_spec_nn.fig ad_spec_nnm.fig awtc/family_settings.fig

    aws->at("oligo_len");
    aws->create_input_field(AWAR_NN_OLIGO_LEN, 3);

    aws->at("mismatches");
    aws->create_input_field(AWAR_NN_MISMATCHES, 3);

    aws->at("mode");
    aws->create_option_menu(AWAR_NN_FAST_MODE, 0, 0);
    aws->insert_default_option("Complete", "", 0);
    aws->insert_option("Quick", "", 1);
    aws->update_option_menu();

    aws->at("score");
    aws->create_option_menu(AWAR_NN_REL_MATCHES, 0, 0);
    aws->insert_option("absolute", "", 0);
    aws->insert_default_option("relative", "", 1);
    aws->update_option_menu();

}

