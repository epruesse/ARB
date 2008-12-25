//  ==================================================================== //
//                                                                       //
//    File      : tree_merge.cxx                                         //
//    Purpose   : merges multiple trees saved by arb_probe_group_design  //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include <deque>
#include <set>
#include <map>
#include <string>
#include <fstream>
#include <list>

#include <stdio.h>
#include <arb_assert.h>
#define tm_assert(cond) arb_assert(cond)

#include <arbdb.h>
#include <arbdbt.h>
#include <smartptr.h>

#define NEED_decodeTreeNode
#define NEED_encodeTreeNode
#include "../common.h"

using namespace std;

// --------------------------------------------------------------------------------
// types and defs

typedef SmartPtr<ifstream> ifstreamPtr;
typedef map<string, string> TokMap;

#define MAXLINELENGTH 4096

// --------------------------------------------------------------------------------
// globals

static deque<string> inputtrees;
static string         outputtree;

static void helpArguments(){
    fprintf(stderr,
            "Usage: pgd_tree_merge <treefile_1> .. <treefile_n> <output_treefile>\n"
            "Merges multiple trees into one treefile.\n"
            "\n"
            );
}
static GB_ERROR scanArguments(int argc,char *argv[]){
    GB_ERROR error = 0;

    if (argc < 3) {
        helpArguments();
        error = "Need at least 1 input- and 1 output-tree.";
    }
    else {
        for (int c = 1; c < argc; ++c) {
            inputtrees.push_back(argv[c]);
        }
        outputtree = inputtrees.back();
        inputtrees.pop_back();
    }

    return error;
}

static string extractNodeInfos(const string& line, deque<string>& extract, GB_ERROR& error) {
    string result = line;
    size_t start  = 0;

    for (size_t open = result.find('\'', start);
         open != string::npos && !error;
         open  = result.find('\'', start))
    {
        size_t close = result.find('\'', open+1);
        if (close != string::npos) { // found '...'
            char *decoded = decodeTreeNode(result.substr(open+1, close-open-1).c_str(), error);
            if (!error) {
                extract.push_back(decoded);
                free(decoded);
                result.replace(open, close-open+1, "%s");
            }
        }
        else {
            error = "Unmatched \'";
        }
    }

    return result;
}

static GB_ERROR splitNodeInfoTokens(const string& s, TokMap& tokens) {
    size_t start = s.empty() ? string::npos : 0;
    while (start != string::npos) {
        size_t equal = s.find('=', start);
        if (equal == string::npos) {
            return GBS_global_string("'=' expected in '%s'", s.c_str());
        }

        size_t komma = s.find(',', equal+1);

        string tok = s.substr(start, equal-start);
        string value;

        if (komma == string::npos) {
            value = s.substr(equal+1);
            start = string::npos;
        }
        else {
            value = s.substr(equal+1, komma-equal-1);
            start = komma+1;
        }

        tokens[tok] = value;
    }
    return 0;
}


static bool testEqualIfExists(const string& tok, const TokMap& tm1, const TokMap& tm2, GB_ERROR& error) {
    // if no error:  returns 'true' if token exists

    bool exists = false;
    if (!error) {
        TokMap::const_iterator found1 = tm1.find(tok);
        TokMap::const_iterator found2 = tm2.find(tok);

        if (found1 == tm1.end() || found2 == tm2.end()) { // one NOT found
            if (found1 != tm1.end() || found2 != tm2.end()) { // and one found
                error = GBS_global_string("token '%s' has to exist in ALL or NONE of the trees", tok.c_str());
            }
        }
        else {
            if (found1->second != found2->second) {
                error = GBS_global_string("Token '%s' has to be equal in all trees ('%s' != '%s')",
                                          tok.c_str(), found1->second.c_str(), found2->second.c_str());
            }
            exists = true;
        }
    }

    return exists;
}

