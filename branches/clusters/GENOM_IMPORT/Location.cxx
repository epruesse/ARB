// ================================================================ //
//                                                                  //
//   File      : Location.cxx                                       //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#include "Location.h"
#include "tools.h"

#include <adGene.h>

#include <cctype>
#include <string>

using namespace std;

typedef SmartPtr<Location>  LocationPtr;
typedef vector<LocationPtr> LocationVector;

DEFINE_ITERATORS(LocationVector);

class SimpleLocation : public Location {
    long pos1;
    long pos2;
    char uncertain1;
    char uncertain2;

public:
    SimpleLocation(long p1, long p2, char u1, char u2)
        : pos1(p1), pos2(p2), uncertain1(u1), uncertain2(u2) {}
    SimpleLocation(long p, char u)
        : pos1(p), pos2(p), uncertain1(u), uncertain2(u)
    {
        arb_assert(u == '='); // do not allow uncertainties with single position ctor
    }

    virtual int count() const { return 1; }
    virtual LocationJoinType getJoinType() const { return LJT_NOT_JOINED; }
    virtual bool isInRange(long p1, long p2) const {
        arb_assert(p1 <= p2);
        return p1 <= pos1 && pos2 <= p2;
    }
    virtual void save(GEN_position *pos, bool complementary) const {
        int p = pos->parts;

        pos->start_pos[p]       = pos1;
        pos->stop_pos[p]        = pos2;
        pos->complement[p]      = char(complementary);
        pos->start_uncertain[p] = uncertain1;
        pos->stop_uncertain[p]  = uncertain2;

        ++pos->parts;
    }
};

class JoinedLocation : public Location {
    LocationVector   locations;
    LocationJoinType joinType;

public:
    JoinedLocation(LocationJoinType jtype) : joinType(jtype) {}

    void push_back(const LocationPtr&loc) {
        LocationJoinType loc_type = loc->getJoinType();
        if (loc_type != LJT_NOT_JOINED && loc_type != joinType) {
            throw "order() and join() cannot be mixed";
        }
        locations.push_back(loc);
    }

    virtual LocationJoinType getJoinType() const { return joinType; }
    virtual int count() const {
        int Count = 0;
        
        LocationVectorCIter e = locations.end();
        for (LocationVectorCIter i = locations.begin(); i != e; ++i) {
            Count += (*i)->count();
        }
        return Count;
    }

    virtual bool isInRange(long p1, long p2) const {
        LocationVectorCIter e = locations.end();
        for (LocationVectorCIter i = locations.begin(); i != e; ++i) {
            if (!(*i)->isInRange(p1, p2)) {
                return false;
            }
        }
        return true;
    }

    virtual void save(GEN_position *pos, bool complementary) const {
        if (complementary) {
            LocationVectorCRIter e = locations.rend();
            for (LocationVectorCRIter i = locations.rbegin(); i != e; ++i) {
                (*i)->save(pos, complementary);
            }
        }
        else {
            LocationVectorCIter e = locations.end();
            for (LocationVectorCIter i = locations.begin(); i != e; ++i) {
                (*i)->save(pos, complementary);
            }
        }
    }
};

class ComplementLocation : public Location {
    LocationPtr location;
public:
    ComplementLocation(const LocationPtr& loc) : location(loc) {}

    virtual int count() const { return location->count(); }
    virtual bool isInRange(long p1, long p2) const { return location->isInRange(p1, p2); }
    virtual void save(GEN_position *pos, bool complementary) const { location->save(pos, !complementary); }
    virtual LocationJoinType getJoinType() const { return location->getJoinType(); }
};

// --------------------------------------------------------------------------------

static size_t parsePosition(const string& source, char& uncertain) {
    // parses one position and returns the value
    // if position contains uncertainties, they are stored in 'uncertain' (as '<' or '>')

    const char *s    = source.c_str();
    size_t      slen = source.length();

    if (s[0] == '>' or s[0] == '<') {
        uncertain = s[0];
        s++;
        slen--;
    }
    else {
        uncertain = '=';
    }

    char   *end;
    size_t  pos = strtoul(s, &end, 10);

    size_t converted = end-s;
    if (converted<slen) {
        throw string("Unexpected char '")+end[0]+"' in '"+source+"'";
    }

    return pos;
}

