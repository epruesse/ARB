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
#include <arb_stdstr.h>

#include <adGene.h>
#include <arbdb.h>

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

    static inline string one_pos_as_string(char uc, long p) {
        string s;
        if (uc != '=') s += uc;
        return s += GBS_global_string("%li", p);
    }

public:
    SimpleLocation(long p1, long p2, char u1, char u2)
        : pos1(p1), pos2(p2), uncertain1(u1), uncertain2(u2)
    {
        arb_assert(u1 && u2);
    }
    SimpleLocation(long p, char u)
        : pos1(p), pos2(p), uncertain1(u), uncertain2(u)
    {
        arb_assert(u == '='); // do not allow uncertainties with single position ctor
    }

    int count() const OVERRIDE { return 1; }
    LocationJoinType getJoinType() const OVERRIDE { return LJT_NOT_JOINED; }
    bool isInRange(long p1, long p2) const OVERRIDE {
        arb_assert(p1 <= p2);
        return p1 <= pos1 && pos2 <= p2;
    }
    void save(GEN_position *pos, bool complementary) const OVERRIDE {
        int p = pos->parts;

        pos->start_pos[p]       = pos1;
        pos->stop_pos[p]        = pos2;
        pos->complement[p]      = char(complementary);
        pos->start_uncertain[p] = uncertain1;
        pos->stop_uncertain[p]  = uncertain2;

        ++pos->parts;
    }

    string as_string() const OVERRIDE {
        if (uncertain1 == '+') {
            if (uncertain2 != '-' || pos2 != (pos1+1)) throw "Invalid uncertainties";
            return GBS_global_string("%li^%li", pos1, pos2);
        }

        string s = one_pos_as_string(uncertain1, pos1);
        if (pos1 != pos2) {
            s += ".."+one_pos_as_string(uncertain2, pos2);
        }
        return s;
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

    LocationJoinType getJoinType() const OVERRIDE { return joinType; }
    int count() const OVERRIDE {
        int Count = 0;

        LocationVectorCIter e = locations.end();
        for (LocationVectorCIter i = locations.begin(); i != e; ++i) {
            Count += (*i)->count();
        }
        return Count;
    }

    bool isInRange(long p1, long p2) const OVERRIDE {
        LocationVectorCIter e = locations.end();
        for (LocationVectorCIter i = locations.begin(); i != e; ++i) {
            if (!(*i)->isInRange(p1, p2)) {
                return false;
            }
        }
        return true;
    }

    void save(GEN_position *pos, bool complementary) const OVERRIDE {
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
    string as_string() const OVERRIDE {
        string joined;
        switch (joinType) {
            case LJT_JOIN: joined  = "join"; break;
            case LJT_ORDER: joined = "order"; break;
            default: throw "Unhandled join type"; break;
        }
        joined += '(';

        LocationVectorCIter e  = locations.end();
        LocationVectorCIter i  = locations.begin();
        if (i != e) {
            joined += (*i++)->as_string();
            for (; i != e; ++i) {
                joined += ',';
                joined += (*i)->as_string();
            }
        }
        joined += ')';
        return joined;
    }
};

class ComplementLocation : public Location {
    LocationPtr location;
public:
    ComplementLocation(const LocationPtr& loc) : location(loc) {}

    int count() const OVERRIDE { return location->count(); }
    bool isInRange(long p1, long p2) const OVERRIDE { return location->isInRange(p1, p2); }
    void save(GEN_position *pos, bool complementary) const OVERRIDE { location->save(pos, !complementary); }
    LocationJoinType getJoinType() const OVERRIDE { return location->getJoinType(); }
    string as_string() const OVERRIDE { return string("complement(")+location->as_string()+')'; }
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

static bool parseInfix(const string &str, const string& prefix, const string& postfix, string& foundInfix) {
    bool parsed = false;
    if (beginsWith(str, prefix) && endsWith(str, postfix)) {
        size_t strlen  = str.length();
    size_t prelen  = prefix.length();
    size_t postlen = postfix.length();

        if (strlen >= (prelen+postlen)) { // otherwise str is to short (prefix and postfix overlap)
            foundInfix = str.substr(prelen, strlen-(prelen+postlen));
            parsed     = true;
        }
    }
    return parsed;
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

            JoinedLocation *join = new JoinedLocation(joinType);
            LocationPtr     res  = join;

            LocationVectorCIter e = locvec.end();
            for (LocationVectorCIter i = locvec.begin(); i != e; ++i) {
                join->push_back(*i);
            }
            return res;
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
            if (uncertain != '=') throw "No uncertainty allowed for single positions";
            return new SimpleLocation(single_pos, uncertain);
        }
    }

    throw string("Unparsable location '"+source+"'");
}