static void testExistAndEqual(const string& tok, const TokMap& tm1, const TokMap& tm2, GB_ERROR& error) {
    if (!error) {
        TokMap::const_iterator found1 = tm1.find(tok);
        TokMap::const_iterator found2 = tm2.find(tok);

        if (found1 == tm1.end() || found2 == tm2.end()) {
            error = GBS_global_string("expected token '%s' in node info", tok.c_str());
        }
        else {
            if (found1->second != found2->second) {
                error = GBS_global_string("Token '%s' has to be equal in all trees ('%s' != '%s')",
                                          tok.c_str(), found1->second.c_str(), found2->second.c_str());
            }
        }
    }
}

static void eraseTok(const string& tok, TokMap& tokens) {
    TokMap::iterator found = tokens.find(tok);
    if (found != tokens.end()) {
        tokens.erase(found);
    }
}

static string mergeNodeInfos(deque<string>& infos, GB_ERROR& error) {
    TokMap tok1;

    error = splitNodeInfoTokens(infos.front(), tok1);
    infos.pop_front();

    for (deque<string>::iterator i = infos.begin(); i != infos.end() && !error; ++i) {
        TokMap tok2;
        error = splitNodeInfoTokens(*i, tok2);

        if (!error) {
            bool is_leaf = false;

            // some tokens have to exist and must be equal!
            if (testEqualIfExists("n", tok1, tok2, error)) { // name
                testExistAndEqual("f", tok1, tok2, error); // full_name
                testExistAndEqual("a", tok1, tok2, error); // acc (aka accession number)
                if (!error) {
                    is_leaf = true;
//                     fprintf(stderr, "merging leaf '%s'\n", tok1.find("f")->second.c_str());
                }
            }

            // some tokens have to be equal IF they exist
            testEqualIfExists("g", tok1, tok2, error); // group name

            if (!error) {
                TokMap::iterator em1        = tok1.find("em");
                TokMap::iterator em2        = tok2.find("em");
                bool             have_exact = true;

                if (em1 == tok1.end()) {
                    if (em2 == tok2.end()) {
                        // we do NOT have exact probes
                        have_exact = false;
                    }
                    else {
                        // only in 2nd tree -> copy to first tree
                        tok1["em"] = em2->second;
                    }
                }
                else {
                    if (em2 != tok2.end()) { // exact matches in both trees -> sum up
                        int m1 = atoi(em1->second.c_str());
                        int m2 = atoi(em2->second.c_str());
                        int em = m1+m2;

//                         if (!is_leaf) {
//                             fprintf(stderr, "merging non-leaf (m1=%i m2=%i em=%i)\n", m1, m2, em);
//                         }

                        em1->second = GBS_global_string("%i", em);
                    }
                }

                if (have_exact) {   // erase other (non-exact) probe info
                    eraseTok("mc", tok1);
                }
            }
        }
    }

    string result;
    if (!error) {
        for (TokMap::iterator tm = tok1.begin(); tm != tok1.end(); ++tm) {
            result += tm->first+"="+tm->second+",";
        }
        size_t len = result.length();
        if (len) result.resize(len-1);
    }
    return result;
}

static string mergeLines(list<string>& lines, GB_ERROR& error) {
    deque< deque<string> > nodeInfos(lines.size());
    string format;
    int    idx = 0;

    for (list<string>::iterator l = lines.begin(); l != lines.end() && !error; ++l, ++idx) {
        string format2 = extractNodeInfos(*l, nodeInfos[idx], error);
        if (error) break;
        if (format.length()) {
            if (format != format2) {
                error = GBS_global_string("line format mismatch\n\"%s\"\n!=\n\"%s\"",
                                          format.c_str(), format2.c_str());
            }
        }
        else {
            format = format2;
        }
    }

    // fprintf(stdout, "format=<%s>\n", format.c_str());

    if (!error) {
        deque<string> mergedInfos;

        while (!nodeInfos.begin()->empty()) { // with every node in line
            deque<string> ni2;
            for (deque< deque<string> >::iterator dds = nodeInfos.begin(); dds != nodeInfos.end(); ++dds) {
                ni2.push_back(dds->front()); // collect first node info from every line
                dds->pop_front();
            }

            string merged = mergeNodeInfos(ni2, error);
            if (error) break;

            mergedInfos.push_back(merged);
        }

        while (!mergedInfos.empty() && !error) {
            size_t pos = format.find("%s");
            tm_assert(pos != string::npos);

            const string& nodeInfo = mergedInfos.front();
            if (nodeInfo.length()) {
                char   *encoded     = encodeTreeNode(nodeInfo.c_str());
                string  replacement = string("\'")+encoded+'\'';
                format.replace(pos, 2, replacement);
                free(encoded);
            }
            else {
                format.erase(pos, 2);
            }

            // fprintf(stdout, "format=<%s>\n", format.c_str());
            mergedInfos.pop_front();
        }

        if (!error) {
            if (format.find("''") != string::npos) {
                tm_assert(0);
            }
            return format;
        }
    }

    return "<error>";
}

