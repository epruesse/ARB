//  ==================================================================== // 
//                                                                       // 
//    File      : probe_match_parser.cxx                                 // 
//    Purpose   : parse the results of a probe match                     // 
//    Time-stamp: <Wed Jun/09/2004 09:59 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in June 2004             // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>

#include "arbdb.h"
#define pm_assert(cond) arb_assert(cond)

#include "probe_match_parser.hxx"

using namespace std;

// --------------- 
//      column     
// --------------- 

struct column {
    const char *title;          // column title (pointer into ProbeMatch_impl::headline)
    int         start_column, end_column;
    column     *next;

    column(const char *t, int sc, int ec)
        : title(t), start_column(sc), end_column(ec), next(0) {}
    ~column() { delete next; }

    column *findPred(const char *tit) {
        if (!next) return 0;
        if (strcmp(next->title, tit) == 0) return this;
        return next->findPred(tit);
    }

    column *toFront(const char *tit) {
        if (strcmp(tit, title) == 0) return this; // already is at front

        column *predecessor = findPred(tit);
        if (!predecessor) return 0;

        column *new_head = predecessor->next;
        pm_assert(new_head);

        predecessor->next = new_head->next; // remove from chain
        new_head->next    = this;
        return new_head;
    }
};

// ------------------------
//      ProbeMatch_impl
// ------------------------

class ProbeMatch_impl {
    char   *headline;
    column *head;

    int probe_region_offset;    // left index of probe region

public:
    ProbeMatch_impl(const char *headline_, char **errPtr)
        : head(0), probe_region_offset(-1)
    {
        pm_assert(headline_);
        headline = strdup(headline_);

        for (char *tok_start = strtok(headline, " "); tok_start; tok_start = strtok(0, " ")) {
            char *tok_end = strchr(tok_start, 0)-1;

            int startPos = tok_start-headline;
            int endPos   = tok_end-headline;

            while (tok_end >= tok_start && tok_end[0] == '-') --tok_end;
            while (tok_start <= tok_end && tok_start[0] == '-') ++tok_start;

            column *col = new column(tok_start, startPos-2, endPos-2); // -2 because headline is 2 shorter than other lines
            col->next = head;
            head      = col;
        }

        if (!head) *errPtr = strdup("No columns found");
    }

    ~ProbeMatch_impl() {
        free(headline);
        delete head;
    }

    column *findColumn(const char *columntitle) {
        if (head) {
            column *new_head = head->toFront(columntitle);
            if (new_head) {
                head = new_head;
                return new_head;
            }
        }
        return 0;
    }

    void set_probe_region_offset(int offset) { probe_region_offset = offset; }
    int get_probe_region_offset() const { return probe_region_offset; }
};

// -------------------------
//      ProbeMatchParser
// -------------------------

ProbeMatchParser::ProbeMatchParser(const char *probe_target, const char *headline)
    : pimpl(0), init_error(0)
{
    if (!headline) {
        init_error = strdup("No headline given");
    }
    else if (!probe_target) {
        init_error = strdup("No probe target given.");
    }
    else {
        pimpl = new ProbeMatch_impl(headline, &init_error);
        if (!init_error) {
            // modify target, so that it matches the target string in headline
            char *probe_target_copy = GBS_global_string_copy("'%s'", probe_target); // add single quotes
            for (int i = 0; probe_target_copy[i]; ++i) { 
                probe_target_copy[i] = toupper(probe_target_copy[i]);
                if (probe_target_copy[i] == 'T') { // replace 'T' by 'U'
                    probe_target_copy[i] = 'U';
                }
            }

            // find that column and 
            column *target_found = pimpl->findColumn(probe_target_copy);
            if (target_found) {
                int probe_region_offset = target_found->start_column - 9;
                pimpl->set_probe_region_offset(probe_region_offset);
            }
            else {
                init_error = GBS_global_string_copy("Could not find target '%s' in headline", probe_target_copy);
            }
            free(probe_target_copy);
        }
    }
}

ProbeMatchParser::~ProbeMatchParser() {
    free(init_error);
}

bool ProbeMatchParser::getColumnRange(const char *columnName, int *startCol, int *endCol) const {
    column *col = pimpl->findColumn(columnName);
    if (!col) return false;

    *startCol = col->start_column;
    *endCol   = col->end_column;
    return true;
}

bool ProbeMatchParser::is_gene_result() const {
    return pimpl->findColumn("organism") && pimpl->findColumn("gene");
}

// -------------------------
//      ParsedProbeMatch
// -------------------------

ParsedProbeMatch::ParsedProbeMatch(const char *match_, const ProbeMatchParser& parser_)
    : parser(parser_), match(0), error(0)
{
    if (match_) match = strdup(match_);
    else error = "No match given";
}

ParsedProbeMatch::~ParsedProbeMatch() {
    free(match);
}

inline char *strpartdup(const char *str, int c1, int c2) {
    int len = c2-c1+1;

    pm_assert(str);
    pm_assert(c1 <= c2);
    pm_assert((int)strlen(str) > c2);

    char *buffer = (char*)malloc(len+1);
    memcpy(buffer, str+c1, len);
    buffer[len]  = 0;
    return buffer;
}

int ParsedProbeMatch::get_position() const {
    int c1, c2;
    if (parser.getColumnRange("pos", &c1, &c2)) {
        char *content = strpartdup(match, c1, c2);
        int   pos     = atoi(content);
        free(content);
        return pos;
    }
    error = "no such column: 'pos'";
    return -1;
}

const char *ParsedProbeMatch::get_probe_region() const {
    int pro      = parser.pimpl->get_probe_region_offset();
    int matchlen = strlen(match);

    if (pro<matchlen) {
        return match+pro;
    }

    error = GBS_global_string("can't parse match info '%s'", match);
    return 0;
}

char *ParsedProbeMatch::get_column_content(const char *columnName) const {
    int sc, ec;
    if (parser.getColumnRange(columnName, &sc, &ec)) {
        return strpartdup(match, sc, ec);
    }
    return 0;
}
