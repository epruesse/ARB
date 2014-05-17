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

struct PT_FF_comImpl {
    aisc_com  *link;
    T_PT_MAIN  com;
    T_PT_LOCS  locs;

    PT_FF_comImpl()
        : link(NULL)
    {
        ff_assert(!com.exists());
        ff_assert(!locs.exists());
    }
};

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

FamilyFinder::FamilyFinder(bool rel_matches_, RelativeScoreScaling scaling_)
    : rel_matches(rel_matches_),
      scaling(scaling_), 
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

PT_FamilyFinder::PT_FamilyFinder(GBDATA *gb_main_, int server_id_, int oligo_len_, int mismatches_, bool fast_flag_, bool rel_matches_, RelativeScoreScaling scaling_)
    : FamilyFinder(rel_matches_, scaling_), 
      gb_main(gb_main_),
      server_id(server_id_),
      oligo_len(oligo_len_),
      mismatches(mismatches_),
      fast_flag(fast_flag_)
{
    // 'mismatches'  = the number of allowed mismatches
    // 'fast_flag'   = 0 -> do complete search, 1 -> search only oligos starting with 'A'
    // 'rel_matches' = 0 -> score is number of oligo-hits, 1 -> score is relative to longer sequence (target or source) * 10

    ci = new PT_FF_comImpl;
}

PT_FamilyFinder::~PT_FamilyFinder() {
    delete_family_list();
    close();
    delete ci;
}

GB_ERROR PT_FamilyFinder::init_communication() {
    const char *user = "PT_FamilyFinder";

    ff_assert(!ci->locs.exists());
    
    // connect PT server
    if (aisc_create(ci->link, PT_MAIN, ci->com,
                    MAIN_LOCS, PT_LOCS, ci->locs,
                    LOCS_USER, user,
                    NULL)) {
        return GB_export_error("Cannot initialize communication");
    }
    return 0;
}


GB_ERROR PT_FamilyFinder::open(const char *servername) {
    GB_ERROR error = 0;
    
    ff_assert(!ci->com.exists() && !ci->link);

    if (arb_look_and_start_server(AISC_MAGIC_NUMBER, servername)) {
        error = "Cannot contact PT  server";
    }
    else {
        const char *socketid = GBS_read_arb_tcp(servername);
        if (!socketid) error = GB_await_error();
        else {
            ci->link = aisc_open(socketid, ci->com, AISC_MAGIC_NUMBER, &error);
            if (!error) {
                if (!ci->link) error                 = "Cannot contact PT server [1]";
                else if (init_communication()) error = "Cannot contact PT server [2]";
            }
        }
    }

    ff_assert(error || (ci->com.exists() && ci->link));
    
    return error;
}

void PT_FamilyFinder::close() {
    if (ci->link) aisc_close(ci->link, ci->com);
    ci->link = 0;
    ff_assert(!ci->com.exists());
    ci->locs.clear();
}

