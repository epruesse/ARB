#include <arbdb.h>
#include <PT_com.h>
#include <client.h>

#include <iostream>

using std::cerr;
using std::endl;

void help() {
    cerr <<
        "Executes a probe match on a running PT server\n"
        "\n"
        "Parameters:\n"
        "\n"
        " --db <FILE>             ARB file\n"
        " --port <PORT>           port of PT server\n"
        " --sequence <SEQUENCE>   probe\n"
        " --reversed              reverse probe before matching\n"
        " --complement            complement probe before matching\n"
        " --max-hits <N>           maximum number of hits on db (1,000,000)\n"
        " --weighted              use weighted matching\n"
        " --weighted-pos          use weighted matching with pos&strength\n"
        " --n-matches <N>         consider N occurances of 'N' as match (2)\n"
        " --n-match-bound <N>     abover N occurancse of 'N', consider all as mismatch (5)\n"
        " --mismatches <N>        allowed mismatches (0)\n"

         << endl;
}

enum SORT_BY_TYPE {
    SORT_BY_MISMATCHES = 0,
    SORT_BY_WEIGHTED_MISMATCHES = 1,
    SORT_BY_WEIGHTED_MISMATCHES_W_POS_AND_STRENGTH = 2
};

int ARB_main(int argc, char ** argv) {
    const char* port = 0;
    const char* sequence = 0;
    int reversed = 0;
    int complement = 0;
    int max_hits = 1000000;
    SORT_BY_TYPE sort_by = SORT_BY_MISMATCHES;
    int n_accept = 2;
    int n_limit = 5;
    int max_mismatches = 0;

    GB_shell shell;

    if (argc<2) {
        help();
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        int err = 0;
        if (!strcmp(argv[i], "--help")) {
            help();
            return 0;
        } else if (!strcmp(argv[i], "--reversed")) {
            reversed = 1;
        } else if (!strcmp(argv[i], "--complement")) {
            complement = 1;
        } else if (!strcmp(argv[i], "--weighted")) {
            sort_by = SORT_BY_WEIGHTED_MISMATCHES;
        } else if (!strcmp(argv[i], "--weighted-pos")) {
            sort_by = SORT_BY_WEIGHTED_MISMATCHES_W_POS_AND_STRENGTH;
        } else if (argc > i+1) {
            if (!strcmp(argv[i], "--port")) {
                port = argv[++i];
            } else if (!strcmp(argv[i], "--sequence")) {
                sequence = argv[++i];
            } else if (!strcmp(argv[i], "--max-hits")) {
                max_hits = atoi(argv[++i]);
            } else if (!strcmp(argv[i], "--n-matches")) {
                n_accept = atoi(argv[++i]);
            } else if (!strcmp(argv[i], "--n-match-bound")) {
                n_limit = atoi(argv[++i]);
            } else if (!strcmp(argv[i], "--mismatches")) {
                max_mismatches = atoi(argv[++i]);
            } else {
                err = 1;
            }
        } else {
            err = 1;
        }
        if (err) {
            cerr << "Error: Did not understand argument '" << argv[i]
                 << "'." << endl << endl;
            help();
            return 1;
        }
    }

    bool err = false;

    if (!port) {
        cerr << "need '--port' parameter" << endl;
        err = true;
    }
    if (!sequence) {
        cerr << "need '--sequence' parameter" << endl;
        err = true;
    }
    if (max_mismatches < 0 || max_mismatches > 10) {
        cerr << "mismatches must be between 0 and 10" << endl;
        err = true;
    }
    if (err) {
        help();
        return 1;
    }

    T_PT_LOCS  locs;
    T_PT_MAIN  com;
    GB_ERROR   error = NULL;
    aisc_com  *link  = aisc_open(port, com, AISC_MAGIC_NUMBER, &error);
    if (!link) {
        cerr << "Cannot contact PT server (" << error << ')' << endl;
        return 1;
    }

    if (aisc_create(link, PT_MAIN, com,
                    MAIN_LOCS, PT_LOCS, locs,
                    NULL)) {
        cerr << "Cannot create PT LOCS structure" << endl;
        return 1;
    }

    if (aisc_nput(link, PT_LOCS, locs,
                  LOCS_MATCH_REVERSED,       (long)reversed,
                  LOCS_MATCH_COMPLEMENT,     (long)complement,
                  LOCS_MATCH_MAX_HITS,       (long)max_hits,
                  LOCS_MATCH_SORT_BY,        (long)sort_by,
                  LOCS_MATCH_N_ACCEPT,       (long)n_accept,
                  LOCS_MATCH_N_LIMIT,        (long)n_limit,
                  LOCS_MATCH_MAX_MISMATCHES, (long)max_mismatches,
                  LOCS_SEARCHMATCH,          sequence,
                  NULL)) {
        cerr << "Connection to PT server lost" << endl;
        return 1;
    }

    T_PT_MATCHLIST match_list;
    long match_list_cnt = 0;
    long matches_truncated = 0;
    bytestring bs;
    bs.data = 0;
    char *locs_error = 0;
    aisc_get(link, PT_LOCS, locs,
             LOCS_MATCH_LIST,        match_list.as_result_param(),
             LOCS_MATCH_LIST_CNT,    &match_list_cnt,
             LOCS_MATCH_STRING,      &bs,
             LOCS_MATCHES_TRUNCATED, &matches_truncated,
             LOCS_ERROR,             &locs_error,
             NULL);
    if (locs_error) {
        cerr << "received error: " << locs_error << endl;
        free(locs_error);
    }

    if (matches_truncated) {
        std::cout << "match list was truncated!" << endl;
    }

    std::cout << "acc     \t"
              << "start\t"
              << "stop\t"
              << "pos\t"
              << "mis\t"
              << "wmis\t"
              << "nmis\t"
              << "dt\t"
              << "rev\t"
              << "seq"
              << std::endl;

    while (match_list.exists()) {
        char *m_acc, *m_sequence;
        long m_start, m_stop, m_pos, m_mismatches, m_n_mismatches;
        long m_reversed;
        double m_wmismatches, m_dt;

        aisc_get(link, PT_MATCHLIST, match_list,
                 MATCHLIST_FIELD_ACC,     &m_acc,
                 MATCHLIST_FIELD_START,   &m_start,
                 MATCHLIST_FIELD_STOP,    &m_stop,
                 MATCHLIST_POS,           &m_pos,
                 MATCHLIST_MISMATCHES,    &m_mismatches,
                 MATCHLIST_WMISMATCHES,   &m_wmismatches,
                 MATCHLIST_N_MISMATCHES,  &m_n_mismatches,
                 MATCHLIST_DT,            &m_dt,
                 MATCHLIST_OVERLAY,       &m_sequence,
                 MATCHLIST_REVERSED,      &m_reversed,
                 MATCHLIST_NEXT,          match_list.as_result_param(),
                 NULL);

        std::cout << m_acc << "\t"
                  << m_start << "\t"
                  << m_stop << "\t"
                  << m_pos << "\t"
                  << m_mismatches << "\t"
                  << m_wmismatches << "\t"
                  << m_n_mismatches << "\t"
                  << m_dt << "\t"
                  << m_reversed << "\t"
                  << m_sequence << "\t"
                  << std::endl;
        fflush(stdout);

        free(m_acc);
        free(m_sequence);
    }

    free(bs.data);
    aisc_close(link, com);
    return 0;
}


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
