// ================================================================ //
//                                                                  //
//   File      : AWTC_next_neighbours.cxx                           //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "awtc_next_neighbours.hxx"

#include <servercntrl.h>
#include <PT_com.h>
#include <client.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <arbdbt.h>
#include <arb_strbuf.h>

#include <climits>

void awtc_ff_message(const char *msg) {
    GB_warning(msg);
}


// -------------------
//      FamilyList

FamilyList::FamilyList()
    : next(NULL),
      name(NULL),
      matches(0),
      rel_matches(0.0)
{}

FamilyList::~FamilyList() {
    free(name);
    delete next;
}

FamilyList *FamilyList::insertSortedBy_matches(FamilyList *other) {
    ff_assert(next == NULL); // only insert unlinked instace!

    if (!other) {
        return this;
    }

    if (matches >= other->matches) {
        next = other;
        return this;
    }

    // insert into other
    FamilyList *rest = other->next;
    other->next      = rest ? insertSortedBy_matches(rest) : this;

    return other;
}

FamilyList *FamilyList::insertSortedBy_rel_matches(FamilyList *other) {
    ff_assert(next == NULL); // only insert unlinked instace!

    if (rel_matches >= other->rel_matches) {
        next = other;
        return this;
    }

    // insert into other
    FamilyList *rest = other->next;
    other->next      = rest ? insertSortedBy_rel_matches(rest) : this;

    return other;

}

// ---------------------
//      FamilyFinder

FamilyFinder::FamilyFinder(bool rel_matches_)
    : rel_matches(rel_matches_),
      family_list(NULL),
      hits_truncated(false),
      real_hits(-1),
      range(-1, -1)
{
}

FamilyFinder::~FamilyFinder() {
    delete_family_list();
}

void FamilyFinder::delete_family_list() {
    delete family_list;
    family_list = NULL;
}

// ------------------------
//      PT_FamilyFinder

PT_FamilyFinder::PT_FamilyFinder(GBDATA *gb_main_, int server_id_, int oligo_len_, int mismatches_, bool fast_flag_, bool rel_matches_)
    : FamilyFinder(rel_matches_), 
      gb_main(gb_main_),
      server_id(server_id_),
      oligo_len(oligo_len_),
      mismatches(mismatches_),
      fast_flag(fast_flag_),
      link(NULL),
      com(0),
      locs(0)
{
    // 'mismatches'  = the number of allowed mismatches
    // 'fast_flag'   = 0 -> do complete search, 1 -> search only oligos starting with 'A'
    // 'rel_matches' = 0 -> score is number of oligo-hits, 1 -> score is relative to longer sequence (target or source) * 10
}

PT_FamilyFinder::~PT_FamilyFinder() {
    delete_family_list();
    close();
}

