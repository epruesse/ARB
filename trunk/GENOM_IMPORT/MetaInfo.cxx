// ================================================================ //
//                                                                  //
//   File      : MetaInfo.cxx                                       //
//   Purpose   :                                                    //
//   Time-stamp: <Mon Mar/16/2009 13:59 MET Coder@ReallySoft.de>    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#include "MetaInfo.h"

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

    if (ref_count == 1) { // only one reference -> dont tag
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

void References::add_dbid(const string& content) {
    // add embl 'RX' entry
    // expects content like 'MEDLINE; id.' or 'PUBMED; id.' etc.

    size_t semi = content.find(';');
    if (semi == string::npos) throw GBS_global_string("Expected ';' in '%s'", content.c_str());

    string db_tag_org = content.substr(0, semi);
    string db_tag     = db_tag_org;
    for (string::iterator s = db_tag.begin(); s != db_tag.end(); ++s) {
        if (!isalpha(*s)) throw GBS_global_string("Illegal character '%c' in database id '%s'", *s, db_tag_org.c_str());
        *s = tolower(*s);
    }
    db_tag += "_id";

    size_t id_start = content.find_first_not_of(' ', semi+1);
    if (semi == string::npos) throw GBS_global_string("Expected id behind ';' in '%s'", content.c_str());

    string id = content.substr(id_start);
    if (id[id.length()-1] != '.') {
        throw GBS_global_string("Expected '.' at end of %s '%s'", db_tag.c_str(), id.c_str());
    }
    id.erase(id.length()-1); // remove terminal '.'

    add(db_tag, id);
}

// --------------------------------------------------------------------------------

void MetaInfo::add(const MetaTag *meta, const string& content, bool allow_multiple_entries) {
    stringMapIter existing = entries.find(meta->field);
    if (existing != entries.end()) { // existing entry
        if (!allow_multiple_entries) {
            throw GBS_global_string("Multiple occurrance of tag '%s'", meta->tag.c_str());
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
