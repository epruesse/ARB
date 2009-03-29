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
#ifndef TYPES_H
#include "types.h"
#endif


typedef vector<int>  intVector;
typedef vector<char> charVector;
typedef vector<bool> boolVector;

class GEN_position;

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

    GEN_position *create_GEN_position() const;
};

typedef SmartPtr<Location> LocationPtr;

LocationPtr parseLocation(const string& source);

#else
#error Location.h included twice
#endif // LOCATION_H

