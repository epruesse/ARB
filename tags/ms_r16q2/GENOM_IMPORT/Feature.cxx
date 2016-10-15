// ================================================================ //
//                                                                  //
//   File      : Feature.cxx                                        //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "Feature.h"
#include "types.h"
#include <cctype>


using namespace std;


Feature::Feature(const string& Type, const string& locationString)
    : type(Type)
    , location(parseLocation(locationString))
{
}

inline void setOrAppendQualifiedEntry(stringMap& qualifiers, const string& qualifier, const string& value) {
    stringMapIter existing = qualifiers.find(qualifier);
    if (existing != qualifiers.end()) { // existing qualifier
        existing->second.append(1, '\n'); // append separated by LF
        existing->second.append(value);
    }
    else {
        qualifiers[qualifier] = value;
    }
}

void Feature::addQualifiedEntry(const string& qualifier, const string& value) {
    // search for quotes
    size_t vlen = value.length();

    gi_assert(vlen>0);

    stringCIter start = value.begin();
    stringCIter end   = start+vlen-1;

    if (*start == '"') {
        if (vlen == 1 || *end != '"') {
            throw GBS_global_string("Unclosed quotes at qualifier '%s'", qualifier.c_str());
        }
        // skip quotes :
        ++start;
        // end points to '"'
    }
    else {
        ++end; // point behind last character
    }

    setOrAppendQualifiedEntry(qualifiers, qualifier, string(start, end));
}

static void appendData(string& id, const string& data, int maxAppend) {
    // extract alphanumeric text portion from start of 'data'
    // until some other character is found

    if (maxAppend >= 2) {
        size_t old_id_len = id.length();

        id.append(1, '_');
        maxAppend--;

        stringCIter end          = data.end();
        bool        insideWord   = false;
        bool        seenNonDigit = false;

        for (stringCIter i = data.begin(); maxAppend>0 && i != end; ++i) {
            char c = *i;
            if (isalnum(c)) {
                if (!insideWord) c = toupper(c);
                id.append(1, c);
                maxAppend--;
                insideWord         = true;
                if (!seenNonDigit && isalpha(c)) { seenNonDigit = true; }
            }
            else if (isspace(c) || c == '-') { // ignore space and '-'
                insideWord = false;
            }
            else {
                break; // anything else -> abort
            }
        }

        if (!seenNonDigit) { // data only contained digits (as far as data has been scanned)
            id.resize(old_id_len); // undo changes
        }
    }
}

string Feature::createGeneName() const
{
    stringMapCIter not_found = qualifiers.end();
    stringMapCIter product   = qualifiers.find("product");
    stringMapCIter gene      = qualifiers.find("gene");

    const size_t maxidlen = 30; // just an approx. limit
    string       id       = type; // use gene type

    id.reserve(maxidlen+10);
    if (gene != not_found) { // append gene name
        appendData(id, gene->second, maxidlen-id.length());
    }

    if (product != not_found) {
        appendData(id, product->second, maxidlen-id.length());
    }

    // now ensure that id doesn't end with digit
    // (if it would, creating unique gene names gets too complicated)
    if (isdigit(id[id.length()-1])) {
        if (id.length() == maxidlen) id.resize(maxidlen-1);
        id.append(1, 'X');
    }

    return id;
}

void Feature::expectLocationInSequence(long seqLength) const
{
    // test whether feature location is inside sequence
    // throw error otherwise

    if (!location->isInRange(1, seqLength)) {
        throw GBS_global_string("Illegal feature location (outside sequence 1..%li)", seqLength);
    }
}

void Feature::fixEmptyQualifiers() {
    // some qualifiers in feature table may be empty

    stringMapIter e = qualifiers.end();
    for (stringMapIter i = qualifiers.begin(); i != e; ++i) {
        if (i->second.empty()) { // with all qualifiers, that have no content, do..
            if (i->first == "replace") {
                // ARB cannot store empty strings!
                // Since '/replace=""' means 'delete location', we need to store this
                // this information differently.
                i->second = "<empty>"; //
            }
        }
    }
}