static GB_ERROR mergeTrees(deque<ifstreamPtr>& in, ofstream& out) {
    GB_ERROR    error      = 0;
    int         linenumber = 0;
    int         inputs     = in.size();
    char        linebuffer[MAXLINELENGTH+1];
    bool        done       = false;
    bool        headerdone = false;

    while (!error && !done) {
        list<string> lines;
        int         eofs = 0;

        for (deque<ifstreamPtr>::iterator i = in.begin(); i != in.end() && !error; ++i) {
            ifstreamPtr ip = *i;
            if (!ip->eof()) {
                if (!ip->good()) {
                    error = "error reading input #1";
                }
                else {
                    ip->getline(linebuffer, MAXLINELENGTH);
                    if (!ip->eof() && !ip->good()) {
                        error = "error reading input #2";
                    }
                    lines.push_back(linebuffer);
                }
            }
            else {
                ++eofs;
            }
        }

        if (eofs) {
            if (eofs == inputs) { // eof found in all files
                done = true;
            }
            else {
                error = "input files differ in length (=number of lines)";
            }
        }

        if (!error && !done) {
            ++linenumber;

            if (!headerdone) {
                string line = *(lines.begin());

                for (list<string>::const_iterator i = lines.begin(); i != lines.end() && !error; ++i) {
                    if (*i != line) {
                        error = "headers are not identical";
                    }
                }

                if (!error) {
                    if (line.find(']') != string::npos) {
                        headerdone = true;
                    }
                    out << line << '\n';
                }
            }
            else {
                string merged = mergeLines(lines, error);
                if (!error) out << merged << "\n";
            }

//             if (lines.size() == 1) { // all lines are equal
//                 string line = *(lines.begin());
//                 if (!headerdone && line.find(']') != string::npos) {
//                     headerdone = true;
//                 }
//                 else {
//                     if (headerdone) {
//                         size_t empty_node;
//                         while ((empty_node = line.find("''")) != string::npos) {
//                             line.erase(empty_node, 2);
//                         }
//                     }
//                 }
//                 out << line << '\n';
//             }
//             else { // lines differ
//                 if (headerdone) {
//                     string merged = mergeLines(lines, error);
//                     if (!error) out << merged << "\n";
//                 }
//                 else {
//                     error = "headers are not identical";
//                 }
//             }
        }
    }

    return error;
}

int main(int argc,char *argv[]) {
    printf("pgd_tree_merge v1.0 -- (C) 2003 by Ralf Westram\n");
    GB_ERROR error = 0;

    try {
        error = scanArguments(argc, argv); // Check and init Parameters

        if (!error) {
            deque<ifstreamPtr> in;
            for (deque<string>::const_iterator s = inputtrees.begin(); s != inputtrees.end() && !error; ++s) {
                const char  *fname = s->c_str();
                ifstreamPtr  i     = new ifstream(fname);
                if (!i->good()) {
                    error = GBS_global_string("Can't open '%s'", fname);
                }
                else {
                    in.push_back(new ifstream(s->c_str()));
                }
            }

            if (!error) {
                ofstream out(outputtree.c_str());
                if (!out.good()) {
                    error = GBS_global_string("Can't write '%s'", outputtree.c_str());
                }
                else {
                    error = mergeTrees(in, out);
                }
            }
        }
    }
    catch (exception &e) {
        error = GBS_global_string("Unexpected exception: '%s'", e.what());
    }

    if (error) {
        fprintf(stderr, "Error in pgd_tree_merge: %s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


