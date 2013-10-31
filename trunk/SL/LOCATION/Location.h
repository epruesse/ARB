// ================================================================ //
//                                                                  //
//   File      : Location.h                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef LOCATION_H
#define LOCATION_H

#ifndef SMARTPTR_H
#include <smartptr.h>
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef _GLIBCXX_STRING
#include <string>
#endif

typedef std::map<std::string, std::string> stringMap;

typedef std::vector<int>  intVector;
typedef std::vector<char> charVector;
typedef std::vector<bool> boolVector;

struct GEN_position;

enum LocationJoinType {
    LJT_UNDEF,                  // undefined
    LJT_NOT_JOINED,             // location does not contain multiple parts
    LJT_JOIN,                   // sequence data may be joined
    LJT_ORDER,                  // nothing is implied about the reasonableness about joining
};


struct Location : public Noncopyable {
    Location() {}
    virtual ~Location() {}

    virtual int count() const                                       = 0;
    virtual bool isInRange(long pos1, long pos2) const              = 0;
    virtual void save(GEN_position *into, bool complementary) const = 0;
    virtual LocationJoinType getJoinType() const                    = 0;
    virtual std::string as_string() const                           = 0;

    GEN_position *create_GEN_position() const;
};

typedef SmartPtr<Location> LocationPtr;

LocationPtr parseLocation(const std::string& source);
LocationPtr to_Location(const GEN_position *gp);

#else
#error Location.h included twice
#endif // LOCATION_H