GB_ERROR PT_FamilyFinder::retrieve_family(const char *sequence, FF_complement compl_mode, int max_results, double min_score) {
    delete_family_list();

    char     *compressed_sequence = GB_command_interpreter(gb_main, sequence, "|keep(acgtunACGTUN)", 0, 0);
    GB_ERROR  error               = NULL;

    if      (!compressed_sequence)    error = GB_await_error();
    else if (!compressed_sequence[0]) error = "No data in sequence(-region)";
    else {
        bytestring bs;
        bs.data = compressed_sequence;
        bs.size = strlen(bs.data)+1;

        /* Start find_family() at the PT_SERVER
         *
         * Here we have to make a loop, until the match count of the
         * first member is big enough
         */

        ff_assert(!range.is_empty());
         
        // create and init family finder object
        T_PT_FAMILYFINDER ffinder;
        if (aisc_create(ci->link, PT_LOCS, ci->locs,
                        LOCS_FFINDER, PT_FAMILYFINDER, ffinder,
                        FAMILYFINDER_PROBE_LEN,       long(oligo_len),          // oligo length (12 hardcoded till July 2008)
                        FAMILYFINDER_MISMATCH_NUMBER, long(mismatches),         // number of mismatches (0 hardcoded till July 2008)
                        FAMILYFINDER_FIND_TYPE,       long(fast_flag),          // 0: complete search, 1: quick search (only search oligos starting with 'A')
                        FAMILYFINDER_SORT_TYPE,       long(uses_rel_matches()), // 0: matches, 1: relative matches (0 hardcoded till July 2008)
                        FAMILYFINDER_REL_SCORING,     long(get_scaling()),      // scaling of relative scores
                        FAMILYFINDER_SORT_MAX,        long(max_results),        // speed up family sorting (only sort retrieved results)
                        FAMILYFINDER_MIN_SCORE,       min_score,                // limit hits by score
                        FAMILYFINDER_COMPLEMENT,      long(compl_mode),         // any combination of: 1 = forward, 2 = reverse, 4 = reverse-complement, 8 = complement (1 hardcoded in PT-Server till July 2008)
                        FAMILYFINDER_RANGE_STARTPOS,  long(range.start()),
                        FAMILYFINDER_RANGE_ENDPOS,    long(range.is_limited() ? range.end() : -1),
                        FAMILYFINDER_FIND_FAMILY,     &bs,                      // RPC (has to be last parameter!)
                        NULL))
        {
            error = "Communication error with PT server ('retrieve_family')";
        }
        else {
            char *ff_error = 0;

            // Read family list
            T_PT_FAMILYLIST f_list;
            aisc_get(ci->link, PT_FAMILYFINDER, ffinder,
                     FAMILYFINDER_FAMILY_LIST,      f_list.as_result_param(),
                     FAMILYFINDER_FAMILY_LIST_SIZE, &real_hits,
                     FAMILYFINDER_ERROR,            &ff_error,
                     NULL);

            if (ff_error[0]) {
                error = GBS_global_string("PTSERVER: %s", ff_error);
            }
            else {
                hits_truncated = false;
                if (max_results<1) max_results = INT_MAX;

                FamilyList *tail = NULL;
                while (f_list.exists()) {
                    if (max_results == 0) {
                        hits_truncated = true;
                        break;
                    }
                    max_results--;

                    FamilyList *fl = new FamilyList;

                    (tail ? tail->next : family_list) = fl;
                    tail                              = fl;
                    fl->next                          = NULL;

                    aisc_get(ci->link, PT_FAMILYLIST, f_list,
                             FAMILYLIST_NAME,        &fl->name,
                             FAMILYLIST_MATCHES,     &fl->matches,
                             FAMILYLIST_REL_MATCHES, &fl->rel_matches,
                             FAMILYLIST_NEXT,        f_list.as_result_param(),
                             NULL);
                }
            }
            free(ff_error);
        }

        free(compressed_sequence);
    }
    return error;
}

