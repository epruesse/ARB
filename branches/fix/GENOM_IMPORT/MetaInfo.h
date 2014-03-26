// ================================================================ //
//                                                                  //
//   File      : MetaInfo.h                                         //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef METAINFO_H
#define METAINFO_H

#ifndef METATAG_H
#include "MetaTag.h"
#endif

class Reference { // holds information of one reference section
    stringMap entries; // reference entries mapped to ARBDB field names

public:
    Reference() {}

    void add(const std::string& field, const std::string& content);
    const std::string *get(const std::string& field) const;

    void getKeys(stringSet& keys) const; // get reference keys
};


class References : virtual Noncopyable { // holds information of all reference sections
    std::vector<Reference>  refs;
    Reference              *latest;
    int                     ref_count;

public:
    References() : latest(0), ref_count(0) {}

    void start();               // start a new reference
    void add(const std::string& field, const std::string& content) {
        gi_assert(latest);
        latest->add(field, content);
    }

    void add_dbid(const std::string& content); // special handling for 'RX' field

    void   getKeys(stringSet& keys) const; // get reference keys
    std::string tagged_content(const std::string& refkey) const;

#if defined(DEBUG)
    void dump() const;
#endif // DEBUG
};

class MetaInfo : virtual Noncopyable {
    stringMap  entries;         // key = arb_field, value = content

public:
    MetaInfo() {}

    void add(const MetaTag *meta, const std::string& content, bool allow_multiple_entries);

#if defined(DEBUG)
    void dump() const;
#endif // DEBUG

    const stringMap& getEntries() const { return entries; }

    const std::string& getAccessionNumber() const;
};

#else
#error MetaInfo.h included twice
#endif // METAINFO_H



