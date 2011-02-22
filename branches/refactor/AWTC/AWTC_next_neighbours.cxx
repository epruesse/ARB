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

#include <arbdbt.h>
#include <servercntrl.h>
#include <PT_com.h>
#include <client.h>
#include <aw_window.hxx>
#include <aw_root.hxx>

#include <climits>

#define awtc_assert(bed) arb_assert(bed)

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
    awtc_assert(next == NULL);                      // only insert unlinked instace!

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
    awtc_assert(next == NULL);                      // only insert unlinked instace!

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
      real_hits(-1)
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
    
    awtc_assert(!com && !link);

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

    awtc_assert(error || (com && link));
    
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
                     LOCS_FF_FIND_FAMILY,     &bs,
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

void PT_FamilyFinder::print() {
    FamilyList *fl;
    for (fl = family_list; fl;  fl = fl->next) {
        printf("%s %li\n", fl->name, fl->matches);
    }
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

#if (UNIT_TESTS == 1)

#include <test_unit.h>

void TEST_SLOW_PT_FamilyFinder() {
    GB_shell shell;
    TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");
    
    GBDATA *gb_main = GB_open("no.arb", "c");

    for (int relativeMatches = 0; relativeMatches <= 1; ++relativeMatches) {
        for (int fastMode = 0; fastMode <= 1; ++fastMode) {
            PT_FamilyFinder ff(gb_main, TEST_SERVER_ID, 18, 1, fastMode, relativeMatches);
            
            // sequence of 'BcSSSS00' in TEST_pt.arb:
            const char *sequence = "UUUAUCGGAGAGUUUGAUCAAGUCGAGCGGACAGAUGGGAGCUUGCUCCCUGAUGUUAGCGGCGGACGGACUCCGGGAAACCGGGGCUAAUACCGGAUGGUGAUGAUUGGGGUGAAGUCGUAACAAGGUAGCCGUAUCGGAAGGUGCGGCUGGAUCACCUCCUUUCU";

            TEST_ASSERT_NO_ERROR(ff.searchFamily(sequence, FF_FORWARD, 4));

#define TEST_ASSERT_NEXT_RELATIVE(NAME, MATCHES, REL_MATCHES)           \
            do {                                                        \
                TEST_ASSERT(fm);                                        \
                TEST_ASSERT((arb_test::test_equal(fm->name, NAME) +     \
                             arb_test::test_equal(fm->matches, MATCHES) + \
                             arb_test::test_equal(fm->rel_matches, REL_MATCHES)) == 3); \
                fm = fm->next;                                          \
            } while(0)

            const FamilyList *fm = ff.getFamilyList();
            TEST_ASSERT(fm);

            if (fastMode == 0) { // full-search
                TEST_ASSERT_NEXT_RELATIVE("BcSSSS00", 150, 0.993377);
                TEST_ASSERT_NEXT_RELATIVE("Bl0LLL00",  65, 0.488722);

                if (relativeMatches == 0) {
                    TEST_ASSERT_NEXT_RELATIVE("ClfPerfr",  24, 0.160000);
                    TEST_ASSERT_NEXT_RELATIVE("Stsssola",  23, 0.161972);
                    TEST_ASSERT(!fm); // checked all?
                }
                else {
                    TEST_ASSERT_NEXT_RELATIVE("Stsssola",  23, 0.161972);
                    TEST_ASSERT_NEXT_RELATIVE("ClfPerfr",  24, 0.160000);
                    TEST_ASSERT(!fm); // checked all?
                }
            }
            else { // fast-search 
                TEST_ASSERT_NEXT_RELATIVE("BcSSSS00", 34, 0.225166);
                TEST_ASSERT_NEXT_RELATIVE("Bl0LLL00", 18, 0.135338);

                if (relativeMatches == 0) {
                    TEST_ASSERT_NEXT_RELATIVE("DcdNodos",  6, 0.041096);
                    TEST_ASSERT_NEXT_RELATIVE("LgtLytic",  6, 0.041379);
                    TEST_ASSERT(!fm); // checked all?
                }
                else {
                    TEST_ASSERT_NEXT_RELATIVE("PslFlave",  6, 0.043478);
                    TEST_ASSERT_NEXT_RELATIVE("HllHalod",  6, 0.041958);
                    TEST_ASSERT(!fm); // checked all? 
                }
            }
        }
    }

    GB_close(gb_main);
}

#endif