GB_ERROR PT_FamilyFinder::searchFamily(const char *sequence, FF_complement compl_mode, int max_results, double min_score) {
    // searches the PT-server for species related to 'sequence'.
    //
    // relation-score is calculated by fragmenting the sequence into oligos of length 'oligo_len' and
    // then summarizing the number of hits.
    //
    // 'max_results' limits the length of the generated result list (low scores deleted first)
    //               if < 1 -> don't limit
    //
    // 'min_score' limits the results by score (use 0 for unlimited results)
    //
    // When using restrict_2_region(), only pass the corresponding part via 'sequence' (not the full alignment)

    GB_ERROR error = range.is_empty() ? "Specified range is empty" : NULL;
    if (!error) {
        error             = open(GBS_ptserver_tag(server_id));
        if (!error) error = retrieve_family(sequence, compl_mode, max_results, min_score);
        close();
    }
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
        aw_root->awar_int(AWAR_NN_REL_SCALING, RSS_BOTH_MIN);

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
    aws->create_option_menu(AWAR_NN_FAST_MODE, true);
    aws->insert_default_option("Complete", "", 0);
    aws->insert_option("Quick", "", 1);
    aws->update_option_menu();

    aws->at("score");
    aws->create_option_menu(AWAR_NN_REL_MATCHES, true);
    aws->insert_option("absolute", "", 0);
    aws->insert_default_option("relative", "", 1);
    aws->update_option_menu();

    aws->at("scaling");
    aws->create_option_menu(AWAR_NN_REL_SCALING, true);
    aws->insert_option        ("to source POC",  "", RSS_SOURCE);
    aws->insert_option        ("to target POC",  "", RSS_TARGET);
    aws->insert_default_option("to maximum POC", "", RSS_BOTH_MAX);
    aws->insert_option        ("to minimum POC", "", RSS_BOTH_MIN);
    aws->update_option_menu();
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#include <test_unit.h>

class ff_tester {
    GBDATA *gb_main;
public:
    bool    relativeMatches;
    bool    fastMode;
    bool    partial;
    bool    shortOligo;
    double  min_score;

    RelativeScoreScaling scaling;

    ff_tester(GBDATA *gb_main_)
        : gb_main(gb_main_),
          relativeMatches(false),
          fastMode(false),
          partial(false),
          shortOligo(false),
          min_score(0.0),
          scaling(RSS_BOTH_MAX)
    {}

    const char *get_result(GB_ERROR& error) {
        int             oligoLen = shortOligo ? (partial ? 3 : 6) : (partial ? 10 : 18);
        PT_FamilyFinder ff(gb_main, TEST_SERVER_ID, oligoLen, 1, fastMode, relativeMatches, scaling);
        
        const char *sequence;
        if (partial) {
            ff.restrict_2_region(PosRange(39, 91)); // alignment range of bases 30-69 of sequence of 'LgtLytic'
            sequence = "UCUAGCUUGCUAGACGGGUGGCGAG" "GGUAACCGUAGGGGA"; // bases 30-54 of sequence of 'LgtLytic' + 15 bases from 'DcdNodos' (outside region)
        }
        else {
            // sequence of 'LgtLytic' in TEST_pt.arb:
            sequence = "AGAGUUUGAUCAAGUCGAACGGCAGCACAGUCUAGCUUGCUAGACGGGUGGCGAGUGGCGAACGGACUUGGGGAAACUCAAGCUAAUACCGCAUAAUCAUGACUGGGGUGAAGUCGUAACAAGGUAGCCGUAGGGGAACCUGCGGCUGGAUCACCUCCUN";
        }

        error = ff.searchFamily(sequence, FF_FORWARD, 4, min_score);
        return ff.results2string();
    }
};


#define TEST_RELATIVES_COMMON(tester,expctd) \
        GB_ERROR    error;                                      \
        const char *result   = tester.get_result(error);        \
        const char *expected = expctd;                          \
        TEST_EXPECT_NO_ERROR(error);                            \
    
#define TEST_EXPECT_RELATIVES(tester,expctd) do {               \
        TEST_RELATIVES_COMMON(tester,expctd);                   \
        TEST_EXPECT_EQUAL(result, expected);                    \
    } while(0)

#define TEST_EXPECT_REL__BROK(tester,expctd) do {               \
        TEST_RELATIVES_COMMON(tester,expctd);                   \
        TEST_EXPECT_EQUAL__BROKEN(result, expected);            \
    } while(0)
    
