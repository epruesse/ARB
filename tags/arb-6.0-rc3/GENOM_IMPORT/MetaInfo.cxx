// ================================================================ //
//                                                                  //
//   File      : MetaInfo.cxx                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "MetaInfo.h"
#include <RegExpr.hxx>


using namespace std;

void Reference::add(const string& field, const string& content)
{
    gi_assert(!field.empty());
    gi_assert(!content.empty());

    stringMapIter existing = entries.find(field);
    if (existing != entries.end()) {
        throw GBS_global_string("Duplicated reference entry for '%s'", field.c_str());
    }
    entries[field] = content;
}

const string *Reference::get(const string& field) const
{
    stringMapCIter existing = entries.find(field);
    return (existing != entries.end()) ? &existing->second :  0;
}

void Reference::getKeys(stringSet& keys) const
{
    stringMapCIter e = entries.end();
    for (stringMapCIter i = entries.begin(); i != e; ++i) {
        keys.insert(i->first);
    }
}

// --------------------------------------------------------------------------------

typedef vector<Reference> RefVector;
DEFINE_ITERATORS(RefVector);

void References::start()
{
    refs.push_back(Reference());
    latest = &refs.back();
    ref_count++;
}


void References::getKeys(stringSet& keys) const
{
    keys.clear();
    RefVectorCIter e = refs.end();
    for (RefVectorCIter i = refs.begin(); i != e; ++i) {
        i->getKeys(keys);
    }
}

string References::tagged_content(const string& refkey) const
{
    string content;

    if (ref_count == 1) { // only one reference -> don't tag
        RefVectorCIter  i           = refs.begin();
        const string   *ref_content = i->get(refkey);

        gi_assert(ref_content);
        content = *ref_content;
    }
    else {
        int            count = 1;
        RefVectorCIter e     = refs.end();

        for (RefVectorCIter i = refs.begin(); i != e; ++i, ++count) {
            const string *ref_content = i->get(refkey);
            if (ref_content) {
                if (!content.empty()) content.append(1, ' ');
                content.append(GBS_global_string("[REF%i] ", count));
                content.append(*ref_content);
            }
        }
    }
    return content;
}

#if defined(DEBUG)
void References::dump() const
{
    stringSet     keys;
    getKeys(keys);
    stringSetIter e = keys.end();

    for (stringSetIter i = keys.begin(); i != e; ++i) {
        string tagged = tagged_content(*i);
        printf("%s='%s'\n", i->c_str(), tagged.c_str());
    }
}
#endif // DEBUG

enum DBID_TYPE {
    DBID_STANDARD,
    DBID_ACCEPT,
    DBID_ILLEGAL,
};
struct DBID {
    const char *id;
    DBID_TYPE   type;
    const char *arb_field;
};

// see http://www.ebi.ac.uk/embl/Documentation/User_manual/usrman.html#3_4_10_4

static const DBID dbid_definition[] = {             // accepted DBIDs (EMBL 'RX'-tag)
    { "DOI",      DBID_STANDARD, "doi_id"      },
    { "PUBMED",   DBID_STANDARD, "pubmed_id"   },
    { "AGRICOLA", DBID_STANDARD, "agricola_id" },
    { "MEDLINE",  DBID_ACCEPT,   "medline_id"  }, // non-standard, but common

    { NULL, DBID_ILLEGAL, NULL }, // end marker
};

void References::add_dbid(const string& content) {
    // add embl 'RX' entry
    //
    // * 'content' has \n inserted at original line breaks and
    //   contains database references like 'MEDLINE; id.' or 'PUBMED; id.' etc.
    // * Multiple database references may be concatenated (each starts on it's own line)
    // * 'id' is possibly split up on several lines

    RegExpr         reg_dbid("^([A-Z]+);\\s+|\n([A-Z]+);\\s+", false);
    const RegMatch *dbid_start = reg_dbid.match(content);

    if (!dbid_start) {
        throw GBS_global_string("Expected database reference id (e.g. 'DOI; ' or 'PUBMED; ')");
    }
    else {
        re_assert(reg_dbid.subexpr_count() == 2);
        while (dbid_start) {
            const RegMatch *sub = reg_dbid.subexpr_match(1);
            if (!sub) sub       = reg_dbid.subexpr_match(2);
            re_assert(sub);

            string dbid     = sub->extract(content);
            size_t id_start = dbid_start->posBehindMatch();

            dbid_start = reg_dbid.match(content, id_start); // search for start of next db-id

            DBID_TYPE   type      = DBID_ILLEGAL;
            const char *arb_field = 0;
            for (int m = 0; ; m++) {
                const char *name = dbid_definition[m].id;
                if (!name) break;
                if (dbid == name) {
                    type      = dbid_definition[m].type;
                    arb_field = dbid_definition[m].arb_field;
                    break;
                }
            }
            if (type == DBID_ILLEGAL) throw GBS_global_string("Unknown DBID '%s'", dbid.c_str());

            string id = content.substr(id_start, dbid_start ? dbid_start->pos()-id_start : string::npos);
            if (id.empty()) throw GBS_global_string("Empty database reference for '%s'", dbid.c_str());
            if (id[id.length()-1] != '.') throw GBS_global_string("Expected terminal '.' in '%s'", id.c_str());
            id.erase(id.length()-1); // remove terminal '.'
            add(arb_field, id);
        }
    }
}

// --------------------------------------------------------------------------------

void MetaInfo::add(const MetaTag *meta, const string& content, bool allow_multiple_entries) {
    stringMapIter existing = entries.find(meta->field);
    if (existing != entries.end()) { // existing entry
        if (!allow_multiple_entries) {
            throw GBS_global_string("Multiple occurrences of tag '%s'", meta->tag.c_str());
        }
        existing->second += '\n'+content; // append content
    }
    else { // non-existing content
        entries[meta->field] = content;
    }
}

const string& MetaInfo::getAccessionNumber() const {
    stringMapCIter found = entries.find("acc");
    if (found == entries.end()) {
        static string no_acc("<Missing accession number>");
        return no_acc;
    }
    return found->second;
}

#if defined(DEBUG)
void MetaInfo::dump() const
{
    stringMapCIter e = entries.end();

    printf("MetaInfo:\n");
    for (stringMapCIter i = entries.begin(); i != e; ++i) {
        printf("%s='%s'\n", i->first.c_str(), i->second.c_str());
    }
}
#endif // DEBUG