GEN_position *Location::create_GEN_position() const {
    GEN_position *pos = GEN_new_position(count(), getJoinType() == LJT_JOIN);
    GEN_use_uncertainties(pos);

#if defined(ASSERTION_USED)
    int org_parts = pos->parts;
#endif // ASSERTION_USED

    pos->parts = 0;             // misuse 'parts' as index for filling 'pos'
    save(pos, false);

    arb_assert(pos->parts == org_parts);

    return pos;
}

inline LocationPtr part2SimpleLocation(const GEN_position *pos, int i, bool inverseComplement) {
    LocationPtr res = new SimpleLocation(pos->start_pos[i], pos->stop_pos[i], pos->start_uncertain[i], pos->stop_uncertain[i]);
    if (contradicted(pos->complement[i], inverseComplement)) {;
        res = new ComplementLocation(res);
    }
    return res;
}

LocationPtr to_Location(const GEN_position *gp) {
    arb_assert(gp->start_uncertain);
    arb_assert(gp->stop_uncertain);

    if (gp->parts == 1) {
        return part2SimpleLocation(gp, 0, false);
    }

    int complemented = 0;
    for (int p = 0; p<gp->parts; ++p) complemented += gp->complement[p];

    JoinedLocation *joined = new JoinedLocation(gp->joinable ? LJT_JOIN : LJT_ORDER);
    LocationPtr     res    = joined;

    if (2*complemented > gp->parts) {
        // create "complement(join())" since it is "smaller"
        for (int p = gp->parts-1; p >= 0; --p) {
            LocationPtr ploc = part2SimpleLocation(gp, p, true);
            joined->push_back(ploc);
        }
        res = new ComplementLocation(res);
    }
    else {
        for (int p = 0; p<gp->parts; ++p) {
            LocationPtr ploc = part2SimpleLocation(gp, p, false);
            joined->push_back(ploc);
        }
    }
    return res;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

static GB_ERROR perform_conversions(const string& in, string& out, string& out2) {
    GB_ERROR error = NULL;
    try {
        LocationPtr loc = parseLocation(in);
        out             = loc->as_string();

        GEN_position *gp = loc->create_GEN_position();
        TEST_EXPECT(gp);

        LocationPtr reloc = to_Location(gp);
        out2              = reloc->as_string();

        GEN_free_position(gp);
    }
    catch (std::string& err) { error = GBS_static_string(err.c_str()); }
    catch (const char *&err) { error = GBS_static_string(err); }
    return error;
}

#define DO_LOCONV_NOERR(str) string reverse, convReverse; TEST_EXPECT_NO_ERROR(perform_conversions(str, reverse, convReverse));

// the following assertions test
// - conversion from string->Location->string and
// - conversion from string->Location->GEN_position->Location->string
// 
// TEST_EXPECT___CONV_IDENT     expects both conversions equal input
// TEST_EXPECT___CONV__INTO     expects 1st conversion changes to 'res'
// TEST_EXPECT_RECONV__INTO     expects 2nd conversion changes to 'res'
// TEST_EXPECT_RECONV_INTO2     expects 1st conversion changes to 'res1' and 2nd to 'res2'

#define TEST_EXPECT___CONV_IDENT(str) do {              \
        DO_LOCONV_NOERR(str);                           \
        TEST_EXPECT_EQUAL(str, reverse.c_str());        \
        TEST_EXPECT_EQUAL(str, convReverse.c_str());    \
    } while(0)

#define TEST_EXPECT___CONV_IDENT__BROKEN(str) do {              \
        DO_LOCONV_NOERR(str);                                   \
        TEST_EXPECT_EQUAL__BROKEN(str, reverse.c_str());        \
    } while(0)

#define TEST_EXPECT___CONV__INTO(str,res) do {          \
        DO_LOCONV_NOERR(str);                           \
        TEST_EXPECT_EQUAL(res, reverse.c_str());        \
        TEST_EXPECT_EQUAL(res, convReverse.c_str());    \
    } while(0)

