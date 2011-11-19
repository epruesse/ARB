// ================================================================ //
//                                                                  //
//   File      : Feature.h                                          //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef FEATURE_H
#define FEATURE_H

#ifndef TYPES_H
#include "types.h"
#endif
#ifndef LOCATION_H
#include "Location.h"
#endif

class Feature {
    string      type;
    LocationPtr location;
    stringMap   qualifiers;     // qualifiers with content (content of multiple identical qualifiers gets merged)

public:
    Feature(const string& Type, const string& locationString);

    void addQualifiedEntry(const string& qualifier, const string& value);

    string createGeneName() const; // creates a (non-unique) default name for gene

    const string& getType() const { return type; }
    const Location& getLocation() const { return *location; }
    const stringMap& getQualifiers() const { return qualifiers; }

    void expectLocationInSequence(long seqLength) const;

    void fixEmptyQualifiers();
};


#else
#error Feature.h included twice
#endif // FEATURE_H

