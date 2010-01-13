//  ==================================================================== // 
//                                                                       // 
//    File      : probe_match_parser.hxx                                 // 
//    Purpose   : parse the results of a probe match                     // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in June 2004             // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

#ifndef PROBE_MATCH_PARSER_HXX
#define PROBE_MATCH_PARSER_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

// --------------------------------------------------------------------------------
// helper class to parse probe match results

class ProbeMatch_impl;

class ProbeMatchParser : Noncopyable {
    ProbeMatch_impl *pimpl;
    char            *init_error;

public:
    ProbeMatchParser(const char *probe_target, const char *headline);
    ~ProbeMatchParser();

    const char * get_error() const { return init_error; }
    bool is_gene_result() const;
    int get_probe_region_offset() const;

    bool getColumnRange(const char *columnName, int *startCol, int *endCol) const;

    friend class ParsedProbeMatch;
};

class ParsedProbeMatch : Noncopyable {
    const ProbeMatchParser&  parser;
    char                    *match;
    mutable const char      *error;
public:

    ParsedProbeMatch(const char *match_, const ProbeMatchParser& parser_);
    ~ParsedProbeMatch();

    const char *get_error() const { return error; }
    int get_position() const;
    const char *get_probe_region() const;

    char *get_column_content(const char *columnName, bool chop_spaces) const;
};


#else
#error probe_match_parser.hxx included twice
#endif // PROBE_MATCH_PARSER_HXX

