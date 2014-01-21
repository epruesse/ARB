// ================================================================ //
//                                                                  //
//   File      : MetaTag.h                                          //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in November 2006   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //
#ifndef METATAG_H
#define METATAG_H

#ifndef TYPES_H
#include "types.h"
#endif

enum MetaTagType {
    MT_HEADER,                                      // Header tag (ID or LOCUS)
    MT_BASIC,                                       // Basic tag -> gets written to DB (into 'field')
    MT_REF_START,                                   // Start of (new) reference
    MT_REF,                                         // Reference data, 'field' describes type of reference
    MT_REF_DBID,                                    // Database reference (PUBMED, MEDLINE, DOI, ...)
    MT_FEATURE_START,                               // Start of feature table
    MT_FEATURE,                                     // Feature table
    MT_SEQUENCE_START,                              // Start of sequence
    MT_CONTIG,                                      // contig entry
    MT_END,                                         // End of file (or section, if multiple entries)
    MT_IGNORE,                                      // is ignored
};

struct MetaTag {
    std::string tag;                                     // tag name (in flatfile)
    std::string field;                                   // field name
    MetaTagType type;
};

typedef std::map<std::string, const MetaTag *> MetaTagMap;

class MetaTagTranslator : virtual Noncopyable {
    MetaTagMap translate;

public:
    MetaTagTranslator(const struct MetaTag *meta_description) {
        for (int idx = 0; !meta_description[idx].tag.empty(); ++idx) {
            const MetaTag& mt  = meta_description[idx];
            translate[mt.tag] = &mt;
        }
    }

    const MetaTag *get(const std::string& tag) const {
        MetaTagMap::const_iterator found = translate.find(tag);
        if (found != translate.end()) return found->second;
        return 0;
    }
};



#else
#error MetaTag.h included twice
#endif // METATAG_H

