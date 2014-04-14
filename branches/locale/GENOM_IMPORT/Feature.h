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

#ifndef LOCATION_H
#include "Location.h"
#endif

class Feature {
    std::string type;
    LocationPtr location;
    stringMap   qualifiers;     // qualifiers with content (content of multiple identical qualifiers gets merged)

public:
    Feature(const std::string& Type, const std::string& locationString);

    void addQualifiedEntry(const std::string& qualifier, const std::string& value);

    std::string createGeneName() const; // creates a (non-unique) default name for gene

    const std::string& getType() const { return type; }
    const Location& getLocation() const { return *location; }
    const stringMap& getQualifiers() const { return qualifiers; }

    void expectLocationInSequence(long seqLength) const;

    void fixEmptyQualifiers();
};


#else
#error Feature.h included twice
#endif // FEATURE_H