void TEST_SLOW_PT_FamilyFinder() {
    GB_shell shell;
    TEST_SETUP_GLOBAL_ENVIRONMENT("ptserver");

    GBDATA *gb_main = GB_open("no.arb", "c");

    // check some error cases
    {
        PT_FamilyFinder ffe(gb_main, TEST_SERVER_ID, 0, 1, 0, 0, RSS_BOTH_MAX);
        TEST_EXPECT_CONTAINS(ffe.searchFamily("whatever", FF_FORWARD, 4, 0.0), "minimum oligo length is 1");
    }
    
    ff_tester test(gb_main);

    ff_tester ______RESET = test; TEST_EXPECT_RELATIVES(test, "LgtLytic/142/97.93103,HllHalod/62/43.05556,AclPleur/59/38.06452,PtVVVulg/51/34.00000");
    test.partial          = true; TEST_EXPECT_RELATIVES(test, "LgtLytic/18/11.76471,VblVulni/5/3.24675,VbhChole/4/2.59740,DcdNodos/4/2.59740");
    test.shortOligo       = true; TEST_EXPECT_RELATIVES(test, "PtVVVulg/38/23.03030,AclPleur/38/22.35294,VbhChole/38/23.60248,VblVulni/38/23.60248");
    test.relativeMatches  = true; TEST_EXPECT_RELATIVES(test, "DsssDesu/38/38.77551,CltBotul/38/34.23423,PsAAAA00/38/32.75862,Bl0LLL00/38/25.67568");
    test.min_score        = 32.6; TEST_EXPECT_RELATIVES(test, "DsssDesu/38/38.77551,CltBotul/38/34.23423,PsAAAA00/38/32.75862");
    test                  = ______RESET;
    test.shortOligo       = true; TEST_EXPECT_RELATIVES(test, "LgtLytic/154/98.08917,VbhChole/133/84.17722,VblVulni/133/84.17722,HllHalod/133/85.25641");
    test.relativeMatches  = true; TEST_EXPECT_RELATIVES(test, "LgtLytic/154/98.08917,HllHalod/133/85.25641,VbhChole/133/84.17722,VblVulni/133/84.17722");
    test.fastMode         = true; TEST_EXPECT_RELATIVES(test, "LgtLytic/42/26.75159,VblVulni/37/23.41772,HllHalod/36/23.07692,Stsssola/36/23.07692");
    test.min_score        = 26.7; TEST_EXPECT_RELATIVES(test, "LgtLytic/42/26.75159");
    test.min_score        = 26.8; TEST_EXPECT_RELATIVES(test, "");
    test                  = ______RESET;
    test.fastMode         = true; TEST_EXPECT_RELATIVES(test, "LgtLytic/40/27.58621,HllHalod/18/12.50000,AclPleur/17/10.96774,PtVVVulg/15/10.00000");
    test.min_score        = 17.0; TEST_EXPECT_RELATIVES(test, "LgtLytic/40/27.58621,HllHalod/18/12.50000,AclPleur/17/10.96774");
    test.min_score        = 17.5; TEST_EXPECT_RELATIVES(test, "LgtLytic/40/27.58621,HllHalod/18/12.50000");
    test                  = ______RESET;

    test.shortOligo      = true;
    test.relativeMatches = true;
    test.scaling         = RSS_BOTH_MAX; TEST_EXPECT_RELATIVES(test, "LgtLytic/154/98.08917,HllHalod/133/85.25641,VbhChole/133/84.17722,VblVulni/133/84.17722");
    test.scaling         = RSS_BOTH_MIN; TEST_EXPECT_RELATIVES(test, "LgtLytic/154/98.71795,DsssDesu/84/88.42105,CltBotul/95/87.96296,PsAAAA00/97/85.84071");
    test.scaling         = RSS_TARGET;   TEST_EXPECT_RELATIVES(test, "LgtLytic/154/98.08917,DsssDesu/84/88.42105,CltBotul/95/87.96296,PsAAAA00/97/85.84071");
    test.scaling         = RSS_SOURCE;   TEST_EXPECT_RELATIVES(test, "LgtLytic/154/98.71795,VbhChole/133/85.25641,VblVulni/133/85.25641,HllHalod/133/85.25641");
    test.partial         = true;
    test.shortOligo      = false;
    test.scaling         = RSS_BOTH_MAX; TEST_EXPECT_RELATIVES(test, "LgtLytic/18/11.76471,VblVulni/5/3.24675,VbhChole/4/2.59740,DcdNodos/4/2.59740");
    test.scaling         = RSS_BOTH_MIN; TEST_EXPECT_RELATIVES(test, "LgtLytic/18/56.25000,VblVulni/5/15.62500,VbhChole/4/12.50000,DcdNodos/4/12.50000");
    test.scaling         = RSS_TARGET;   TEST_EXPECT_RELATIVES(test, "LgtLytic/18/11.76471,VblVulni/5/3.24675,VbhChole/4/2.59740,DcdNodos/4/2.59740");
    test.scaling         = RSS_SOURCE;   TEST_EXPECT_RELATIVES(test, "LgtLytic/18/56.25000,VblVulni/5/15.62500,VbhChole/4/12.50000,DcdNodos/4/12.50000");
    test                 = ______RESET;

    GB_close(gb_main);
}

#endif // UNIT_TESTS