GB_ERROR PT_FamilyFinder::init_communication() {
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


GB_ERROR PT_FamilyFinder::open(const char *servername) {
    GB_ERROR error = 0;
    
    ff_assert(!com && !link);

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

    ff_assert(error || (com && link));
    
    return error;
}

void PT_FamilyFinder::close() {
    if (link) aisc_close(link);
    link = 0;
    com  = 0;
}

GB_ERROR PT_FamilyFinder::retrieve_family(const char *sequence, FF_complement compl_mode, int max_results) {
    delete_family_list();

    char     *compressed_sequence = GB_command_interpreter(gb_main, sequence, "|keep(acgtunACGTUN)", 0, 0);
    GB_ERROR  error               = NULL;

    if (!compressed_sequence) {
        error = GB_await_error();
    }
    else {
        bytestring bs;
        bs.data = compressed_sequence;
        bs.size = strlen(bs.data)+1;

        /* Start find_family() at the PT_SERVER
         *
         * Here we have to make a loop, until the match count of the
         * first member is big enough
         */

        if (aisc_put(link, PT_LOCS, locs,
                     LOCS_FF_PROBE_LEN,       long(oligo_len),        // oligo length (12 hardcoded till July 2008)
                     LOCS_FF_MISMATCH_NUMBER, long(mismatches),       // number of mismatches (0 hardcoded till July 2008)
                     LOCS_FF_FIND_TYPE,       long(fast_flag),   // 0: complete search, 1: quick search (only search oligos starting with 'A')
                     LOCS_FF_SORT_TYPE,       long(uses_rel_matches()), // 0: matches, 1: relative matches (0 hardcoded till July 2008)
                     LOCS_FF_SORT_MAX,        long(max_results),      // speed up family sorting (only sort retrieved results)
                     LOCS_FF_COMPLEMENT,      long(compl_mode),       // any combination of: 1 = forward, 2 = reverse, 4 = reverse-complement, 8 = complement (1 hardcoded in PT-Server till July 2008)
                     LOCS_RANGE_STARTPOS,     long(range.get_start()),
                     LOCS_RANGE_ENDPOS,       long(range.get_end()),
                     LOCS_FF_FIND_FAMILY,     &bs, // RPC (has to be last parameter!)
                     NULL))
        {
            error = "Communication error with PT server ('retrieve_family')";
        }
        else {
            // Read family list
            T_PT_FAMILYLIST f_list;
            aisc_get(link, PT_LOCS, locs,
                     LOCS_FF_FAMILY_LIST,      &f_list,
                     LOCS_FF_FAMILY_LIST_SIZE, &real_hits,
                     NULL);

            hits_truncated = false;
            if (max_results<1) max_results = INT_MAX;

            FamilyList *tail = NULL;
            while (f_list) {
                if (max_results == 0) {
                    hits_truncated = true;
                    break;
                }
                max_results--;

                FamilyList *fl = new FamilyList();

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
        }
        free(compressed_sequence);
    }
    return error;
}

GB_ERROR PT_FamilyFinder::searchFamily(const char *sequence, FF_complement compl_mode, int max_results) {
    // searches the PT-server for species related to 'sequence'.
    //
    // relation-score is calculated by fragmenting the sequence into oligos of length 'oligo_len' and
    // then summarizing the number of hits.
    //
    // 'max_results' limits the length of the generated result list (low scores deleted first)
    //               if < 1 -> don't limit

    GB_ERROR error = open(GBS_ptserver_tag(server_id));
    if (!error) {
        error = init_communication();
        if (!error) {
            error = retrieve_family(sequence, compl_mode, max_results);
        }
    }
    close();
    return error;
}

const char *PT_FamilyFinder::results2string() {
    GBS_strstruct *out = GBS_stropen(1000);
    for (FamilyList *fl = family_list; fl; fl = fl->next) {
        GBS_strnprintf(out, 100, "%s/%li/%3.5f,", fl->name, fl->matches, fl->rel_matches*100);
    }
    GBS_str_cut_tail(out, 1);
    RETURN_LOCAL_ALLOC(GBS_strclose(out));
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
    // used in several figs:
    // - ad_spec_nn.fig
    // - ad_spec_nnm.fig
    // - faligner/family_settings.fig

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

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

void TEST_SLOW_PT_FamilyFinder() {
    GB_shell shell;
    TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");
    
    GBDATA *gb_main = GB_open("no.arb", "c");

    for (int relativeMatches = 0; relativeMatches <= 1; ++relativeMatches) {
        for (int fastMode = 0; fastMode <= 1; ++fastMode) {
            for (int partial = 0; partial <= 1; ++partial) {
                PT_FamilyFinder ff(gb_main, TEST_SERVER_ID, partial ? 10 : 18, 1, fastMode, relativeMatches);

                // database: ../UNIT_TESTER/run/TEST_pt.arb

                const char *sequence;
                if (partial) {
                    ff.restrict_2_region(TargetRange(39, 91)); // alignment range of bases 30-69 of sequence of 'LgtLytic' 
                    sequence = "UCUAGCUUGCUAGACGGGUGGCGAG" "GGUAACCGUAGGGGA"; // bases 30-54 of sequence of 'LgtLytic' + 15 bases from 'DcdNodos' (outside region)
                }
                else {
                    // sequence of 'LgtLytic' in TEST_pt.arb:
                    sequence = "AGAGUUUGAUCAAGUCGAACGGCAGCACAGUCUAGCUUGCUAGACGGGUGGCGAGUGGCGAACGGACUUGGGGAAACUCAAGCUAAUACCGCAUAAUCAUGACUGGGGUGAAGUCGUAACAAGGUAGCCGUAGGGGAACCUGCGGCUGGAUCACCUCCUN";
                }

                TEST_ASSERT_NO_ERROR(ff.searchFamily(sequence, FF_FORWARD, 4));

                if (partial) {
                    // len: 40 bp (-15 bp)
                    // Ns: 0 bp
                    // oligolen: 10bp
                    // est. matches: len-oligolen+1-Ns = 40-15-10+1-0 = 16

                    if (fastMode == 0) { // full-search
                        TEST_ASSERT_EQUAL(ff.results2string(), "LgtLytic/19/59.37500,VblVulni/5/15.62500,VbhChole/4/12.50000,DcdNodos/4/12.50000");
                    }
                    else { // fast-search
                        TEST_ASSERT_EQUAL(ff.results2string(), "LgtLytic/3/9.37500,VblVulni/1/3.12500,FrhhPhil/1/3.12500,DcdNodos/1/3.12500");
                    }
                }
                else {
                    // len: 160 bp
                    // Ns: 1 bp
                    // oligolen: 18bp
                    // est. matches: len-oligolen+1-Ns = 160-18+1-1 = 142

                    if (fastMode == 0) { // full-search
                        TEST_ASSERT_EQUAL(ff.results2string(), "LgtLytic/142/98.61111,HllHalod/62/43.35664,AclPleur/59/40.97222,PtVVVulg/52/36.11111");
                    }
                    else { // fast-search
                        TEST_ASSERT_EQUAL(ff.results2string(), "LgtLytic/40/27.77778,HllHalod/18/12.58741,AclPleur/17/11.80556,VbhChole/15/10.41667");
                    }
                }
            }
        }
    }

    GB_close(gb_main);
}

#endif