#define TEST_EXPECT_RECONV__INTO(str,res) do {                                  \
        do {                                                                    \
            DO_LOCONV_NOERR(str);                                               \
            TEST_EXPECT_EQUAL(str, reverse.c_str());                            \
            TEST_EXPECT_EQUAL(res, convReverse.c_str());                        \
            TEST_EXPECT_MORE_EQUAL(reverse.length(), convReverse.length());  \
        } while(0);                                                             \
        TEST_EXPECT___CONV_IDENT(res);                                          \
    } while(0)

#define TEST_EXPECT_RECONV_INTO2(str,res1,res2) do {                            \
        DO_LOCONV_NOERR(str);                                                   \
        TEST_EXPECT_EQUAL(res1, reverse.c_str());                               \
        TEST_EXPECT_EQUAL(res2, convReverse.c_str());                           \
        TEST_EXPECT_MORE_EQUAL(reverse.length(), convReverse.length());      \
    } while(0)

#define TEST_EXPECT__PARSE_ERROR(str,err) do {                                  \
        string reverse, convReverse;                                            \
        TEST_EXPECT_EQUAL(perform_conversions(str, reverse, convReverse), err); \
    } while(0)

void TEST_gene_location() {
    // see also ../ARBDB/adGene.cxx@TEST_GEN_position

    TEST_EXPECT___CONV_IDENT("1725");
    TEST_EXPECT__PARSE_ERROR("3-77", "Unexpected char '-' in '3-77'");
    TEST_EXPECT___CONV_IDENT("3..77");
    TEST_EXPECT___CONV_IDENT("77..3"); // @@@ could be interpreted as reverse (but not complement)

    TEST_EXPECT___CONV_IDENT("<3..77");
    TEST_EXPECT___CONV_IDENT("3..>77");
    TEST_EXPECT___CONV_IDENT(">3..<77");
    
    TEST_EXPECT___CONV_IDENT("7^8");
    TEST_EXPECT__PARSE_ERROR("7^9", "Can only handle 'pos^pos+1'. Can't parse location '7^9'");

    TEST_EXPECT___CONV_IDENT("complement(3..77)");
    TEST_EXPECT___CONV_IDENT("complement(77..3)");
    TEST_EXPECT___CONV_IDENT("complement(77)");

    TEST_EXPECT___CONV_IDENT("join(3..77,100..200)");
    TEST_EXPECT_RECONV__INTO("join(3..77)", "3..77");
    TEST_EXPECT___CONV_IDENT("join(3..77,100,130..177)");
    TEST_EXPECT__PARSE_ERROR("join(3..77,100..200, 130..177)", "Unparsable location ' 130..177'");

    TEST_EXPECT_RECONV__INTO("order(1)", "1");
    TEST_EXPECT___CONV_IDENT("order(0,8,15)");
    TEST_EXPECT___CONV_IDENT("order(10..12,7..9,1^2)");
    TEST_EXPECT_RECONV__INTO("order(complement(1^2),complement(7..9),complement(10..12))",
                             "complement(order(10..12,7..9,1^2))");
    TEST_EXPECT___CONV_IDENT("order(10..12,complement(7..9),1^2)");
    TEST_EXPECT_RECONV__INTO("order(complement(1^2),7..9,complement(10..12))",
                             "complement(order(10..12,complement(7..9),1^2))");

    TEST_EXPECT__PARSE_ERROR("join(order(0,8,15),order(3,2,1))", "order() and join() cannot be mixed");

    TEST_EXPECT_RECONV__INTO("join(complement(3..77),complement(100..200))",
                             "complement(join(100..200,3..77))");
    TEST_EXPECT_RECONV__INTO("join(complement(join(3..77,74..83)),complement(100..200))",
                             "complement(join(100..200,3..77,74..83))");
    TEST_EXPECT_RECONV__INTO("complement(complement(complement(100..200)))",
                             "complement(100..200)");
    TEST_EXPECT_RECONV__INTO("join(complement(join(complement(join(1)))))",
                             "1");

    // cover errors
    TEST_EXPECT__PARSE_ERROR("join(abc()", "Expected 1 closing parenthesis in 'abc('");

    // strange behavior
    TEST_EXPECT___CONV__INTO("", "0");
    TEST_EXPECT_RECONV_INTO2("join()", "join(0)", "0");
    TEST_EXPECT___CONV__INTO("complement()", "complement(0)");
    TEST_EXPECT_RECONV_INTO2("complement(complement())", "complement(complement(0))", "0");
}

#endif // UNIT_TESTS