static void parseLocationList(const string& source, size_t startPos, LocationVector& locvec) {
    size_t comma = source.find_first_of("(,", startPos);

    while (comma != string::npos && source[comma] == '(') {
        size_t pos    = comma+1;
        size_t paren_count = 1;

        while (paren_count>0) {
            size_t paren = source.find_first_of("()", pos);
            if (paren == string::npos) {
                throw GBS_global_string("Expected %zu closing parenthesis in '%s'", paren_count, source.c_str());
            }
            if (source[paren] == ')') paren_count--;
            else paren_count++;

            pos = paren+1;
        }
        comma = source.find_first_of("(,", pos);
    }

    if (comma == string::npos) { // no comma on top level
        locvec.push_back(parseLocation(source.substr(startPos)));
    }
    else {
        arb_assert(source[comma] == ',');
        locvec.push_back(parseLocation(source.substr(startPos, comma-startPos)));
        parseLocationList(source, comma+1, locvec); // continue after comma
    }
}

LocationPtr parseLocation(const string& source) {
    char first = source[0];
    if (first == 'c') {
        string infix;
        if (parseInfix(source, "complement(", ")", infix)) {
            return new ComplementLocation(parseLocation(infix));
        }
    }
    else if (first == 'j' || first == 'o') {
        string           infix;
        LocationJoinType joinType = LJT_UNDEF;

        if (parseInfix(source, "join(", ")", infix)) {
            joinType = LJT_JOIN;
        }
        else if (parseInfix(source, "order(", ")", infix)) {
            joinType = LJT_ORDER;
        }

        if (joinType != LJT_UNDEF) {
            LocationVector locvec;
            parseLocationList(infix, 0, locvec);

            JoinedLocation      *join = new JoinedLocation(joinType);
            LocationVectorCIter  e    = locvec.end();
            for (LocationVectorCIter i = locvec.begin(); i != e; ++i) {
                join->push_back(*i);
            }
            return join;
        }
    }
    else if (isdigit(first) || strchr("<>", first) != 0) {
        size_t dots = source.find("..");
        if (dots != string::npos) {
            char   uncertain1, uncertain2;
            size_t pos1 = parsePosition(source.substr(0, dots), uncertain1);
            size_t pos2 = parsePosition(source.substr(dots+2), uncertain2);

            return new SimpleLocation(pos1, pos2, uncertain1, uncertain2);
        }

        size_t in_between = source.find("^");
        if (in_between != string::npos) {
            char   uncertain1, uncertain2;
            size_t pos1 = parsePosition(source.substr(0, in_between), uncertain1);
            size_t pos2 = parsePosition(source.substr(in_between+1), uncertain2);

            if (uncertain1 == '=' && uncertain2 == '=' && pos2 == pos1+1) {
                return new SimpleLocation(pos1, pos2, '+', '-');
            }
            throw string("Can only handle 'pos^pos+1'. Can't parse location '"+source+"'");
        }
        else {
            // single base position
            char   uncertain;
            size_t single_pos = parsePosition(source, uncertain);
            return new SimpleLocation(single_pos, uncertain);
        }
    }

#if defined(DEVEL_RALF)
    arb_assert(0);
#endif // DEVEL_RALF
    throw string("Unparsable location '"+source+"'");
}

GEN_position *Location::create_GEN_position() const {
    GEN_position *pos = GEN_new_position(count(), GB_BOOL(getJoinType() == LJT_JOIN));
    GEN_use_uncertainties(pos);

#if defined(DEBUG)
    int org_parts = pos->parts;
#endif // DEBUG
    
    pos->parts = 0;             // misuse 'parts' as index for filling 'pos'
    save(pos, false);

    gi_assert(pos->parts == org_parts);

    return pos;
}


